/// ----------------------------------------*- mode: C++; -*--
/// @file ntlp_statemodule_api.cpp
/// GIST state module - implements API interactions for GIST
/// ----------------------------------------------------------
/// $Id: ntlp_statemodule_api.cpp 6274 2011-06-16 14:47:59Z bless $
/// $HeadURL: https://svn.tm.kit.edu/nsis/ntlp/trunk/src/ntlp_statemodule_api.cpp $
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
/** @ingroup statemodule
 * Statemodule
 *
 * This module implements API functions for GIST
 * 
 * Messages from network are processed and routing state is managed.
 *
 */
#include "ntlp_statemodule.h"
#include <sstream>

#include "gist_conf.h"
#include "rfc5014_hack.h"

#ifdef USE_OPPBSD
#include <interface_manager.h>
#endif

namespace ntlp {

/** @deftogroup gistapi GIST application programming interface interactions
 *  @ingroup statemodule
 *  @ingroup ntlp
 * @{
 */

using namespace protlib;


/**
 * process API messages from NSLP modules or from APIWrapper module
 * @param apimsg -- the messge to process
 */
void 
Statemodule::process_api_msg(APIMsg* apimsg) 
{
  // Putting Message Source and NSLPID into nslptable
  param.nslptable.save_address(apimsg->get_nslpid(), apimsg->get_source());

  EVLog(param.name, color[blue] << "API call received: " << color[off] 
	<< apimsg->get_subtype_name() << " msg#" << apimsg->get_id() << " from NSLPID " << apimsg->get_nslpid() 
      << " at API interface address " << param.nslptable.get_address(apimsg->get_nslpid()));

    
  // ===============================================
  // Start Processing
  // ===============================================
  

  switch (apimsg->get_subtype()) 
  {
    case APIMsg::Register: 
    { // ================
      EVLog(param.name, "NSLP registered with NSLPID " << apimsg->get_nslpid() << " at address " << apimsg->get_source());
      
      uint16 rao = apimsg->get_rao();
      
      param.rt.add_rao(apimsg->get_nslpid(), rao);
      EVLog(param.name, "Registered RAO value " << rao << " for this NSLPID");
      
      bool found = false;
      for (unsigned int i = 0; i < param.raovec.size(); i++) {
	if (param.raovec[i] == rao) found = true;
      }
	
      if (!found) param.raovec.push_back(apimsg->get_rao());
	
      ostringstream os;
      
      for (unsigned int i = 0; i < param.raovec.size(); i++) {
	os << "|" << param.raovec[i];
      }
      
      EVLog(param.name, "Now intercepting the following RAOs: " << os.str().c_str());
      
      break;
    }


    case APIMsg::SendMessage: 
    { // ===================
      tg_send_message(apimsg);
      break;
    }

    case APIMsg::RecvMessageAnswer: 
    { // =========================
      tg_recv_message_answer(apimsg);
      break;
    }


    case APIMsg::InvalidateRoutingState: 
    { // ==============================
      DLog(param.name, "InvalidateRoutingState received, carrying out the instructions");
      
      if (apimsg->get_status_change()==APIMsg::bad) 
      {
	DLog(param.name, "We are requested to put state to BAD, this means we put it to state DEAD");
	param.rt.invalidate_routing_state(apimsg->get_mri()->copy(), apimsg->get_nslpid(), apimsg->get_status_change(), apimsg->get_urgency());
      }
	
      if (apimsg->get_status_change()==APIMsg::tentative) 
      {
	DLog(param.name, "We are requested to put state to TENTATIVE, this means we put it to state REFRESH and carry out a Refresh, even if we are not requested via 'urgency' flag");
	param.rt.invalidate_routing_state(apimsg->get_mri()->copy(), apimsg->get_nslpid(), apimsg->get_status_change(), apimsg->get_urgency());
	
      }

      if ((apimsg->get_status_change()!=APIMsg::bad) && (apimsg->get_status_change()!=APIMsg::tentative)) {
	WLog(param.name, "It is not foreseen that NSLP may set Routing State to something other than BAD or TENTATIVE - doing nothing");
      }
      break;
    }
      

    case APIMsg::SetStateLifetime: 
    { // =========================
      DLog(param.name, "SetStateLifetime received, carrying out the instructions");
      
      param.rt.set_state_lifetime(apimsg->get_mri()->copy(), apimsg->get_nslpid(), apimsg->get_state_lifetime());
      break;
    }

    default: 
      {
	ERRCLog(param.name, "API call unimplemented"); 
      }
    } // end switch
    
    delete apimsg;

} // end process_api_message


/**
 * Deliver received data to NSLP instance
 * @param peer -- the peer from which we received the data
 * @param own_addr -- the IP address of the interface we received the data on
 * @param pdu -- the pdu containing the data (may NOT be NULL)
 * @param r_key -- the routing key (may be NULL)
 * @param r_entry -- the routing entry itself (may be NULL)
 */
void
Statemodule::deliver(const appladdress* peer, const appladdress* own_addr, const known_ntlp_pdu* pdu, const routingkey* r_key, const routingentry* r_entry) 
{
  APIMsg* msg = new APIMsg;

  assert (pdu != NULL);

  nslpdata* mydata = pdu->get_nslpdata();
  
  assert( pdu->get_mri() != NULL );
  mri* mymr=pdu->get_mri()->copy();
    
  // get SII-Handle from PDU (from NLI info is present), if unavailable (GIST Data Message or Message in C-Mode) take it from entry

  // check NLI information
  nli* nlinfo=  pdu->get_nli();

  uint32 sii_handle = nlinfo ? param.rt.get_sii_handle(nlinfo) : 0;

  tx_attr_t myattr;
    
  if (r_entry) {
	  myattr.secure=r_entry->is_secure();
	  myattr.reliable=!(r_entry->is_dmode());

	  if (!sii_handle)
	  {
		  sii_handle= r_entry->get_sii_handle();
	  }
  } else {
	  // Initial Query/Initial Confirm, D-Mode, unsecure
	  myattr.secure=false;
	  myattr.reliable=false;
  }

  /*
   * Check if this GIST node is the final hop for this flow. This is the
   * case if either
   *     1. this is a downstream message and we are the destination hop, or
   *     2. this is an upstream message and we are the source hop
   *
   * We have to compare the relevant address with each of our own addresses,
   * given in the configuration.
   */
  if ( mymr->get_downstream() )
    myattr.final_hop = is_local_address(mymr->get_destaddress());
  else
    myattr.final_hop = is_local_address(mymr->get_sourceaddress());

  DLog("statemodule", "GIST node is the final hop: " << myattr.final_hop);

  nslpdata* senddata = NULL;

  if (mydata) senddata = new nslpdata(*mydata);

  uint8 ip_distance = 0;
  uint8 ip_ttl = 0;

  if (nlinfo) 
  {
    ip_ttl = nlinfo->get_ip_ttl();
  } 
  else 
  {
    if (r_entry && r_entry->get_peer_nli()) 
	    ip_ttl = r_entry->get_peer_nli()->get_ip_ttl();
  }

  if (nlinfo && peer->get_ip_ttl()) 
  {
    ip_distance = ip_ttl - peer->get_ip_ttl();
  } 
  else 
  {
    if (r_entry && peer->get_ip_ttl()) {
	    ip_distance = r_entry->get_peer_nli()->get_ip_ttl() - peer->get_ip_ttl();
    }
  }

  msg->set_recvmessage(senddata, own_addr->copy(), pdu->get_nslpid(), new sessionid(*(pdu->get_sessionid())), mymr, false, sii_handle, myattr, ip_ttl, ip_distance, pdu->get_hops(), r_entry ? r_entry->get_incoming_if() : 0 );
    
  msg->send_to(param.nslptable.get_address(pdu->get_nslpid()));

  DLog(param.name, "Message passed to NSLP instance at address "
       << param.nslptable.get_address(pdu->get_nslpid()));
}




/**
 * Deliver received data to NSLP instance and set adjacency check flag
 * @param peer -- the peer from which we received the data
 * @param own_addr -- the IP address of the interface we received the data on
 * @param pdu -- the pdu containing the data (may NOT be NULL)
 * @param r_entry -- the routing entry itself
 */
void
Statemodule::deliver_adjacency_flag(const appladdress* peer, const appladdress* own_addr, const known_ntlp_pdu* pdu, const routingentry* r_entry) 
{
  APIMsg* msg = new APIMsg;

  nslpdata* mydata = pdu->get_nslpdata();

  
  if( pdu->get_mri() == NULL )
  {
	  // at this stage it is not possible that an MRI in the PDU is missing since it should have
	  // been checked twice, i.e., during parsing and in ntlp_statemodule_main
	  ERRCLog("statemodule","deliver_adjacency_flag(): PDU contains no MRI, which is required though. This should not happen and is an internal error.");
	  return;
  }
  
  nattraversal* nattravobj= pdu->get_nattraversal();  
  mri* pdumri= pdu->get_mri()->copy();
  // use MRI from NTO if present
  mri* ntomri= nattravobj ? nattravobj->get_embedded_mri()->copy() : 0;
	
  tx_attr_t myattr;
	
  if (r_entry) {
	  myattr.secure= r_entry->is_secure();
	  myattr.reliable=!(r_entry->is_dmode());
  } else {
    // Initial Query/Initial Confirm, D-Mode, unsecure
    myattr.secure= false;
    myattr.reliable= false;
  }

  // check for final hop, same as in Statemodule::deliver()
  // this must be done using the outer flow destination address
  if ( pdumri->get_downstream() )
    myattr.final_hop = is_local_address(pdumri->get_destaddress());
  else
    myattr.final_hop = is_local_address(pdumri->get_sourceaddress());

  DLog("statemodule", "GIST node is the final hop: " << myattr.final_hop);

  nslpdata* senddata = NULL;

  if (mydata) senddata = new nslpdata(*mydata);

  uint8 ip_distance = 0;
  // if we can do distance calculation, DO IT!
  if (peer->get_ip_ttl()) {
    if (pdu->get_nli()) {
      ip_distance = pdu->get_nli()->get_ip_ttl() - peer->get_ip_ttl();
    }
  }

  // TODO: for MA setup only
  msg->set_recvmessage(senddata, own_addr->copy(), 
		       pdu->get_nslpid(), 
		       pdu->get_sessionid()->copy(), 
		       ntomri ? ntomri : pdumri,  // use MRI from NAT traversal object if present
		       true, 0, myattr, 
		       pdu->get_nli()->get_ip_ttl(), 
		       ip_distance, 
		       pdu->get_hops(), 
		       r_entry ? r_entry->get_incoming_if() : 0 );
    
  msg->send_to(param.nslptable.get_address(pdu->get_nslpid()));

  DLog(param.name, "Message passed to NSLP instance at address "<< param.nslptable.get_address(pdu->get_nslpid()));
}


/**
 * Notify NSLP via NetworkNotification about Changes, store state in r_entry (esentially, give MRI, SII-Handle and state)
 * @param r_key -- the routing key (may NOT be NULL)
 * @param r_entry -- the routing entry itself (may NOT be NULL)
 * @param status -- the Status to report to NSLP
 * @param force -- force notification, even if the same status was rported before (used on rerouting events)
 */
void 
Statemodule::networknotification(const routingkey* r_key, routingentry* r_entry, APIMsg::error_t status, bool force) {
  if ((r_entry->get_errstate() != status) || force) 
  {
    // save the change
    r_entry->set_errstate(status);
	
    mri* mymri = r_key->mr->copy();
    APIMsg* msg = new APIMsg;
	
    sessionid* mysid = r_key->sid->copy();
    
    msg->set_source(message::qaddr_coordination);
	
    msg->set_networknotification(r_key->nslpid, mysid, mymri, status, r_entry->get_sii_handle());
	
    msg->send_to(param.nslptable.get_address(r_key->nslpid));
	
    EVLog(param.name, color[yellow] << "NetworkNotification passed to NSLP instance at address "<< param.nslptable.get_address(r_key->nslpid) << ": " << msg->get_error_str(status) << color[off]);
  } else {
    DLog(param.name, color[yellow] << "Routing State unchanged, no need to notify NSLP" << color[off]);
  }
}


/**
 * Notify NSLP via MessageStatus about Message Failures
 * @param r_key -- the routing key (may NOT be NULL)
 * @param r_entry -- the routing entry itself (may NOT be NULL)
 * @param nslpmsghandle -- the Msg Handle of the message referred to
 * @param status -- the status to report
 */
void 
Statemodule::messagestatus(const routingkey* r_key, routingentry* r_entry, msghandle_t nslpmsghandle, APIMsg::error_t status) {
  
  APIMsg* msg = new APIMsg;

  msg->set_source(message::qaddr_coordination);
    
  tx_attr_t txattr;
  txattr.secure=r_entry->is_secure();
  txattr.reliable=!r_entry->is_dmode();
  txattr.final_hop = false; // meaningless in this context

  sessionid *sid = NULL;
  if ( r_key->sid != NULL )
    sid = r_key->sid->copy(); // ApiMsg's destructor will delete it

  msg->set_messagestatus(r_key->nslpid, sid, nslpmsghandle, txattr, status);

  msg->send_to(param.nslptable.get_address(r_key->nslpid));
    
  EVLog(param.name, color[yellow] << "MessageStatus passed to NSLP instance at address "<< param.nslptable.get_address(r_key->nslpid) << ": " << msg->get_error_str(status) << color[yellow]);

}


/**
 * SendMessage API Call processing, set up state or send via existing state a GIST Data
 * @param apimsg -- the API message from caller containing all data we need
 */
void 
Statemodule::tg_send_message(APIMsg* apimsg) 
{
  assert(apimsg != NULL);

  if (!apimsg->get_mri()) 
  {
    ERRCLog(param.name, "SendMessage Call without given MRI");
    return;
  }

  bool explicit_routed = false;

  if (apimsg->get_sii_handle()) 
  {
    DLog("param.name", "Got an explicit routing request, using entry derived from SII-Handle");
    explicit_routed = true;
  }

  
  // if MRI contains IPv4 and IPv6 mix, we bail out and inform via LOG entry. This is the NSLP's fault and an error
  hostaddress src = apimsg->get_mri()->get_sourceaddress();
  hostaddress dst = apimsg->get_mri()->get_destaddress();

  if (src.is_ipv6() != dst.is_ipv6()) 
  {
    ERRCLog(param.name, "MRI of API call contains mixed IPv4/IPv6 addresses, dropping");
    delete apimsg;
    return;
  }
    
  // print some info
  DLog(param.name, color[yellow] << "IP TTL: " << apimsg->get_ip_ttl() << " GHC: " << apimsg->get_ghc() << color[off]);
    
  // 1st step: Look for Routing State
    
  routingentry* r_entry=NULL;
    
  // create a new routingkey, this will in the end given to timermessage, a copy is stored in routingtable
  routingkey* r_key=new routingkey();
    
  r_key->mr=(apimsg->get_mri())->copy();
  r_key->sid=new sessionid(*(apimsg->get_sessionid()));
  r_key->nslpid=apimsg->get_nslpid();
    
  //DLog(param.name, "Routing map key created from API Message for NSLPID " << r_key->nslpid);
    
  //r_key->mr->print(cout, 0, 0);
  //r_key->sid->print(cout, 0, 0);
    
    
  //=======================================================
  // look up routing state and print a status dump of routing table
  //=======================================================
  param.rt.status();
  
  // try to do explicit routing and return
  if (explicit_routed) 
  {
    DLog(param.name, "Explicit Routing requested");
	
    send_data_explicit(apimsg);
	
//    delete apimsg;
    return;
  } 
      
  DLog(param.name, "No explicit routing, try to look up the normal way");
  // look up routing state in routing map. If there is none, r_entry remains NULL
  r_entry=param.rt.lookup(r_key);

  // 2nd step: Consider which type of transmit is needed for the message
    
  // look for nslp data payload size, decide initially between C- and D-Mode
  if (!apimsg->get_data()) 
  {
    ERRCLog(param.name, "No Data Payload in SendMessage. We will not process this further");
    delete apimsg;
    if (r_entry)
      param.rt.unlock(r_key);
    return;
  }

  // look for GIST Hop Count
  // Sec. 4.3.3.: GIST MUST reject messages for which the signalling application 
  // has specified a value of zero. 
  if (apimsg->get_ghc() == 0)
  {
    ERRCLog(param.name, "Not allowed to send messages with GIST Hop Count = 0. We will not process this further.");
    delete apimsg;
    if (r_entry)
      param.rt.unlock(r_key);
    return;
  }

  if (apimsg->get_data()->get_size() > MAX_NSLP_PAYLOAD)
	  DLog(param.name, "Payload too large for D-Mode (" << apimsg->get_data()->get_size() << " bytes, Max: " << MAX_NSLP_PAYLOAD 
	       << ", switching to C-Mode");
  // check whether fragmentation is required
  transfer_t transtype= (apimsg->get_data()->get_size() > MAX_NSLP_PAYLOAD) ? tx_cmode : tx_dmode;

  // look if we were asked to transfer reliable -> C-Mode instead of DMode
  if (apimsg->get_tx_attr().reliable) transtype= tx_cmode;
    
  // look if we were asked to transfer secure -> Secure C-Mode
  if (apimsg->get_tx_attr().secure) transtype= tx_cmode_sec;
    
  // if we have no routing state we must change our attitude and consider transfer change based on that
  if (!r_entry) 
  {
    // if we can transfer the data in D-Mode, we can transfer queryencapsulated, else we have to queue
    // the data and set up C-Mode (sec/unsec) prior to be able to send it.
    if (transtype==tx_dmode) 
    { 
      // Here we can send QueryEncapsulated and set up our local state in "awaiting response", as we use only D-Mode
      DLog(param.name, "No Routing State - Sending Payload Query Encapsulated (in a QUERY), creating local routing state in 'qn_awaiting_response' mode");
	    
      // create new routingentry (as a querier)
      r_entry= new routingentry(false);
#ifdef NSIS_OMNETPP_SIM
  #ifdef USE_OPPBSD
//    std::cout << "destination of package: " << dst << " vSize: " << Interface_Manager::instance()->get_map()->size() << " nsisid: " << ModuleManager::getCurrentNsisId() <<std::endl;
//    for (interfaceAddressMap_t::iterator iter = Interface_Manager::instance()->get_map()->begin(); iter != Interface_Manager::instance()->get_map()->end(); iter++)
//      std::cout << "address in map: " << iter->second << " for target ip " << iter->first << std::endl;
    hostaddress laddr(Interface_Manager::instance()->get_map()->at(dst));
    r_entry->set_local_src(laddr);
    DLog(param.name, "set laddr for routing entry to " << laddr << " for nsis id " << ModuleManager::getCurrentNsisId());
  #else
    unsigned int nsis_id = ModuleManager::getCurrentNsisId();
    hostaddress *laddr = param.addresses.get_first(AddressList::ConfiguredAddr_P, src.is_ipv4());
    r_entry->set_local_src(*laddr);
    DLog(param.name, "set laddr for routing entry to " << *laddr << " for nsis id " << nsis_id);
  #endif
#else
      if (param.addresses.addr_is(src, AddressList::HomeAddr_P)) {
        r_entry->set_local_src(src);
      } else {
      	uint32_t pref = IPV6_PREFER_SRC_COA;
      	hostaddress *laddr = param.addresses.get_src_addr(dst, &pref);
	DLog(param.name, "Selecting Address (src): " << src << ", "
			<< "laddr: " << *laddr << ", "
			<< "laddr == HA: " << param.addresses.addr_is(*laddr, AddressList::HAAddr_P) << ", "
			<< "src == HA: " << param.addresses.addr_is(src, AddressList::HAAddr_P));
      	if (param.addresses.addr_is(*laddr, AddressList::HAAddr_P))
          laddr = param.addresses.get_first(AddressList::AltHAAddr_P, src.is_ipv4());
        //cout << *laddr << endl;
      	r_entry->set_local_src(*laddr);
      }
#endif

      apimsg->set_local_addr(r_entry->get_local_src()->copy());

      // set it to qn_awaiting_response
      r_entry->set_state(qn_awaiting_response);
      
      // save IP TTL for Query Repetitions (NSLP gives us the IP TTL value)
      r_entry->set_ip_ttl(apimsg->get_ip_ttl());
      
      // save GIST Hop count for Query Repetitions (NSLP gives us the hop count value)
      r_entry->set_gist_hop_count(apimsg->get_ghc());
      
      // set D-Mode unsecure (we set it here, so we remember it when processing response)
      r_entry->set_dmode(true);
      r_entry->set_secure(false);
      
      // set local default RS_Validity Time
      r_entry->set_rs_validity_time(gconf.getpar<uint32>(gistconf_rs_validity_time));
      
      // initiate sending the Query
      send_query(apimsg,r_key,r_entry);

      // start NoResponse timer
      starttimer(r_key, r_entry, noresponse, 0, r_entry->reset_retry_timeout());
   	    
      // add routing state
      param.rt.add(r_key, r_entry);
    } 
    else 
    { // Routing State Setup with Message Association
      DLog(param.name, "No Routing State - 'Reliable' or 'Secure' requested, queueing data, initiating C-Mode setup, creating local routing state in 'qn_awaiting_response' mode");
	    
      // create new routingentry (as querier)
      r_entry= new routingentry(false);
      if (param.addresses.addr_is(src, AddressList::HomeAddr_P)) {
        r_entry->set_local_src(src);
      } else {
      	uint32_t pref = IPV6_PREFER_SRC_COA;
      	hostaddress *laddr = param.addresses.get_src_addr(dst, &pref);
      	r_entry->set_local_src(*laddr);
      }

      apimsg->set_local_addr(r_entry->get_local_src()->copy());
    
      // set it to qn_awaiting_response
      r_entry->set_state(qn_awaiting_response);
	    
      // set C-Mode (we set it here, so we remember it when processing response)
      r_entry->set_cmode();
	    
      // save IP TTL for Query Repetitions (NSLP gives us the IP TTL value)
      r_entry->set_ip_ttl(apimsg->get_ip_ttl());

      // set security option in local state, so we choose a secure protocol if needed when processing response
      r_entry->set_secure((transtype==tx_cmode) ? false : true);

      // set local Default RS_Validity Time
      r_entry->set_rs_validity_time(gconf.getpar<uint32>(gistconf_rs_validity_time));
	    
      // send a Query requesting handshake, this will NOT transfer data payload!
      send_query_handshake(apimsg, r_key, r_entry);
	    	
      // start NoResponse timer
      starttimer(r_key, r_entry, noresponse, 0, r_entry->reset_retry_timeout());
	    
      // Here we enqueue the data
      enqueuedata(r_key, r_entry, apimsg->get_data(), apimsg->get_timeout(), apimsg->get_nslpmsghandle());
	    
      // add routing state
      param.rt.add(r_key, r_entry);
    }
  } // endif no routing state exists 
  else 
  { // routing state exists already
    // save GIST Hop count for Query Repetitions (NSLP gives us the hop count value)
    r_entry->set_gist_hop_count(apimsg->get_ghc());
	
    // save IP TTL for any Messages sent on behalf of this SendMessage
    r_entry->set_ip_ttl(apimsg->get_ip_ttl());

    apimsg->set_local_addr(r_entry->get_local_src()->copy());
	
    switch (r_entry->get_state())
    {
      // there is a handshake in progress, we must queue the data
      case rn_awaiting_confirm:
      case qn_awaiting_response: 
	{
	
	  ERRCLog(param.name, "Routing State available - Handshake in progress, queueing data");
	  enqueuedata(r_key, r_entry, apimsg->get_data(), apimsg->get_timeout(), apimsg->get_nslpmsghandle());
	  break;
	}

	// Routing State is established. However we must decide if it suits our transfer requirements   
      case qn_awaiting_refresh:
      case rn_awaiting_refresh:
      case rn_established:
      case qn_established: 
	{
	  // if it is set up in D-Mode and MA re-use not requested
	  if (r_entry->is_dmode() && (r_entry->is_ma_reuse_requested() == false)) 
	  {
	    // if transfer type is dmode -> OK, send in D-MODE
	    if (transtype==tx_dmode) 
	    {
	      DLog(param.name, "Routing State set up in D-Mode, this suits, we send data Message in D-Mode");
	      send_data_dmode(apimsg->get_data(), r_key, r_entry, apimsg->get_nslpid());
	    }
		    
	    // if transfer type is something else -> New Handshake required
	    if (transtype!=tx_dmode) 
	    {
	      DLog(param.name, "Routing State set up in D-Mode, however, we need to use C-Mode, so we enqueue the data and prepare a new handshake.");
			
	      if (r_entry->get_state() == qn_established) 
	      {
		DLog(param.name, color[red] << "Upgrading to C-Mode from Querier side" << color[off]);
			    
		if (apimsg->get_data()) 
		{
		  enqueuedata(r_key, r_entry, apimsg->get_data(), apimsg->get_timeout(), apimsg->get_nslpmsghandle());
		  DLog(param.name, color[red] << "Enqueued data payload" << color[off]);	
		}
			    
		r_entry->set_state(qn_awaiting_response);
		r_entry->set_dmode(false);  // was !(apimsg->get_tx_attr().reliable); unreliabe but secure currently not supported
		r_entry->set_secure(apimsg->get_tx_attr().secure);

		// start NoResponse timer
		starttimer(r_key, r_entry, noresponse, 0, r_entry->reset_retry_timeout());
			    
	      } // if state established
	    }
	  } // endif D-Mode
	  else 
	  { // C-Mode or MA re-use desired

	    // if transfer type is dmode -> Transfer in C-Mode instead
	    if (transtype==tx_dmode) 
	    {
	      DLog(param.name, "Routing State set up in C-Mode, we use it however it was not required.");
	      send_data_cmode(apimsg->get_data(), r_key, r_entry, apimsg->get_nslpmsghandle());
	    }
		    
	    // if transfer type is cmode -> OK, send in C-Mode
	    if (transtype==tx_cmode) 
	    {
	      DLog(param.name, "Routing State set up in C-Mode, we use it as it was required.");
	      send_data_cmode(apimsg->get_data(), r_key, r_entry, apimsg->get_nslpmsghandle());
	    }
		    
	    // if set up cmode with security and transfer type is cmode_sec -> OK, send in C-Mode
	    if (r_entry->is_secure() && (transtype==tx_cmode_sec)) 
	    {
	      DLog(param.name, "Routing State set up in C-Mode with security, we use it.");
	      send_data_cmode(apimsg->get_data(), r_key, r_entry, apimsg->get_nslpmsghandle());
	    }
		    
	    // if set up cmode without security and transfer type is cmode_sec -> Bad, queue and re-initiate
	    if (!r_entry->is_secure() && (transtype==tx_cmode_sec)) 
	    {
	      DLog(param.name, "Routing State currently in C-Mode without security, queueing data and initiating secure C-Mode setup.");
		    
	      if (apimsg->get_data()) 
	      {
		enqueuedata(r_key, r_entry, apimsg->get_data(), apimsg->get_timeout(), apimsg->get_nslpmsghandle());
		DLog(param.name, color[red] << "Enqueued data payload" << color[off]);	
	      }
			    
	      r_entry->set_state(qn_awaiting_response);

	      r_entry->set_dmode(false); // unreliable but secure currently not supported
	      r_entry->set_secure(apimsg->get_tx_attr().secure);

	      // start NoResponse timer
	      starttimer(r_key, r_entry, noresponse, 0, r_entry->reset_retry_timeout());
			
	    } // endif set up cmode without security and transfer type is cmode_sec
	  } // endif C-Mode or MA re-use desired
	  break;
	} // end case
		
      case dead: 
	{
	  DLog(param.name, "Encountered Request to send data for DEAD Routing State, this is possible via explicit routing, only D-Mode possible");
		
	  send_data_dmode(apimsg->get_data(), r_key, r_entry, apimsg->get_nslpid());
	  break;
	}
		
      default: 
	{ 
		ERRCLog(param.name, "Routing State available - Invalid State " << r_entry->get_state());
		break;
	}		
    } // end switch
  } // end else routing state exists already 
  param.rt.unlock(r_key);
}


/**
 * RecvMessageAnswer API Call: set up state backwards or relay the received message on behalf of NSLP
 * @param apimsg -- the API message from caller containing all data we need
 */
void 
Statemodule::tg_recv_message_answer(APIMsg* apimsg) 
{
  assert(apimsg != NULL);
    
  // Here we initiate backwards routing state (install state and send response)
  // or we relay the message (not yet implemented)
    
  // 1st step: Look for Routing State
    
  routingentry* r_entry=NULL;
    
  // create a new routinkey
  routingkey* r_key= new routingkey();
  r_key->mr=  (apimsg->get_mri())->copy();
  r_key->sid= new sessionid(*(apimsg->get_sessionid()));
  r_key->nslpid= apimsg->get_nslpid();
    
  //=======================================================
  // look up routing state and print a status dump of routing table
  //=======================================================
    
  // we communicated via uninverted MRI with API client!
  // but status is saved with inverted MRI!
  r_key->mr->invertDirection();
    
  r_entry=param.rt.lookup(r_key);
    
  if (!r_entry) 
  {
    ERRCLog(param.name, "Cannot find routing state for RecvMessageAnswer. Ignoring API call.");
    // apimsg will be deleted in calling method anyway
    return;
  }

  // do something in case of Routing State in rn_querynslp
  if (r_entry->get_state() == rn_querynslp) 
  {
    DLog(param.name, "State in RN_QUERYNSLP");
	
    // take the saved PDU, remove it from entry
    known_ntlp_pdu* reqpdu = r_entry->deliver_tmppdu();
	
    if (!reqpdu) 
      ERRCLog(param.name, "There should be a saved PDU but there isn't!");
	
    // take the saved Peer
    appladdress* peer = r_entry->deliver_tmppeer();
	
    if (!peer) 
      ERRCLog(param.name, "There should be a saved Peer address but there isn't!");
	
    // decide what to do
    if (apimsg->get_directive() == APIMsg::directive_establish) 
    {
      // save data payload in entry, for repetitions of responses
      if (apimsg->get_data()) 
	      r_entry->set_mod_data(apimsg->get_data());
	    
      //##############################################################
      // State setup
      //##############################################################
	    
      DLog(param.name, "Dispatching GIST Response");
      send_response(reqpdu, r_key, r_entry, peer);
	    
      // do we enforce delayed state installation?
      if ( gconf.getpar<bool>(gistconf_delayedstate) )
      {
	DLog(param.name, "delayed state installation configured, no state is saved");                 
	param.rt.destroy_entry(r_key);

	r_key= 0;
	//param.rt.delete_entry(r_key);
      } 
      else 
      {
	DLog(param.name, "Immediate state installation configured, set up state");
		
	r_entry->set_rs_validity_time(gconf.getpar<uint32>(gistconf_rs_validity_time));
	
	// put in state "rn_established"
	if ( gconf.getpar<bool>(gistconf_confirmrequired) == false ) 
	{
	  DLog(param.name, "No full handshake is configured, state set up");
		    
	  // install Routing State
	  if (!r_entry->is_dmode()) 
	  {
	    // install Routing State, note that add_ma will store the address as IPv6
		  param.rt.add_ma(r_entry->get_peer_nli()->copy(), *peer, r_entry->get_ma_hold_time(), true);
	  }
	  networknotification(r_key, r_entry, APIMsg::route_changed_status_good, false);
		    
	  r_entry->set_state(rn_established);
	  // Start Expire_RNode timer!
	  starttimer(r_key, r_entry, expire_rnode, 0, r_entry->get_rs_validity_time());
		    
	  // set retry timeout back to default
	  r_entry->reset_retry_timeout();
		   
	  // check and restart downstream Refresh_QNode timer if necessary
	  check_refresh_timer(r_key);
	} 
	else 
	{
	  DLog(param.name, "Full handshake is configured, waiting for GIST Confirm");
	  r_entry->set_state(rn_awaiting_confirm);
		    
	  starttimer(r_key, r_entry, noconfirm, 0, r_entry->reset_retry_timeout());
	}
      } //end if param.delayedstate (else)
    } 
    else 
    {
      //************************************************************
      // Relay on NSLP's order
      //************************************************************
	    
      EVLog(param.name, "NSLP told us to relay only");
	    
      nslpdata* mydata=apimsg->get_data();
      if (mydata) 
      {
	DLog(param.name, "NSLP provided data for relaying");
	reqpdu->set_data(mydata->copy());
      }
	    
      mri* mymri=reqpdu->get_mri();
      nli* mynli=reqpdu->get_nli();
	    
      // this cannot happen, if everything is all right
      assert(mymri != NULL);
      assert(mynli != NULL);
	    
      appladdress* peer = mymri->determine_dest_address();
      DLog(param.name, "Constructed target address for relaying");
	    
      if (SignalingNTLP::is_local_addr(peer)) 
      {
	ERRCLog(param.name, "NSLP told us to relay, but we are the end");
	ERRCLog(param.name, "Sending Error Message is redundant, as we would send it to ourself.");
	// apimsg will be deleted in calling method anyway
	
	if (r_entry)
          param.rt.unlock(r_key);
	delete apimsg;
	return;
      }

      peer->set_ip_ttl(mynli->get_ip_ttl());
      DLog(param.name, "Set IP TTL of target address to the one in the original NLI");
	    
      peer->set_rao(param.rt.get_rao(r_key->nslpid));
      DLog(param.name, "Set RAO value to the one of the NSLP (we know it for sure)");

      // send it to signaling
      SignalingMsgNTLP* sigmsg = new SignalingMsgNTLP;
      sigmsg->set_local_addr(r_entry->get_local_src()->copy());
      sigmsg->set_req(peer, reqpdu);
      sigmsg->send_or_delete();
      
      param.rt.destroy_entry(r_key);

      DLog(param.name, "Message sent to signaling for Relaying");
      if (r_key) delete r_key;
      r_key= 0;
    }
  } // end if state==rn_querynslp
    
  if (r_key) param.rt.unlock(r_key);
}

//@}

} // end namespace ntlp
