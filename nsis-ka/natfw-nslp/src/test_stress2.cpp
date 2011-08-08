/// ----------------------------------------*- mode: C++; -*--
/// @file test_stress2.cpp
/// A non-interactive client for stress testing.
/// 
/// CREATE messages are sent to a given destination at configurable speed.
/// ----------------------------------------------------------
/// $Id: test_stress2.cpp 6161 2011-05-18 20:02:40Z bless $
/// $HeadURL: https://svn.tm.kit.edu/nsis/natfw-nslp/trunk/src/test_stress2.cpp $
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
#include <ctime>

#include "logfile.h"
#include "threads.h"
#include "threadsafe_db.h"
#include "queuemanager.h"

#include "apimessage.h" // from NTLP
#include "ntlp_starter.h"

#include "natfw_config.h"
#include "msg/natfw_ie.h"
#include "msg/natfw_msg.h"
#include "gistka_mapper.h"

#include <openssl/ssl.h>


using namespace protlib;

#include "gist_conf.h"
#include "protlibconf.h"

namespace protlib {
	protlibconf plibconf;
}

namespace ntlp {
// configuration class
gistconf gconf;
}


/*
 * Configuration.
 */
static const uint16 CONFIG_PORT_UDP		= 4;
static const uint16 CONFIG_PORT_TCP		= 40002;
static const uint16 CONFIG_PORT_TLS		= 40003;
static const uint16 CONFIG_PORT_SCTP		= 0;

// The interval to send messages. If both are 0: send as fast as possible.
static const time_t CONFIG_REPEAT_SEC		= 0;	// seconds
static const long CONFIG_REPEAT_NSEC		= 0;	// nanoseconds

// The following variables can be set via the command line:
static hostaddress config_sender_addr;
static hostaddress config_receiver_addr;
static uint16 config_sender_port;
static uint16 config_receiver_port;
static const char *config_protocol_name;
static uint32 config_session_lifetime		= 30;
static int config_num_messages			= 1000;


/*
 * Internal GIST configuration, nothing to change down here.
 */
static const uint16 CONFIG_NSLP_ID		= 2;

static const message::qaddr_t CONFIG_OUTPUT_QUEUE_ADDRESS
	= message::qaddr_coordination;
static const message::qaddr_t CONFIG_INPUT_QUEUE_ADDRESS
	= message::qaddr_api_2;
static FastQueue *input_queue;

static ThreadStarter<ntlp::NTLPStarter, ntlp::NTLPStarterParam> *ntlp_starter;


static natfw::session_id sid;			// the session_id to use


logfile commonlog("natfwd.log", natfw::natfw_config::USE_COLOURS);
logfile &protlib::log::DefaultLog(commonlog);


/**
 * Called exactly once before the first thread executes main_loop.
 */
static void startup() {
	using namespace ntlp;
	using namespace natfw;

	/*
	 * Initialize libraries.
	 */
	tsdb::init();
	SSL_library_init();
	OpenSSL_add_ssl_algorithms();
	SSL_load_error_strings();
	NATFW_IEManager::register_known_ies();

	natfw_config conf;
	

	// create the global configuration parameter repository 
	conf.repository_init();

	// register all NATFW configuration parameters at the registry
	conf.setRepository();

	// register all GIST configuration parameters at the registry
	ntlp::gconf.setRepository();

	// register all protlib configuration parameters at the registry
	plibconf.setRepository();


	ILog("test_stress", "starting GIST threads ...");

	/*
	 * Start the GIST thread.
	 */
	NTLPStarterParam param;

	ntlp_starter = new ThreadStarter<NTLPStarter, NTLPStarterParam>(1, param);
	ntlp_starter->start_processing();

	/*
	 * Register our input queue with the queue manager.
	 */
	input_queue = new FastQueue();
	QueueManager::instance()->register_queue(
		input_queue, CONFIG_INPUT_QUEUE_ADDRESS);

	/*
	 * Register the queue created above with the NTLP. The NTLP will then
	 * append all NATFW messages it receives to our input queue.
	 */
	ntlp::APIMsg *api_msg = new ntlp::APIMsg();
	api_msg->set_source(CONFIG_INPUT_QUEUE_ADDRESS);
	api_msg->set_register(CONFIG_NSLP_ID, 0); // NSLPID, RAO

	/*
	 * TODO: We have no way to find out if the NTLP thread is up and has
	 * already registered an input queue. Because of this, we try to send
	 * our registration message until there is a queue to accept it.
	 */
	bool success;
	do {
		success = api_msg->send_to(CONFIG_OUTPUT_QUEUE_ADDRESS);
		sleep(1);
	}
	while ( ! success );
	assert( success );


	DLog("test_stress", "GIST threads started");
}


static ntlp::APIMsg *create_message(
		hostaddress source_addr, port_t source_port,
		hostaddress dest_addr, port_t dest_port,
		const char *proto_name) {

	using namespace ntlp;
	using namespace natfw;

	static uint32 msg_sequence_number = 0;
	static gistka_mapper mapper;

	mri_pathcoupled *mri_pc = new mri_pathcoupled(
		source_addr, 32,			// source + prefix
		dest_addr, 32,				// destination + prefix
		true					// direction: downstream
	);
	mri_pc->set_protocol(proto_name);
	mri_pc->set_sourceport(source_port);
	mri_pc->set_destport(dest_port);

	natfw_create *create = new natfw_create();
	create->set_session_lifetime(config_session_lifetime);
	create->set_extended_flow_info(extended_flow_info::ra_allow, 0);
	create->set_msg_sequence_number(msg_sequence_number++);

	ntlp_msg *msg = new ntlp_msg(
		session_id(),
		create,
		mri_pc,
		0				// No SII handle
	);

	return mapper.create_api_msg(msg);
}


static void parse_commandline(int argc, char *argv[]) {
	using namespace std;

	string usage("usage: test_stress2 [-l lifetime] [-n num_messages]\n"
			"             ds_ip dr_ip ds_port dr_port protocol\n");

	/*
	 * Parse command line options.
	 */
	while ( true ) {
		int c = getopt(argc, argv, "l:n:");

		if ( c == -1 )
			break;

		switch ( c ) {
			case 'l':
				config_session_lifetime = atoi(optarg);
				break;
			case 'n':
				config_num_messages = atoi(optarg);
				break;
			default:
				cerr << usage;
				exit(1);
		}
	}

	argv += optind;
	argc -= optind;

	/*
	 * Only non-option arguments are left. The first is in argv[0].
	 */
	if ( argc < 5 || config_sender_addr.set_ip(argv[0]) == false
			|| config_receiver_addr.set_ip(argv[1]) == false ) {
		cerr << usage;
		exit(1);
	}

	config_sender_port = atoi(argv[2]);
	config_receiver_port = atoi(argv[3]);
	config_protocol_name = argv[4];
}


int main(int argc, char *argv[]) {

	parse_commandline(argc, argv);
	startup();

	struct timespec ts = { CONFIG_REPEAT_SEC, CONFIG_REPEAT_NSEC };

	std::cout << config_num_messages << " messages will be sent ...\n";

	for (int i = 0; i < config_num_messages; i++) {
		//std::cout << "sending message #" << i << std::endl;

		ntlp::APIMsg *msg = create_message(
			config_sender_addr, config_sender_port,
			config_receiver_addr, config_receiver_port,
			"tcp"
		);
		bool success = msg->send_to(CONFIG_OUTPUT_QUEUE_ADDRESS);
		assert( success );

		// discard everything from the input queue to save memory
		message *in_msg;
		while ( (in_msg=input_queue->dequeue(false)) != NULL )
			delete in_msg;

		if ( CONFIG_REPEAT_SEC > 0 || CONFIG_REPEAT_NSEC > 0 )
			nanosleep(&ts, NULL);
	}

	std::cout << config_num_messages << " messages sent." << std::endl;
	std::cout << "End of stress test. "
		<< "Sleeping until the process is killed." << std::endl;

	while ( true )
		sleep(1);

	exit(0);
}

// EOF
