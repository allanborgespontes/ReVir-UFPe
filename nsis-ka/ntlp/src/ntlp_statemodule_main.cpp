/// ----------------------------------------*- mode: C++; -*--
/// @file ntlp_statemodule_main.cpp
/// GIST state module - implements GIST main protocol logic
/// ----------------------------------------------------------
/// $Id: ntlp_statemodule_main.cpp 5842 2011-01-25 13:51:08Z roehricht $
/// $HeadURL: https://svn.tm.kit.edu/nsis/ntlp/trunk/src/ntlp_statemodule_main.cpp $
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
/**
 * Statemodule
 *
 * This module implements protocol logic as described in NSIS protocol draft.
 * 
 * Messages from network are processed and routing state is managed.
 *
 * Messages from API are also processed here and timer handling is done here, too.
 *
 */

#include "ntlp_statemodule.h"
#include "queuemanager.h"
#include "secretmanager.h"
#include "timer_module.h"
#include "signalingmodule_ntlp.h"
#include "gist_exceptions.h"

#include "ntlp_global_constants.h"
#include "ntlp_error.h"
#ifdef USE_FLOWINFO
#include "flowinfo.h"
#endif
#ifdef USE_AHO
#include "network_notification_module.h"
#include "nwn_uds_msg.h"
#include "nslpdata.h"
#endif

#include "rfc5014_hack.h"

#include <sstream>
#include <list>

#include "gist_conf.h"

using namespace protlib;
using namespace protlib::log;



namespace ntlp {

/** 
 * @defgroup statemodule State Machine Module
 * @ingroup ntlp
 * @{
 */

using namespace protlib;

/***** struct StatemoduleParam *****/

StatemoduleParam::StatemoduleParam(routingtable& rt, 
				   nli &nli_tmpl,
				   AddressList &addresses,
				   Flowinfo* fi_service,
				   secretmanager& secrets, 
				   NSLPtable& nslptable, 
				   vector<uint32>& raovec, 
				   uint32 sleep)
  : ThreadParam(sleep,"GIST Processing",2),       ///< common thread parameter
    qaddr_ext(message::qaddr_coordination),          ///< queue for messages to coordinator
    qaddr_int(message::qaddr_coordination_internal), ///< queue for self messages
    rt(rt),                                ///< routing table
    nli_tmpl(nli_tmpl),
    addresses(addresses),
    fi_service(fi_service),
    secrets(secrets),
    nslptable(nslptable),
    raovec(raovec),
    timermsgsource(message::qaddr_coordination),
    requesttimererrors(true) 
{

  qenc = prot_query_encap;
  udp  = tsdb::getprotobyname("udp");
  tcp  = tsdb::getprotobyname("tcp");
  sctp = tsdb::getprotobyname("sctp");
    	    
  rt.set_own_ma_hold_time(gconf.getpar<uint32>(gistconf_ma_hold_time));

  // show RAOs that will be intercepted
  ostringstream os;
  for (unsigned int i = 0; i < raovec.size(); i++) {
    os << "|" << raovec[i];
  }
    
  EVLog("GIST Processing", "RAOs to intercept: " << os.str().c_str());
    
} // end constructor

/***** class Statemodule *****/

Statemodule::Statemodule(const StatemoduleParam& p)
  : Thread(p), 
    cap(gconf.getpar<uint16>(gistconf_udpport), gconf.getpar<uint16>(gistconf_tcpport), gconf.getpar<uint16>(gistconf_tlsport), gconf.getpar<uint16>(gistconf_sctpport)),
    param(p),
    external_fq(new FastQueue(message::get_qaddr_name(p.qaddr_ext),true))
{
  // register queues
  QueueManager::instance()->register_queue(get_fqueue(),p.qaddr_int);
  QueueManager::instance()->register_queue(external_fq,p.qaddr_ext);
  DLog(param.name, "Created " << p.name << " object");	
} // end constructor


Statemodule::~Statemodule() 
{
  DLog(param.name, "Destroying " << param.name << " object");
  QueueManager::instance()->unregister_queue(param.qaddr_int);
  QueueManager::instance()->unregister_queue(param.qaddr_ext);
  
  if (external_fq)
  {
    delete external_fq;
    external_fq= 0;
  }
} // end destructor



void 
Statemodule::main_loop(uint32 nr) 
{
	ILog(param.name, "Starting " << param.name << " module thread #" << nr <<", "); 

	// Initially starting timer for secret refresh, only Task #1 will do this!!
	if (nr == 1) {
		TimerMsg* msg = new TimerMsg(message::qaddr_coordination, true);

		uint32* timer_type = new uint32;
		*timer_type = refresh_secrets;

		msg->start_relative(gconf.getpar<uint32>(gistconf_secrets_refreshtime), 0, (void*) timer_type, NULL);

		msg->send_to(message::qaddr_timer);
		ILog(param.name, "Secrets Refresh Timer activated, refreshing every " << gconf.getpar<uint32>(gistconf_secrets_refreshtime) << " seconds");     
	}

	// process message queue
	process_queue(nr);

	ILog(param.name, "Module thread #" << nr << " stopped");

} // end main_loop




/**
 *  called to process message
 */
void Statemodule::handleInternalMessage(message *msg)
{
    switch (msg->get_type()) 
    {
    // internal message from signaling (message from IP network)
	case message::type_signaling:
	 // -----------------------
	 {
	  SignalingMsgNTLP* sigmsg= dynamic_cast<SignalingMsgNTLP*>(msg);
	  if (sigmsg)
	  {
	    // =====================================================
	    process_sig_msg(sigmsg); ///< process signaling messages
	    // =====================================================
	  }
	  else 
	  {
	    Log(ERROR_LOG,LOG_ALERT, param.name, 
		"Cannot cast message from " 
		<< msg->get_qaddr_name() << " of " << msg->get_type_name() << " to SignalingMsgNTLP");
	    
	    delete msg;
	  } // end if sigmsg
	 } break;

	// internal message from API
	case message::type_API:
	 // -----------------------
	 { 
	  APIMsg* apimsg= dynamic_cast<APIMsg*>(msg);
	  if (apimsg)
	  {
	    // =====================================================
	    process_api_msg(apimsg); ///< process API messages
	    // =====================================================
	  }
	  else 
	  {
	    Log(ERROR_LOG,LOG_ALERT, param.name, 
		"Cannot cast message from " 
		<< msg->get_qaddr_name() << " of " << msg->get_type_name() << " to APIMsg");
	    
	    delete msg;
	  } // end if sigmsg
	 } break;

	// internal message from timer module
	case message::type_timer:
	 // -----------------------
	 {
	  TimerMsg* timermsg= dynamic_cast<TimerMsg*>(msg);
	  if (timermsg)
	  {
	    // =====================================================
	    process_timer_msg(timermsg); ///< process timer messages
	    // =====================================================
	  }
	  else 
	  {
	    Log(ERROR_LOG,LOG_ALERT, param.name, 
		"Cannot cast message from " 
		<< msg->get_qaddr_name() << " of " << msg->get_type_name() << " to TimerMsg");
	    
	    delete msg;
	  } // end if timermsg
	 } break;

#ifdef USE_FLOWINFO
	case message::type_mobility:
	{
	  // -----------------------
	  MobilityMsg *mobmsg= dynamic_cast<MobilityMsg*>(msg);
	  if (mobmsg)
	  {
	    // =====================================================
	    process_mobility_msg(mobmsg); ///< process timer messages
	    // =====================================================
	  }
	  else 
	  {
	    Log(ERROR_LOG,LOG_ALERT, param.name, 
		"Cannot cast message from " 
		<< msg->get_qaddr_name() << " of " << msg->get_type_name() << " to MobilityMsg");
	    
	    delete msg;
	  } // end if mobmsg
	}
	break;
#endif

#ifdef USE_AHO
	case message::type_handover:
	{
	  // -----------------------
	  HandoverMsg *homsg = dynamic_cast<HandoverMsg*>(msg);
	  if (homsg)
	  {
	    // =====================================================
	    process_handover_msg(homsg);
	    // =====================================================
	  }
	  else 
	  {
	    Log(ERROR_LOG,LOG_ALERT, param.name, 
		"Cannot cast message from " 
		<< msg->get_qaddr_name() << " of " << msg->get_type_name() << " to HandoverMsg");
	    
	    delete msg;
	  } // end if homsg
	}
	break;
#endif

        // everything else is handled by default
	default:
	  ERRLog(param.name, "Received a message from " << msg->get_qaddr_name() 
		             << " that cannot be processed here: " << msg->get_type_name());
				
	  delete msg;
    } // end switch message type
}


/**
 * called if Timeout happened by dequeue_timedwait()
 */
void Statemodule::handleTimeout()
{
	//nothing to do
}

/**
 * process internal message queue, i.e. messages from other modules
 * @param number -- thread instance number
 */
void 
Statemodule::process_queue(uint32 number) 
{
  // get internal message queue depending on thread instance number
  FastQueue* fq = NULL;
  if (number==1) 
    fq= QueueManager::instance()->get_queue(param.qaddr_ext); // message::qaddr_statemodule
  else 
    fq= QueueManager::instance()->get_queue(param.qaddr_int); // message::qaddr_statemodule_internal

  if (!fq) 
  {
    ERRCLog(param.name, "Cannot find input queue");
    
    return;
  } // end if not fq

  message* msg = NULL;
  // maximum wait period (in ms) at queue
  uint32 wait= param.sleep_time;
  // while in running state
  // wait for incoming internal messages
  while (get_state()==STATE_RUN) 
  {
    msg = fq->dequeue_timedwait(wait);
    if (msg) 
    {
    	handleInternalMessage(msg);
    }  // end if msg
	else handleTimeout();
	
    sched_yield();
  } // end while running
} // end process_queue





/**
 * process various timer messages from timer module
 * @param timermsg -- the message to process
 */
void 
Statemodule::process_timer_msg(TimerMsg* timermsg) 
{
  routingkey* rk=NULL;

  uint32* type = static_cast<uint32*>(timermsg->get_param1());

  if (type) 
  {    
    switch (*type)  
    {
      // ExpireRNode
      case expire_rnode:   
	rk = static_cast<routingkey*>(timermsg->get_param2());
	to_expire_rnode(rk, timermsg);
	break;
      
      // NoConfirm
      case noconfirm:
	rk = static_cast<routingkey*>(timermsg->get_param2());
	to_no_confirm(rk, timermsg);
	break;
      
      // NoResponse
      case noresponse:
	rk = static_cast<routingkey*>(timermsg->get_param2());
	to_no_response(rk, timermsg);
	break;
      
      // InactiveQNode
      case inactive_qnode:
	rk = static_cast<routingkey*>(timermsg->get_param2());
	to_inactive_qnode(rk, timermsg);
	break; 

      // RefreshQNode
      case refresh_qnode:
	rk = static_cast<routingkey*>(timermsg->get_param2());
	to_refresh_qnode(rk, timermsg);
	break;
 
      // SendHello
      case send_hello:
	param.rt.process_timer_msg(timermsg);
	break;

      // NoActivity
      case no_activity:
	param.rt.process_timer_msg(timermsg);
	break;

      // NoHello
      case no_hello:
	param.rt.process_timer_msg(timermsg);
	break;

      // QueuePoll
      case queue_poll:
	rk = static_cast<routingkey*>(timermsg->get_param2());
	to_queue_poll(rk, timermsg);
	break;
	
      // SecretsRefresh
      case refresh_secrets:
	to_secrets_refresh();
	break;

      // Stopped
      case none:
	ERRCLog(param.name, "This timer was started with the type 'none', which is an error!");
	break;

      // Default
      default: 
	DLog(param.name, "Unknown timer type set?");
	break;
    } // end switch
  } // endif type available 
  else 
  { 
    ERRCLog(param.name, "Timer Message (ID:" << timermsg->get_id() << ") without type set found, presumably error message from TimerModule");
    return;
  }

  // timer messages are created and deleted here in Statemodule (or its member classes), nowhere else
  
  delete timermsg;
  delete type;
  
}


/**
 * process mobility events dispatched from the Flowinfoservice
 * @param mobmsg -- the message to process
 */
void 
Statemodule::process_mobility_msg(MobilityMsg* mobmsg) 
{
  Flowstatus *fs = mobmsg->get_flowstatus();

  switch (fs->type) {
  case Flowstatus::fs_home:
  {
    // Send a NetworkNotification to all registered NSLPs
    api_iter cur = param.nslptable.nslptable.begin();
    while (cur != param.nslptable.nslptable.end()) {
      mri* mymri = fs->orig_mri.copy();
      APIMsg* msg = new APIMsg;
	
      msg->set_source(message::qaddr_coordination);
	
      msg->set_networknotification((*cur).first, 0, mymri, APIMsg::route_changed_status_home_binding, 0);
	
      msg->send_to((*cur).second);
	
      EVLog(param.name, color[yellow] << "NetworkNotification passed to NSLP instance at address "<< (*cur).second << ": " << msg->get_error_str(APIMsg::route_changed_status_home_binding) << color[off]);

      cur++;
    }
    break;
  }
  case Flowstatus::fs_normal:
  {
    // Send a NetworkNotification to all registered NSLPs
    api_iter cur = param.nslptable.nslptable.begin();
    while (cur != param.nslptable.nslptable.end()) {
      mri* mymri = fs->orig_mri.copy();
      APIMsg* msg = new APIMsg;
	
      msg->set_source(message::qaddr_coordination);
	
      msg->set_networknotification((*cur).first, 0, mymri, APIMsg::route_changed_status_binding, 0);
	
      msg->send_to((*cur).second);
	
      EVLog(param.name, color[yellow] << "NetworkNotification passed to NSLP instance at address "<< (*cur).second << ": " << msg->get_error_str(APIMsg::route_changed_status_binding) << color[off]);

      cur++;
    }
    break;
  }
  default:
    break;
  }

  delete mobmsg;  
}

#ifdef USE_AHO
/**
 * process handover events dispatched from the unix domain socket interface
 * @param homsg -- the message to process
 */
void
Statemodule::process_handover_msg(HandoverMsg* homsg) 
{
  const struct nwn_uds_msg uds_msg = homsg->get_uds_msg();

  // Send a NetworkNotification to all registered NSLPs
  api_iter cur = param.nslptable.nslptable.begin();
  while (cur != param.nslptable.nslptable.end()) {
		APIMsg* msg = new APIMsg;
		msg->set_source(message::qaddr_coordination);

		if (uds_msg.nwn_msg_type == NWN_MSG_TYPE_AHO) {
			nslpdata *data = new nslpdata((uchar *)&uds_msg.aho_msg, sizeof(aho_msg_body));
			msg->set_networknotification(data, (*cur).first, 0, NULL,
					APIMsg::anticipated_handover_event, 0);
		}
		else if (uds_msg.nwn_msg_type == NWN_MSG_TYPE_HO) {
			nslpdata *data = new nslpdata((uchar *)&uds_msg.ho_msg, sizeof(ho_msg_body));
			msg->set_networknotification(data, (*cur).first, 0, NULL,
					APIMsg::handover_event, 0);
		}

		msg->send_to((*cur).second);

		if (uds_msg.nwn_msg_type == NWN_MSG_TYPE_AHO) {
			EVLog(param.name, color[yellow] << "NetworkNotification passed to NSLP instance at address "<< (*cur).second << ": " << msg->get_error_str(APIMsg::anticipated_handover_event) << color[off]);
		}
		else if (uds_msg.nwn_msg_type == NWN_MSG_TYPE_HO) {
			EVLog(param.name, color[yellow] << "NetworkNotification passed to NSLP instance at address "<< (*cur).second << ": " << msg->get_error_str(APIMsg::handover_event) << color[off]);
		}

		cur++;
	}

  delete homsg;  
}
#endif // USE_AHO

/**
 * Create a local NLI (take the right one from StatemoduleParam, modify RS_Validity_Time)
 * @param ip_version -- which NLI to take, the one for IPv4 or the one for IPv6
 * @param re -- the routing entry, allowed to be NULL (only used for determining validity time, if not given default is used)
 * @return freshly allocated NLI object
 */
nli* 
Statemodule::build_local_nli(uint8 ip_version, const netaddress &dest, routingentry* re) 
{
  nli *mynl;
  uint32 rs_validity_time= re ? re->get_rs_validity_time() : gconf.getpar<uint32>(gistconf_rs_validity_time);

  if (re) {
    mynl = param.nli_tmpl.copy2(*re->get_local_src());
    mynl->set_rs_validity_time(rs_validity_time);
  } else {
    uint32_t pref = IPV6_PREFER_SRC_COA;
    netaddress *src = param.addresses.get_src_addr(dest, &pref);
    mynl = param.nli_tmpl.copy2(*src);
  }


  return mynl;
}



/**
 * handle incoming error pdu
 * @param errpdu - incoming error pdu
 */

void
Statemodule::handle_error_pdu(const known_ntlp_pdu* errpdu)
{
  EVLog(param.name, color[red] << "GIST Error received:" << color[red] << *errpdu << color[off]);

  if (errpdu->get_errorobject()) 
  {
    errorobject* tmp = errpdu->get_errorobject();
    

    if ((tmp->get_errorclass() == errorobject::ProtocolError) && 
	(tmp->get_errorcode() == errorobject::err_NoRoutingState)) 
    {
	    // A "No Routing State" error was received. 
	    // er_NoRSM[3](from all states) 
	    // according to RFC 5971 p. 78:
	    //    Rule 3:
	    //    // Assume the Confirm was lost in transit or the peer has reset;
	    //    // restart the handshake
	    //    send Query
	    //    (re)start No_Response timer

      EVLog(param.name, "A Confirm was lost or peer has been reset. Try to resend Query.");
      mri* tmpmri= tmp->get_embedded_mri();
      sessionid* tmpsid= tmp->get_embedded_sessionid();
      
      if (tmpmri && tmpsid) 
      {
	routingkey tmpkey;
	tmpkey.sid = tmpsid;
	tmpkey.mr = tmpmri;
	tmpkey.nslpid = tmp->get_referring_nslpid();

	// lookup routing state
	routingentry* tmpentry = param.rt.lookup(&tmpkey);
	if (tmpentry) 
	{
		EVLog(param.name,"Resending Query from state " << tmpentry->get_state_name() << " now entering Awaiting Response");
		send_query(NULL, &tmpkey, tmpentry, tmpentry->is_dmode());

		// better stop any running timers now
		canceltimers(tmpentry);
		
		// reset state
		tmpentry->set_state( qn_awaiting_response );

		// start NoResponse timer
		starttimer(&tmpkey, tmpentry, noresponse, 0, tmpentry->reset_retry_timeout());
		
		param.rt.unlock(&tmpkey);
	}
	else
		ERRLog(param.name,"Against my expectations, no routing state found for Session " << *tmpsid << " - may be an attack will do nothing");
		
      } // endif tmpmri && tmpsid

      return;
    } // endif ErrNoRoutingState

    // ErrEndpointFound -> notify application
    // if the next node sent back an Endpoint Found error, the NSLP at the last node is no
    // longer available. We should notify the NSLP about this fact.
    if ((tmp->get_errorclass() == errorobject::PermanentFailure) && 
	(tmp->get_errorcode() == errorobject::err_EndpointFound)) 
    {
      EVLog(param.name, "Next hop is not expecting our NSLP message, so I'm the last node. Will notify application.");
      mri* tmpmri= tmp->get_embedded_mri();
      sessionid* tmpsid= tmp->get_embedded_sessionid();
      
      if (tmpmri && tmpsid) 
      {
	routingkey tmpkey;
	tmpkey.sid = tmpsid;
	tmpkey.mr = tmpmri;
	tmpkey.nslpid = tmp->get_referring_nslpid();
	
	routingentry* tmpentry = param.rt.lookup(&tmpkey);
	if (tmpentry) 
	{
		// notify NSLP that we are the last node now
		networknotification(&tmpkey, tmpentry, APIMsg::last_node, false);

		// invalidate routing state, not urgent
		param.rt.invalidate_routing_state(tmpmri, tmp->get_referring_nslpid(), APIMsg::bad, false);
	}
	
	param.rt.unlock(&tmpkey);
	
      } // endif tmpmri && tmpsid
    } // endif ErrEndpointFound


  } // endif get errorobject
}



/** process incoming message from signaling module
 *
 * These are network messages which follow the path:
 * TPoverXY -> SignalingNTLP -> Statemodule
 *
 * SignalingNTLP acts as "preprocessor", so only syntactically correct network messages
 * reach Statemodule!
 * @param generic_sigmsg -- the message to process
 */
void
Statemodule::process_sig_msg(SignalingMsgNTLP* generic_sigmsg)
{
  if (generic_sigmsg->get_error()!=SignalingNTLP::error_ok) 
  {
    DLog(param.name, "Received SignalingMsg with error code, something went wrong");
    return;
  }

  //=====================================================
  // general preparations, get infos from PDU and so on
  // this is largely PDU independent and state independent
  // stuff
  //=====================================================

  // bail out, if we were given NULL from some evil method
  assert(generic_sigmsg!=NULL);
  
  // get PDU object
  known_ntlp_pdu* incoming_pdu= generic_sigmsg->get_pdu();  
  assert(incoming_pdu!=NULL);

  // get peer address given from Signaling
  const appladdress* peer= generic_sigmsg->get_peer();
  const appladdress* own_addr= generic_sigmsg->get_ownaddr();

  // We log, for debugging needs
  EVLog(param.name, color[blue] << "Received message" << color[off] << " #" << generic_sigmsg->get_id() << " (" << generic_sigmsg->get_subtype_name() << ") from: " 
	<< generic_sigmsg->get_qaddr_name() 
        << ", peer: " << (peer ? peer->get_ip_str() : "(unknown)")
	<< ", PDU: "  << endl << color[blue] << (incoming_pdu ? incoming_pdu->to_string() : "N/A (NULL)") << color[off]);

  // the following allows for easier grepping in log files
  if (peer && incoming_pdu)
  {
    if (incoming_pdu->get_sessionid())
    {
      DLog(param.name, "Received <" << incoming_pdu->get_ie_name() << "> sid: " << incoming_pdu->get_sessionid()->to_string() <<  " from " << *peer);
    }
    else
      DLog(param.name, "Received <" << incoming_pdu->get_ie_name() << "> sid: N/A from " << *peer);

  }

  //gp_id_t msgid= generic_sigmsg->get_id();

  // Special treatment if it is a MA Hello (No objects carried), we process it here and this function returns
  if (incoming_pdu->is_hello()) {
    DLog(param.name, color[yellow] << "MA Hello received" << color[off]);
    helloid *hid = incoming_pdu->get_helloid();
    param.rt.hello_ind_ma(peer, incoming_pdu->get_R(), hid?hid->get_helloid():0);
    // check whether some reply action is required (R Flag indicates that a reply should be sent)
    if (incoming_pdu->get_R())
    { // the easiest thing is to send an MA-Hello back, but this time without R flag set
      param.rt.sendhello(peer->copy(), false, hid?hid->get_helloid():0);
    }
    delete generic_sigmsg;
    return;
  }

  // Special treatment for Error PDUs!!
  if (incoming_pdu->is_error()) 
  {
    handle_error_pdu(incoming_pdu);

    delete generic_sigmsg;
    return;
  } // endif error PDU


  // check for NAT Traversal object (NTO) and any untranslated mandatory NATed objects
  const nattraversal* nattravobj= incoming_pdu->get_nattraversal();
  if ( checkNATTraversal(nattravobj, incoming_pdu, peer) == false )
  {
      delete generic_sigmsg;
      return;
  }

  // new empty routing key struct, will be filled later
  routingkey* r_key= new routingkey();

  // if it is a query/response carrying a NAT Traversal object, 
  // we ought to use the MRI from it instead the one from the PDU
  // because all later messages use untranslated the local querier addresses in MRI/NLI
  if (nattravobj) 
  {
    DLog(param.name, "GIST Message carrying a NAT Traversal object, taking MRI from it");
    r_key->mr= nattravobj->get_embedded_mri()->copy();
    // must invert the direction since original MRI will have 
    // the D-bit set, but not the Embedded MRI in the NTO
    // since code below assumes that D is set, we have to do it explicitly here
    if (incoming_pdu->is_response())
	    r_key->mr->invertDirection();
  } 
  else 
  {
    DLog(param.name, "No NATed GIST PDU, taking original MRI of the PDU");
    if (incoming_pdu->get_mri()) 
      r_key->mr= incoming_pdu->get_mri()->copy();
  }

  // get session ID for key
  if (incoming_pdu->get_sessionid())
	  r_key->sid= incoming_pdu->get_sessionid() ? incoming_pdu->get_sessionid()->copy() :  NULL;

  // get NSLPID for key
  r_key->nslpid= incoming_pdu->get_nslpid();

  // MRI and SessionID are mandatory in Query, Response, and Data messages
  // Error and Hellos are processed earlier
  if (!r_key->mr) 
  {
    ERRCLog(param.name, "No MRI in message, this will not be processed");
    senderror(incoming_pdu, peer, GIST_Error::error_gist_missing_object, known_ntlp_object::MRI);

    delete r_key;

    return;
  }

  if (!r_key->sid) 
  {
    ERRCLog(param.name, "No SessionID in message, this will not be processed");
    senderror(incoming_pdu, peer, GIST_Error::error_gist_missing_object, known_ntlp_object::SessionID);

    delete r_key;

    return;
  }

  // if stack proposals are available check for consistency here
  // note: check for profile count == 0 and empty list are already handled during parsing
  if ( capability::is_consistent(incoming_pdu->get_stackprop(), incoming_pdu->get_stackconf()) ==  false )
  {
          /**
	     Stack-Proposal - Stack-Configuration-Data Mismatch:  This subcode
	     is used if a GIST node receives a message in which the data in the
	     Stack-Proposal object is inconsistent with the information in the
	     Stack Configuration Data object.  In this case, both the Stack-
	     Proposal object and Stack-Configuration-Data object MUST be
	     included in separate Object Value Info fields in that order.
	  **/ 
	  senderror(incoming_pdu, peer, GIST_Error::error_gist_sp_scd_mismatch, known_ntlp_object::StackProposal, incoming_pdu->get_stackprop());

	  delete r_key;

	  return;
  }



  // special treatment for Explicitly Routed Messages ->only deliver payload
  bool explicitly_routed= incoming_pdu->get_E();

  if (explicitly_routed) 
  {
    /**
     *
     *  if the message was explicitly routed (see Section 7.1.5) or is a
     *  Data message delivered without routing state (see Section 5.3.2),
     *  the payload is delivered but flagged to the receiving NSLP to
     *  indicate that routing state was not validated;
     *
     */
    
    if ( incoming_pdu->get_nslpdata() )
    {
	    EVLog(param.name, "Explicit routed message: simply delivering payload to NSLP");
	    deliver(peer, own_addr, incoming_pdu, r_key);
    }

    EVLog(param.name, "Incoming Message was explicitly routed - no further processing by GIST.");

    delete r_key;

    return;
  }
    
  // invert, as we got this over the network and our state Maintenance is for the inverse direction
  r_key->mr->invertDirection();

  //=======================================================
  // look up routing state and print a status dump of routing table
  //=======================================================
    
  param.rt.status();
    
  // A packet over the network will always arrive with MRI inverted to our routing state
  // DLog(param.name, "Looking up Routing State in inverse direction");
        
  // pointer to a routing entry
  routingentry* r_entry=param.rt.lookup(r_key);
    
  //======================================================
  // first check what to do if we have got no routing state
  //======================================================

  if (!r_entry) 
  {
    DLog(param.name, "No Routing State available, processing state setup mechanisms");

    switch (incoming_pdu->get_type()) 
    {
      // Initial QUERY
      case known_ntlp_pdu::Query: 
	{
	  handle_initial_query(incoming_pdu, r_key, peer, own_addr);
	  break;
	} // end case Query


      case known_ntlp_pdu::Response : 
	{

	  ERRCLog(param.name, "Not explicitly routed GIST Response received whilst no Routing State, sending back error");
		
	  senderror(incoming_pdu, peer, GIST_Error::error_gist_no_routing_state);
	  break;
	} // end case known_ntlp_pdu::Response


      case known_ntlp_pdu::Data : 
	{
	  // if Data came via Q-Mode we should deliver it to the application and indicate that no routing state exists 
	  // since draft -17 we have to check for the C flag
	  if (incoming_pdu->get_C())
	  {
	    DLog(param.name, " Data PDU via Q-Mode, simply deliver DATA to NSLP");
	    deliver(peer, own_addr, incoming_pdu, r_key);
	  }
	  else
	  {
	    ERRCLog(param.name, "Not explicitly routed GIST Data received whilst no Routing State, sending back error, dropping data");		
	    senderror(incoming_pdu, peer, GIST_Error::error_gist_no_routing_state);
	  }
		
	  break;
	}
		
      case known_ntlp_pdu::Confirm : 
	{
	  // got confirm but have no routing state yet
	  handle_confirm_delayed_state_install(incoming_pdu, r_key, peer, own_addr);
		    
	  break;
	} // end case known_ntlp_pdu::Confirm
    } // end switch
	
  } // end if !r_entry
  else
  {
    //============================================================
    //
    // We have got routing state, now we define the state machine
    //
    //============================================================
 
    if ((peer->get_protocol()!= param.udp) && (peer->get_protocol()!= param.qenc)) 
    { // not UDP or Query encapsulated => C-Mode

      // all messages arriving via an MA must belong to a session that is signaled in this MA
      // check, if the MA is allowed for this MRI/SesssionID/NSLPID
      //
      // Some cases must be considered here:
      // - message arrived via new transport which is not registered as MA yet
      //   The transport must meet the transfer attributes, e.g. upgrading from 
      //   non-secure to secure mode.
      //   
      // - message arrived via existing MA:
      //   in this case we must check whether the MA meets the requirements
      //   of the transfer attributes. 

      // - new MA setup: 
      //   either MA is not existing yet or new MA must be set in order to meet
      //   transfer attributes, e.g. upgrading from non-secure to secure mode.
      //   This includes all cases of receiving a Confirm after MA setup
      //   or receiving a response via an existing MA
      //   MA re-use in inverse direction, i.e. a Response is sent via an existing MA
      //   that was initiated by the node who is now the responder
      //
      // - wrong MA re-use:
      //   if Response for setting up a secure MA arrived via non-secure MA   
      //
      // may be the case if querier promoted session to different MA (tx_attr change)
      // in this case we are in RN_AWAITING_REFRESH and handle_confirm_in_awaiting_refresh() will
      // check for correct MA use anyway
      // second case: MA re-use in inverse direction, i.e. a Response is sent via an existing MA
      // that was initiated by the node who is now the responder

      if (!param.rt.checkMA(r_entry, peer, cap)) 
      {
	ERRLog(param.name, "Message arrived via MA that is NOT associated to this MRI/SessionID/NSLPID combination! - address:" << *peer);
	senderror(incoming_pdu, peer, GIST_Error::error_gist_incorrectly_delivered_message);
	  
	param.rt.unlock(r_key);
	return;	     
      }

      // call MA timer reset
      // we do not reset the timer if message came via D-Mode
      if (incoming_pdu->get_nli()) 
      {
	      param.rt.activity_ind_ma(incoming_pdu->get_nli());
      } 
      else 
      {
	      param.rt.activity_ind_ma(r_entry->get_peer_nli());
      }
    } // endif message came via C-Mode
    
    if (r_entry->get_mod_data()) 
    {
      DLog(param.name, "Cleaning away saved modified data payload for Response repeats");

      r_entry->set_mod_data(NULL);
    }

    // deliver DATA PDUs, but only if in state established or awaiting refresh
    if ( incoming_pdu->get_nslpdata() )
    {
	    if (r_entry->get_state()==rn_established || r_entry->get_state()==rn_awaiting_refresh || 
		r_entry->get_state()==qn_established || r_entry->get_state()==qn_awaiting_refresh ) 
      {
	// if QNode, reStart inactive_QNode timer
	if (r_entry->is_querying_node())
		starttimer(r_key, r_entry, inactive_qnode, 0, r_entry->get_rs_validity_time());

	EVLog(param.name, "Delivering Payload to NSLP");
	deliver(peer, own_addr, incoming_pdu, r_key, r_entry);
      }
      else
	WLog(param.name,"Dropped DATA since in wrong routing state: " << r_entry->get_state_name());
    }
    

    DLog(param.name, "Evaluating current routing state");
    switch (r_entry->get_state()) 
    {
      case rn_awaiting_confirm: 
	{ // we waited for a confirm 
	  if (incoming_pdu->is_confirm()) 
	  { // We got a Confirm
	    handle_confirm_in_awaiting_confirm(incoming_pdu,r_key,r_entry, peer, own_addr);
	  } // end if is_confirm
	  else
	  if (incoming_pdu->is_query())
	  { // other side resend query, maybe our response got lost or came too late
	    // send new response and update routing state
	    handle_query_in_awaiting_confirm_or_refresh(incoming_pdu, r_key, r_entry, peer, own_addr);
	  }
	  else // if incoming PDU is DATA we must send back "No Routing State" error
	  if (incoming_pdu->is_data())
	  {
	    ERRLog(param.name, "DATA while in awaiting confirm - probably confirm was lost, sending back error");
	    senderror(incoming_pdu, peer, GIST_Error::error_gist_no_routing_state);
	  }
	  
	  break;
	} // end case rn_awaiting_confirm
		
      case rn_established: 
	{
	  // we received a query, first step in refreshing state (in delayedstate case, else we refresh here instantly)
	  if (incoming_pdu->is_query()) 
	  {
	    handle_query_in_established(incoming_pdu, r_key, r_entry, peer);
	  }
		
	  break;
	} // endif rn_established

      case rn_awaiting_refresh: 
	{
	  // we got a confirm in state rn_awaiting_refresh, so this is a refreshing handshake
	  if (incoming_pdu->is_confirm()) 
	  {
	    handle_confirm_in_awaiting_refresh(incoming_pdu, r_key, r_entry, peer, own_addr);
	  }
	  else 
          if (incoming_pdu->is_query())
	  {
	    handle_query_in_awaiting_confirm_or_refresh(incoming_pdu, r_key, r_entry, peer, own_addr);
	  }
	  else
          { // every other message (DATA is handled above)
	    ERRCLog(param.name, "Unacceptable message received in QN_AWAITING_REFRESH, dropping");
	  }
	  break;
	}
		
	case qn_awaiting_response: 
	  {
	    if (incoming_pdu->is_response()) 
	    {
	      handle_response_in_awaiting_response(incoming_pdu, r_key, r_entry, peer);
	    } 
	    else 
	    {
	      ERRCLog(param.name, "Unacceptable message received in QN_AWAITING_RESPONSE, dropping");
	    }
	    
	    break;
	  } // end case qn_waiting_response
	  
	  
	case qn_established: 
	  {
	    DLog(param.name, "Local Routing state is established, processing");
	    
	    // if a response is received, this is interpreted as that the routing state on the
	    // Responder node is valid and 

	    if (incoming_pdu->is_response()) {
	      handle_response_in_established(incoming_pdu, r_key, r_entry);
	    }

	    break;
	}
	    
	    
	case qn_awaiting_refresh: 
	  {
	    if (incoming_pdu->is_response()) 
	    {
	      handle_response_in_awaiting_refresh(incoming_pdu, r_key, r_entry);
	    } 
	    else 
	    {
	      ERRCLog(param.name, "Only processing GIST Responses in QN_AWAITING_REFRESH");
	    }
	    
	    break;
	}
	    
	// We are waiting for a decision from the NSLP 
      case rn_querynslp: 
	{
	  // in this state there should be no other messages than incoming queries that are retransmitted
	  // It may be the case that queries are coming back to back
	  if (incoming_pdu->is_query())
	  {
	    WLog(param.name, "Did not yet complete querying NSLP for State Setup - new query will override old one");

	    // NSLPID, SessionID and MRI are exactly the same
	    // basically we should update the query cookie

	    // update query cookie
	    r_entry->set_querycookie(incoming_pdu->get_querycookie());

	    // update temporary saved PDU
	    r_entry->set_tmppdu(incoming_pdu);

	    // update MA hold time if it is different from the first query
	    if (incoming_pdu->get_stackconf()
		&&
		incoming_pdu->get_stackconf()->get_ma_hold_time() != r_entry->get_ma_hold_time()) 
	    {
		    r_entry->set_ma_hold_time(incoming_pdu->get_stackconf()->get_ma_hold_time());
	      DLog(param.name, "Updated MA Hold time: " << r_entry->get_ma_hold_time());
	    }
	  } // endif is query
	  else
	  {
	    ERRLog(param.name, "Did not yet complete querying NSLP for State Setup - " << 
                               "discarding further messages that are not a query, like this " << incoming_pdu->get_ie_name());
	  }

	  break;
	}

	// We are dead
      case dead: 
	{
	  DLog(param.name, "Got message from peer of dead routing state, pass Data only");
	}

      default: 
	{
	  ERRCLog(param.name, "Encountered routing state in state DEAD/MA_* - Node dies");
	  abort();
	  
	  break;
	}
    } // end switch state
	    
    param.rt.unlock(r_key);
	
  } // end if r_entry
    
  // delete sigmsg, !! Beware !! If we need to use object contents, we must create copies of the objects we need !!
  // this will also delete the object that incoming_pdu points to!
  delete generic_sigmsg;
  
  // unlock routing State!! (is been done in each case seperately)
}


/**
 * Check for errors and consistency of NAT traversal object
 * @return success, i.e., false if check failed and error was sent or true if check passed
 */
bool
Statemodule::checkNATTraversal(const nattraversal* nattravobj, const known_ntlp_pdu* incoming_pdu, const appladdress* peer)
{
  // check for NAT Traversal object (NTO) and any untranslated mandatory NATed objects
  if ( nattravobj )
  {
    // if the NAT Traversal object does NOT indicate NLI and stack_conf_data to be translated, 
    // send back an error.
    bool nli_found = false;
    bool stackconf_found = false;
	
    uint16 tmp_trans;
    for (unsigned int i = 0; i < nattravobj->get_translations().size(); i++) 
    {
      tmp_trans= nattravobj->get_translations()[i];

      if (tmp_trans == known_ntlp_object::NLI) nli_found=true;
      if (tmp_trans == known_ntlp_object::StackConfiguration) stackconf_found=true;

    } // end for

    if (!nli_found) 
    {
      uint16 oti = known_ntlp_object::NLI;
      ERRCLog(param.name, "NLI is untranslated in NATed Response");
      senderror(incoming_pdu, peer, GIST_Error::error_gist_untranslated_objects, oti);
    }

    if (!stackconf_found) 
    {
      uint16 oti = known_ntlp_object::StackConfiguration;
      ERRCLog(param.name, "Stack Configuration Data is untranslated in NATed Response");
      senderror(incoming_pdu, peer, GIST_Error::error_gist_untranslated_objects, oti);
    }
    
    if (!nli_found || !stackconf_found)
    {
      return false;
    }
  } // end checking for untranslated NAT Objects

  // success
  return true;
}


/**
 * Send a GIST Error Message, calculate the peer correctly from PDU flags and NLI and/or IP source
 * @param pdu -- the pdu causing the error
 * @param peer -- the IP message source
 * @param error -- the error type
 * @param oti -- Object Type Information to refer to a specific object in the pdu
 * @param ovi_object -- Object Value Info: an NTLP object whose value was wrong
 */
void 
Statemodule::senderror(const known_ntlp_pdu* pdu, const appladdress* peer, GIST_Error::gist_error_code_t error,  uint16 oti, const ntlp_object* ovi_object) {

  DLog(param.name, "Error Message Sender called");

  errorobject* errobj = NULL;

  bool dgram = !cap.is_reliable(peer);
  bool qe = (peer->get_protocol() == param.qenc);
	    

  // from here on, we need at least some PDU object (parsing must have been successful)
  if (pdu == NULL)
  {
    ERRCLog(param.name, "senderror() - need a PDU object, but none is given - this is an internal error");
    return;
  }

  if (pdu->is_query()) qe = true;
	    
  mri* mr = NULL;
  sessionid* sid = NULL;

  switch (error) {

    case GIST_Error::error_gist_no_routing_state: 
	    
      ERRCLog(param.name, "ERROR_GIST_NO_ROUTING_STATE");

      if (pdu->get_mri()) mr = pdu->get_mri()->copy();
      if (pdu->get_sessionid()) sid = pdu->get_sessionid()->copy();
	    
      errobj = new errorobject(mr, sid, errorobject::ProtocolError, errorobject::err_NoRoutingState, 0, 
			       pdu->get_version(), pdu->get_hops(), pdu->get_length(), pdu->get_nslpid(), 
			       pdu->get_type(), pdu->get_flags(), dgram, qe, NULL);
	    	
      break;


    case GIST_Error::error_gist_invalid_qcookie: 

      ERRCLog(param.name, "ERROR_GIST_INVALID_QCOOKIE");
	    
      if (pdu->get_mri()) mr = pdu->get_mri()->copy();
      if (pdu->get_sessionid()) sid = pdu->get_sessionid()->copy();
	    
      errobj = new errorobject(mr, sid, errorobject::ProtocolError, 
			       errorobject::err_ObjectValueError, 
			       errorobject::errsub_InvalidCookie, 
			       pdu->get_version(), 
			       pdu->get_hops(), 
			       pdu->get_length(), 
			       pdu->get_nslpid(), 
			       pdu->get_type(), 
			       pdu->get_flags(), 
			       dgram, qe, 
			       pdu->get_querycookie()->copy());
	    	
      break;


    case GIST_Error::error_gist_invalid_rcookie: 

      ERRCLog(param.name, "ERROR_GIST_INVALID_RCOOKIE");

	    
      if (pdu->get_mri()) mr = pdu->get_mri()->copy();
      if (pdu->get_sessionid()) sid = pdu->get_sessionid()->copy();
	    
      errobj = new errorobject(mr, sid, errorobject::ProtocolError, 
			       errorobject::err_ObjectValueError, 
			       errorobject::errsub_InvalidCookie, 
			       pdu->get_version(), 
			       pdu->get_hops(), 
			       pdu->get_length(), 
			       pdu->get_nslpid(), pdu->get_type(), pdu->get_flags(), dgram, qe, pdu->get_respcookie()->copy());
	    	
      break;

    case GIST_Error::error_gist_untranslated_objects: 

      ERRCLog(param.name, "ERROR_GIST_UNTRANSLATED_OBJECTS");

	    
      if (pdu->get_mri()) mr = pdu->get_mri()->copy();
      if (pdu->get_sessionid()) sid = pdu->get_sessionid()->copy();
	    
      errobj = new errorobject(mr, sid, errorobject::ProtocolError, 
			       errorobject::err_ObjectTypeError, 
			       errorobject::errsub_UntranslatedObject, 
			       pdu->get_version(), pdu->get_hops(), pdu->get_length(), 
			       pdu->get_nslpid(), pdu->get_type(), pdu->get_flags(), dgram, qe, NULL, oti);
	    	
      break;

    case GIST_Error::error_gist_missing_object: 

      ERRCLog(param.name, "ERROR_GIST_MISSING_OBJECT");

      if (pdu->get_mri()) mr = pdu->get_mri()->copy();
      if (pdu->get_sessionid()) sid = pdu->get_sessionid()->copy();

      errobj = new errorobject(mr, sid, errorobject::ProtocolError,
			       errorobject::err_ObjectTypeError, 
			       errorobject::errsub_MissingObject, pdu->get_version(), pdu->get_hops(), 
			       pdu->get_length(), pdu->get_nslpid(), pdu->get_type(), pdu->get_flags(), dgram, qe, NULL, oti);

      break;

    case GIST_Error::error_gist_incorrectly_delivered_message: 
	    
      ERRCLog(param.name, "ERROR_GIST_INCORRECTLY_DELIVERED_MESSAGE");

      if (pdu->get_mri()) mr = pdu->get_mri()->copy();
      if (pdu->get_sessionid()) sid = pdu->get_sessionid()->copy();
	    
      errobj = new errorobject(mr, sid, errorobject::ProtocolError,
			       errorobject::err_IncorrectlyDeliveredMessage, 
			       errorobject::errsub_Default, 
			       pdu->get_version(), pdu->get_hops(), pdu->get_length(), pdu->get_nslpid(), pdu->get_type(), pdu->get_flags(), dgram, qe, NULL);
	    	
      break;

    case GIST_Error::error_gist_incorrect_encapsulation:
      ERRCLog(param.name, "ERROR_GIST_INCORRECT_ENCAPSULATION");
      
      errobj = new errorobject(mr, sid, errorobject::ProtocolError,
			       errorobject::err_IncorrectEncapsulation, 
			       errorobject::errsub_Default, pdu->get_version(), pdu->get_hops(), 
			       pdu->get_length(), pdu->get_nslpid(), pdu->get_type(), pdu->get_flags(), dgram, qe, NULL);
      break;

    case GIST_Error::error_gist_value_not_supported: 
	    
      ERRCLog(param.name, "ERROR_GIST_VALUE_NOT_SUPPORTED");

      if (pdu->get_mri()) mr = pdu->get_mri()->copy();
      if (pdu->get_sessionid()) sid = pdu->get_sessionid()->copy();
      
      errobj = new errorobject(mr, sid, errorobject::ProtocolError, errorobject::err_ObjectValueError, errorobject::errsub_ValueNotSupported, 
			       pdu->get_version(), pdu->get_hops(), pdu->get_length(), pdu->get_nslpid(), 
			       pdu->get_type(), pdu->get_flags(), dgram, qe, ovi_object ? ovi_object->copy() : NULL, oti);
	    	
      break;


    case GIST_Error::error_gist_sp_scd_mismatch:
	    
      ERRCLog(param.name, "ERROR_GIST_STACK_PROFILE_STACKCONFIG_DATA_MISMATCH");

      if (pdu->get_mri()) mr = pdu->get_mri()->copy();
      if (pdu->get_sessionid()) sid = pdu->get_sessionid()->copy();
      
      errobj = new errorobject(mr, sid, errorobject::ProtocolError, errorobject::err_ObjectValueError, errorobject::errsub_SP_SCD_Mismatch, 
			       pdu->get_version(), pdu->get_hops(), pdu->get_length(), pdu->get_nslpid(), 
			       pdu->get_type(), pdu->get_flags(), dgram, qe, ovi_object ? ovi_object->copy() : NULL, oti);
	    	
      break;

    default:
      ERRCLog(param.name, "senderror(): Yet unhandled error occured (code:" << error << "), will not send Error Message");
      delete errobj;
      return;
      break;
  } // end switch

  uint8 ip_version;
  if (mr) {
    ip_version = mr->get_ip_version();
  } else {
    ip_version = 4;
  }

  netaddress na(*peer);
  nli* mynl = build_local_nli(ip_version, na);

  ntlp::error* errpdu = new ntlp::error(mynl, errobj);

  errpdu->set_hops(1);


  // calculate the target address where this error message should be sent to
  appladdress* target = NULL;
  // IP source is NOT the last GIST hop
  if (!pdu->get_S()) {

    if (pdu->get_nli()) {
      DLog(param.name, "IP Source is NOT last GIST hop, sending to GIST denoted by NLI");
      target = new appladdress(pdu->get_nli()->get_if_address(), param.udp, gconf.getpar<uint16>(gistconf_udpport));
	    
	    
    } else {
      ERRCLog(param.name, "No valid target address for Error Message found. Dropping errormessage: " << pdu);
	    
    }

  } else {
    DLog(param.name, "IP Source IS last GIST hop, sending to it.");
    target = new appladdress(*peer);
    if (target->get_protocol()== param.qenc) target->set_protocol(param.udp);
    if (target->get_protocol()== param.udp) target->set_port(gconf.getpar<uint16>(gistconf_udpport));
  }
    

  SignalingMsgNTLP* sigmsg = new SignalingMsgNTLP;
  sigmsg->set_req(target, errpdu);
  sigmsg->set_local_addr(mynl->get_if_address().copy());
  sigmsg->send_or_delete();

}



/**
 * Start Timers on Routing Entries
 * @param r_key -- the routing key (may NOT be NULL)
 * @param r_entry -- the routing entry itself (may NOT be NULL)
 * @param timer -- the timer type to start
 * @param sec -- seconds to start
 * @param msec -- milliseconds to start (allowed to be larger than 999)
 * @param stop -- specify to stop an old timer on the same slot. Set to "false", if you are sure you are starting the same timer which is currently not running
 */
void 
Statemodule::starttimer(const routingkey* r_key, routingentry* r_entry, timer_type_t timer, uint32 sec, uint32 msec, bool stop) 
{
  assert( r_key != NULL );
  assert( r_entry != NULL );
  /// decide about timer slot, 2 slots are available per entry

  uint64 mid = 0;

  // timers on slot #3 (queue_poll)
  if (timer == queue_poll) 
  {
    if ((r_entry->get_timer_id_3() != 0) && stop) 
    {
      // stop the current timer
      TimerMsg* stopmsg = new TimerMsg(message::qaddr_coordination, true);
      stopmsg->stop(r_entry->get_timer_id_3());
      stopmsg->send_to(message::qaddr_timer);
    }

    TimerMsg* msg = new RoutingTableTimerMsg(*r_key, timer, sec, msec);

    r_entry->set_timer_type_3(timer);
    r_entry->set_timer_id_3(msg->get_id());
    mid = r_entry->get_timer_id_3();

    msg->send_to(message::qaddr_timer);
  }
  
    
  // timers on slot #2 (no_response (QN) or refresh_qnode (QN))
  if ((timer == noresponse) || (timer==refresh_qnode)) 
  {
    if ((r_entry->get_timer_id_2() != 0) && stop) 
    {
      // stop the current timer
      TimerMsg* stopmsg = new TimerMsg(message::qaddr_coordination, true);
      stopmsg->stop(r_entry->get_timer_id_2());
      stopmsg->send_to(message::qaddr_timer);
    }
	
    TimerMsg* msg = new RoutingTableTimerMsg(*r_key, timer, sec, msec);

    r_entry->set_timer_type_2(timer);
    r_entry->set_timer_id_2(msg->get_id());
    mid = r_entry->get_timer_id_2();

    msg->send_to(message::qaddr_timer);
  }

  // timers on slot #1 (no_confirm (RN), expire_rnode (RN), inactive_qnode (QN))
  if ((timer == noconfirm) || (timer == expire_rnode) || (timer == inactive_qnode)) 
  {
    if ((r_entry->get_timer_id_1() != 0) && stop) 
    {
      // stop the current timer
      TimerMsg* stopmsg = new TimerMsg(message::qaddr_coordination, true);
      stopmsg->stop(r_entry->get_timer_id_1());
      stopmsg->send_to(message::qaddr_timer);
    }
		
    TimerMsg* msg = new RoutingTableTimerMsg(*r_key, timer, sec, msec);

    r_entry->set_timer_type_1(timer);
    r_entry->set_timer_id_1(msg->get_id());
    mid = r_entry->get_timer_id_1();

    msg->send_to(message::qaddr_timer);
  }    

  DLog(param.name, "Started timer: " << color[magenta] << timername(timer)<< " " << ((sec * 1000) + msec) << " ms" << " mid " << mid << color[off]);
}


/// Stop Timers on Routing Entries
void Statemodule::stoptimer(routingentry* r_entry, timer_type_t timer)
{
  // timers on slot #2
  if (timer==refresh_qnode) 
  {
	  r_entry->set_timer_type_2(none);
  }
}


/// Cancel running timers on a Routing Entry
void 
Statemodule::canceltimers(routingentry* r_entry)
{
  // timers on slot #2
  if (r_entry->get_timer_type_2() != none)
  {
    // this will cause only ignoring any timer
    r_entry->set_timer_type_2(none);

    EVLog(param.name, color[magenta] << "Trying to stop timer on slot 2" << color[off]);
    // really stop the current timer
    TimerMsg* stopmsg = new TimerMsg(message::qaddr_coordination, true);
    stopmsg->stop(r_entry->get_timer_id_2());
    stopmsg->send_to(message::qaddr_timer);
    // this will cause only ignoring any timer
    stoptimer(r_entry, refresh_qnode);
  }

  // timers on slot #3
  if (r_entry->get_timer_type_3() != none)
  {
    // this will cause only ignoring any timer
    r_entry->set_timer_type_3(none);

    EVLog(param.name, color[magenta] << "Trying to stop timer on slot 3" << color[off]);
    // really stop the current timer
    TimerMsg* stopmsg = new TimerMsg(message::qaddr_coordination, true);
    stopmsg->stop(r_entry->get_timer_id_3());
    stopmsg->send_to(message::qaddr_timer);
  }
}



/**
 * SecretsRefresh Timer processing. Do a local secrets rollover
 */
void 
Statemodule::to_secrets_refresh() 
{
  //DLog(param.name, color[magenta] << "Secret Generation Timer went off");
  // Forward Secrets
  param.secrets.forward();
  
  
  TimerMsg* msg = new TimerMsg(message::qaddr_coordination, true);
    
  uint32* timer_type=new uint32;
  *timer_type=refresh_secrets;
  
  msg->start_relative(gconf.getpar<uint32>(gistconf_secrets_refreshtime), 0, (void*)timer_type, NULL);
  
  msg->send_to(message::qaddr_timer);
}




/** 
 * Calculate a randomly distributed time in order to avoid synchonization effects
 * The target value is always distributed around 0.5*value
 *
 * @param value -- the upper limit of the value range
 * @param uppermargin -- the factor to apply before randomization, should be (0.5,1.0)
 *
 * @note -- The factor specifies the upper (or lower) margin for the interval, e.g. factor=0.75 
 * means that the returned values are randomly distributed in [value*0.25 ... value*0.75]
 * (this is identical to factor 0.25). Factor = 0.8 would yield [value*0.2 ... value*0.8]
 * Factor = 0.5 will always give back 0.5*value as result (no randomization)
 */
uint32
Statemodule::randomized(uint32 value, float uppermargin) 
{
  const double target= 0.5;

  if (uppermargin < 0.0 || uppermargin > 1.0)
  {
    WLog("StateModule","randomized: Wrong uppermargin " << uppermargin << " used, setting to 0.75");
    uppermargin= 0.75;
  }

  double variance = uppermargin - target;
  // this is the interval for picking a number
  double interval = 2.0 * variance;

  // get a normalized random value
  double randfact = (float) random() / (float) RAND_MAX;

  // apply the normalized random value to interval
  double rinterval =   interval < 0 ? -interval * randfact :  interval * randfact;

  // return the value
  return (uint32) ((double) value * (target - variance + rinterval));
}


/// Check refresh timer for downstream path and restart if necessary
void 
Statemodule::check_refresh_timer(routingkey* r_key) 
{
  routingkey tmpkey;
  tmpkey.mr = r_key->mr->copy();
  tmpkey.sid = r_key->sid->copy();
  tmpkey.nslpid = r_key->nslpid;
  
  tmpkey.mr->invertDirection();
  routingentry* tmpentry = param.rt.lookup(&tmpkey);
  
  if (tmpentry) 
  {
    if (tmpentry->get_timer_type_2() == none) 
    {
      EVLog(param.name, "Route Flapping occured, restarting refresh_qnode timer for downstream path");
	    
      starttimer(&tmpkey, tmpentry, refresh_qnode, 0, randomized(tmpentry->get_peer_nli()->get_rs_validity_time(), gconf.getpar<float>(gistconf_retryfactor)));
    }
  }
  param.rt.unlock(&tmpkey);
}


/**
 * Check if the given address is one of ours.
 *
 * There are two lists of addresses: one for IPv4, and the other one for IPv6.
 * Both lists are parameters of the state module and can be configured
 * via the NTLPStarter.
 *
 * @param addr the address to check
 */
bool 
Statemodule::is_local_address(const hostaddress &addr) 
{
    netaddress na(addr);
    return param.addresses.addr_is(na, AddressList::ConfiguredAddr_P);
}


//@}

} // end namespace ntlp

