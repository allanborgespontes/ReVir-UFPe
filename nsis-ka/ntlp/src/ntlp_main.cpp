/// ----------------------------------------*- mode: C++; -*--
/// @file ntlp_main.cpp
/// The NTLP main program (option parsing etc.)
/// ----------------------------------------------------------
/// $Id: ntlp_main.cpp 6149 2011-05-18 12:54:22Z bless $
/// $HeadURL: https://svn.tm.kit.edu/nsis/ntlp/trunk/src/ntlp_main.cpp $
// ===========================================================
//                      
// Copyright (C) 2005-2010, all rights reserved by
// - Institute of Telematics, Karlsruhe Institute of Technology
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
/** @addtogroup ntlp NTLP Instance
 * @{
 */

/** @file
 * This is the main NTLP Instance, being extended during development. 
 * 
 */

#include <unistd.h>
#include <signal.h>

#include <iostream>
#include <string>
#include <sstream>
#include <fstream>

#ifndef _GNU_SOURCE
  #define _GNU_SOURCE	// for getopt_long()
#endif
#include <getopt.h>

#include "timer.h"
#include "timer_module.h"
#include "query.h"
#include "response.h"
#include "confirm.h"
#include "hello.h"
#include "ntlp_error.h"
#include "data.h"
#include "routingtable.h"
#include "tp_over_udp.h"
#include "tp_over_tcp.h"
#include "tp_over_tls_tcp.h"
#include "tp_queryencap.h"
#include "signalingmodule_ntlp.h"
#include "logfile.h"
#include "queuemanager.h"
#include "ntlp_statemodule.h"
#include "threadsafe_db.h"
#include "apimessage.h"
#include "flowinfo.h"

#include "ntlp_global_constants.h"
#include "ntlp_starter.h"

#include "gist_conf.h"
#include "protlibconf.h"
#include "configfile.h"

#include "GISTConsole.h"


#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/evp.h>

using namespace ntlp;
using namespace protlib;
using namespace protlib::log;

logfile commonlog;
protlib::log::logfile& protlib::log::DefaultLog(commonlog);


bool done = false;
bool running = false;
bool startapimsgchecker = false;

pthread_t apimsgreceivethread;

Thread* global_ntlpthread_p= 0;

namespace protlib {
        protlibconf plibconf;
}

namespace ntlp {
// configuration class
gistconf gconf;
}

/// config array
map<std::string, std::string> config;

bool stopped= false;

uint16 echo_nslpid= 0;

static
void sig(int signo)
{
  if ((signo == SIGTERM) || (signo == SIGINT) ||
      (signo == SIGQUIT) || (signo == SIGSEGV))
  {
    switch (signo)
    {
      case SIGSEGV:
        cerr << "Segmentation fault, aborting." << endl;
        exit(1);
        break;
      default:
        if (!stopped)
        {
          cerr << "User abort, stopping GIST." << endl;
          stopped= true;
        } // end if not stopped
    } // end switch
  } // end if signo
} // end signal handler


void
init_signals()
{
  signal(SIGTERM, sig);
  signal(SIGINT, sig);
  signal(SIGQUIT, sig);
  signal(SIGSEGV, sig);
} // end init_signals

void
restore_signals()
{
  signal(SIGTERM, SIG_DFL);
  signal(SIGINT, SIG_DFL);
  signal(SIGQUIT, SIG_DFL);
  signal(SIGSEGV, SIG_DFL);
} // end restore_signals




// Helper functions for config parsing

int StringToInt(std::string stringValue)
{
    std::stringstream ssStream(stringValue);
    int iReturn;
    ssStream >> iReturn;

    return iReturn;
}

std::string IntToString(int iValue)
{
    std::stringstream ssStream;
    ssStream << iValue;
    return ssStream.str();
} 


double StringToDouble(std::string stringValue)
{
    std::stringstream ssStream(stringValue);
    double iReturn;
    ssStream >> iReturn;

    return iReturn;
}


bool StringToBool(std::string stringValue)
{
  if (stringValue == "on" || stringValue == "yes" || stringValue == "true" || stringValue == "1")
    return true;
  else
    return false;
}



/// print the usage help
static void printhelp() {

    std::cout
	<< endl
	<< "GISTka v" << gist_releasestring << endl << endl
	<< "usage: gist [options]" << endl << endl

	<< "options:" << endl

	<< "  -h" << endl
	<< "  --help                        "
	<< "  display this help text" << endl

	<< "  --" << gconf.parname(gistconf_conffilename) << " <filename> "
	<< "\t\t" << gconf.getDescription(gistconf_conffilename) << endl

	<< "  --" << gconf.parname(gistconf_localaddrv4) << " <addr> "
	<< "\t\t" << gconf.getDescription(gistconf_localaddrv4) << endl

	<< "  --" << gconf.parname(gistconf_localaddrv6) << " <addr> "
	<< "\t\t" << gconf.getDescription(gistconf_localaddrv6) << endl

	<< "  --" << gconf.parname(gistconf_udpport) << " <port> "
	<< "\t\t" << gconf.getDescription(gistconf_udpport) << endl

	<< "  --" << gconf.parname(gistconf_tcpport) << " <port> "
	<< "\t\t" << gconf.getDescription(gistconf_tcpport) << endl

	<< "  --" << gconf.parname(gistconf_tlsport) << " <port> "
	<< "\t\t" << gconf.getDescription(gistconf_tlsport) << endl

	<< "  --" << gconf.parname(gistconf_retrylimit) << " <time> "
	<< "\t\t" << gconf.getDescription(gistconf_retrylimit) << endl

	<< "  --" << gconf.parname(gistconf_retryperiod) << " <time> "
	<< "\t\t" << gconf.getDescription(gistconf_retryperiod) << endl

	<< "  --" << gconf.parname(gistconf_retryfactor) << " <real> "
	<< "\t\t" << gconf.getDescription(gistconf_retryfactor) << endl

	<< "  --" << gconf.parname(gistconf_ma_hold_time) << " <time> "
	<< "\t\t" << gconf.getDescription(gistconf_ma_hold_time) << endl

	<< "  --" << gconf.parname(gistconf_refresh_limit) << " <time> "
	<< "\t\t" << gconf.getDescription(gistconf_refresh_limit) << endl

	<< "  --" << gconf.parname(gistconf_rs_validity_time) << " <time> "
	<< "\t\t" << gconf.getDescription(gistconf_rs_validity_time) << endl

	<< "  --" << gconf.parname(gistconf_secrets_count) << " <number> "
	<< "\t\t" << gconf.getDescription(gistconf_secrets_count) << endl

	<< "  --" << gconf.parname(gistconf_secrets_length) << " <number> "
	<< "\t\t" << gconf.getDescription(gistconf_secrets_length) << endl

	<< "  --" << gconf.parname(gistconf_secrets_refreshtime) << " <time> "
	<< "\t\t" << gconf.getDescription(gistconf_secrets_refreshtime) << endl

	<< "  --" << gconf.parname(gistconf_delayedstate) << " <on/off> "
	<< "\t\t" << gconf.getDescription(gistconf_delayedstate) << endl

	<< "  --" << gconf.parname(gistconf_confirmrequired) << " <on/off> "
	<< "\t\t" << gconf.getDescription(gistconf_confirmrequired) << endl

	<< "  --" << gconf.parname(gistconf_senddatainquery) << " <on/off> "
	<< "\t\t" << gconf.getDescription(gistconf_senddatainquery) << endl

	<< "  --" << gconf.parname(gistconf_reqhelloecho) << " <on/off> "
	<< "\t\t" << gconf.getDescription(gistconf_reqhelloecho) << endl

	<< "  --" << gconf.parname(gistconf_advertise_sctp) << " <on/off> "
	<< "\t\t" << gconf.getDescription(gistconf_advertise_sctp) << endl

	<< "  --" << gconf.parname(gistconf_debug_tp) << " <on/off> "
	<< "\t\t" << gconf.getDescription(gistconf_debug_tp) << endl

	<< "  --" << gconf.parname(gistconf_intercept_cmd) << " <filename> "
	<< "\t\t" << gconf.getDescription(gistconf_intercept_cmd) << endl

	<< "  --echo <nslpid>               "
	<< "  make this node listen to RAO <nslpid>, " << endl
	<< "  NSLPID <nslpid> and accept connections on " << endl
	<< "  it (for testing)" << endl;
}



/**
 * Parse command line options.
 *
 * @return true if the parsing was successful, and false otherwise
 */
static bool read_cmdline(int argc, char* argv[]) 
{
/*
 * A list of all valid command line options.
 */
  char short_opts[] = "h";
  struct option long_opts[] = {
	{ "help",			0, NULL, 'h' },
	{ gconf.parname(gistconf_conffilename).c_str(),    1, NULL, 'c' },
	{ gconf.parname(gistconf_delayedstate).c_str(),       1, NULL, 'l' },
	{ gconf.parname(gistconf_senddatainquery).c_str(), 1, NULL, 'D' },
	{ "echo",                                                   1, NULL, 'e' },
	{ gconf.parname(gistconf_confirmrequired).c_str(), 1, NULL, 'C' },
	{ gconf.parname(gistconf_reqhelloecho).c_str(),    1, NULL, 'H' },
	{ gconf.parname(gistconf_localaddrv4).c_str(),	    1, NULL, 'a' },
	{ gconf.parname(gistconf_localaddrv6).c_str(),	    1, NULL, 'A' },
	{ gconf.parname(gistconf_udpport).c_str(),	    1, NULL, 'u' },
	{ gconf.parname(gistconf_tcpport).c_str(),	    1, NULL, 't' },
	{ gconf.parname(gistconf_tlsport).c_str(),	    1, NULL, 'T' },
	{ gconf.parname(gistconf_sctpport).c_str(),	    1, NULL, 'S' },
	{ gconf.parname(gistconf_retrylimit).c_str(),	    1, NULL, 'r' },
	{ gconf.parname(gistconf_retryperiod).c_str(),	    1, NULL, 'p' },
	{ gconf.parname(gistconf_retryfactor).c_str(),	    1, NULL, 'f' },
	{ gconf.parname(gistconf_rs_validity_time).c_str(),1, NULL, 'L' },
	{ gconf.parname(gistconf_refresh_limit).c_str(),    1, NULL, 'R' },
	{ gconf.parname(gistconf_ma_hold_time).c_str(),    1, NULL, 'm' },
	{ gconf.parname(gistconf_secrets_count).c_str(),   1, NULL, 's' },
	{ gconf.parname(gistconf_secrets_length).c_str(),  1, NULL, 'n' },
	{ gconf.parname(gistconf_secrets_refreshtime).c_str(),	1, NULL, 'o' },
	{ gconf.parname(gistconf_debug_tp).c_str(),		0, NULL, 'd' },
	{ NULL,				0, NULL, 0 },
  };

  int c, opt_index;

  startapimsgchecker = false;

  while ( (c = getopt_long(argc, argv,
			   short_opts, long_opts, &opt_index)) != -1 ) 
  {
    switch ( c ) {
      case 'h':	printhelp(); exit(0); break;
      case 'c': config[gconf.parname(gistconf_conffilename)] = optarg; 
	      gconf.setpar(gistconf_conffilename, string(optarg));
	        break;
      case 'l':	config[gconf.parname(gistconf_delayedstate)] = optarg; break;
      case 'e':	echo_nslpid = StringToInt(optarg);
	startapimsgchecker = true;
	break;
      case 'd':	config[gconf.parname(gistconf_debug_tp)] = optarg; break;
      case 'C':	config[gconf.parname(gistconf_confirmrequired)] = optarg; break;
      case 'H':	config[gconf.parname(gistconf_reqhelloecho)] = optarg; break;
      case 'D':	config[gconf.parname(gistconf_senddatainquery)] = optarg; break;
      case 'a':	config[gconf.parname(gistconf_localaddrv4)] = optarg; break;
      case 'A':	config[gconf.parname(gistconf_localaddrv6)] = optarg; break;
      case 'u':	config[gconf.parname(gistconf_udpport)] = optarg; break;
      case 't':	config[gconf.parname(gistconf_tcpport)] = optarg; break;
      case 'T':	config[gconf.parname(gistconf_tlsport)] = optarg; break;
      case 'S':	config[gconf.parname(gistconf_sctpport)] = optarg; break;
      case 'r':	config[gconf.parname(gistconf_retrylimit)] = optarg; break;
      case 'p':	config[gconf.parname(gistconf_retryperiod)] = optarg; break;
      case 'f':	config[gconf.parname(gistconf_retryfactor)] = optarg; break;
      case 'L':	config[gconf.parname(gistconf_rs_validity_time)] = optarg; break;
      case 'R':	config[gconf.parname(gistconf_refresh_limit)] = optarg; break;
      case 'm':	config[gconf.parname(gistconf_ma_hold_time)] = optarg; break;
      case 's':	config[gconf.parname(gistconf_secrets_count)] = optarg; break;
      case 'n':	config[gconf.parname(gistconf_secrets_length)] = optarg; break;
      case 'o':	config[gconf.parname(gistconf_secrets_refreshtime)] = optarg; break;
	
    default:	std::cerr << "Unknown option '" << (char) c << "'" << std::endl;
	return false;
	break;
    }
  } // end while
  return true;
}


const char *
SSLerrmessage(void)
{
  unsigned long	errcode;
  const char	*errreason;
  static char	errbuf[32];
    
  errcode = ERR_get_error();
  if (errcode == 0)
    return "No SSL error reported";
  errreason = ERR_reason_error_string(errcode);
  
  if (errreason != NULL)
    return errreason;
  snprintf(errbuf, sizeof(errbuf), "SSL error code %lu", errcode);
  return errbuf;
}


/// Thread to emulate a NSLP on NSLPID echo_nslpid which enables to do manual tests for connection setup
void* 
apimsgchecker(void *p) 
{
  message* msg = NULL;
  APIMsg* apimsg = NULL;
  FastQueue* fq = QueueManager::instance()->get_queue(message::qaddr_api_1);
  if (!fq)
  {
    Log(ERROR_LOG,LOG_CRIT,"apimsgchecker","apimsgchecker cannot attach to qaddr_api_1 message queue");
    return NULL;
  } // end if not fq

  DLog("apimsgchecker", "Registering at GIST instance");
  // register echo_nslpid 
  APIMsg* regmsg = new APIMsg;
  regmsg->set_register(echo_nslpid, echo_nslpid);
  regmsg->set_source(message::qaddr_api_1);
  regmsg->send_to(message::qaddr_coordination);

  running = true;
  Log(EVENT_LOG, LOG_NORMAL, "apimsgchecker","apimsgchecker started");
  while (!done) 
  {
    msg = fq->dequeue_timedwait(5000);
    if (msg) {
      Log(EVENT_LOG, LOG_NORMAL, "apimsgchecker", "apimsgchecker received a message");
			
      apimsg = dynamic_cast<APIMsg*>(msg);
      if (apimsg) {
	// evaluate api message
	//cout<<"request ID: "<<apimsg->get_id()<<endl;
	cout<<"subtype: "<<apimsg->get_subtype_name()<<endl;
	switch (apimsg->get_subtype()) 
	{
	  case APIMsg::RecvMessage: 
	  {
	    // we give always order to establish peering

	    if (apimsg->get_adjacency_check()) {
	      APIMsg* mesg = new APIMsg;
	      mesg->set_source(message::qaddr_api_1);
	      mesg->set_recvmessageanswer(apimsg->get_nslpid(), apimsg->get_sessionid()->copy(), apimsg->get_mri()->copy(), NULL, APIMsg::directive_establish);
	      mesg->send_to(message::qaddr_coordination);
	    }

	    break;
	  }
	  default:
	    break;
	} // end switch

	cout.flush();
	apimsg = NULL;
      } 
      else 
      {
	Log(ERROR_LOG, LOG_ALERT, "apimsgchecker", "sigmsgchecker cannot cast message of type "
	    << msg->get_type_name() << " to APIMsg");
      } // end if sigmsg
      delete msg;
      msg = NULL;
    } // end if msg

    sched_yield();
  } // end while not done
  fq->cleanup();
  Log(EVENT_LOG, LOG_ALERT, "apimsgchecker", "apimsgchecker stopped");
  running = false;
  return NULL;
} // end sigmsgchecker



/// main method for manual testing
void 
test(const char* logname) 
{
  hostaddresslist_t& ntlpv4addr= gconf.getparref< hostaddresslist_t >(gistconf_localaddrv4);
  hostaddresslist_t& ntlpv6addr= gconf.getparref< hostaddresslist_t >(gistconf_localaddrv6);

  // check whether all addresses are really pure IPv4
  if (ntlpv4addr.empty() == false) {
    hostaddresslist_t::iterator it= ntlpv4addr.begin();
    while (it != ntlpv4addr.end()) 
    {
      if ( !it->is_ipv4() )
      {
	WLog("main", "Detected non IPv4 address, removing " << *it );
	it= ntlpv4addr.erase(it);
      }
      else
	it++;
    } // end while
  }

  // check whether all addresses are really pure IPv6
  if (ntlpv6addr.empty() == false) {
    hostaddresslist_t::iterator it= ntlpv6addr.begin();
    while (it != ntlpv6addr.end()) 
    {
      if ( !it->is_ipv6() )
      {
	WLog("main", "Detected non-IPv6 address, removing " << *it );
	it= ntlpv6addr.erase(it);
      }
      else
	it++;
    } // end while
  }

    // this will set default values
    NTLPStarterParam ntlppar;
    AddressList *addresses = new AddressList();

    if (ntlpv4addr.size() == 0 && ntlpv6addr.size() == 0) {
        addresses->add_host_prop(NULL, AddressList::ConfiguredAddr_P);
    }

    if (!ntlpv4addr.empty())
    {
	    hostaddresslist_t::iterator it= ntlpv4addr.begin();
	    while (it != ntlpv4addr.end()) 
	    {
		    netaddress na(*it);
		    na.set_pref_len(32);
		    addresses->add_property(na, AddressList::ConfiguredAddr_P);
		    it++;
	    } // end while
    }

    if (!ntlpv6addr.empty())
    {
	    hostaddresslist_t::iterator it= ntlpv6addr.begin();
	    
	    while (it != ntlpv6addr.end()) 
	    {
		    netaddress na(*it);
		    na.set_pref_len(128);
		    addresses->add_property(na, AddressList::ConfiguredAddr_P);
		    it++;
	    } // end while
    }
    
    // set specified IP addresses
    ntlppar.addresses = addresses;
    
    // fill the parameters from configfile or command line
    // (parameters given by command line will override these)
    if (config[gconf.parname(gistconf_udpport)].empty() == false)  gconf.setpar(gistconf_udpport, (uint16) StringToInt(config[gconf.parname(gistconf_udpport)]));
    if (config[gconf.parname(gistconf_tcpport)].empty() == false)  gconf.setpar(gistconf_tcpport, (uint16) StringToInt(config[gconf.parname(gistconf_tcpport)]));
    if (config[gconf.parname(gistconf_sctpport)].empty() == false) gconf.setpar(gistconf_sctpport, (uint16) StringToInt(config[gconf.parname(gistconf_sctpport)]));
    if (config[gconf.parname(gistconf_tlsport)].empty() == false)  gconf.setpar(gistconf_tlsport, (uint16) StringToInt(config[gconf.parname(gistconf_tlsport)]));
    
    if (config[gconf.parname(gistconf_retrylimit)].empty() == false) gconf.setpar(gistconf_retrylimit, (uint32) StringToInt(config[ gconf.parname(gistconf_retrylimit)]));
    if (config[gconf.parname(gistconf_retryperiod)].empty() == false) gconf.setpar(gistconf_retryperiod, (uint32) StringToInt(config[ gconf.parname(gistconf_retryperiod)]));
    if (config[gconf.parname(gistconf_retryfactor)].empty() == false) gconf.setpar(gistconf_retryfactor, StringToDouble(config[ gconf.parname(gistconf_retryfactor)]));
    if (config[gconf.parname(gistconf_rs_validity_time)].empty() == false) gconf.setpar(gistconf_rs_validity_time, (uint32) StringToInt(config[ gconf.parname(gistconf_rs_validity_time)]));
    if (config[gconf.parname(gistconf_refresh_limit)].empty() == false) gconf.setpar(gistconf_refresh_limit, (uint32) StringToInt(config[ gconf.parname(gistconf_refresh_limit)]));
    if (config[gconf.parname(gistconf_ma_hold_time)].empty() == false) gconf.setpar(gistconf_ma_hold_time, (uint32) StringToInt(config[ gconf.parname(gistconf_ma_hold_time)]));
    if (config[gconf.parname(gistconf_secrets_refreshtime)].empty() == false) gconf.setpar(gistconf_secrets_refreshtime, (uint32) StringToInt(config[ gconf.parname(gistconf_secrets_refreshtime)]));
    if (config[gconf.parname(gistconf_secrets_count)].empty() == false) gconf.setpar(gistconf_secrets_count, (uint32) StringToInt(config[ gconf.parname(gistconf_secrets_count)]));
    if (config[gconf.parname(gistconf_secrets_length)].empty() == false) gconf.setpar(gistconf_secrets_length, (uint16) StringToInt(config[ gconf.parname(gistconf_secrets_length)]));
    if (config[gconf.parname(gistconf_delayedstate)].empty() == false) gconf.setpar(gistconf_delayedstate, StringToBool(config[ gconf.parname(gistconf_delayedstate)]));
    if (config[gconf.parname(gistconf_senddatainquery)].empty() == false) gconf.setpar(gistconf_senddatainquery, StringToBool(config[ gconf.parname(gistconf_senddatainquery)]));
    if (config[gconf.parname(gistconf_confirmrequired)].empty() == false) gconf.setpar(gistconf_confirmrequired, StringToBool(config[ gconf.parname(gistconf_confirmrequired)]));
    if (config[gconf.parname(gistconf_reqhelloecho)].empty() == false) gconf.setpar(gistconf_reqhelloecho, StringToBool(config[ gconf.parname(gistconf_reqhelloecho)]));
    if (config[gconf.parname(gistconf_advertise_sctp)].empty() == false) gconf.setpar(gistconf_advertise_sctp, StringToBool(config[ gconf.parname(gistconf_advertise_sctp)]));
    if (config[gconf.parname(gistconf_verbose_error_responses)].empty() == false) gconf.setpar(gistconf_verbose_error_responses, StringToBool(config[ gconf.parname(gistconf_verbose_error_responses)]));
    if (config[gconf.parname(gistconf_debug_tp)].empty() == false) gconf.setpar(gistconf_debug_tp, StringToBool(config[ gconf.parname(gistconf_debug_tp)]));

#ifdef USE_FLOWINFO
    // Create a flowinfoservice thread
    FlowinfoParam fiparam;
    ThreadStarter<Flowinfo, FlowinfoParam> fithread(1, fiparam);
    fithread.start_processing();

    // record the flowinfoservice thread in the ntlp_param
    ntlppar.fi_service = fithread.get_thread_object();
#endif

    // give the parameters to NTLP Starter
    ThreadStarter<NTLPStarter,NTLPStarterParam> ntlpthread(1,ntlppar);
    global_ntlpthread_p= ntlpthread.get_thread_object();
    // NTLP Starter will start all the remaining modules/threads
    init_signals();
    ntlpthread.start_processing();

    sleep(2);

    if (startapimsgchecker) {

	FastQueue* apimsgrecv_fq= new FastQueue("apimsgchecker",true);
	QueueManager::instance()->register_queue(apimsgrecv_fq, message::qaddr_api_1);
	pthread_create(&apimsgreceivethread,NULL,apimsgchecker,NULL);
    }	


    DLog("ntlp", "Starting GISTConsole");
    
    GISTConsoleParam gistcon_par(getpid(), message::qaddr_api_2);
    ThreadStarter<GISTConsole, GISTConsoleParam> GISTconsole_thread(1, gistcon_par);
    
    GISTconsole_thread.start_processing();

    while(!stopped)
    {
      sleep(2);
    }

    // stop TPchecker
    Log(EVENT_LOG, LOG_NORMAL, "ntlp", "Stopping apimsgchecker thread");
    done = true;
    while (running) {
	sleep(1);
    } // end while not done

    GISTconsole_thread.stop_processing();
    GISTconsole_thread.wait_until_stopped();

    if (startapimsgchecker) {
      QueueManager::instance()->unregister_queue(message::qaddr_api_1);
    }

    // stop threads
    Log(EVENT_LOG, LOG_NORMAL, "ntlp", "Stopping GIST instance");
    
    ntlpthread.stop_processing();
    restore_signals();    
} // end test




///main loop
int 
main(int argc, char** argv) 
{
  ostringstream logname; 
  // construct log file name
  logname<<argv[0]<<"."<<getpid()<<".log";

  // create the global configuration parameter repository
  gconf.repository_init();

  // register all GIST configuration parameters at the registry
  gconf.setRepository();

  // register all protlib configuration parameters at the registry
  plibconf.setRepository();

  // process command line options, they should override the config file values, exit on error
  // it's not that critical since we will use default values otherwise
  if ( read_cmdline(argc, argv) == false )
	return 1;

  // read configuration from config file
  configfile read_cfgfile(configpar_repository::instance());

  // read in the config file
  read_cfgfile.load(gconf.getpar<string>(gistconf_conffilename));


  tsdb::init(true);


  // initialize OpenSSL stuff 
  SSL_library_init();
  OpenSSL_add_ssl_algorithms();
  // 2nd: load in all digests from OpenSSL
  OpenSSL_add_all_digests();
  SSL_load_error_strings();

  SSL_CTX *ssl_ctx = NULL;

  ssl_ctx=SSL_CTX_new(TLSv1_client_method());

  if (!ssl_ctx)
  {
      cerr << "could not create SSL context: " << SSLerrmessage() << endl;
  }

  cin.tie(&cout);
  // start test and catch exceptions
  try {
    test(logname.str().c_str());
  } 
  catch(ProtLibException& e) {
    cerr<<"Fatal: "<<e.getstr()<<endl;
  } 
  catch(...) {
    cerr<<"Fatal: an error occured, no info, sorry."<<endl;
  } // end try-catch

  tsdb::end();
  QueueManager::clear();

  return 0;
} // end main

//@}
