/// ----------------------------------------*- mode: C++; -*--
/// @file ntlp_starter.cpp
/// The NTLP/GIST starter module
/// ----------------------------------------------------------
/// $Id: ntlp_starter.cpp 6195 2011-05-25 13:31:46Z bless $
/// $HeadURL: https://svn.tm.kit.edu/nsis/ntlp/trunk/src/ntlp_starter.cpp $
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
/** @ingroup ntlp
 * @{
 */

/** @file
 * This is the NTLP Starter Module which fires up all necessary framework and NTLP modules. 
 * 
 */

#include "ntlp_starter.h"

#include <unistd.h>
#include <iostream>
#include <string>
#include <sstream>

#include <netdb.h>

#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/evp.h>

#include "timer.h"
#include "timer_module.h"

// need all PDUs since they are registered here
#include "query.h"
#include "response.h"
#include "confirm.h"
#include "hello.h"
#include "ntlp_error.h"
#include "data.h"

#include "routingtable.h"

// transport modules
#include "tp_over_udp.h"
#include "tp_over_tcp.h"
#include "tp_over_tls_tcp.h"
#include "tp_over_uds.h"
#ifdef _USE_SCTP
#include "tp_over_sctp.h"
#endif
#include "tp_queryencap.h"

#include "apimessage.h"
#include "apiwrapper.h"
#include "logfile.h"
#include "queuemanager.h"
#include "signalingmodule_ntlp.h"
#include "threadsafe_db.h"

#include "ntlp_statemodule.h"
#include "mri_le.h"
#include "mri_est.h"

#include "gist_conf.h"

#ifdef USE_AHO
#include "network_notification_module.h"
#endif


namespace ntlp {
    using namespace protlib;
    using namespace protlib::log;

  // global handle to access state module object (initialized by constructor)
  NTLPStarter* global_ntlpstarterthread_p= 0;


/** Initialize Thread base class and GIST protocol. */
NTLPStarter::NTLPStarter(const NTLPStarterParam& p)
  : Thread(p),
    param(p),
    statemodule_p(0)
{
  global_ntlpstarterthread_p= this;
}


NTLPStarter::~NTLPStarter() {
  DLog(param.name, "Destroying NTLPStarter object");
  global_ntlpstarterthread_p= 0;
}


void 
NTLPStarter::main_loop(uint32 n) 
{
#ifndef _USE_SCTP
  // sctp cannot be used if not compiled
  gconf.setpar(gistconf_advertise_sctp, false);
#endif
  cout << color[blue] << gist_name << " v" << gist_releasestring 
       << " ($Rev: 6195 $ - development build) (C) 2005-2011 Karlsruhe Institute of Technology" << color[off] << endl;
  cout << endl;
  cout << "Parameters:" << endl;
  cout << "-----------------------------------------------------------------" << endl;
  cout << gconf.to_string();
  cout << "-----------------------------------------------------------------" << endl;
  cout << endl;
  
  // Register IEs
  
  NTLP_IE *tmp;
  tmp = new query;
  DLog(param.name, "Startup phase 1: Creating and registering stencil objects");
  
  NTLP_IEManager::instance()->register_ie(tmp);
  tmp = new mri_pathcoupled;
  NTLP_IEManager::instance()->register_ie(tmp);
  tmp = new mri_looseend;
  NTLP_IEManager::instance()->register_ie(tmp);
  tmp = new mri_explicitsigtarget;
  NTLP_IEManager::instance()->register_ie(tmp);
  tmp = new nattraversal;
  NTLP_IEManager::instance()->register_ie(tmp);
  tmp = new ntlp::sessionid;
  NTLP_IEManager::instance()->register_ie(tmp);
  tmp = new nslpdata;
  NTLP_IEManager::instance()->register_ie(tmp);
  tmp = new nli;
  NTLP_IEManager::instance()->register_ie(tmp);
  tmp = new querycookie;
  NTLP_IEManager::instance()->register_ie(tmp);
  tmp = new stack_conf_data;
  NTLP_IEManager::instance()->register_ie(tmp);
  tmp = new stackprop;
  NTLP_IEManager::instance()->register_ie(tmp);
  tmp = new response;
  NTLP_IEManager::instance()->register_ie(tmp);
  tmp = new confirm;
  NTLP_IEManager::instance()->register_ie(tmp);
  tmp = new hello;
  NTLP_IEManager::instance()->register_ie(tmp);
  tmp = new error;
  NTLP_IEManager::instance()->register_ie(tmp); 
  tmp = new helloid;
  NTLP_IEManager::instance()->register_ie(tmp); 
  tmp = new respcookie;
  NTLP_IEManager::instance()->register_ie(tmp); 
  tmp = new data;
  NTLP_IEManager::instance()->register_ie(tmp);
  tmp = new errorobject;
  NTLP_IEManager::instance()->register_ie(tmp);
  
  SignalingNTLPParam::protocol_map_t pm;
  
  DLog(param.name, "Startup phase 2: Starting Transport protocols");
  
#ifdef _USE_SCTP
  // start TPoverSCTP
  TPoverSCTPParam sctppar(ntlp_pdu::common_header_length,
			  ntlp_pdu::decode_common_header_ntlpv1_clen,
			  gconf.getpar<uint16>(gistconf_sctpport));
  
  ThreadStarter<TPoverSCTP,TPoverSCTPParam> sctptpthread(1,sctppar);
  
  TP* sctpproto = sctptpthread.get_thread_object();
  protocol_t sctpprotoid = sctpproto->get_underlying_protocol();
  pm[sctpprotoid] = sctpproto;
#endif
  
  // start TPoverTCP
  TPoverTCPParam tcppar(ntlp_pdu::common_header_length,
			ntlp_pdu::decode_common_header_ntlpv1_clen,
			(port_t) gconf.getpar<uint16>(gistconf_tcpport),"TPoverTCP", 5000UL,
			gconf.getpar<bool>(gistconf_debug_tp));
  
  
  ThreadStarter<TPoverTCP,TPoverTCPParam> tcptpthread(1,tcppar);
  
  TP* tcpproto = tcptpthread.get_thread_object();
  protocol_t protoid = tcpproto->get_underlying_protocol();
  pm[protoid] = tcpproto;
  
  
  // start TPoverTLS
  TPoverTLS_TCPParam tlspar(gconf.getpar<string>(gistconf_tls_client_cert).c_str(),
			    gconf.getpar<string>(gistconf_tls_client_privkey).c_str(),
			    gconf.getpar<string>(gistconf_tls_cacert).c_str(),
			    ntlp_pdu::common_header_length,
			    ntlp_pdu::decode_common_header_ntlpv1_clen,
			    gconf.getpar<uint16>(gistconf_tlsport),"TPoverTLS_TCP", 5000UL,
			    gconf.getpar<bool>(gistconf_debug_tp));
  
  
  ThreadStarter<TPoverTLS_TCP,TPoverTLS_TCPParam> tlstpthread(1,tlspar);
  
  TP* tlsproto = tlstpthread.get_thread_object();
  protoid = prot_tls_tcp; //tlsproto->get_underlying_protocol();
  pm[protoid] = tlsproto;
  
  tlstpthread.start_processing();
  
  
  // UDP
  TPoverUDPParam udppar(ntlp_pdu::common_header_length,
			ntlp_pdu::decode_common_header_ntlpv1_clen,
			gconf.getpar<uint16>(gistconf_udpport),
			"TPoverUDP",
			5000UL, 
			gconf.getpar<bool>(gistconf_debug_tp));
  
  ThreadStarter<TPoverUDP,TPoverUDPParam> udptpthread(1,udppar);
  
  TPoverUDP* udpproto = udptpthread.get_thread_object();
  protoid = udpproto->get_underlying_protocol();
  pm[protoid] = static_cast<TP*>(udpproto);
  
  
  DLog(param.name, "TPqueryEncap UDP interceptor/injector"); 
  
  // initial fill of RAOVec. We push "0" onto it
  vector<uint32> raovec;
  raovec.push_back(0);
  
  // start TPqueryEncap
  TPqueryEncapParam qepar(ntlp_pdu::common_header_length,
			  ntlp_pdu::decode_common_header_ntlpv1_clen,
			  (port_t) gconf.getpar<uint16>(gistconf_udpport),
			  raovec,
			  udpproto,
			  gconf.getparref<bool>(gistconf_strict_rao),
			  GIST_magic_number,
			  5000UL, 
			  gconf.getpar<bool>(gistconf_debug_tp));
  
  ThreadStarter<TPqueryEncap,TPqueryEncapParam> qetpthread(1,qepar);
  
  TP* qeproto = qetpthread.get_thread_object();
  protoid = qeproto->get_underlying_protocol();
  pm[protoid] = qeproto;
  
  
  DLog(param.name, "ports used: "
       << gconf.getpar<uint16>(gistconf_udpport) << " (UDP), "
       << gconf.getpar<uint16>(gistconf_tcpport) << " (TCP), "
       << gconf.getpar<uint16>(gistconf_sctpport) << " (SCTP), "
       << gconf.getpar<uint16>(gistconf_tlsport) << " (TLS over TCP)");
  
  
  udptpthread.start_processing();
  tcptpthread.start_processing();
  
  if (gconf.getpar<bool>(gistconf_dontstartqe) == false)
	  qetpthread.start_processing();

#ifdef _USE_SCTP
  sctptpthread.start_processing();
#endif  

  DLog(param.name, "Startup phase 3: Initiating local instance data");
  
  // NSLPtable is used by both Signaling Module AND StateModule
  NSLPtable nslptable;
  
  // init routingtable
  routingtable rt;

  /**
      Initialization of StateModule:
  
  - instantiate routingtable object (takes no initializers)
  - instantiate secretmanager object (secret table size, secretsize [bit])
  - instantiate StatemoduleParam
  
  @ Routing Table Object
  @ Secretmanager Object
  @ nslptable hashmap
  @ RAO vector (for adding RAOs)
  @ refresh period for secrets in seconds
  @ Queue Listen time in msec (framework param)
  @ Well-Known GIST port (UDP)
  @ Delayed State setup: Require ALWAYS a confirm to responses AND wait with state setup until Confirm arrives
  @ Confirm required: Require ALWAYS a confirm to responses
  - ThreadStarter<Statemodule,StatemoduleParam> smthread(1,smpar);
  
  */
  
  secretmanager secrets(gconf.getpar<uint32>(gistconf_secrets_count), 
			gconf.getpar<uint16>(gistconf_secrets_length));
   
  // PI
  
  EVLog(param.name, "Peer Identity initialized: " <<  color[magenta] << gconf.getparref<peer_identity>(gistconf_peerid) << color[off]); 
  
  // ================================================================================================
  // configure local IP adresses
  // ================================================================================================

  AddressList *addresses= NULL;
  if (param.addresses == NULL) {
	  addresses = new AddressList();
	  addresses->add_host_prop(NULL, AddressList::ConfiguredAddr_P);
  }
  else {
	  addresses = param.addresses;
  }

  AddressList::addrlist_t *alist = addresses->get_addrs(
                                     AddressList::ConfiguredAddr_P);
  if (alist != 0) {
    DLog(param.name, "Address list: ");
    AddressList::addrlist_t::iterator it;
    for (it = alist->begin(); it != alist->end(); it++)
      DLog(param.name, "            " << *it);
  }
  delete alist;


#ifdef USE_FLOWINFO
  if (param.fi_service == NULL) {
    WLog(param.name, "No Flowinfoservice provided in ntlp parameter!");
    abort();
  }
#endif  
  
  DLog(param.name,
       "NLI initialisation, IP TTL is set to 0 (unspecified)");

  uint32_t ttl = 0;

  netaddress na;
  nli nli_tmpl(ttl, gconf.getpar<uint32>(gistconf_rs_validity_time), &gconf.getparref<peer_identity>(gistconf_peerid), na);
  
  DLog(param.name, "Startup phase 4: Starting Processing Module");
  
  
  // =====================================================================
  // Start StateModule 
  // =====================================================================
  
  StatemoduleParam smpar(rt, 
			 nli_tmpl,
			 *addresses,
			 param.fi_service,
			 secrets, 
			 nslptable, 
			 raovec, 
			 10 // default sleep time
			);

  ThreadStarter<Statemodule,StatemoduleParam> smthread(1,smpar);
  statemodule_p= smthread.get_thread_object();

  // APIWrapper needs an IPv4 and an IPv6 Address, we cannot take those
  // pointers above, as they are in static memory

  DLog(param.name,
       "Startup phase 5: Starting UNIX Domain socket interface");

  // start as SERVER and connect it to qaddr_api_0 (which is the APIWrapper!)

  // kill a left-over socket file
  unlink(gist_unix_domain_socket_defaultpath);
  TPoverUDSParam udspar(APIWrapper::api_header_length,
			APIWrapper::decode_api_header_clen,
			gist_unix_domain_socket_defaultpath, 
			true,5000UL, false, message::qaddr_tp_over_uds, message::qaddr_api_0);
	
  ThreadStarter<TPoverUDS,TPoverUDSParam> udstpthread(1,udspar);
	
  TP* udsproto = udstpthread.get_thread_object();
	
  udstpthread.start_processing();

  // Start API Wrapper module (!!! Run with 'instantestablish' to enable full Goettingen GIST API compliance !!!)

  DLog(param.name, "Startup phase 6: Starting Unix Domain Socket API translator");

	
  APIWrapperParam apipar = APIWrapperParam(udsproto, 
					   message::qaddr_coordination,
					   *addresses,
					   true);
  ThreadStarter<APIWrapper,APIWrapperParam> apithread(1,apipar);

  DLog(param.name, "Startup phase 7: Starting Signaling Module");

	
  SignalingNTLPParam sigpar(pm,  nli_tmpl, *addresses, nslptable, gconf.getpar<uint16>(gistconf_udpport), param.fi_service);
  ThreadStarter<SignalingNTLP,SignalingNTLPParam> signaling(1,sigpar);	
	
  DLog(param.name, "Startup phase 8: Starting Timer Module, Signaling Module, StateManager, API");

  //TimerMsg* msg = NULL;
  // what for are these?!?!?
  // message::id_t saved_timer = 0, first_timer = 0;
  TimerModuleParam tmpar(3,false,false,false);
  ThreadStarter<TimerModule,TimerModuleParam> tmod(1,tmpar);
  tmod.start_processing();
  signaling.start_processing();  
  smthread.start_processing();
  apithread.start_processing();
	
  sleep(2);

  DLog(param.name, "Startup phase 9: Enabling Packet Interception");

  if ( gconf.getpar<string>(gistconf_intercept_cmd).length() == 0 )
    DLog(param.name, "NOT enabling packet interception, as requested");
  else {
    if ( system(gconf.getpar<string>(gistconf_intercept_cmd).c_str()) != 0 ) {
      ERRLog(param.name,
	     "Packet interception could not be enabled. Aborting");
      abort();
    }
    else {
      EVLog(param.name, "successfully called the intercept script. "
	    "Note: Make the script only writable for root!");
    }
  }

#ifdef USE_AHO
  DLog(param.name, "Startup phase 10: Starting NetworkNotification Module");

  NetworkNotificationModuleParam nwnpar(gist_nwn_unix_domain_socket_defaultpath);
  ThreadStarter<NetworkNotificationModule, NetworkNotificationModuleParam> nwnthread(1, nwnpar);
  nwnthread.start_processing();
#endif


  ILog(param.name, color[green] << "Startup completed" << color[off]);

  while (get_state()==STATE_RUN) {

    sleep(param.sleep_time);
  }

  ILog(param.name, color[green] << "Shutting down ... " << color[off]);

  // flush the debug timestamp file

  DLog(param.name, "Shutdown phase 1: Asking Modules to stop.");

#ifdef USE_AHO
  nwnthread.stop_processing();
#endif
  udptpthread.stop_processing();
  udstpthread.stop_processing();
  tcptpthread.stop_processing();
  tlstpthread.stop_processing();
  if (gconf.getpar<bool>(gistconf_dontstartqe) == false)
	  qetpthread.stop_processing();
#ifdef _USE_SCTP
  sctptpthread.stop_processing();
#endif
  signaling.stop_processing();
  smthread.stop_processing();
  tmod.stop_processing();
  apithread.stop_processing();

  DLog(param.name, "Shutdown phase 2: Forcing down leftover Modules");

#ifdef USE_AHO
  nwnthread.abort_processing(true);
#endif
  udptpthread.abort_processing(true);
  udstpthread.abort_processing(true);
  tcptpthread.abort_processing(true);
  tlstpthread.abort_processing();
  if (gconf.getpar<bool>(gistconf_dontstartqe) == false)
	  qetpthread.abort_processing(true);
#ifdef _USE_SCTP
  sctptpthread.abort_processing();
#endif
  signaling.abort_processing(true);
  smthread.abort_processing(true);
  tmod.abort_processing(true);
  apithread.abort_processing(true);

  DLog(param.name, "Shutdown phase 3: Waiting for threads to stop");

#ifdef USE_AHO
  nwnthread.wait_until_stopped();
#endif
  udptpthread.wait_until_stopped();
  udstpthread.wait_until_stopped();
  tcptpthread.wait_until_stopped();
  tlstpthread.wait_until_stopped();
  if (gconf.getpar<bool>(gistconf_dontstartqe) == false)
	  qetpthread.wait_until_stopped();
#ifdef _USE_SCTP
  sctptpthread.wait_until_stopped();
#endif
  signaling.wait_until_stopped();
  smthread.wait_until_stopped();
  tmod.wait_until_stopped();
  apithread.wait_until_stopped();

  DLog(param.name, "Shutdown phase 4: Destroying Protocolmap");
	
  pm.clear();

  DLog(param.name, "Shutdown phase 5: Cleaning Up");

}


} // namespace ntlp

//@}

