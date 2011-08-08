/// ----------------------------------------*- mode: C++; -*--
/// @file qosnslpd.cpp
/// Main QoS NSLP Instance:
/// - command line parsing
/// - starts QoS NSLP modules via qosnslp_starter
/// - starts optional modules like UDS API
/// ----------------------------------------------------------
/// $Id: qosnslpd.cpp 6287 2011-06-18 04:37:17Z bless $
/// $HeadURL: https://svn.tm.kit.edu/nsis/qos-nslp/trunk/src/qosnslpd.cpp $
// ===========================================================
//                      
// Copyright (C) 2005-2010, all rights reserved by
// - Institute of Telematics, Karlsruhe Institute of Technology
//
// More information and contact:
// https://projekte.tm.uka.de/trac/NSIS
//                      
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; version 2 of the License
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
// 02110-1301 USA.
//
// ===========================================================

#include <unistd.h> // for getopt()
#include <signal.h>
#include <sstream>

#include <openssl/ssl.h>

#include "protlibconf.h"
#include "logfile.h"
#include "tp_over_uds.h"
#include "gist_conf.h"
#include "ntlp_starter.h"
#include "configfile.h"
#include "qos_nslp_conf.h"
#include "TestConsole.h"
#include "SignalingAppl.h"
#include "QoS_NSLP_UDS_API.h"
#include "qos_nslp.h"
#include "queuemanager.h"
#include "qosnslp_starter.h"
#include "../src/pdu/ntlp_pdu.h"

#include "hmac_keyregistry.h"
using namespace nslp_auth;

#include "benchmark_journal.h"


using namespace qos_nslp;
using namespace ntlp;
using namespace protlib;
using namespace protlib::log;

namespace protlib {
	protlibconf plibconf;
}

namespace ntlp {
	// configuration class
	gistconf gconf;
}

namespace qos_nslp {
	// configuration class
	qos_nslp_conf qosnslpconf;

#ifdef BENCHMARK
benchmark_journal journal(1000000, "qosnslpd_benchmark_journal.txt"); 
#endif

}

logfile commonlog;

protlib::log::logfile& protlib::log::DefaultLog(commonlog);


// indicates whether QoS NSLP daemon is stopped
bool stopped = false;
// if true, dont read /dev/random for seed initialization
bool seedhack = false;


bool 
session_auth_check_hmac(NetMsg& msg)
{
	return nslp_auth::session_auth_object::check_hmac(msg,IE::protocol_v1, true);
}

bool 
session_auth_serialize_hmac(NetMsg& msg)
{
	return nslp_auth::session_auth_object::serialize_hmac(msg,IE::protocol_v1, false);
}

static void signal_handler(int signo)
{
	if ((signo != SIGTERM) && (signo != SIGINT) &&
	    (signo != SIGQUIT) && (signo != SIGSEGV))
		return;

	switch (signo) {
	case SIGSEGV:
		cerr << "Segmentation fault, aborting." << endl;
		exit(1);
		break;
	default:
		if (!stopped) {
			cerr << "User abort, stopping QoS NSLP." << endl;
			stopped = true;
		}
	}
}

void initSignals()
{
	signal(SIGTERM, signal_handler);
	signal(SIGINT, signal_handler);
	signal(SIGQUIT, signal_handler);
	signal(SIGSEGV, signal_handler);
}

void restoreSignals()
{
	signal(SIGTERM, SIG_DFL);
	signal(SIGINT, SIG_DFL);
	signal(SIGQUIT, SIG_DFL);
	signal(SIGSEGV, SIG_DFL);
}


void parse_commandline(int argc, char *argv[]) {
	std::string usage("usage: qosnslpd [-s] -c config_file\n");

	string configfilename;
	/*
	 * Parse command line options.
	 */
	while ( true ) {
		int c = getopt(argc, argv, "sc:");

		if ( c == -1 )
			break;

		switch ( c ) {

		case 'c':
		  configfilename = optarg;
		  break;

		case 's':
		  seedhack= true;
		  break;

		default:
		  std::cerr << usage;
		  exit(1);

		}
	}

	if ( configfilename == "" ) {
		std::cerr << usage;
		exit(1);
	}
	else
	{
		gconf.getparref<string>(gistconf_conffilename)= configfilename;
	}

	argv += optind;
	argc -= optind;

	/*
	 * Only non-option arguments are left. The first is in argv[0].
	 */
	if ( argc > 0 ) {
		std::cerr << usage;
		exit(1);
	}
}


/** Main function of the QoS NSLP.  */
int main(int argc, char** argv)
{
	bool done = false;

	ostringstream logname;
	// construct log file name
	logname << argv[0] << "." << getpid() << ".log";

	cin.tie(&cout);
	// start test and catch exceptions
	try {
		// initalize netdb
		tsdb::init(true);

		SSL_library_init();
		OpenSSL_add_ssl_algorithms();
		SSL_load_error_strings();

		qos_nslp::register_ies();
		AUTH_ATTR_IEManager::register_known_ies();

		FastQueue* timerchecker_fq = new FastQueue("timerchecker", true);
		QueueManager::instance()->register_queue(timerchecker_fq, message::qaddr_qos_nslp_timerprocessing);
		
		// create the global configuration parameter repository 
		qosnslpconf.repository_init();

		// register all QoS NSLP configuration parameters at the registry
		qosnslpconf.setRepository();

		// register all GIST configuration parameters at the registry
		gconf.setRepository();

		// register all protlib configuration parameters at the registry
		plibconf.setRepository();

		// check for configuration file name parameter from command line 
		parse_commandline(argc, argv);

		// read all config values from config file
		configfile cfgfile(configpar_repository::instance());
		try {
		  cfgfile.load(gconf.getparref<string>(gistconf_conffilename));
		}
		catch(configParException& cfgerr)
		{
		  ERRLog("qosnslpd", "Error occurred while reading the configuration file: " << cfgerr.what());
		  cerr << cfgerr.what() << endl << "Exiting." << endl;
		  return 1;
		}
 		catch(configfile_error& cfgerr)
		{
		  ERRLog("qosnslpd", "Error occurred while reading the configuration file: " << cfgerr.what());
		  cerr << cfgerr.what() << endl << "Exiting." << endl;
		  return 1;
		}

		if (qosnslpconf.getpar<bool>(qosnslpconf_sauth_hmac_verify))
		{
		  // set verification function pointers for NTLP verification
		  ntlp_pdu::check_hmac= session_auth_check_hmac;
		  ntlp_pdu::serialize_hmac = session_auth_serialize_hmac;
		}

		// three pre-shared keys
		const string psk1("12345678901234567890123456789012");
		const string psk2("abcdefghijklmnopqrstuvwxyz-.,+%$");
		const string psk3("xabcdefghijklmnopqrstuvwxyz.,+$x");
		nslp_auth::key_id_t pskey_id1= 0x1;
		nslp_auth::key_id_t pskey_id2= 0x2;
		nslp_auth::key_id_t pskey_id3= 0x1a1a1b1bL;
		// register keys
		nslp_auth::sign_key_registry.storeKey(pskey_id1, psk1.c_str(), psk1.length(), 2);
		nslp_auth::sign_key_registry.storeKey(pskey_id2, psk2.c_str(), psk2.length(), 2);
		nslp_auth::sign_key_registry.storeKey(pskey_id3, psk3.c_str(), psk3.length(), 2);


		AddressList *addresses = new AddressList();

		hostaddresslist_t& ntlpv4addr= gconf.getparref< hostaddresslist_t >(gistconf_localaddrv4);
		hostaddresslist_t& ntlpv6addr= gconf.getparref< hostaddresslist_t >(gistconf_localaddrv6);

		if (ntlpv4addr.size() == 0 && ntlpv6addr.size() == 0) {
			addresses->add_host_prop(NULL, AddressList::ConfiguredAddr_P);
			addresses->add_host_prop(NULL, AddressList::ConfiguredAddr_P);
			addresses->add_host_prop(NULL, AddressList::ConfiguredAddr_P);
		}

		// fill in configured IPv4 addresses
		if (ntlpv4addr.size() != 0) {
			hostaddresslist_t::const_iterator it;
			for (it = ntlpv4addr.begin(); it != ntlpv4addr.end(); it++) {
				netaddress na((*it));
				addresses->add_property(na);
			}
		}

		// fill in configured IPv6 addresses
		if (ntlpv6addr.size() != 0) {
			hostaddresslist_t::const_iterator it;
			for (it = ntlpv6addr.begin(); it != ntlpv6addr.end(); it++) {
				netaddress na((*it));
				addresses->add_property(na);
			}
		}

		// MOBILITY (mobility extension): configure net prefix of home network
		const netaddress& na= gconf.getparref<netaddress>(gistconf_home_netprefix);
		if (!na.is_ip_unspec()) {
			addresses->add_property(na, AddressList::HomeNet_P);
		}

		// MOBILITY: configure home address
		const hostaddress& homeaddr= gconf.getparref< hostaddress >(gistconf_home_address);
		if (!homeaddr.is_ip_unspec())
		{
			const netaddress na(homeaddr);
			addresses->add_property(na, AddressList::HomeAddr_P);
			addresses->add_property(na, AddressList::ConfiguredAddr_P);
		}

		// MOBILITY: care-of interfaces
		const string& coa_iface= gconf.getparref< string >(gistconf_coa_interfaces);
		if (!coa_iface.empty())
		{
			std::stringstream in(coa_iface);

			while (in) {
				std::string token;
				in >> token;
				
				addresses->ignore_locals();
				addresses->by_interface(true);
				// XXX: memleak
				addresses->add_interface(strdup(token.c_str()));
			}
		}

		// MOBILITY: home agent address
		const hostaddress& homeagent_addr= gconf.getparref< hostaddress >(gistconf_homeagent_address);
		if (!homeagent_addr.is_ip_unspec())
		{
			const netaddress na(homeagent_addr);
			addresses->add_property(na, AddressList::HAAddr_P);
			addresses->add_property(na, AddressList::ConfiguredAddr_P);
		}

		// MOBILITY: home agent address
		const hostaddress& alt_homeagent_addr= gconf.getparref< hostaddress >(gistconf_homeagent_address_alt);
		if (!alt_homeagent_addr.is_ip_unspec())
		{
			const netaddress na(alt_homeagent_addr);
			addresses->add_property(na, AddressList::AltHAAddr_P);
			addresses->add_property(na, AddressList::ConfiguredAddr_P);
		}

		QoSNSLPStarterParam qospar;

		qospar.addresses = addresses;
		  AddressList::addrlist_t *alist = addresses->get_addrs(
                                     AddressList::ConfiguredAddr_P);
		  if (alist != 0) {
			  DLog("qosnslpd", "Address list: ");
			  AddressList::addrlist_t::iterator it;
			  for (it = alist->begin(); it != alist->end(); it++)
				  DLog("qosnslpd", "            " << *it);
		  }
		  delete alist;


#ifdef USE_FLOWINFO
		// Create a flowinfoservice thread
		FlowinfoParam fiparam;
		ThreadStarter<Flowinfo, FlowinfoParam> fithread(1, fiparam);
		fithread.start_processing();

		// record the flowinfoservice thread in the ntlp_param
		qospar.fi_service = fithread.get_thread_object();
#else
		qospar.fi_service = NULL;
#endif

		// override parameters given from command line 
		// read_cmdline();

		ntlp::NTLPStarterParam ntlppar;
		qospar.ntlppar = &ntlppar;
		// propagate configures addresses to GIST
		ntlppar.addresses= addresses;

		initSignals();

		// give the parameters to QoS NSLP Starter
		ThreadStarter<QoSNSLPStarter, QoSNSLPStarterParam> qosthread(1, qospar);
		qosthread.start_processing();

		// start UDS SERVER and connect it to qaddr_qos_appl_signaling
		// kill a left-over socket file
		unlink(qos_nslp_unix_domain_socket);
		TPoverUDSParam udspar(uds_api_header_length,
				UDS_API::decode_api_header_clen,
				qos_nslp_unix_domain_socket, 
				true, 1, false, message::qaddr_uds_appl_qos, message::qaddr_qos_appl_signaling);

		ThreadStarter<TPoverUDS, TPoverUDSParam> udstpthread(1, udspar);

		TP* udsproto = udstpthread.get_thread_object();

		UDS_API_Param uds_api_par(udsproto, 
				message::qaddr_qos_appl_signaling, // clientqueue
				message::qaddr_appl_qos_signaling, // internalqueue
				4);
		ThreadStarter<UDS_API, UDS_API_Param> appl_thread(1, uds_api_par);

		/*
		SignalingApplParam sig_appl_par(4);
		ThreadStarter<SignalingAppl, SignalingApplParam> appl_thread(1, sig_appl_par);
		*/

		TestConsoleParam testcon_par(4, "localhost", "30023");
		ThreadStarter<TestConsole, TestConsoleParam> appl_thread2(1, testcon_par);

		FastQueue* applmsgrecv_fq = new FastQueue("applmsgchecker",true);
		QueueManager::instance()->register_queue(applmsgrecv_fq, message::qaddr_qos_appl_signaling);

		appl_thread.start_processing();
		udstpthread.start_processing();
		appl_thread2.start_processing();

		EVLog("QoS NSLP Daemon", "All threads started");

		// to garantee real good random numbers - do srand() 
		// with really different number
		// /proc/sys/kernel/random/uuid -> man urandom
		ifstream urandfile;
		const char *const randomdevname = "/dev/random";
		urandfile.open(randomdevname);
		if (!urandfile || seedhack) {
		  if (!seedhack)
		    WLog("QoS NSLP Daemon", "Cannot open " << randomdevname);
		  else
		    WLog("QoS NSLP Daemon", "Using workaround seed init");

			char ac[80];

			if (gethostname(ac, sizeof(ac)) == -1)
				cerr << "Error when getting local host name." << endl;

			uint32 help_rand_number = 0;
			for (uint32 i = 0; i < sizeof(ac); i++)
				help_rand_number += (uint32) ac[i];

			srand((unsigned) time(NULL) + help_rand_number);
		} 
		else {
		  EVLog("QoS NSLP Daemon", "Reading seed from /dev/random");
			unsigned long seed = 0;
			urandfile >> seed;
			urandfile.close();
			srand(seed);
		}

		EVLog("QoS NSLP Daemon", "Waiting for a signal to stop");
		// may be stopped by signal
		while (!stopped)
			sleep(1);

		EVLog("QoS NSLP Daemon", "Stopping Threads");
		// stop threads
		EVLog("QoS NSLP Daemon", "Stopping Testconsole");
		appl_thread2.stop_processing();
		EVLog("QoS NSLP Daemon", "Stopping API");
		appl_thread.stop_processing();
		udstpthread.stop_processing();
		qosthread.stop_processing();
		done = true;

		while (!done)
			sleep(1);
#ifdef BENCHMARK
		journal.write_journal();
#endif
		EVLog("QoS NSLP Daemon", "Stopped processing of all threads");
		appl_thread2.abort_processing();
		appl_thread.abort_processing();
		udstpthread.abort_processing();
		qosthread.abort_processing();

		nslp_auth::sign_key_registry.deleteKey(pskey_id1);
		nslp_auth::sign_key_registry.deleteKey(pskey_id2);
		nslp_auth::sign_key_registry.deleteKey(pskey_id3);

		QueueManager::instance()->unregister_queue(message::qaddr_qos_nslp_timerprocessing);
		QueueManager::instance()->unregister_queue(message::qaddr_qos_appl_signaling);

		// destroy QueueManager
		QueueManager::clear();

		tsdb::end();

	} catch(ProtLibException& e) {
		cerr << "Fatal: " << e.getstr() << endl;
	} catch(...) {
		cerr << "Fatal: an error occured, no info, sorry." << endl;
	} // end try-catch

	restoreSignals();
	return 0;
} // end main

