/// ----------------------------------------*- mode: C++; -*--
/// @file signalingmodule_ntlp.cpp
/// Signaling Module - sending and receiving PDUs incl. parsing
/// ----------------------------------------------------------
/// $Id: signalingmodule_ntlp.cpp 6282 2011-06-17 13:31:57Z bless $
/// $HeadURL: https://svn.tm.kit.edu/nsis/ntlp/trunk/src/signalingmodule_ntlp.cpp $
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
/** @defgroup sigmodule Signaling Module
 *  @ingroup ntlp
 * 
 * This module receives and sends messages and calls the serialization/deserialization routines
 * of NTLP Protocol class.
 * 
 * Also Error checking and relaying on messages is done in this module.
 *
 */
#include <signalingmodule_ntlp.h>
#include "queuemanager.h"
#include "logfile.h"
#include "ntlp_proto.h"
#include "rfc5014_hack.h"

#ifndef NSIS_OMNETPP_SIM

#include <net/if.h>
#include <linux/types.h> // in case that this one is not included in netlink.h
#include <linux/netlink.h>
#include <linux/rtnetlink.h>

#else

#include "ip_to_nsis_id.h"

#endif // end ifndef NSIS_OMNETPP_SIM

#include <cerrno>

#include "readnl.h"
#include "ntlp_global_constants.h"
#include "gist_conf.h"

namespace ntlp {

 using namespace protlib;

 const unsigned int max_mtu_size= 576;


/** 
 * SignalingNTLPParam constructor, take over parameters for Signaling Module
 * @param pm -- The Protocol map containing pointers to all TP Modules in use 
 * @param addresses -- an Address List containing IP addresses of the node
 * @param nslptable -- The table holding the addresses of registered NSLP clients
 * @param well_known_port -- The UDP Well-Known Port of GIST
 * @param sleep_time -- Thread sleep time
 * @param see, sre -- No idea. Something obsolete from GIST
 * @note
 *       This will create a time period in [factor - ((1-factor)/2) * value .. factor + ((1-factor)/2) * value]
 */
SignalingNTLPParam::SignalingNTLPParam(protocol_map_t pm,
				       nli &nli_tmpl,
				       AddressList &addresses,
				       NSLPtable& nslptable,
				       uint16 well_known_port,
				       Flowinfo *fi_service,
				       uint32 sleep_time,
				       bool see, bool sre
				       ) 
  : ThreadParam(sleep_time,"GIST Signaling"),
    protomap(pm), 
    nli_tmpl(nli_tmpl),
    addresses(addresses),
    source(message::qaddr_signaling),
    send_error_expedited(see),
    send_reply_expedited(sre),
    nslptable(nslptable),
    well_known_port(well_known_port),
    fi_service(fi_service)
{
} // end constructor
  


/// Return a handle to a TP module handling the given protocol ID
TP* 
SignalingNTLPParam::get_tp(protocol_t id) const 
{
  protocol_map_const_it_t mit = protomap.find(id);
  if (mit==protomap.end()) return NULL;
  else return mit->second;
} // end get_tp


SignalingMsgNTLP::SignalingMsgNTLP(bool conf) 
	: message(type_signaling), 
	subtype(ignore), err(SignalingNTLP::error_ok),
	pdu(NULL), peer(NULL), ownaddr(NULL), encap_pdu(NULL), ref_id(0), confirm(conf),
	use_existing_connection(false), local_src(NULL)
{}


SignalingMsgNTLP::~SignalingMsgNTLP() {
	if (pdu) delete pdu;
	if (peer) delete peer;
	if (ownaddr) delete ownaddr;
	if (encap_pdu) delete encap_pdu;
	if (local_src) delete local_src;
} // end destructor


/* A request from the coordinator to send a signaling message
 */
void 
SignalingMsgNTLP::set_req(const appladdress* peeraddr, known_ntlp_pdu* p, gp_id_t refid, uint16 oiface) {
	set_source(qaddr_coordination);
	subtype = SigReq;
	err = SignalingNTLP::error_ok;
	pdu = p;
	peer = peeraddr;
	ownaddr = NULL;
	encap_pdu = NULL;
	ref_id = refid;
	oif = oiface;
} // end set_req


/* An indication to the coordinator that a signaling message arrived
 */
void 
SignalingMsgNTLP::set_ind(gp_id_t id, const appladdress* peeraddr, const appladdress* own_addr, known_ntlp_pdu* p,
		gp_id_t rid, known_ntlp_pdu* epdu) {
	set_source(qaddr_signaling);
	set_id(id);
	subtype = SigInd;
	err = SignalingNTLP::error_ok;
	pdu = p;
	peer = peeraddr;
	ownaddr = own_addr;
	encap_pdu = epdu;
	ref_id = rid;
} // end set_ind


/* A confirmation to the coordinator that a signaling message was processed/sent
 */
void 
SignalingMsgNTLP::set_cnf(gp_id_t id, known_ntlp_pdu* p, known_ntlp_pdu* epdu) {
	set_source(qaddr_signaling);
	set_id(id);
	subtype = SigCnf;
	err = SignalingNTLP::error_ok;
	pdu = p;
	peer = NULL;
	ownaddr = NULL;
	encap_pdu = epdu;
	ref_id = 0;
} // end set_cnf


/* A response from the coordinator that has to be sent back as a signaling message
 */
void 
SignalingMsgNTLP::set_rsp(gp_id_t id, known_ntlp_pdu* p, bool setencap) {
	set_source(qaddr_coordination);
	set_id(id);
	subtype = SigRsp;
	err = SignalingNTLP::error_ok;
	pdu = p;
	peer = NULL;
	ownaddr = NULL;
	encap_pdu = NULL;
	if (setencap) ref_id = id;
	else ref_id = 0;
} // end set_rsp


/** Set up  a cancel message. If reply is set true, send error or
 * confirmation.
 */
void 
SignalingMsgNTLP::set_cancel(gp_id_t id, bool reply) 
{
  set_source(qaddr_coordination);
  set_id(id);
  subtype = SigCancel;
  err = SignalingNTLP::error_ok;
  pdu = NULL;
  peer = NULL;
  ownaddr = NULL;
  encap_pdu = NULL;
  ref_id = 0;
  confirm = reply;
} // end set_cancel


/* Terminate (i.e., shut down) a signaling association/connection
 */
void 
SignalingMsgNTLP::set_trm(const appladdress* peeraddr) 
{
  set_source(qaddr_coordination);
  subtype = SigTrm;
  err = SignalingNTLP::error_ok;
  pdu = NULL;
  peer = peeraddr;
  ownaddr = NULL;
  encap_pdu = NULL;
  ref_id = 0;
  confirm = false;
} // end set_ind


void 
SignalingMsgNTLP::clear_pointers() 
{
  pdu = NULL;
  peer = NULL;
  ownaddr = NULL;
  encap_pdu = NULL;
  local_src = NULL;
} // end clear_pointers


bool 
SignalingMsgNTLP::send(bool exp) 
{
  qaddr_t s = get_source();
  if (s==qaddr_signaling) return send_to(qaddr_coordination,exp);
  else if (s!=qaddr_unknown) return send_to(qaddr_signaling,exp);
  else return false;
} // end send


bool SignalingMsgNTLP::set_confirm(bool conf) 
{
  register bool o = confirm;
  confirm = conf;
  return o;
} // end set_confirm


const char* const SignalingMsgNTLP::subtypename[] = 
{
  "ignore",
  "SigReq",
  "SigInd",
  "SigCnf",
  "SigRsp",
  "SigTrm",
  "SigCancel"
}; // end subtypename

//@}


const char* const SignalingNTLP::errstr[] = 
{
  "success",
  "Message sent to peer.",
  "No address or wrong address.",
  "Peer unreachable.",
  "Internal transport protocol error",
  "No transport protocol found for peer address.",
  "Wrong PDU type.",
  "Malformed PDU or PDU state invalid, cannot serialize.",
  "No matching request found.",
  "PDU too big for maximum payload size.",
  "Bad message reference or reference already in use.",
  "Internal GIST protocol error.",
  "Internal signaling module error.",
  "Bad SignalingMsg, wrong subtype for receiver."
}; // end errstr


const char* 
SignalingNTLP::get_error_string(error_t e) 
{ 
	if (e < SignalingNTLP::error_MAX_msg)
		return errstr[e];
	else
		return "invalid error code";
}

/** 
 * Initialize Thread base class and GIST protocol. 
 * @param p -- Parameter for Module
 */
SignalingNTLP::SignalingNTLP(const SignalingNTLPParam& p)
  : Thread(p),
    param(p), 
    gistproto(p.addresses, p.well_known_port)
{
  // register queue
  
  QueueManager::instance()->register_queue(get_fqueue(),p.source);
  Log(DEBUG_LOG,LOG_NORMAL, param.name, "Created SignalingModule object");
	
} // end constructor


SignalingNTLP::~SignalingNTLP() 
{
  Log(DEBUG_LOG,LOG_NORMAL, param.name, "Destroying SignalingModule object");
  QueueManager::instance()->unregister_queue(param.source);
} // end destructor


/** 
 * Main Loop 
 * @param nr -- Number of thread
 */
void 
SignalingNTLP::main_loop(uint32 nr) 
{
  Log(DEBUG_LOG,LOG_NORMAL, param.name, "Starting thread #" << nr);

  // process queue of internal messages
  process_queue();

  Log(DEBUG_LOG,LOG_NORMAL, param.name, "Thread #" << nr << " stopped");
} // end main_loop


/**
 *  called to process internal message
 */
void
SignalingNTLP::handleInternalMessage(message *msg)
{
  Log(DEBUG_LOG,LOG_NORMAL, param.name, "Received incoming message #" << msg->get_id() << " on Queue");
  switch( msg->get_type() )
    {
	// message from transport
	case message::type_transport:
	  {
	    TPMsg* tpmsg = dynamic_cast<TPMsg*>(msg);
	    if (tpmsg)
	    {
	      // process incoming message from TP module (network)
	      process_tp_msg(tpmsg);
	      Log(DEBUG_LOG,LOG_NORMAL, param.name, "Received incoming message #" << msg->get_id() << " from TP Module");
	    }
	    else
	    {
	      Log(ERROR_LOG,LOG_ALERT, param.name, "Cannot cast message from source " << msg->get_qaddr_name() << " of type " << msg->get_type_name() << " to TPMsg");

	      delete msg;
	    } // end if tpmsg
	  } break;

	  // message from coordinator
	case message::type_signaling:
	  {
	    SignalingMsgNTLP* sigmsg = dynamic_cast<SignalingMsgNTLP*>(msg);
	    if (sigmsg)
	    {
	      // process message from coordinator to TP
	      // (outgoing message to network)
	      process_sig_msg(sigmsg);
	    }
	    else
	    {
	      Log(ERROR_LOG,LOG_ALERT, param.name, "Cannot cast message from source " << msg->get_qaddr_name() << " of type " << msg->get_type_name() << " to SignalingMsgNTLP");
	      delete msg;
	    } // end if sigmsg
	  } break;

	  default:
	    {
	      Log(ERROR_LOG,LOG_NORMAL, param.name, "Received a message from " << msg->get_qaddr_name() << " of type " << msg->get_type_name() << " that cannot be processed here");

	      delete msg;
	    }
    } // end switch
}


/**
 * called if Timeout happened by process_queue()
 */
void
SignalingNTLP::handleTimeout()
{
	//nothing to do
}


/**
 * process queue of internal messages for the signaling module
 * (usually message::qaddr_signaling)
 */
void
SignalingNTLP::process_queue()
{
  uint32 wait = param.sleep_time*1000; // max. waiting time at internal msg queue
  message* msg = NULL;

  // get queue (param.source is usually message::qaddr_signaling)
  FastQueue* fq = QueueManager::instance()->get_queue(param.source);
  if (!fq)
  {
    Log(ERROR_LOG,LOG_ALERT, param.name, "Cannot find input msg queue");

    return;
  } // end if not fq


  /// wait for messages if in state running
  while (get_state()==STATE_RUN)
  {
    // dequeue message from internal message queue
    msg= fq->dequeue_timedwait(wait);
    if (msg)
    {
    	handleInternalMessage(msg);
    } // end if msg
    else handleTimeout();

    sched_yield();
  } // end while running
} // end process_queue





/** process incoming message from TP module
  * (incoming signaling message from network)
  * @param tpmsg Internal TPMsg from TP module
  */
void 
SignalingNTLP::process_tp_msg(TPMsg* tpmsg) 
{
  SignalingMsgNTLP* sigmsg = NULL;
  gp_id_t result_id = 0;
  known_ntlp_pdu* result_pdu = NULL;
  gp_id_t refid = 0;
  known_ntlp_pdu* encappdu = NULL;


  assert(tpmsg!=NULL);

  Log(DEBUG_LOG,LOG_NORMAL,param.name,"process_tp_msg() - received message #"<< tpmsg->get_id() <<" from TP");

  // remember netmsg and peer from TPmsg
  NetMsg* netmsg= tpmsg->get_message();
  // peer may be updated by process_response()
  appladdress* peer= 0;
  if (tpmsg->get_peeraddress())
  { // this will copy the peer address
    peer= dynamic_cast<appladdress*>(tpmsg->get_peeraddress()->copy());
  }
  else
  {
    // peer address not set, create empty address
    peer= new appladdress();
  }

  const appladdress* ownaddr= dynamic_cast<appladdress*>(tpmsg->get_ownaddress()->copy());
  if (ownaddr == 0)
  {
    Log(ERROR_LOG,LOG_NORMAL,param.name,"process_tp_msg() - ownaddr not set");
  }

  //Log(DEBUG_LOG,LOG_NORMAL,param.name,"process_tp_msg() - before get_error()");

  // If an incoming tpmsg contains an error it is deleted.
  if (tpmsg->get_error()) 
  {
    Log(ERROR_LOG,LOG_NORMAL, param.name, "Received error " << tpmsg->get_error()->getstr() 
	                                  << " from " << tpmsg->get_qaddr_name() << " peer " ); //<< peer->to_string());
  } 
  else 
  {
    //Log(DEBUG_LOG,LOG_NORMAL,param.name,"process_tp_msg() - before deleting peer address");
    if (tpmsg->get_peeraddress()) 
      delete tpmsg->get_peeraddress();
    //Log(DEBUG_LOG,LOG_NORMAL,param.name,"process_tp_msg() - before clear_pointers()");
    // don't need TPMsg any longer, clear pointers
    tpmsg->clear_pointers();

    // Log(DEBUG_LOG,LOG_NORMAL,param.name,"process_tp_msg() - before assertions");
    if (peer==0 || ownaddr==0 || netmsg==0)
      Log(ERROR_LOG,LOG_NORMAL,param.name,"process_tp_msg() - precondition for peer, ownaddr, or netmsg failed");

    //assert(peer!=NULL);
    //assert(ownaddr!=NULL);
    //assert(netmsg!=NULL);

    //#define DEBUG_HARD
    //#ifdef DEBUG_HARD
    DLog(param.name,"process_tp_msg() - received PDU (transport: " << tpmsg->get_qaddr_name() << ") now parsing...") ;
    //#endif

    bool qe= tpmsg->get_source() == message::qaddr_tp_queryencap;
    // check for magic number presence if D-Mode or Q-Mode datagram
    if (tpmsg->get_source() == message::qaddr_tp_over_udp || qe)
    {
      DLog(param.name,"process_tp_msg() - checking for correct magic number");
      if (netmsg->decode32() != GIST_magic_number)
      {
	WLog(param.name,"process_tp_msg() - " << color[red] << "WRONG/Missing magic number - dropping packet" << color[off]);

	delete sigmsg;
	return;
      }
    }

    NTLP::error_t gisterror= NTLP::error_ok;
    // process NetMsg by GIST (generates C++ objects from byte stream)
    try
    {
      gisterror = gistproto.process_tp_recv_msg(*netmsg,*peer,result_id,result_pdu,refid,encappdu);
    }
    catch(IEError& iee)
    {
      ERRCLog(param.name,"Exception occurred during parsing a received PDU:" << iee.getstr());
    } // end catch
    catch(...)
    {
      ERRCLog(param.name,"Uncaught Exception occured during parsing a received PDU");
    } // end catch

    DLog(param.name,"process_tp_msg() - parsed PDU ");

    // now result_id, result_pdu and optionally refid, encappdu are returned
    // check for result/error during decoding
    switch (gisterror) 
    {
      case NTLP::error_ok_in_auth  :
      case NTLP::error_ok_out_auth :
		ERRLog(param.name, "Authentication failure, deleting PDU");

	if (peer) delete peer;
	if (result_pdu) delete result_pdu;
	if (encappdu) delete encappdu;
      break;

      // everything ok
      case NTLP::error_ok :
      {  
	sigmsg = new SignalingMsgNTLP;

	assert(result_pdu!=NULL);
	assert(sigmsg!=NULL);

	/** sec: 4.3.1
	 * Immediately after reception, the GIST hop count is checked.  Any
	 * message with a GIST hop count of zero MUST be rejected with a "Hop
	 * Limit Exceeded" error message (Appendix A.4.4.2); note that a correct
	 * GIST implementation will never send such a message.  Otherwise, the
	 * GIST hop count MUST be decremented by one before the next stage.
	 **/

	// if we received with hops == 0, the sender has probably gone mad, but we throw error at him
	if (!result_pdu->get_hops()) 
	{
	  ERRCLog(param.name, "We received an invalid message with hop limit == 0");
	  senderror(result_pdu, peer, GIST_Error::error_gist_hop_limit_exceeded);
	  delete sigmsg;
	  return;
	}

	// Decrement the GIST Hop Counter
	result_pdu->dec_hops();

	// Implement the relaying if no suitable NSLPID is available on the machine
	uint32 nslpid = result_pdu->get_nslpid();

	DLog(param.name, color[blue] << "Incoming message is addressed to NSLP ID " << nslpid);

	if ((nslpid==0) || (param.nslptable.get_address(nslpid) != message::qaddr_unknown)) 
	{
	  if (nslpid) 
	  {
	    DLog(param.name, color[blue] << "Accepted incoming message, it is addressed to NSLP ID " 
		<< result_pdu->get_nslpid() << " at API interface " << param.nslptable.get_address(result_pdu->get_nslpid()));
	  } 
	  else 
	  {
	    DLog(param.name, color[blue] << "Accepted incoming message, it is marked as being not addressed to a specific NSLP at all (maybe GIST MA Hello or GIST Error)");
	  } 

	  // if this is an incoming response the 
	  // service primitive is a confirmation
	  // otherwise (incoming request) it is an indication
	  // if (result_pdu->is_response())  // incoming response -> cnf
	  // sigmsg->set_cnf(result_id,result_pdu,result_pdu);
	  // else // incoming request -> ind
	  sigmsg->set_ind(result_id,peer->copy(),ownaddr->copy(),result_pdu,refid,encappdu);
	      
	  Log(DEBUG_LOG,LOG_NORMAL,param.name,"Passing " << color[red] <<  result_pdu->get_ie_name() << color[off] 
	                                       << " msg#" << sigmsg->get_id() << " to coordination");
	      
	  // send this to the coordinator
	  peer = NULL;
	  result_pdu = NULL;
	  encappdu = NULL;
	  if (!sigmsg->send_to(message::qaddr_coordination,false)) 
	  {
	    Log(ERROR_LOG,LOG_NORMAL, param.name, "Cannot send a message to " << message::get_qaddr_name(message::qaddr_coordination));
	   
	    delete sigmsg;
	  } // end if not sent
	} 
	else 
	{ // RELAY message!

	  //******************** Relay ONLY, if it is a GIST Query Else throw gist_error_unknown_nslpid ***********
	  if ((!result_pdu->is_query()) && (!(result_pdu->is_data() && (peer->get_protocol()==prot_query_encap))))  
	  {
	    
	    ERRCLog(param.name, "Will not relay other messages than GIST Query or Query Encapsulated GIST Data!");
	    senderror(result_pdu, peer, GIST_Error::error_gist_unknown_nslpid);
	    
	    delete sigmsg;
	    return;
	  }

	  appladdress* target = result_pdu->get_mri()->determine_dest_address();
	  //******************** Relay ONLY, if it is addressed to someone else, not to US! ***********************
	  if (is_local_addr(target)) 
	  {
	    ERRCLog(param.name, "Will NOT relay this message, it is directly addressed to us, the NSLP is unknown -> Spooling Error Message");

	    senderror(result_pdu, peer, GIST_Error::error_gist_endnode_found);
	    
	    delete target;
	    delete sigmsg;
	    return;
	  }

	  delete target;

	  //******************** Relay ONLY, if the serialized PDU length is not exceeding 576 byte **************

	  // 576 - 8 byte UDP header (?) -> 568byte for PDU
	  if (result_pdu->get_serialized_size(IE::protocol_v1) > max_mtu_size-8)
	  {
	    ERRCLog(param.name, "Will NOT relay this message, it is too large for our MTU limit of " 
		<< max_mtu_size << " bytes");

	    senderror(result_pdu, peer, GIST_Error::error_gist_message_too_large);

	    delete sigmsg;
	    return;
	  }

	  //******************************************************************************************************
	  DLog(param.name, color[blue] << "Incoming Message for which there is no registered NSLP available, relaying message" << color[off]);

	  // We only relay, if there are any hops left
	  if (result_pdu->get_hops()>0) 
	  {
	    // We must decrement IP TTL on relaying, first grab it
	    uint8 ip_ttl = peer->get_ip_ttl();

	    if (!ip_ttl) 
	      DLog(param.name, "Got no IP TTL information for this message, which is to relay. TPqueryEncap not working properly? - Message cannot be stopped here based on IP TTL!?!?!");

	    // construct target address
	    appladdress* target = result_pdu->get_mri()->determine_dest_address();
	    target->set_port(param.well_known_port);
	    // if use RAO option
	    if ( gconf.getpar<bool>(gistconf_send_rao) )
	    {
		    target->set_protocol(prot_query_encap);
		    target->set_rao(peer->get_rao());
	    }
	    else // send it via UDP
	    {
		    target->set_protocol(prot_udp);
	    }

		  //decrement IP TTL
		  if (ip_ttl) {
		      target->set_ip_ttl(ip_ttl-1);
		      
		      if (ip_ttl-1 == 0) {
			  ERRCLog(param.name, "Discarded the Message to relay, because we had IP TTL information, decremented it as defined in GIST and it is now ZERO.");
			  delete sigmsg;
			  return;
		      }
		  }
		  
		  // here we also call the MRI outgoing interface lookup routine to provide us with a outgoing interface index
		  sigmsg->set_req(target, result_pdu, 0);
		  sigmsg->send_or_delete();
		  
	  } 
	  else // GIST hop limit now 0
	  {
	    DLog(param.name, "Hop limit exceeded. Discarding Message which does not fit to any local NSLP");

	    senderror(result_pdu, peer, GIST_Error::error_gist_hop_limit_exceeded);
	    
	    delete sigmsg;
	  }
	} // end if RELAY message  
      } // end case error_ok
      break;
      
      
      // parsing resulted in error, send back error PDU
      case NTLP::error_send_error_pdu :
      {
	assert(result_pdu!=NULL);
	if (netmsg) delete netmsg;
	netmsg = NULL;
	Log(INFO_LOG,LOG_NORMAL, param.name, "Trying to send error PDU " << endl << color[magenta] <<  result_pdu->to_string() << color[off] << " to peer " << *peer);
	
	gisterror = gistproto.process_outgoing(*result_pdu,*peer,result_id,0,netmsg);
	if (gisterror==NTLP::error_ok) 
	{
	  // send the returned NetMsg
	  assert(netmsg!=NULL);
	  // netmsg should not be accessed after this call because it may have been already deleted
	  peer->convert_to_ipv6();
	  // we should not establish a new connection for sending back an error:
          // either we use the existing one or drop the message
	  send_tp(netmsg, *(peer->copy()), 0, true);
	} // end if OK
	if (peer) delete peer;
	if (result_pdu) delete result_pdu;
	if (encappdu) delete encappdu;
	return;
      }
      break;

      // parsing resulted in error, do not send back error PDU, only log error
      case NTLP::error_log_incpduerror_only:
      {
	if (netmsg) delete netmsg;
	netmsg = NULL;
	Log(ERROR_LOG,LOG_NORMAL, param.name, "Error on incoming PDU, but not sending back error PDU");
	
      	if (peer) delete peer;
	if (result_pdu) delete result_pdu;
	if (encappdu) delete encappdu;
	return;
      }
      break;
	
      default:
      {
	Log(ERROR_LOG,LOG_NORMAL, param.name, "Received a GIST error that cannot be handled. Deleting NetMsg");
	
	if (peer) delete peer;
	if (result_pdu) delete result_pdu;
	if (encappdu) delete encappdu;
      }
      break;
    } // end switch gisterror

    // NetMsg is no longer needed
    if (netmsg) 
      delete netmsg;
  } // end if get_error

  //Log(DEBUG_LOG,LOG_NORMAL,param.name,"process_tp_msg() - before delete");

  // because pointers are cleared, objects peer and netmsg 
  // are not deleted by calling tpmsg destructor now
  delete tpmsg;

  //Log(DEBUG_LOG,LOG_NORMAL,param.name,"process_tp_msg() - done.");
} // end process_tp_msg


/**
 * process message from State Module
 * (outgoing signaling message, sent subsequently to TP module)
 */
void 
SignalingNTLP::process_sig_msg(SignalingMsgNTLP* sigmsg) 
{
  NTLP::error_t gisterror = NTLP::error_ok;
  appladdress peer;
  const hostaddress *laddr;
  known_ntlp_pdu* pdu = NULL;
  NetMsg* netmsg = NULL;
  error_t sendres = error_ok;

  assert(sigmsg!=NULL);
  if (sigmsg->get_error()!=SignalingNTLP::error_ok) 
  {
    Log(ERROR_LOG,LOG_NORMAL, param.name, 
	"Received SignalingMsg with error " << sigmsg->get_error_string() << ", deleting");
    
    delete sigmsg;
  } // end if error

  switch ( sigmsg->get_subtype() )
  {
    case SignalingMsgNTLP::ignore:
      delete sigmsg;
      break;

    case SignalingMsgNTLP::SigCancel: 
      {
	bool revres = gistproto.revoke(sigmsg->get_id()); 
	if (sigmsg->get_confirm()) 
	{
	  if (revres) 
	    send_back_sig_error(error_ok,sigmsg);
	  else 
	    send_back_sig_error(error_no_request_found,sigmsg);
	} 
	else 
	  delete sigmsg;
      } break;

    case SignalingMsgNTLP::SigReq:
      {
	// look for PDU and peer address
	if (sigmsg->get_peer() && sigmsg->get_pdu()) 
	{
	  // extract data from sigmsg
	  peer = *sigmsg->get_peer();
	  pdu = sigmsg->get_pdu();
	  laddr = sigmsg->get_local_addr();

	  if (laddr != NULL)
	    DLog(param.name, "Sending from: " << *laddr);

	  // add basic header if necessary
	  // Log output
	  if (pdu->get_sessionid())
	  {
	    DLog(param.name, "Sending <" << pdu->get_ie_name() << "> sid: " << pdu->get_sessionid()->to_string() << " to " << peer);
	  }
	  else
	  {
	    DLog(param.name, "Sending <" << pdu->get_ie_name() << "> sid: N/A to " << peer);
	  }

	  EVLog(param.name, color[magenta] << "Sending (msg#"<< sigmsg->get_id() << "):" << endl << *pdu << color[off]); 

	  // if message is send via Query Encapsulation or D-Mode it must be prepended by the magic number
	  

	  // create GIST message
	  gisterror = gistproto.process_outgoing(*pdu,peer,sigmsg->get_id(),sigmsg->get_ref_id(),netmsg);
	  if (gisterror==NTLP::error_ok) 
	  {
	    // send the returned NetMsg
	    assert(netmsg!=NULL);
	    // netmsg should not be accessed after this call because it may have been already deleted
	    peer.convert_to_ipv6();
	    sendres = send_tp(netmsg, peer, laddr, sigmsg->get_useexistconn());
	    if (sendres==error_ok) 
	    {
	      if (sigmsg->get_confirm()) 
		send_back_confirmation(sigmsg);
	      else 
		delete sigmsg;
	    } 
	    else 
	    {
	      // inform GIST
	      gistproto.revoke(sigmsg->get_id());
	      send_back_sig_error(sendres,sigmsg);
	    } // end if senderror
	  } 
	  //send_back_gist_error(gisterror,sigmsg);
	} 
	else 
	  send_back_sig_error(error_bad_sigmsg,sigmsg);
      }
      break;
      
    case SignalingMsgNTLP::SigRsp:
      { // send response PDU from coordinator to peer
	// look for PDU
	if (sigmsg->get_pdu()) 
	{
	  pdu = sigmsg->get_pdu();
	  peer = *(sigmsg->get_peer());
	  laddr = sigmsg->get_local_addr();

	  if (laddr != NULL)
	    DLog(param.name, "Sending from:" << *laddr);

	  // Log output
	  if (pdu->get_sessionid())
	  {
	    DLog(param.name, "Sending <" << pdu->get_ie_name() << "> " << *pdu->get_sessionid() << " to " << peer << " (SigRSP)");
	  }
	  else
	  {
	    DLog(param.name, "Sending <" << pdu->get_ie_name() << "> <sessionid>: N/A to " << peer << " (SigRSP)");
	  }

	  EVLog(param.name, color[magenta] << "Sending (msg#"<< sigmsg->get_id() << "):" << endl << *pdu << color[off]); 

	  // GIST must serialize this PDU now
	  gisterror = gistproto.process_response(sigmsg->get_id(),*pdu,sigmsg->get_ref_id(),netmsg,peer);
	  if (gisterror == NTLP::error_ok) 
	  {
	    // send the returned NetMsg
	    assert(netmsg!=NULL);
	    // netmsg should not be accessed after this call because it may have been already deleted
	    peer.convert_to_ipv6();
	    // as a responder we are not allowed to establish any transport connections
	    sendres= send_tp(netmsg, peer, laddr, sigmsg->get_useexistconn());
	    if (sendres==error_ok) 
	    {
	      // SignalingMsg with PDU and address is no longer needed
		if (sigmsg->get_confirm()) {
		

		  //==================================================================

		  //    !!! Confirmation to message sender temporarily deactivated !!!

		  //==================================================================

  
                //send_back_confirmation(sigmsg);
		}	      
		else 
		    delete sigmsg;
	    } 
	    else 
	    {
	      send_back_sig_error(sendres,sigmsg);
	    } // end if senderror
	  } 
      
	} 
	else 
	  send_back_sig_error(error_bad_sigmsg,sigmsg);
      } // end process sig response
      break;

    case SignalingMsgNTLP::SigTrm:
      {
	// terminate a signaling connection
	error_t res= error_ok;
	if (sigmsg->get_peer())
	  peer= *sigmsg->get_peer();

	TP* tp = param.get_tp(peer.get_protocol());
	if (tp) {
	  // terminate
	  try {
	    Log(EVENT_LOG,LOG_NORMAL,param.name,"Terminating connection/association " << peer);
			tp->terminate(peer);
	  } catch(TPError& e) 
	  {
	    switch (e.errtype) {
	      case TPError::TPERR_BAD_ADDRESS: res = error_bad_address; break;
	      case TPError::TPERR_BAD_NETMSG: res = error_internal; break;
	      case TPError::TPERR_UNREACHABLE: res = error_unreachable; break;
	      case TPError::TPERR_PAYLOAD: res = error_max_payload; break;
	      default: res = error_tp_internal; break;
	    }
	  } catch(...) {
	    res = error_tp_internal;
	  } // end try-catch
	} 
	else
	{
	  Log(ERROR_LOG,LOG_NORMAL,param.name,"Protocol #" << (int) peer.get_protocol() << " (" << peer.get_protocol_name() << ") not found");
	}
	
	if (sigmsg->get_confirm())
	{
	  sigmsg->set_error(res);
	  if (!sigmsg->send_back(param.source,param.send_error_expedited)) delete sigmsg;
	}
      }
      break;

      
    default:
      {
	Log(ERROR_LOG,LOG_NORMAL, param.name, "Received SignalingMsg of wrong subtype, trying to send error");
	
	sigmsg->set_error(error_bad_sigmsg);
	if (!sigmsg->send_back(param.source,param.send_error_expedited)) delete sigmsg;
      }
      break;


  } // end switch get_subtype
} // end process_sig_msg

void 
SignalingNTLP::send_back_sig_error(error_t err, SignalingMsgNTLP* msg) 
{
  assert(msg!=NULL);
  msg->set_error(err);
  Log(ERROR_LOG,LOG_NORMAL, param.name, "An error occured, trying to send error [" << msg->get_error_string() << "] to " << msg->get_qaddr_name());
	
  if (!msg->send_back(param.source,param.send_error_expedited)) delete msg;
} // end send_back_sig_error

void 
SignalingNTLP::send_back_confirmation(SignalingMsgNTLP* msg) 
{
  assert(msg!=NULL);
  msg->set_error(error_msg_sent);
  if (!msg->send_back(param.source,param.send_error_expedited)) 
  {
    Log(ERROR_LOG,LOG_NORMAL, param.name, "Cannot send message-sent confirmation to " << msg->get_qaddr_name());
		
    delete msg;
  } // end if not sent
} // end send_back_confirmation


/**
 * Simple method that acts as demultiplexer and sends a network message 
 * towards the corresponding transport protocol thread
 */
SignalingNTLP::error_t 
SignalingNTLP::send_tp(NetMsg* msg, const appladdress& addr, const hostaddress *laddr, bool use_existing_connection) 
{
  error_t res = error_ok; 
  TP* tp = param.get_tp(addr.get_protocol());
  if (tp) 
  {
    // send message
    try 
    {
      EVLog(param.name,"Sending msg to Protocol #" << (int) addr.get_protocol() << " (" << addr.get_protocol_name() << ") Dest:" << addr.get_ip_str());

      // cannot send to unknown address
      if (addr.is_ip_unspec())
      {
	delete(msg);
	throw TPErrorBadDestAddress();
      }

      // the following call should only return after the message has been sent (synchronous call)
      // or the TP must ensure that it works on copies of both parameters since they might get freed
      // soon after this method returns...
      tp->send(msg, addr, use_existing_connection, laddr);
    } // end try 
    catch(TPError& e) 
    { // note that this is only relevant for synchronous send() methods, usually
      // an asynchronous sender thread will cause failures that cannot be handled here
      ERRLog(param.name,"Error occurred while sending: " << e.what());
      switch (e.errtype) 
      {
	case TPError::TPERR_BAD_ADDRESS: res = error_bad_address; break;
	case TPError::TPERR_BAD_NETMSG: res = error_internal; break;
	case TPError::TPERR_UNREACHABLE: res = error_unreachable; break;
	case TPError::TPERR_PAYLOAD: res = error_max_payload; break;
	default: res = error_tp_internal; break;
      }
    } 
    catch(...) 
    {
      res = error_tp_internal;
    } // end try-catch
  } 
  else
  {
    ERRLog(param.name,"Protocol #" << (int) addr.get_protocol() << " (" << addr.get_protocol_name() << ") not found");
    res = error_no_tp;
  }
  return res;
} // end send_tp


void 
SignalingNTLP::senderror(const known_ntlp_pdu* pdu, const appladdress* peer, GIST_Error::gist_error_code_t error) 
{
  DLog(param.name, "Error Message Sender called");

  errorobject* errobj = NULL;

  bool qe = peer->get_protocol() == prot_query_encap || pdu->get_C();
  bool dgram = qe || (peer->get_protocol() == tsdb::get_udp_id());
	    
  mri* mr = NULL;
  sessionid* sid = NULL;

  switch (error) 
  {
    case GIST_Error::error_gist_no_routing_state: 
	    
      if (pdu->get_mri()) mr = pdu->get_mri()->copy();
      if (pdu->get_sessionid()) sid = pdu->get_sessionid()->copy();
	    
      errobj = new errorobject(mr, sid, errorobject::ProtocolError, errorobject::err_NoRoutingState, 0, pdu->get_version(), pdu->get_hops(), pdu->get_length(), pdu->get_nslpid(), pdu->get_type(), pdu->get_flags(), dgram, qe, NULL, 0);
      break;

    case GIST_Error::error_gist_endnode_found:
      if (pdu->get_mri()) mr = pdu->get_mri()->copy();
      if (pdu->get_sessionid()) sid = pdu->get_sessionid()->copy();
      
      errobj = new errorobject(mr, sid, errorobject::PermanentFailure, errorobject::err_EndpointFound, 0, pdu->get_version(), pdu->get_hops(), pdu->get_length(), pdu->get_nslpid(), pdu->get_type(), pdu->get_flags(), dgram, qe, NULL, 0);
      break;

    case GIST_Error::error_gist_unknown_nslpid:
      if (pdu->get_mri()) mr = pdu->get_mri()->copy();
      if (pdu->get_sessionid()) sid = pdu->get_sessionid()->copy();
      
      errobj = new errorobject(mr, sid, errorobject::PermanentFailure, errorobject::err_UnknownNSLPID, 0, pdu->get_version(), pdu->get_hops(), pdu->get_length(), pdu->get_nslpid(), pdu->get_type(), pdu->get_flags(), dgram, qe, NULL, 0);
      break;
      
      
    case GIST_Error::error_gist_message_too_large:
      if (pdu->get_mri()) mr = pdu->get_mri()->copy();
      if (pdu->get_sessionid()) sid = pdu->get_sessionid()->copy();
      
      errobj = new errorobject(mr, sid, errorobject::PermanentFailure, errorobject::err_MessageTooLarge, 0, pdu->get_version(), pdu->get_hops(), pdu->get_length(), pdu->get_nslpid(), pdu->get_type(), pdu->get_flags(), dgram, qe, NULL, 0, max_mtu_size);
      break;
      
      
    case GIST_Error::error_gist_hop_limit_exceeded:
      if (pdu->get_mri()) mr = pdu->get_mri()->copy();
      if (pdu->get_sessionid()) sid = pdu->get_sessionid()->copy();
      
      errobj = new errorobject(mr, sid, errorobject::PermanentFailure, errorobject::err_HopLimitExceeded, 0, pdu->get_version(), pdu->get_hops(), pdu->get_length(), pdu->get_nslpid(), pdu->get_type(), pdu->get_flags(), dgram, qe, NULL, 0, max_mtu_size);
      break;

    case GIST_Error::error_gist_invalid_r_flag:
      if (pdu->get_mri()) mr = pdu->get_mri()->copy();
      if (pdu->get_sessionid()) sid = pdu->get_sessionid()->copy();
      
      errobj = new errorobject(mr, sid, errorobject::ProtocolError, errorobject::err_CommonHeaderParseError, errorobject::errsub_InvalidRFlag, pdu->get_version(), pdu->get_hops(), pdu->get_length(), pdu->get_nslpid(), pdu->get_type(), pdu->get_flags(), dgram, qe, NULL, 0, max_mtu_size);
      break;

    case GIST_Error::error_gist_invalid_c_flag:
      if (pdu->get_mri()) mr = pdu->get_mri()->copy();
      if (pdu->get_sessionid()) sid = pdu->get_sessionid()->copy();
      
      errobj = new errorobject(mr, sid, errorobject::ProtocolError, errorobject::err_CommonHeaderParseError, errorobject::errsub_InvalidCFlag, pdu->get_version(), pdu->get_hops(), pdu->get_length(), pdu->get_nslpid(), pdu->get_type(), pdu->get_flags(), dgram, qe, NULL, 0, max_mtu_size);
      break;

    case GIST_Error::error_gist_invalid_e_flag:
      if (pdu->get_mri()) mr = pdu->get_mri()->copy();
      if (pdu->get_sessionid()) sid = pdu->get_sessionid()->copy();
      
      errobj = new errorobject(mr, sid, errorobject::ProtocolError, errorobject::err_CommonHeaderParseError, errorobject::errsub_InvalidEFlag, pdu->get_version(), pdu->get_hops(), pdu->get_length(), pdu->get_nslpid(), pdu->get_type(), pdu->get_flags(), dgram, qe, NULL, 0, max_mtu_size);
      break;
      
    default:
      DLog(param.name, "Yet unhandled Error thrown (code: " << error << "), will not send Error Message");
      break;
      
  } // end switch
  
  // calculate the address
  appladdress* target = NULL;
  nli *mynl = NULL;

  // IP source is NOT the last GIST hop
  if (pdu && !pdu->get_S()) 
  {
    if (pdu->get_nli()) 
    { // if IP Source is NOT last GIST hop, sending to GIST denoted by NLI
      DLog(param.name, "IP Source is NOT last GIST hop, sending to GIST denoted by NLI");
      target = new appladdress(pdu->get_nli()->get_if_address(), "udp", param.well_known_port);
      uint32_t pref = IPV6_PREFER_SRC_COA;
      netaddress *src_addr = param.addresses.get_src_addr(
        pdu->get_nli()->get_if_address(), &pref);
      if (src_addr == 0)
      {
        ERRCLog(param.name, "Could not determine source address for " << target << ".  Dropping errormessage: " << pdu);
      }
      mynl = param.nli_tmpl.copy2(*src_addr);
    } 
    else 
    {
      ERRCLog(param.name, "No valid target address for Error Message found. Dropping errormessage: " << pdu);
    }   
  } 
  else 
  {
    DLog(param.name, "IP Source IS last GIST hop, sending to it.");
    target = new appladdress(*peer);
    uint32_t pref = IPV6_PREFER_SRC_COA;
    netaddress *src_addr = param.addresses.get_src_addr(*peer, &pref);
    mynl = param.nli_tmpl.copy2(*src_addr);
    if (target->get_protocol() == prot_query_encap) target->set_protocol(tsdb::get_udp_id());
    if (target->get_protocol() == tsdb::get_udp_id()) target->set_port(param.well_known_port);
  }

  ntlp::error* errpdu = new ntlp::error(mynl, errobj);

  errpdu->set_hops(1);

  SignalingMsgNTLP* sigmsg = new SignalingMsgNTLP;
  sigmsg->set_req(target, errpdu);
  // for transmitting errors we don't want to re-open any connection again
  sigmsg->set_useexistconn();
  sigmsg->send_or_delete();
}


#ifndef NSIS_OMNETPP_SIM

bool 
SignalingNTLP::is_local_addr(const hostaddress* addr) 
{

  DLog("GIST Signaling", "Checking if " << addr->get_ip_str() << " is a local one");

  // First: Copy the address, convert it to IPv6 (it will make NO difference)

  hostaddress* myaddr=addr->copy();
  myaddr->convert_to_ipv6();
  
  char* ifname= new char[IF_NAMESIZE];
 
  //so we can decide via "is_mapped_ip" if its v4 or v6, v4 in_addr can be derived from it too!!
    
  if (!myaddr->is_mapped_ip()) 
  {
    // Now do the RTNetlink Magic
	
    // sockaddr_in6?
    struct in6_addr dstaddr;
    
    // Netlink Message header plus RTNetlink Message Header plus 1024byte data
    struct {
      struct nlmsghdr n;
      struct rtmsg r;
      char data[1024];
    } req;
    
    // Struct rtattr
    struct rtattr *attr;
    
    // int outgoinginterface
    uint16 outgoinginterface = 0;
    
    // OPEN an RTNETLINK SOCKET
    int sock = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
    if (sock < 0) 
    {
      Log(ERROR_LOG, LOG_NORMAL, "GIST Signaling", "Error Opening Netlink Socket: " << strerror(errno));
    }
	
    myaddr->get_ip(dstaddr);
	
    memset(&req, 0, sizeof(req));
    req.n.nlmsg_len =
      NLMSG_ALIGN(NLMSG_LENGTH(sizeof(req.r))) +
      RTA_LENGTH(sizeof(dstaddr));
    req.n.nlmsg_flags = NLM_F_REQUEST;
    req.n.nlmsg_type = RTM_GETROUTE;
    req.r.rtm_family = AF_INET6;
    req.r.rtm_dst_len = 128;
    req.r.rtm_src_len = 0;
    
    attr = reinterpret_cast<rtattr*>(reinterpret_cast<char *>(&req) + NLMSG_ALIGN(NLMSG_LENGTH(sizeof(req.r))));
    attr->rta_type = RTA_DST;
    attr->rta_len = RTA_LENGTH(sizeof(dstaddr));
    
    memcpy(RTA_DATA(attr), &dstaddr, sizeof(dstaddr));
    
    /* Send the message */
    if (send(sock, &req, req.n.nlmsg_len, 0) < 0) 
    {
      Log(ERROR_LOG, LOG_NORMAL, "GIST Signaling", "Error sending netlink message: " << strerror(errno));
      close(sock);
    }
	
    char* returnbuf = new char[NL_BUFSIZE];
	
    protlib::util::readnl(sock, returnbuf);
	
    nlmsghdr* nlhdrp = reinterpret_cast<struct nlmsghdr *>(returnbuf);
	
    rtmsg* rtmsg = static_cast<struct rtmsg*>(NLMSG_DATA(nlhdrp));
	
    // Iterate through data area serachin
	
    rtattr* attrp = RTM_RTA(rtmsg);
    uint32 attrlen = RTM_PAYLOAD(nlhdrp);
	
    for( ; RTA_OK(attrp, attrlen); attrp = RTA_NEXT(attrp, attrlen) )
    {
      if ((attrp->rta_type) == RTA_OIF) 
      {
	if_indextoname(*(int *)RTA_DATA(attrp), ifname);
		
	Log(EVENT_LOG, LOG_NORMAL, "mri_pathcoupled",  "Kernel decided the outgoing interface index would be " << *(int *)RTA_DATA(attrp) << ", interface Name " << ifname);
		
	outgoinginterface=*(int *)RTA_DATA(attrp);
      }
    } // end for  
  } 
  else 
  {
    // IPv4!

    // Now do the RTNetlink Magic
    
    // sockaddr_in
    struct in_addr dstaddr;
	
    // Netlink Message header plus RTNetlink Message Header plus 1024byte data
    struct {
      struct nlmsghdr n;
      struct rtmsg r;
      char data[1024];
    } req;
    
    // Struct rtattr
    struct rtattr *attr;
	
    // int outgoinginterface
    uint16 outgoinginterface = 0;
	
    // OPEN a RTNETLINK SOCKET
	
    int sock = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
    if (sock < 0) 
    {
      Log(ERROR_LOG, LOG_NORMAL, "GIST Signaling", "Error Opening Netlink Socket: " << strerror(errno));
    }
	
    myaddr->get_ip(dstaddr);
	
    memset(&req, 0, sizeof(req));
    req.n.nlmsg_len =
      NLMSG_ALIGN(NLMSG_LENGTH(sizeof(req.r))) +
      RTA_LENGTH(sizeof(dstaddr));
    req.n.nlmsg_flags = NLM_F_REQUEST;
    req.n.nlmsg_type = RTM_GETROUTE;
    req.r.rtm_family = AF_INET;
    req.r.rtm_dst_len = 32;
    req.r.rtm_src_len = 0;
	
    attr = reinterpret_cast<rtattr*>(reinterpret_cast<char *>(&req) + NLMSG_ALIGN(NLMSG_LENGTH(sizeof(req.r))));
    attr->rta_type = RTA_DST;
    attr->rta_len = RTA_LENGTH(sizeof(dstaddr));
    
    memcpy(RTA_DATA(attr), &dstaddr, sizeof(dstaddr));
    
    
    /*Send the message*/
    
    if (send(sock, &req, req.n.nlmsg_len, 0) < 0) {
      Log(ERROR_LOG, LOG_NORMAL, "GIST Signaling", "Error while sending to Netlink Socket: " << strerror(errno));
      close(sock);
    }
	
    char* returnbuf = new char[NL_BUFSIZE];
	
    protlib::util::readnl(sock, returnbuf);
	
    nlmsghdr* nlhdrp = reinterpret_cast<struct nlmsghdr *>(returnbuf);
	
    rtmsg* rtmsg = static_cast<struct rtmsg*>(NLMSG_DATA(nlhdrp));
	
    // Iterate through data area 
	
    rtattr* attrp = RTM_RTA(rtmsg);
    uint32 attrlen = RTM_PAYLOAD(nlhdrp);
	
    for( ; RTA_OK(attrp, attrlen); attrp = RTA_NEXT(attrp, attrlen) )
    {
      if ((attrp->rta_type) == RTA_OIF)
      {	
	if_indextoname(*(static_cast<int *>(RTA_DATA(attrp))), ifname);
		
	Log(EVENT_LOG, LOG_NORMAL, "mri_pathcoupled",  "Kernel decided the outgoing interface index would be " << *(int *)RTA_DATA(attrp) << ", interface Name " << ifname);
		
	outgoinginterface=*(static_cast<int *>(RTA_DATA(attrp)));
      }
    } // end for

    close(sock);
    
  }
    
  if (!strncmp(ifname, "lo", 2)) {
    DLog("Signaling", "This address is a local one - should not be used for relaying");
    return true;  
  }

  return false;
}

#else

bool
SignalingNTLP::is_local_addr(const hostaddress* addr)
{
  in6_addr ip;
  unsigned int cur_nsis_id = 0;
  addr->get_ip(ip);
  // can`t get NsisId direct via this->getNsisId(), because can`t access the object (static context)
  cSimpleModule *runMod = simulation.getContextSimpleModule();
  Thread *thrMod = dynamic_cast<Thread*>(runMod);
  if(thrMod){
    cur_nsis_id = thrMod->getNsisId();
  } else {
    if(runMod) {
      opp_error("Error in SignalingNTLP::is_local_addr() occured. Current running cSimpleModule (fullPath: %s) seems to be not derived from Thread class.",runMod->getFullPath().c_str());
    } else {
      opp_error("Error in SignalingNTLP::is_local_addr() occured. There seemed to be no SimpleModule context, might be because of deletion of modules. You have to use nsis_id direct, something like QueueManager::instance()->get_QueueManager(nsis_id)->unregister_queue(qaddr)");
    }
    return false;
  }
  return ( cur_nsis_id == Ip2NsisId::instance()->get_nsis_id(ip) );
}

#endif // end ifndef NSIS_OMNETPP_SIM


//@}

}//end namespace protlib
