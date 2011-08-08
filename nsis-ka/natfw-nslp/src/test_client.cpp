/// ----------------------------------------*- mode: C++; -*--
/// @file test_client.cpp
/// An interactive test client to imitate an NSIS Initiator or Responder.
/// ----------------------------------------------------------
/// $Id: test_client.cpp 6161 2011-05-18 20:02:40Z bless $
/// $HeadURL: https://svn.tm.kit.edu/nsis/natfw-nslp/trunk/src/test_client.cpp $
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
#include <unistd.h>
//#include <csignal>

#include "logfile.h"

#include "natfw_config.h"
#include "natfw_daemon.h"
#include "events.h"

#include "protlibconf.h"
#include "gist_conf.h"
#include "configfile.h"

namespace protlib {
	protlibconf plibconf;
}

namespace ntlp {
// configuration class
gistconf gconf;
}

using namespace protlib;
using namespace protlib::log;
using namespace natfw;
using namespace ntlp;


logfile commonlog("natfwd.log", natfw_config::USE_COLOURS);
logfile &protlib::log::DefaultLog(commonlog);


static natfw_daemon *natfwd;
static session_id sid;


static void tg_create(const hostaddress &ds_addr, const hostaddress &dr_addr,
		uint16 ds_port, uint16 dr_port, uint8 protocol,
		uint32 session_lifetime, bool proxy) {

	using namespace std;

	cout << "tg_CREATE(lifetime=" << session_lifetime
		<< "sec, proxy=" << proxy << ") for a data stream" << endl;

	cout << "  from     : " << ds_addr << ", port " << ds_port << endl;
	cout << "  to       : " << dr_addr << ", port " << dr_port << endl;
	cout << "  protocol : " << int(protocol) << endl;

	FastQueue ret;
	event *e = new api_create_event(
		ds_addr, dr_addr, ds_port, dr_port, protocol,
		std::list<uint8>(), session_lifetime, proxy, &ret);

	NatFwEventMsg *msg = new NatFwEventMsg(session_id(), e);
	natfwd->get_fqueue()->enqueue(msg);

	message *ret_msg = ret.dequeue_timedwait(1000);
	NatFwEventMsg *r = dynamic_cast<NatFwEventMsg *>(ret_msg);

	sid = r->get_session_id();
	delete r;

	cout << "session created, id = " << sid << "\n";
}


static void tg_ext(fw_policy_rule::action_t action,
		const hostaddress &ds_addr, uint16 ds_port,
		const hostaddress &dr_addr, uint16 dr_port, uint8 protocol,
		uint32 session_lifetime, bool ds_known, bool proxy) {

	using namespace std;

	cout << "tg_EXT(lifetime=" << session_lifetime
		<< "sec, proxy=" << proxy << ") for a data stream" << endl;

	cout << "  from     : " << ds_addr
		<< ", port " << ds_port << (ds_known ? "" : " (SDA) ") << endl;
	cout << "  to       : " << dr_addr << ", port " << dr_port << endl;
	cout << "  protocol : " << int(protocol) << endl;

	FastQueue ret;
	event *e = new api_ext_event(action,
			dr_addr, 32, dr_port, ds_addr, 32, ds_port, protocol,
			session_lifetime, ds_known, proxy, &ret);

	NatFwEventMsg *msg = new NatFwEventMsg(session_id(), e);
	natfwd->get_fqueue()->enqueue(msg);

	message *ret_msg = ret.dequeue_timedwait(10000);
	NatFwEventMsg *r = dynamic_cast<NatFwEventMsg *>(ret_msg);

	sid = r->get_session_id();
	delete r;

	cout << "session created, id = " << sid << "\n";
}


static void tg_teardown(session_id id) {
	cout << "tg_TEARDOWN(session=" << sid << ")" << endl;

	event *e = new api_teardown_event(new session_id(id));

	NatFwEventMsg *msg = new NatFwEventMsg(session_id(), e);
	natfwd->get_fqueue()->enqueue(msg);
}


static std::string config_filename;
static hostaddress sender_addr;
static hostaddress receiver_addr;
static uint16 sender_port = 0;
static uint16 receiver_port = 0;
static uint8 protocol = 0;
static uint32 lifetime = 30;
static bool proxy_mode = false;


void parse_commandline(int argc, char *argv[]) {
	using namespace std;

	string usage("usage: test_client [-p] [-l lifetime] -c config_file "
			"ds_ip dr_ip [[[ds_port] dr_port] protocol]\n");

	/*
	 * Parse command line options.
	 */
	while ( true ) {
		int c = getopt(argc, argv, "l:c:p");

		if ( c == -1 )
			break;

		switch ( c ) {
			case 'c':
				config_filename = optarg;
				break;
			case 'l':
				lifetime = atoi(optarg);
				break;
			case 'p':
				proxy_mode = true;
				break;
			default:
				cerr << usage;
				exit(1);
		}
	}

	if ( config_filename == "" ) {
		cerr << usage;
		exit(1);
	}

	argv += optind;
	argc -= optind;

	/*
	 * Only non-option arguments are left. The first is in argv[0].
	 */
	if ( argc < 2 || argc > 5 || sender_addr.set_ip(argv[0]) == false
			|| receiver_addr.set_ip(argv[1]) == false ) {
		cerr << usage;
		exit(1);
	}

	if ( argc > 2 )
		sender_port = atoi(argv[2]);

	if ( argc > 3 )
		receiver_port = atoi(argv[3]);

	if ( argc > 4 )
		protocol = atoi(argv[4]);
}


int main(int argc, char *argv[]) {
	using namespace std;

	hostaddress source;

	parse_commandline(argc, argv);
	init_framework();

	natfw_config conf;
	
	// create the global configuration parameter repository 
	conf.repository_init();

	// register all NATFW configuration parameters at the registry
	conf.setRepository();

	// register all GIST configuration parameters at the registry
	ntlp::gconf.setRepository();

	// register all protlib configuration parameters at the registry
	plibconf.setRepository();

	// read all config values from config file
	configfile cfgfile(configpar_repository::instance());

	try {
		cfgfile.load(conf.getparref<string>(natfwconf_conffilename));
	}
	catch(configParException& cfgerr)
	{
		ERRLog("natfwd", "Error occurred while reading the configuration file: " << cfgerr.what());
		cerr << cfgerr.what() << endl << "Exiting." << endl;
		return 1;
	}

	/*
	 * Start the NATFW daemon thread. It will in turn start the other
	 * threads it requires.
	 */
	natfw_daemon_param param("natfwd", conf);
	ThreadStarter<natfw_daemon, natfw_daemon_param> starter(1, param);
	
	// returns after all threads have been started
	starter.start_processing();

	natfwd = starter.get_thread_object();


	// initialize the global session_id, the only one we store
	uint128 raw_id(0, 0, 0, 0);
	sid = session_id(raw_id);

	cout << "config file        : " << config_filename << endl;
	cout << "data sender        : " << sender_addr
		<< ", port " << sender_port << endl;
	cout << "data receiver      : " << receiver_addr
		<< ", port " << receiver_port << endl;
	cout << "IP protocol        : " << int(protocol) << endl;
	cout << "proxy mode         : " << proxy_mode << endl;
	cout << "session lifetime   : " << lifetime << " sec" << endl;

	int input;
	do {
		cout << endl;
		cout << "       Current session: " << sid << endl;
		cout << endl;
		cout << "  (1)  tg_CREATE" << endl;
		cout << "  (2)  tg_EXT, DS' address is known" << endl;
		cout << "  (3)  tg_EXT, DS' address is NOT known" << endl;
		cout << "  (4)  tg_TEARDOWN for current session " << endl;
		cout << "  (5)  tg_EXT, DENY" << endl;
		cout << "  (6)  quit" << endl;

		cout << "Input: ";
		cin >> input;
		cout << endl;

		if ( input == 1 )
			tg_create(sender_addr, receiver_addr,
				sender_port, receiver_port, protocol,
				lifetime, proxy_mode);
		else if ( input == 2 )
			tg_ext(fw_policy_rule::ACTION_ALLOW,
				sender_addr, sender_port,
				receiver_addr, receiver_port, protocol,
				lifetime, true, proxy_mode);
		else if ( input == 3 )
			tg_ext(fw_policy_rule::ACTION_ALLOW,
				sender_addr, sender_port,
				receiver_addr, receiver_port, protocol,
				lifetime, false, proxy_mode);
		else if ( input == 4 )
			tg_teardown(sid);
		else if ( input == 5 )
			tg_ext(fw_policy_rule::ACTION_DENY,
				sender_addr, sender_port,
				receiver_addr, receiver_port, protocol,
				lifetime, true, proxy_mode); // TODO: false?

		cin.clear();
	}
	while ( input != 6 );

	cout << "Stopping NATFWD thread ..." << endl;

	// shutdown natfwd thread
	starter.stop_processing();
	starter.wait_until_stopped();

	cleanup_framework();
}

// EOF
