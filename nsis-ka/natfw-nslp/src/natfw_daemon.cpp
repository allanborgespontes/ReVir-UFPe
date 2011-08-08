/// ----------------------------------------*- mode: C++; -*--
/// @file natfw_daemon.cpp
/// The natfw_daemon class.
/// ----------------------------------------------------------
/// $Id: natfw_daemon.cpp 4118 2009-07-16 16:13:10Z bless $
/// $HeadURL: https://svn.tm.kit.edu/nsis/natfw-nslp/trunk/src/natfw_daemon.cpp $
// ===========================================================
//                      
// Copyright (C) 2005-2007, all rights reserved by
// - Institute of Telematics, Universitaet Karlsruhe (TH)
//
// More information and contact:
// https://projekte.tm.uka.de/trac/NSIS
//                      
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; version 2 of the License
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
//
// ===========================================================
#include "logfile.h"
#include "threads.h"
#include "threadsafe_db.h"
#include "queuemanager.h"

#include "apimessage.h" // from NTLP

#include "natfw_config.h"
#include "msg/natfw_ie.h"
#include "msg/natfw_msg.h"
#include "dispatcher.h"
#include "natfw_daemon.h"
#include "benchmark_journal.h"

#include <openssl/ssl.h>

using namespace protlib;
using namespace protlib::log;
using namespace natfw;
using namespace natfw::msg;


#define LogError(msg) ERRLog("natfw_daemon", msg)
#define LogWarn(msg) WLog("natfw_daemon", msg)
#define LogInfo(msg) ILog("natfw_daemon", msg)
#define LogDebug(msg) DLog("natfw_daemon", msg)


#ifdef BENCHMARK
  extern benchmark_journal journal;
#endif


/**
 * Constructor.
 */
natfw_daemon::natfw_daemon(const natfw_daemon_param &param)
		: Thread(param), config(param.config),
		  session_mgr(&config), nat_mgr(&config),
		  rule_installer(NULL), ntlp_starter(NULL) {

	startup();
}


/**
 * Destructor.
 */
natfw_daemon::~natfw_daemon() {
	shutdown();
}


/**
 * Called exactly once before the first thread executes main_loop.
 */
void natfw_daemon::startup() {
	LogInfo("starting NATFW daemon ...");

	/*
	 * Instantiate an operating system dependent policy rule installer.
	 * We use the iptables policy rule installer only on NF nodes which
	 * have it enabled in the configuration. NIs and NRs don't have to
	 * install policy rules.
	 */
	if ( config.get_nf_install_policy_rules() == true
			&& (config.is_nf_nat() || config.is_nf_firewall()) )
		rule_installer = new iptables_policy_rule_installer(&config);
	else
		rule_installer = new nop_policy_rule_installer(&config);

	try {
		rule_installer->setup();
	}
	catch ( policy_rule_installer_error &e ) {
		LogError("unable to setup the policy rule installer: " << e);
	}

	/*
	 * Start the GIST thread.
	 */
	NTLPStarterParam ntlpparam;
	ntlp_starter= new ThreadStarter<NTLPStarter, NTLPStarterParam>(1, ntlpparam);
	ntlp_starter->start_processing();


	/*
	 * Register our input queue with the queue manager.
	 */
	QueueManager::instance()->register_queue(
		get_fqueue(), natfw_config::INPUT_QUEUE_ADDRESS);

	/*
	 * Register the queue created above with the NTLP. The NTLP will then
	 * append all NATFW messages it receives to our input queue.
	 */
	ntlp::APIMsg *api_msg = new ntlp::APIMsg();
	api_msg->set_source(natfw_config::INPUT_QUEUE_ADDRESS);
	api_msg->set_register(natfw_config::NSLP_ID, 0); // NSLPID, RAO

	/*
	 * TODO: We have no way to find out if the NTLP thread is up and has
	 * already registered an input queue. Because of this, we try to send
	 * our registration message until there is a queue to accept it.
	 */
	bool success;
	do {
		success = api_msg->send_to(natfw_config::OUTPUT_QUEUE_ADDRESS);
		sleep(1);
	}
	while ( ! success );
	assert( success );


	LogDebug("NAFTW daemon startup complete");
}


/**
 * Called exactly once after the last thread executes main_loop.
 *
 * TODO: at least I hope there are no active threads left!
 */
void natfw_daemon::shutdown() {
	LogDebug("NATFW daemon shutting down ...");

	try {
		rule_installer->remove_all();
	}
	catch ( policy_rule_installer_error &e ) {
		LogError("unable to remove the installed policy rules: " << e);
		LogError("You have to remove them manually!");
	}

	// Shut down the NTLP threads.
	ntlp_starter->stop_processing();
	ntlp_starter->wait_until_stopped();

	delete ntlp_starter;

	delete rule_installer;

	QueueManager::instance()->unregister_queue(
			natfw_config::INPUT_QUEUE_ADDRESS);

	LogInfo("NATFW deamon shutdown complete");
}


/**
 * The implementation of the main routine of a worker thread.
 */
void natfw_daemon::main_loop(uint32 thread_id) {

	/* 
	 * The dispatcher handles incoming messages. It is the top-level state
	 * machine which delegates work to the state machines on session level.
	 *
	 * For each main_loop, and thus POSIX thread, there is a dispatcher.
	 */
	dispatcher disp(&session_mgr, &nat_mgr, rule_installer, &config);
	gistka_mapper mapper;


	/*
	 * Wait for messages in the input queue and process them.
	 */
	LogInfo("dispatcher thread #"
			<< thread_id << " waiting for incoming messages ...");

	FastQueue *natfw_input = get_fqueue();

	while ( get_state() == Thread::STATE_RUN ) {
		// A timeout makes sure the loop condition is checked regularly.
		message *msg = natfw_input->dequeue_timedwait(1000);

		if ( msg == NULL )
			continue;	// no message in the queue

		LogDebug("dispatcher thread #" << thread_id
			<< " processing received message #" << msg->get_id());

		MP(benchmark_journal::PRE_PROCESSING);

		// Analyze message and create an event from it.
		event *evt = mapper.map_to_event(msg);

		// Then feed the event to the dispatcher.
		if ( evt != NULL ) {
			MP(benchmark_journal::PRE_DISPATCHER);
			disp.process(evt);
			MP(benchmark_journal::POST_DISPATCHER);
			delete evt;
		}

		delete msg;

		MP(benchmark_journal::POST_PROCESSING);
	}
}


void natfw::init_framework() {
	/*
	 * Initialize libraries.
	 */
	tsdb::init();
	SSL_library_init();		// TODO: seed random generator
	OpenSSL_add_ssl_algorithms();
	SSL_load_error_strings();
	NATFW_IEManager::register_known_ies();
}


void natfw::cleanup_framework() {
	QueueManager::clear();
	NATFW_IEManager::clear();
	//tsdb::end();
	LogError("natfw::cleanup_framework() doesn't call tsdb::end()!");
}

// EOF
