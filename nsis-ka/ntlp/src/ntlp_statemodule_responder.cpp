/// ----------------------------------------*- mode: C++; -*--
/// @file ntlp_statemodule_responder.cpp
/// GIST state module - implements responder specific GIST protocol logic
/// ----------------------------------------------------------
/// $Id: ntlp_statemodule_responder.cpp 5946 2011-03-01 20:55:01Z stud-mueller $
/// $HeadURL: https://svn.tm.kit.edu/nsis/ntlp/trunk/src/ntlp_statemodule_responder.cpp $
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
 * This module implements responder protocol logic as described in NSIS protocol draft.
 * 
 * Messages from network are processed and routing state is managed.
 *
 */
#include <sstream>
#include <iomanip>


#include "ntlp_statemodule.h"
#include "response.h"

#include "authorized_peer_db.h"

#include "rfc5014_hack.h"

#include "gist_conf.h"

#ifdef USE_OPPBSD
#include <interface_manager.h>
#endif

extern "C"
{
// hashing functions from OpenSSL
#include <openssl/evp.h>
}

namespace ntlp {

/** @defgroup responder Responder Actions
 *  @ingroup statemodule
 * @{
 */

using namespace protlib;

void
Statemodule::handle_initial_query(const known_ntlp_pdu* query_pdu, routingkey* r_key, const appladdress* peer, const appladdress* own_addr)
{
  // we must first check with NSLP and install local state "rn_querynslp"!
  // This cannot be done until NSLP is available to be run with this GIST instance
  // This is an internal temporary state that will either be deleted when
  // the response is sent (delayed state installation) or that will persist until 
  // a confirm is received

  DLog(param.name, "Initial GIST Query received");
		
  // create new entry for routing table (we are a responder now)
  routingentry* r_entry = new routingentry(true);
  const hostaddress *src = dynamic_cast<const hostaddress *>(own_addr);
#ifdef NSIS_OMNETPP_SIM
  #ifdef USE_OPPBSD
//    std::cout << "responder" << std::endl;
//    std::cout << "destination of package: " << *(dynamic_cast<const hostaddress *>(peer)) << " vSize: " << Interface_Manager::instance()->get_map()->size() << std::endl;
//    for (interfaceAddressMap_t::iterator iter = Interface_Manager::instance()->get_map()->begin(); iter != Interface_Manager::instance()->get_map()->end(); iter++)
//        std::cout << "address in map: " << iter->second << " for target ip " << iter->first << std::endl;
    hostaddress laddr(Interface_Manager::instance()->get_map()->at(*(dynamic_cast<const hostaddress *>(peer))));
    r_entry->set_local_src(laddr);
    DLog(param.name, "set laddr for routing entry to " << laddr << " for nsis id " << ModuleManager::getCurrentNsisId());
  #else
    unsigned int nsis_id = ModuleManager::getCurrentNsisId();
    src = dynamic_cast<const hostaddress *>(param.addresses.get_first(AddressList::ConfiguredAddr_P, false));
    r_entry->set_local_src(*src);
    DLog(param.name, "set src for routing entry to " << *src << " for nsis id " << nsis_id);
  #endif
#else
  if (src->is_ip_unspec()) {
    DLog(param.name, "Couldn't determine own address on receipt - using reverse path forwarding lookup");
    uint32_t pref = IPV6_PREFER_SRC_COA;
    src = param.addresses.get_src_addr(*(dynamic_cast<const hostaddress *>(peer)), &pref);
  }
  r_entry->set_local_src(*src);
#endif

  // save potentially present NAT Traversal payload in routing entry
  if (nattraversal* nattrav = query_pdu->get_nattraversal()) 
  {
    EVLog(param.name, color[green]<< "This message traversed a NAT, saving NAT information locally" << color[off]);

    // NTO contains the original (local to the Querier) MRI, NLI and SCD
    r_entry->set_nattraversal(nattrav);
  }
	
  // set our RS_Validity time
  r_entry->set_rs_validity_time(gconf.getpar<uint32>(gistconf_rs_validity_time));
	  
  // save Query Cookie for Response retries
  r_entry->set_querycookie(query_pdu->get_querycookie());

  nli* query_nli= query_pdu->get_nli();
  if (query_nli)
  { // use NLI from the PDU
    // save NLI
    r_entry->set_peer_nli(query_nli);
    // if this Query traversed a GIST-aware NAT we have a translated NLI to save
    if (r_entry->get_nattraversal())
	    r_entry->set_translated_nli(query_nli);
  }

  // check whether this is our own query by comparing the Peer ID
  if (r_entry->get_peer_nli())
  {
	  const peer_identity* pid= r_entry->get_peer_nli()->get_pi();
	  if ( pid && (*pid == *(param.nli_tmpl.get_pi())) )
	  { // peer id from query nli is the same as ours, this is usually an error
		  WLog(param.name,"Detected Query coming from myself, will be ignored and dropped to avoid loopback to myself.");
		  delete r_entry;
		  return;
	  }
		  
  }

  // SII handle in the entry
  r_entry->set_sii_handle(param.rt.generate_sii_handle(r_entry->get_peer_nli()));
  DLog(param.name, "Saved NLI, SII handle = " << r_entry->get_sii_handle());
  
  // Here we decide on C/D-Mode and security, this depends on the presence of a stack proposal
  // If MA setup is required, Stack Proposal _and_ Stack Configuration Data must be present
  const stackprop* remote_stackprop= query_pdu->get_stackprop();
  // if stack proposal is present => setup of an MA is requested
  if (remote_stackprop)
  { 
	  r_entry->set_cmode();
	  // secure MA requested?
	  // currently we check this by creating an own proposal consisting of only secure protocols
	  // and check whether the querier proposal could be accepted
	  r_entry->set_secure(cap.accept_proposal(remote_stackprop, true));
  }
  else
  {
	  r_entry->set_dmode();
	  r_entry->set_secure(false); // no security without stack proposal
  }

  DLog(param.name, "Requested properties of transfer: " << (r_entry->is_dmode() ? "D-Mode" : "C-Mode") << ", " << (r_entry->is_secure() ? "secure" : "non-secure"));

  if (query_pdu->get_stackconf())
  {
    if (query_pdu->get_stackconf()->get_ma_hold_time() == 0) 
    {
      ERRCLog(param.name, "Tried to install an MA with peer MA Hold time of ZERO");
      // send back an error indicating an invalid MA_Hold_Time
      senderror(query_pdu, peer, GIST_Error::error_gist_value_not_supported, known_ntlp_object::StackConfiguration, query_pdu->get_stackconf());
      // do not accept anything
      delete r_entry;
      return;
    }
    else
	    r_entry->set_ma_hold_time(query_pdu->get_stackconf()->get_ma_hold_time());

    DLog(param.name, "Saved MA Hold time: " << r_entry->get_ma_hold_time());
  }
  else
  {
    if (remote_stackprop)
    {
      ERRCLog(param.name, "MA setup requested, Stack Proposal present, but Stack Configuration Data missing");
      senderror(query_pdu, peer, GIST_Error::error_gist_missing_object, known_ntlp_object::StackConfiguration);

      // do not accept anything
      delete r_entry;
      return;
    }

    // it is ok that no stack conf is present if D-mode was requested
  }
  
  // fill temporary storage items
  r_entry->set_tmppdu(query_pdu);
  r_entry->set_tmppeer(peer);

  // remember incoming interface for query
  r_entry->set_incoming_if(peer->get_if_index());
  DLog(param.name, "Saved incoming interface index " << r_entry->get_incoming_if());
  
  // set up temporary state
  r_entry->set_state(rn_querynslp);
  
  param.rt.add(r_key, r_entry);
  
  DLog(param.name, "Prepared for State setup");

  EVLog(param.name, "Notifying NSLP of initial Query requesting to peer");
  deliver_adjacency_flag(peer, own_addr, query_pdu, r_entry);

}


/** handle incoming query in rn_established
 *  this is basically a route refresh (probably not complete if we request a handshake, i.e. a Confirm)
 */
void
Statemodule::handle_query_in_established(const known_ntlp_pdu* query_pdu, routingkey* r_key, routingentry* r_entry, const appladdress* peer)
{
  // stop Expire_RNode timer
  r_entry->set_timer_type_1(none);
		    
  // save Query Cookie
  r_entry->set_querycookie(query_pdu->get_querycookie());

  const stackprop* remote_stackprop= query_pdu->get_stackprop();
  // save Stackprop
  if (remote_stackprop)
  { 
    r_entry->set_sp(remote_stackprop);
    
    if (r_entry->is_dmode())
    {
      EVLog(param.name, "Querier requested to promote from D-Mode to C-Mode");
      r_entry->set_cmode();
    }
    // secure MA requested?
    // currently we check this by creating an own proposal consisting of only secure protocols
    // and check whether the querier proposal could be accepted
    if (!r_entry->is_secure())
    {
      r_entry->set_secure(cap.accept_proposal(remote_stackprop, true));
      if ( r_entry->is_secure() )
      { // now security requested
	EVLog(param.name, "Querier requested to promote to secure MA");	
      }
    } // endif not secure
  }
		    
  // save Stackconf
  if (query_pdu->get_stackconf())
	  r_entry->set_sc(query_pdu->get_stackconf());
  else
  {
    if (remote_stackprop)
    {
      ERRCLog(param.name, "MA setup requested, Stack Proposal present, but Stack Configuration Data missing. Sending back error.");
      senderror(query_pdu, peer, GIST_Error::error_gist_missing_object, known_ntlp_object::StackConfiguration);

      return;
    }

    // it is ok that no stack conf is present if D-mode was requested
  }
		    
  if (gconf.getpar<bool>(gistconf_confirmrequired)) 
  {
    DLog(param.name, "Full handshake configured, waiting for GIST Confirm");
    send_response(query_pdu, r_key, r_entry, peer, true);

    // start NoConfirm timer
    starttimer(r_key, r_entry, noconfirm, 0, r_entry->reset_retry_timeout());
	      
    r_entry->set_state(rn_awaiting_refresh);
  }
  else 
  {
    DLog(param.name, "No full handshake configured, " << color[green] << " routing state refreshed successfully" << color[off]);

    send_response(query_pdu, r_key, r_entry, peer);

    starttimer(r_key, r_entry, expire_rnode, 0, r_entry->get_rs_validity_time());
	      
    r_entry->set_state(rn_established);
  }
}


// same action when receiving query in "Awaiting Confirm" or "Awaiting Refresh"
void
Statemodule::handle_query_in_awaiting_confirm_or_refresh(const known_ntlp_pdu* query_pdu, routingkey* r_key, routingentry* r_entry, const appladdress* peer, const appladdress* own_addr)
{
  // draft says: send Response with R=1, (re)start No_Confirm timer
  // stop Expire_RNode timer

  r_entry->set_timer_type_1(none);
		    
  // save Query Cookie
  r_entry->set_querycookie(query_pdu->get_querycookie());
		    
  // save newer Stackprop
  if (query_pdu->get_stackprop()) 
	  r_entry->set_sp(query_pdu->get_stackprop());
		    
  // save newer Stackconf
  if (query_pdu->get_stackconf())  
	  r_entry->set_sc(query_pdu->get_stackconf());
		    
  DLog(param.name, "Query arrived when waiting for " <<  ((r_entry->get_state()==rn_awaiting_refresh) ? "refresh" : ((r_entry->get_state()==rn_awaiting_confirm) ?  "confirm" : (r_entry->get_state_name()))) << ", requesting GIST Confirm");
  send_response(query_pdu, r_key, r_entry, peer, true);

  // start NoConfirm timer
  starttimer(r_key, r_entry, noconfirm, 0, r_entry->reset_retry_timeout());

  if (r_entry->get_state() == rn_awaiting_refresh || r_entry->get_state() == rn_awaiting_confirm)
  {
    DLog(param.name, "staying in state " << r_entry->get_state_name());
  }
  else
    ERRCLog(param.name, "internal error - unexpected state " << r_entry->get_state_name());
}


/** Handle incoming Confirm when routing state is already available:
 *  is initial confirm during establishment when delayed state installation is not required
 */
void
Statemodule::handle_confirm_in_awaiting_confirm(const known_ntlp_pdu* cnfpdu, routingkey* r_key, routingentry* r_entry, const appladdress* peer, const appladdress* own_addr)
{
  bool accept = true;

  const stackprop* echoed_resp_proposal= cnfpdu->get_stackprop();
  const respcookie* echoed_resp_cookie= cnfpdu->get_respcookie();

  // we must use the original NLI from the query since the one in Confirm is untranslated
  // the NLI is contained in the Responder Cookie then
  nli* translated_nli= echoed_resp_cookie ? echoed_resp_cookie->get_nli() : NULL;
  // peer_nli is the NLI for responder cookie verification
  const nli* peer_nli= translated_nli ? translated_nli : cnfpdu->get_nli();

  bool respcookie_matched= false;
  // evaluate Responder Cookie in CONFIRM
  if (echoed_resp_cookie && (respcookie_matched= evaluate_resp_cookie(peer_nli, r_key, echoed_resp_cookie)) == false)
  {
    WLog(param.name, "Responder Cookie did not match, discarding GIST Confirm");
    // send back error
    // but only during initial deployment or debugging, because
    // sending the error in general makes a node a source of backscatter
    if (gconf.getpar<bool>(gistconf_verbose_error_responses)) 
      senderror(cnfpdu, peer, GIST_Error::error_gist_invalid_rcookie);

    delete translated_nli;

    accept= false;
  }
  else
  {
	  if (echoed_resp_cookie)
	  {
		  DLog(param.name, "Responder Cookie matched, processing GIST Confirm");
		  
		  if (echoed_resp_cookie->get_if_index() != r_entry->get_incoming_if())
		  {
			  ILog(param.name, "Responder Cookie indicated that Query used a different interface: old idx=" 
			       << r_entry->get_incoming_if() << " new idx=" << echoed_resp_cookie->get_if_index());
		  }
	  }
  

	  if (translated_nli && r_entry->get_translated_nli() && (*translated_nli != *(r_entry->get_translated_nli())))
	  {
		  WLog(param.name,"Translated NLI differs between initial query and routing state entry");
	  }
	  // translated_nli is not required anymore
	  delete translated_nli;
	  
	  // For C-Mode first we must check what is required by us, i.e.
	  // if we require security we have only secure protocols in the proposal
	  // we must check with the peers policy though
	  if (echoed_resp_proposal)
	  {
		  // verify stack proposal
		  DLog(param.name, "GIST Confirm included Stack Proposal, validating");
		  if ( cap.accept_proposal(echoed_resp_proposal, auth_peer_db.get_policy(*peer).secure_ma) == false )
		  {
			  ERRLog(param.name, color[red] << "Echoed stack proposal validation failed - MA not accepted" << color[off]);

			  // At the end of Sec 5.7.1 it is required that the "MA" must be torn down
			  // Now send a SigTrm to Signaling to tear down TPoverTCP (or whatever else)
			  SignalingMsgNTLP* sigmsg = new SignalingMsgNTLP;
			  sigmsg->set_local_addr(r_entry->get_local_src()->copy());
			  sigmsg->set_trm(peer->copy());	    
			  sigmsg->send_or_delete();
	
			  delete r_entry;
			  return;
		  }
		  else
		  {
			  // proposal accepted
			  // the following must be checked for C-mode now:
			  // is the CONFIRM coming via a transport connection which matches
			  // our transfer attributes? It should not accept a TCP connection
			  // if we requested only secure protocols
			  
			  // if we requested secure protocols the echoed proposal only contains
			  // secure protocols in its profile
			  bool secure_ma_requested= cap.accept_proposal(echoed_resp_proposal, true);
			  if (secure_ma_requested)
			  {
				  r_entry->set_secure(true);
			  }

			  // since either no secure MA was requested or at least a stack proposal was present, 
			  // we have reliable MA at least
			  r_entry->set_cmode();
		  } // end proposal accepted
	  } // endif echoed stack proposal present

	  // Here we verify C/D-Mode and security requirements
	  if ( r_entry->is_secure() && cap.is_secure(peer)==false )
	  {
		  // Confirm came over non-secure connection
		  ERRLog(param.name, color[red] << "Confirm came via non-secure connection although we required it - MA not acceptable, terminating" << color[off]);      
		  // At the end of Sec 5.7.1 it is required that the "MA" must be torn down
		  // Now send a SigTrm to Signaling to tear down TPoverTCP (or whatever else)
		  SignalingMsgNTLP* sigmsg = new SignalingMsgNTLP;
		  sigmsg->set_local_addr(r_entry->get_local_src()->copy());
		  sigmsg->set_trm(peer->copy());	    
		  sigmsg->send_or_delete();
		  
		  delete r_entry;
		  return;
	  }

	  // Freshen NLI
	  // check for valid source addresses
	  if (cnfpdu->get_nli())
	  {
		  if (cnfpdu->get_nli()->get_if_address().is_bogus_source())
		  {
			  ERRCLog(param.name,"Source address passed in the NLI is not usable (e.g. localhost, multicast, broadcast, etc.)");
			  senderror(cnfpdu, peer, GIST_Error::error_gist_value_not_supported, known_ntlp_object::NLI, cnfpdu->get_nli());
			  
			  delete r_entry;
			  return;
		  }
		  r_entry->set_peer_nli(cnfpdu->get_nli());
		  
		  // check for new SII handle and update it
		  uint32 old_sii_handle=  r_entry->get_sii_handle();
		  r_entry->set_sii_handle(param.rt.generate_sii_handle(cnfpdu->get_nli()));
		  if (r_entry->get_sii_handle() != old_sii_handle)
		  {
			  ERRLog(param.name, "Consistency check failure for SII handle: SII handle did change for Confirm old: " 
				 << old_sii_handle << " new: " << r_entry->get_sii_handle());
		  }
	  }
	  else
	  { // this should probably catched earlier during parsing?
		  ERRCLog(param.name, "NLI missing in Confirm!");
		  
		  // we are polite and send back an error to the misbehaving peer
		  // since we already ensured by the responder cookie that this is not a blind attacker
		  // chances are high that this is a broken implementation and not an adversary
		  if (gconf.getpar<bool>(gistconf_verbose_error_responses)) 
			  senderror(cnfpdu, peer, GIST_Error::error_gist_missing_object, known_ntlp_object::NLI);
	  }
    
	  // disposal of saved responder data
	  r_entry->set_mod_data(NULL);
    
	  // set up C-Mode if proposal came via an MA (C-Mode setup requires that confirm is sent over MA)
	  if (r_entry->is_cmode())
	  {
		  // we do accept if stackprop is contained and is acceptable
		  if (echoed_resp_proposal) 
		  {
			  if (cap.accept_proposal(echoed_resp_proposal)) 
			  {
				  param.rt.add_ma(r_entry->get_peer_nli()->copy(), *peer, r_entry->get_ma_hold_time(), true);
				  DLog(param.name, color[green] << "State setup in C-Mode");
				  if (r_entry->is_secure()) DLog(param.name, "Security is set for this connection");
			  } 
			  else 
			  {
				  ERRLog(param.name, color[red] << "Stack proposal validation failed - MA not accepted");
				  
				  accept = false;
				  
				  // we must close the MA
				  // At the end of Sec 5.7.1 it is required that the "MA" must be torn down
				  // Now send a SigTrm to Signaling to tear down TPoverTCP (or whatever else)
				  SignalingMsgNTLP* sigmsg = new SignalingMsgNTLP;
				  sigmsg->set_local_addr(r_entry->get_local_src()->copy());
				  sigmsg->set_trm(peer->copy());	    
				  sigmsg->send_or_delete();
			  }
		  }
		  else
		  {
			  // should be ok if we sent the response back via a suitable MA and the confirm comes back
			  DLog(param.name, color[green] << "State setup in C-Mode, MA re-use (no stack proposal in Confirm present)");
		  }
	  } // endif set up C-Mode
  } // end else responder cookie OK

  if (accept) 
  {
	  // stop NoConfirm timer, restart it as expire_rnode timer (implicit, they are running on the same slot)
	  // start Expire_RNode timer
	  starttimer(r_key, r_entry, expire_rnode, 0, r_entry->get_rs_validity_time());
	  
	  // put in state "rn_established"
	  DLog(param.name, color[green] << "State installed: rn_awaiting_confirm -> rn_established");
	  
	  r_entry->set_state(rn_established);
	  
	  // notify application about this event
	  networknotification(r_key, r_entry, APIMsg::route_changed_status_good, false);
	  
	  sendqueue(r_key, r_entry);
  } 
  else
  {
	  // if we do nothing a timeout will erase the routing state sometime
	  // the best thing now is to do invalidate the message routing state
	  ERRLog(param.name, color[red] << "State not installed.");
	  
	  param.rt.invalidate_routing_state(cnfpdu->get_mri()->copy(), cnfpdu->get_nslpid(), APIMsg::bad, false);
  }
}


/** handle an initial incoming Confirm in delayed state installation mode
 * @note this procedure creates the routingentry after performing initial validation checks
 */
void
Statemodule::handle_confirm_delayed_state_install(const known_ntlp_pdu* cnfpdu, routingkey* r_key,  const appladdress* peer, const appladdress* own_addr)
{
  DLog(param.name, "GIST Confirm while no Routing State - Assuming Delayed State Installation");

  const respcookie* responder_cookie= cnfpdu->get_respcookie();
  // check whether Responder Cookie is present (we should have sent one, but is it echoed?)
  if (responder_cookie == NULL)
  {
    ERRCLog(param.name,"Confirm in Delayed State installation: Responder Cookie missing in Confirm, although we sent one for sure.");
    // send back error for missing responder cookie
    if (gconf.getpar<bool>(gistconf_verbose_error_responses)) 
      senderror(cnfpdu, peer, GIST_Error::error_gist_missing_object, known_ntlp_object::RespCookie);

    return;
  }

  // MUST CHECK with different NLI and r_key if NTO is present...
  // we need to store the translated NLI in our routing entry, because
  // responses have to go back to this external NAT address
  nli* translated_nli= responder_cookie->get_nli();
  nli* peer_nli= translated_nli ? translated_nli : cnfpdu->get_nli();

  // evaluate Responder Cookie in CONFIRM
  bool rsp_cookie_matched = evaluate_resp_cookie(peer_nli, r_key, responder_cookie);

  // is Responder Cookie valid?
  if (rsp_cookie_matched == false)
  {
    WLog(param.name, "Responder Cookie did not match, discarding GIST Confirm");

    // send back error if enabled
    if (gconf.getpar<bool>(gistconf_verbose_error_responses))
      senderror(cnfpdu, peer, GIST_Error::error_gist_invalid_rcookie);

    // Now send a SigTrm to Signaling to tear down TPoverTCP (or whatever else pseudo-MA)
    SignalingMsgNTLP* sigmsg = new SignalingMsgNTLP;
    sigmsg->set_trm(peer->copy());	    
    sigmsg->send_or_delete();

    return;
  } // end if Responder Cookie did not match

  DLog(param.name, "Responder Cookie matched, processing GIST Confirm");
		
  // we got a valid confirm, we must set up routing state and begin in "established"
  // as we ignored the Query to the Confirm and did nothing
  
  routingentry* r_entry= new routingentry(true);
  const hostaddress *src = dynamic_cast<const hostaddress *>(own_addr);
  if (src->is_ip_unspec()) {
    DLog(param.name, "Couldn't determine own address on receive - using reverse lookup");
    uint32_t pref = IPV6_PREFER_SRC_COA;
    src = param.addresses.get_src_addr(*(dynamic_cast<const hostaddress *>(peer)), &pref);
  }
  r_entry->set_local_src(*src);

  if (translated_nli)
  {	  // please note that translated_nli will be freed by rentry destructor
	  r_entry->set_translated_nli(peer_nli);
  }

  bool ma_reuse= false;
  const nli* cnfnli= cnfpdu->get_nli();
  // save the NLI in the entry if reasonable (check for valid source addresses)
  if (cnfnli)
  {
    if (cnfnli->get_if_address().is_bogus_source())
    {
      ERRCLog(param.name,"Source address passed in the NLI is not usable (e.g. localhost, multicast, broadcast, etc.), will not install state");
      // send back error message object value error pointing to the NLI
      senderror(cnfpdu, peer, GIST_Error::error_gist_value_not_supported, known_ntlp_object::NLI, cnfpdu->get_nli());
      delete r_entry;

      return;
    }
    r_entry->set_peer_nli(cnfnli);

    // install SII handle
    r_entry->set_sii_handle(param.rt.generate_sii_handle(cnfnli));
    DLog(param.name, "SII handle = " << r_entry->get_sii_handle());

    // now check for MA re-use. 
    //
    // we detect this by checking for an existing and matching MA using the NLI
    // in the confirm. The problem with delayed state installation is that we don't
    // have any state that could tell us how the response was sent (since we
    // must have already decided about MA re-use when sending the response)
    //
    // Whether used MA matches the requirements is checked later
    const ma_entry* ma_p = param.rt.lookup_ma(cnfnli);
    if (ma_p && (ma_p->get_peer_address() == *peer))
    {
      DLog(param.name,"Detected MA re-use: Confirm came via already existing MA");
      ma_reuse= true;
      // in this case stack proposal and stack configuration data may be missing
      // because they were not present in the response

      // if this confirm came via an existing MA, it's a C-mode setup
      r_entry->set_cmode();

      // must set MA hold time to meaningful value
      r_entry->set_ma_hold_time(ma_p->get_ma_hold_time());
    }
  } 
  else
  { // this should probably catched earlier during parsing?
    ERRCLog(param.name, "NLI missing in Confirm!");

    // we are polite and send back an error to the misbehaving peer
    // since we already ensured by the responder cookie that this is not a blind attacker
    // chances are high that this is a broken implementation and not an adversary
    if (gconf.getpar<bool>(gistconf_verbose_error_responses)) 
      senderror(cnfpdu, peer, GIST_Error::error_gist_missing_object, known_ntlp_object::NLI);
  }

  // record incoming interface (is contained in Responder Cookie that we sent back on receiving the query)
  r_entry->set_incoming_if(responder_cookie->get_if_index());
  DLog(param.name, "saved incoming interface index " << r_entry->get_incoming_if());

  // save any NAT traversal object that comes with the responder_cookie
  // since cookie data was protected by our HMAC and found to be correct
  // we can be sure, that the NTO is unmodified and correct
  if (responder_cookie->get_transparent_data())
  {
	  r_entry->set_nattraversal(responder_cookie->get_nto(peer_nli->get_serialized_size(IE::protocol_v1)));
  }

  // set our RS_Validity time
  r_entry->set_rs_validity_time(gconf.getpar<uint32>(gistconf_rs_validity_time));

  // what we want is in the echoed response stack proposal
  const stackprop* echoed_resp_proposal = cnfpdu->get_stackprop();

  // For C-Mode first we must check what is required by us, i.e.
  // if we require security we have only secure protocols in the proposal
  // we must check with the peers policy though
  //
  // Note that the proposal may be missing due to MA re-use
  if (echoed_resp_proposal)
  {
    // verify stack proposal
    DLog(param.name, "GIST Confirm included Stack Proposal, validating");
    if ( cap.accept_proposal(echoed_resp_proposal, auth_peer_db.get_policy(*peer).secure_ma) == false )
    {
      ERRLog(param.name, color[red] << "Echoed stack proposal validation failed - MA not accepted" << color[off]);

      // At the end of Sec 5.7.1 it is required that the "MA" must be torn down
      // Now send a SigTrm to Signaling to tear down TPoverTCP (or whatever else)
      SignalingMsgNTLP* sigmsg = new SignalingMsgNTLP;
      sigmsg->set_local_addr(r_entry->get_local_src()->copy());
      sigmsg->set_trm(peer->copy());	    
      sigmsg->send_or_delete();
	
      delete r_entry;
      return;
    }
    else
    {
      // proposal accepted
      // the following must be checked for C-mode now:
      // is the CONFIRM coming via a transport connection which matches
      // our transfer attributes? It should not accept a TCP connection
      // if we requested only secure protocols
	
      // if we requested secure protocols the echoed proposal only contains
      // secure protocols in its profile
      bool secure_ma_requested= cap.accept_proposal(echoed_resp_proposal, true);
      if (secure_ma_requested)
      {
	      r_entry->set_secure();
      }

      // since either no secure MA was requested or at least a stack proposal was present, 
      // we have reliable MA at least
      r_entry->set_cmode();
    } // end proposal accepted
  } // endif echoed stack proposal present

  // Here we verify C/D-Mode and security requirements
  if ( r_entry->is_secure() && cap.is_secure(peer)==false )
  {
    // Confirm came over non-secure connection
    ERRLog(param.name, color[red] << "Confirm came via non-secure connection although we required it - MA not acceptable, terminating" << color[off]);
    // At the end of Sec 5.7.1 it is required that the "MA" must be torn down
    // Now send a SigTrm to Signaling to tear down TPoverTCP (or whatever else)
    SignalingMsgNTLP* sigmsg = new SignalingMsgNTLP;
    sigmsg->set_local_addr(r_entry->get_local_src()->copy());
    sigmsg->set_trm(peer->copy());	    
    sigmsg->send_or_delete();
      
    delete r_entry;
    return;
  }

  // we need also to consider the case were the other side 
  // upgraded the connection to be secure or reliable
  if ( r_entry->is_secure() == false && cap.is_secure(peer) )
	  r_entry->set_secure();

  // if C-Mode required, the Confirm must have been sent over the MA
  // if not, we can only ignore  
  if ( r_entry->is_cmode() )
  {
    if (!cap.is_reliable(peer))
    {
      ERRLog(param.name, color[red] << "Confirm came not over offered reliable MA protocols - ignoring this CONFIRM" << color[off]);
      // GIST spec defines that an incorrecpt encapsulation error should
      // we should send back an "Incorrect Encapsulation"
      senderror(cnfpdu, peer, GIST_Error::error_gist_incorrect_encapsulation);
	
      delete r_entry;
      return;
    }
  }


  //=============================================
  // Code for C-Mode/D-Mode setup, Protocol handshake
  //=============================================

  // StackConfiguation data (even if abbreviated) is only present if a new MA is set up
  // install C-Mode state, timers are started from Routing
  stack_conf_data* sc=cnfpdu->get_stackconf();
  if (sc != NULL || ma_reuse==true)
  {
    if (sc)
    {
      if (sc->get_ma_hold_time() == 0) 
      {
	ERRCLog(param.name, "Tried to install an MA with peer MA Hold time of ZERO");
	// send back an error indicating an invalid MA_Hold_Time
	senderror(cnfpdu, peer, GIST_Error::error_gist_value_not_supported, known_ntlp_object::StackConfiguration, sc);
	delete r_entry;
	return;
      }
      else
      {
	      r_entry->set_ma_hold_time(sc->get_ma_hold_time());
      }
    }
    // if ma_reuse is true then ma_hold_time was already set
    // (see above where me_reuse is set to true)

    // establish MA
    param.rt.add_ma(r_entry->get_peer_nli()->copy(), *peer, r_entry->get_ma_hold_time(), true);

    DLog(param.name, "State setup: C-Mode");
  } // stack conf present
  else 
  { // required if not in D-Mode and no MA re-use: peer must repeat ma_hold_time in Confirm 
	  if (r_entry->is_cmode() && ma_reuse == false)
    {
      ERRCLog(param.name, "Encountered GIST protocol error: GIST Confirm lacks a stack configuration data object! - State NOT installed");
      ERRCLog(param.name, "Could not setup MA: Delayed State Installation and got no ma_hold_time in stack configuration, sending back error");
      senderror(cnfpdu, peer, GIST_Error::error_gist_missing_object, known_ntlp_object::StackConfiguration);
      delete r_entry;
      return;
    }
    else
    { // D-Mode setup
      DLog(param.name, "State setup: D-Mode");
    }
  } // end else no stack-conf or no MA re-use

  // put in state "rn_established"
  DLog(param.name, "State " << color[green] << "established" << color[off]);
  r_entry->set_state(rn_established);
  // stop NoConfirm Timer (without state, there is no timer running...)
		    
  // start Expire_RNode timer
  starttimer(r_key, r_entry, expire_rnode, 0, r_entry->get_rs_validity_time());

  // if NSLP data is contained in the Confirm, deliver it
  if (cnfpdu->get_nslpdata())
  {
	  EVLog(param.name, "NSLP payload present, delivering it to NSLP");
	  deliver(peer, own_addr, cnfpdu, r_key);
  }

  networknotification(r_key, r_entry, APIMsg::route_changed_status_good, false);

  // enter the state into table
  param.rt.add(r_key, r_entry);
	    
  // check if refresh timer for downstream path is stopped
  check_refresh_timer(r_key);	    
} // end handle_confirm_delayed_state_install


/** Handles incoming Confirm after receiving a Query in Established state
 *  - confirm during refresh (when we set R-flag in our response to a refreshing query)
 */
void
Statemodule::handle_confirm_in_awaiting_refresh(const known_ntlp_pdu* cnfpdu, routingkey* r_key, routingentry* r_entry, const appladdress* peer, const appladdress* own_addr)
{
  // Message Association refresh includes the ability to freshen MA
	
  // several cases to consider
  // - Confirm arrives in D-Mode (only allowed when no MA exists for this routing state)
  // - Confirm arrives via MA: 
  //   - must check for message association upgrade (either unreliable -> reliable or non-secure -> secure)
  //   - must arrive over secure MA if secure mode requested
  //   - must arrive over reliable MA if C-mode request
  //   - connection updgrades D-Mode -> C-Mode or non-secure -> secure should be 
  //     handled by the incoming query already, i.e. this should update the
  //     r_entry accordingly

  // Confirm MUST arrive via MA if one is being used for the rounting state:
  // Sec. 5.1
  // "Confirm: A Confirm MUST be sent in C-mode if a messaging association
  //  is being used for this routing state, and MUST be sent before other
  //  messages for this routing state if an association is being set up.
  //  If no messaging association is being used, the Confirm MUST be sent
  //  in D-mode.  The Confirm MUST include the MRI (with inverted"

  // refreshing Query must also contain a stack proposal, because it may hit a 
  // new node: Sec. 5.1:
  //
  // "payload.  A stack proposal and configuration data MUST be included if
  //  the message exchange relates to setup of a messaging association, and
  //  this is the case even if the Query is intended only for refresh
  //  (since a routing change might have taken place in the meantime)."
  //
  // FIXME: do not change transfer attributes simply on looking
  // at the peer address

  // if Responder Cookie is present evaluate Responder Cookie from CONFIRM
  // if confirm came via MA responder cookie is optional, it is only required for D-mode responses
  if (cnfpdu->get_respcookie())
  {
    bool rsp_cookie_matched = evaluate_resp_cookie(cnfpdu->get_nli(), r_key, cnfpdu->get_respcookie());

    // is Responder Cookie valid?
    if (rsp_cookie_matched == false)
    {
      WLog(param.name, "Responder Cookie did not match, discarding GIST Confirm");

      // send back error if enabled
      if (gconf.getpar<bool>(gistconf_verbose_error_responses))
	senderror(cnfpdu, peer, GIST_Error::error_gist_invalid_rcookie);

      // Now send a SigTrm to Signaling to tear down TPoverTCP (or whatever else pseudo-MA)
      SignalingMsgNTLP* sigmsg = new SignalingMsgNTLP;
      sigmsg->set_trm(peer->copy());	    
      sigmsg->send_or_delete();
      
      return;
    } // end if Responder Cookie did not match
  }
	          		    
  // *****************+ Detect Routing Change +*************************
  const peer_identity* mypi = r_entry->get_peer_nli()->get_pi();
  const peer_identity* peerpi = cnfpdu->get_nli()->get_pi();
  assert(mypi!=NULL);
  assert(peerpi!=NULL);
	    
  bool rerouted = false;

  // new/different peer identity for this routing state
  if (!(*mypi == *peerpi)) 
  {
    rerouted = true;
    EVLog(param.name, color[red] << "Routing Change detected" << color[off]);
    DLog(param.name, color[red] << "Discarding old Routing State, establishing new one, notifying NSLP" << color[off]);
			
    // create new entry as a copy (copy connection settings, objects which are stored while issuing refresh query)
    routingentry* new_entry = new routingentry(*r_entry);
    const hostaddress *src = dynamic_cast<const hostaddress *>(own_addr);
    if (src->is_ip_unspec()) {
      DLog(param.name, "Couldn't determine own address on receipt - using reverse path forwarding lookup");
      uint32_t pref = IPV6_PREFER_SRC_COA;
      src = param.addresses.get_src_addr(*(dynamic_cast<const hostaddress *>(peer)), &pref);
    }
    r_entry->set_local_src(*src);

    // match it with all the new data delivered from peer
    new_entry->set_peer_nli(cnfpdu->get_nli());

    // check for new SII handle and update it
    uint32 old_sii_handle=  r_entry->get_sii_handle();
    new_entry->set_sii_handle(param.rt.generate_sii_handle(new_entry->get_peer_nli()));
    if (new_entry->get_sii_handle() == old_sii_handle)
    {
	    ERRLog(param.name, "Consistency check failure for SII handle: I have a new peer, but SII handle did not change, it is still: " << new_entry->get_sii_handle());
    }

    if (cnfpdu->get_stackconf()) 
    {
      if (cnfpdu->get_stackconf()->get_ma_hold_time() == 0) 
      {
	      ERRCLog(param.name, "Tried to install an MA with peer MA Hold time of ZERO");
	      // send back an error indicating an invalid MA_Hold_Time
	      senderror(cnfpdu, peer, GIST_Error::error_gist_value_not_supported, known_ntlp_object::StackConfiguration, cnfpdu->get_stackconf());
	      // use old MA hold time entry
	      new_entry->set_ma_hold_time(r_entry->get_ma_hold_time());
      }
      else
	      new_entry->set_ma_hold_time(cnfpdu->get_stackconf()->get_ma_hold_time());
    }
			
    // delete old entry
    param.rt.delete_entry(r_key);
	      
    // add new entry to Routing Data
    param.rt.add(r_key, new_entry);
			
    // look it up again, it should return the new one
    r_entry = param.rt.lookup(r_key);
	      
  }
  // *************************+ End Detect Routing Change +**********************+
		    
  // refresh for C-Mode
  if (r_entry->is_cmode()) 
  {
    // stack configuration data MUST be present for the very first Confirm, but is optional otherwise
    if (cnfpdu->get_stackconf()) 
    {
      if (cnfpdu->get_stackconf()->get_ma_hold_time() == 0) 
      {
	ERRCLog(param.name, "Tried to install an MA with peer MA Hold time of ZERO");
	// send back an error indicating an invalid MA_Hold_Time
	senderror(cnfpdu, peer, GIST_Error::error_gist_value_not_supported, known_ntlp_object::StackConfiguration, cnfpdu->get_stackconf());
      }
      else
	      r_entry->set_ma_hold_time(cnfpdu->get_stackconf()->get_ma_hold_time());

      DLog(param.name, "Saved peer MA Hold time of " << r_entry->get_ma_hold_time() << "ms");
    } // endif stack configuration data present

    // Here we verify C/D-Mode and security requirements
    if ( r_entry->is_secure() && cap.is_secure(peer)==false )
    {
      // Confirm came over non-secure connection
      ERRLog(param.name, color[red] << "Confirm came via non-secure connection although we required it - MA not acceptable, terminating" << color[off]);
      senderror(cnfpdu, peer, GIST_Error::error_gist_incorrect_encapsulation);

      // At the end of Sec 5.7.1 it is required that the "MA" must be torn down
      // Now send a SigTrm to Signaling to tear down TPoverTCP (or whatever else)
      SignalingMsgNTLP* sigmsg = new SignalingMsgNTLP;
      sigmsg->set_local_addr(r_entry->get_local_src()->copy());
      sigmsg->set_trm(peer->copy());	    
      sigmsg->send_or_delete();
      
      return;
    }

    if (!cap.is_reliable(peer))
    {
      ERRLog(param.name, color[red] << "Confirm came not over offered reliable MA protocols - ignoring this CONFIRM" << color[off]);
      // GIST spec defines that an incorrect encapsulation error should be sent now
      // we should send back an "Incorrect Encapsulation"
      senderror(cnfpdu, peer, GIST_Error::error_gist_incorrect_encapsulation);

      return;
    }

    param.rt.add_ma(r_entry->get_peer_nli()->copy(), *peer, r_entry->get_ma_hold_time(), true);
    EVLog(param.name, color[green] << "State refresh in C-Mode" << color[off]);
  }


  // notify about routing change
  networknotification(r_key, r_entry, APIMsg::route_changed_status_good, rerouted);
		    
  // stop NoConfirm timer, restart it as expire_rnode timer (implicit)
		    
  // (re)start Expire_RNode timer
  starttimer(r_key, r_entry, expire_rnode, 0, r_entry->get_rs_validity_time());
		    
  r_entry->set_state(rn_established);
}

/**
 * Send a GIST Response
 * @param pdu -- The incoming GIST Query eliciting the Response (if available, may be NULL)
 * @param r_key -- the routing key specifying the routing entry (may NOT be NULL)
 * @param r_entry -- the routing entry itself (may NOT be NULL)
 * @param peer -- this is the peer address from which we received the incoming pdu
 * @param forcerequestconfirm -- force setting of the R flag in the response, i.e., we definitely want a confirm
 */
void
Statemodule::send_response(const known_ntlp_pdu* pdu, const routingkey* r_key, routingentry* r_entry, const appladdress* peer, bool forcerequestconfirm) {

  // We must:
  // - generate responder cookie
  // - mirror query cookie
  // - mirror MRI, SESSIONID, Common Header (NSLPID!!)
  // - include NLI describing OURSELF
  // - store responder locally in routing data table for checking later
  // - cookie is only sent when setting up routing state
  // - set R-Flag if delayedstate or C-Mode is requested
  
  // - send via UDP using NLI in query!!!!
  // - send via a messaging Association, if the NLI of the query resolves to an existing MA!
  
  // Handshake logic (sending of stackprop/stack_conf_data) if there is a pdu given to us
  
  // take stackprop/stack_conf_data from earlier saving (in case of responses, if we have a pdu, these would be regenerated)

  assert(r_entry!=0);

  // take stack proposal and stack proposal from routing entry if available
  stackprop*       own_sp = r_entry->get_sp() ? r_entry->get_sp()->copy() : 0;
  stack_conf_data* own_sc = r_entry->get_sc() ? r_entry->get_sc()->copy() : 0;

  querycookie* qc = NULL;

  // confirm is required either if delayed state installation is used 
  // or at the initial handshake
  // we don't need a confirm only in case of a refresh

  bool requestconfirm= ( (r_entry->get_state() == rn_established) && !gconf.getpar<bool>(gistconf_confirmrequired) ) ? false: true;

  // if initial handshake and delayed state installation required, we need a confirm back
  if ( (r_entry->get_state() == rn_querynslp) && gconf.getpar<bool>(gistconf_delayedstate) )
    requestconfirm= true;

  // forcerequestconfirm will override any previous decision
  if (forcerequestconfirm)
    requestconfirm= true;

  // will be set to true if stack proposal was present in query
  bool ma_requested= false;

  if (pdu) 
  {
    if (!pdu->get_querycookie()) 
    {
      ERRLog(param.name, "No Query Cookie was contained in PDU, not continuing handshake");
      
      delete own_sp;
      delete own_sc;

      return;      
    }
    
    // store or update query cookie
    qc = new querycookie(*(pdu->get_querycookie()));
    if (r_entry) 
    {
      r_entry->set_querycookie(qc);
    }

    // look, if we were challenged by a stack proposal. Then compute a feasible own stack proposal!
    const stackprop* remote_proposal = pdu->get_stackprop() ? pdu->get_stackprop()->copy() : 0;
    if (remote_proposal) 
    {
      DLog(param.name, "We were challenged for C-Mode, seeing what we can do");
      
      requestconfirm = true;
      ma_requested= true;
      
      unsigned int profilecount=remote_proposal->get_count();

      if (profilecount==0) 
	ERRCLog(param.name, "An invalid Stack Proposal has reached us - should have been checked prior during PDU parsing!");
      
      // Build our own stackprop/stack_conf_data objects
      delete own_sp;
      delete own_sc;
      own_sp = cap.resp_stackprop(NULL, false);
      own_sc = cap.resp_stackconf(NULL, false);
      
      own_sc->set_ma_hold_time(gconf.getpar<uint32>(gistconf_ma_hold_time));
      
      if (capability::is_consistent(own_sp,own_sc))
      {
	      DLog(param.name, "Successfully constructed Stack Proposal/Stack Configuration data for C-Mode handshake");
      }
      else
      {
	      ERRCLog(param.name, "Constructed inconsistent Stack Proposal/Stack Configuration data for C-Mode handshake, not sending any query. Programming error, please fix the code.");
      }

      // save them for retries
      r_entry->set_sp(own_sp);
      r_entry->set_sc(own_sc);
      
      delete remote_proposal;
    }
    else
    { // no stack proposal present, we do not send our own stack proposal stuff
      delete own_sp;
      delete own_sc;
      own_sp= 0;
      own_sc= 0;
    }
  }
  else 
  { // only called without PDU if no_response timer went off in stateful mode
    DLog(param.name, "send_response() triggered without a PDU -> repeating last response (stateful mode)");
    if (!r_entry) {
      ERRCLog(param.name, "Cannot proceed. send_response() needs at least a routing entry as argument");
      return;
    }

    // take the query cookie we saved prior (this is a repetative response)
    if (r_entry->get_querycookie())
	    qc = r_entry->get_querycookie()->copy();
  }

  // query cookie (qc) should now be set, either to the new value read from the PDU
  // or from an existing r_entry

  SignalingMsgNTLP* sigmsg = new SignalingMsgNTLP();
  sigmsg->set_local_addr(r_entry->get_local_src()->copy());
 
  // Generate our target address
  // if we have a PDU, take the address from its NLI
  // if we have no PDU, only routing entry (r_entry), take the address from it
  // if we have neither, DO NOTHING (NLI is mandatory in Query PDU)
  
  appladdress* response_dest=NULL;
  // answer goes back to source port or well known port (wkp)
  port_t response_port= peer ? peer->get_port() : gconf.getpar<uint16>(gistconf_udpport);

  // interface index from query
  uint16 if_index= peer ? peer->get_if_index() : 0;

  // *********** Messaging Association Re-Use ****************************************

  // if we have a PDU, look for an existing MA via it's NLI
  // if we have no pdu, but are sending in repeat mode, 
  // use the saved nli (in delayedstate case, we would not repeat but have a PDU)
  // this is ususally the case only when we have stored state and no_confirm timer went off
  // note that in case of gist aware-NAT traversal an initial query carries the translated NLI (external addr)
  // and further refreshing queries carry untranslated addresses (local/internal addr)
  // for MA lookup we'll always use the untranslated NLI
  const nli* qnli= pdu ? pdu->get_nli() : r_entry->get_peer_nli();

  if (qnli)
  {
    DLog(param.name, "Looking up MA for this peer (" << qnli->get_if_address().get_ip_str() << ") - " << (pdu ? "answer Query mode" : "repeat Response mode"));
    const ma_entry* ma_p = param.rt.lookup_ma(qnli);
    // reuse only possible if tx_attributes are matching
    // must be false in case r_entry->secure==true and map_p->is_secure()==false
    if (ma_p)
    {
      if (ma_p->is_secure() || r_entry->is_secure()==false)
      {
	response_dest= ma_p->get_peer_address().copy();
	DLog(param.name, "Suitable MA exists, will re-use MA");	

	// not necessary to send stack proposal for MA re-use
	delete own_sp;
	delete own_sc;
	own_sp= 0;
	own_sc= 0;
      }
      else
      {
	DLog(param.name, "MA for this peer has different transfer attributes, not possible to re-use");
      }
    } // if MA found
  }

  // *************************************************************************
  bool ma_reuse= response_dest != NULL ? true : false;

  if (ma_reuse == false) 
  {
    DLog(param.name, "No existing Messaging Association exists, we send our Response via UDP");

    response_dest= new appladdress;
 
    // for NAT traversal scenario: must try translated NLI information first
    if (r_entry->get_translated_nli())
    { // must sent back to NAT public address
	    response_dest->set_ip(r_entry->get_translated_nli()->get_if_address());
    }
    else
    if (pdu)
    { // should use most current information from PDU
      if (pdu->get_nli()) 
	response_dest->set_ip(pdu->get_nli()->get_if_address()); 
    }
    else
    { // use what we already know from routing entry
      if (r_entry->get_peer_nli()) 
	response_dest->set_ip(r_entry->get_peer_nli()->get_if_address());
      else
      {
	ERRCLog(param.name, "No NLI available to derive Querier address, cannot send response");
      }
    }

    if (response_dest->is_ip_unspec())
    {
      ERRCLog(param.name, "Could not figure out Querier address for sending our response (dest addr=unspecified), cannot send response");
    }
    else
    {
      DLog(param.name, "Response via UDP is going to be sent to " << *response_dest);
    }
    
    response_dest->set_port(response_port);
    response_dest->set_protocol(param.udp);
  }

  mri* mr = r_key->mr->copy();
  sessionid* sid = new sessionid(*r_key->sid);
  
  if (!r_entry) 
  {
    ERRCLog(param.name, "Cannot proceed. send_response() needs at least a routing entry as argument");
    return;
  }

  netaddress na(*response_dest);
  nli* nl = build_local_nli(mr->get_ip_version(), na, r_entry);

  // test if we have a saved nat traversal object (NTO)
  // we only need to send it back for an initial response, not for refreshing responses
  const nattraversal* nto = (r_entry->get_state() == rn_querynslp) ? dynamic_cast<const nattraversal*>(r_entry->get_nattraversal()) : NULL;
  NetMsg* ntobuf_p= NULL;
  // put NLI and NTO as transparent data into hash if present
  if (nto)
  {
	  if (!qnli)
		  ERRLog(param.name,"Querier NLI not present when trying to insert it into NAT traversal object");

	  // put NLI first, because we need it for validation
      	  uint32 ntobuf_size= qnli->get_serialized_size(IE::protocol_v1) + nto->get_serialized_size(IE::protocol_v1);
	  ntobuf_p= new NetMsg(ntobuf_size);
	  // should be serialized including the common object header
	  uint32 written= 0;
	  // write NLI first
	  qnli->serialize(*ntobuf_p, IE::protocol_v1, written);
	  // write NTO as second object
	  nto->serialize(*ntobuf_p, IE::protocol_v1, written);
  }

  // generate a new responder cookie and include it
  // but only if MA re-use is not possible or we want delayed state installation
  respcookie* rc = ( (ma_requested && ma_reuse == false) || requestconfirm ) ? create_resp_cookie(qnli, r_key, 0, if_index, ntobuf_p) : NULL;
  
  // ntobuf not required anymore
  delete ntobuf_p;
  
  // confirm MUST be requested if responder cookie is present
  if (rc)
  {
    if (!requestconfirm)
    {
      WLog(param.name, "should not happen: no Confirm requested so far, but responder cookie is set so we must request a confirm");
    }
    requestconfirm= true;
  }
  
  nslpdata* mydata = NULL;
  if (r_entry->get_mod_data()) 
  {
    if (gconf.getpar<bool>(gistconf_delayedstate) || (!gconf.getpar<bool>(gistconf_confirmrequired))) 
    {
      DLog(param.name, "There will be no repetitions of this Response, cleaning replacement NSLP data");
      r_entry->set_mod_data(NULL);
    } 
    else 
    {
	    mydata=r_entry->get_mod_data()->copy();
	    DLog(param.name, "Replacing original NSLP payload as required by NSLP");
    }
  }

  response* rsp = new response(mr, sid, nl, qc, rc, own_sp, own_sc, mydata);

  assert (rsp != NULL);
  
  if (nto) {
	EVLog(param.name, color[green] << "Included NAT traversal object in GIST Response" << color[off]);
	rsp->add_nat(nto);
  }
  
  rsp->set_nslpid(r_key->nslpid);
  
  // set IP TTL in target address
  response_dest->set_ip_ttl(nl->get_ip_ttl());
  
  DLog(param.name, "Set IP TTL in target address");
   
  // set R-Flag if required
  if (requestconfirm) 
    rsp->set_R();
    
  // set S-Flag
  rsp->set_S();
  
  // set GHC to "1"
  rsp->set_hops(1);
  
  // initiate transmission
  sigmsg->set_req(response_dest, rsp);
  // as a responder we are not allowed to set up an MA (only to reuse one)
  sigmsg->set_useexistconn();
  sigmsg->send_or_delete();
}


/**
 * Generate a Responder cookie by using OpenSSL Hash function and hashing over objects in key
 * Our cookie is currently constructed as follows:
 * Secret generation number(32 bit) + 
 * query receiving interface index +
 * MAC Data Offset +
 * MAC Data= Hash(Q-Node Peer-ID, Q-Node i/f address, MRI, Session-ID, NSLPID, generation number, secret)
 *
 * @param querier_nli -- the nli (peer-id, ip address) of the querier
 * @param key -- The key containing the objects to hash over 
 * @param gennumber -- the generation number to use, if 0 use the current generation number
 * @param if_index -- interface index
 * @param transparent_data -- e.g., nattraversal object if used, this must be included in the responder cookie if present
 * @return responder cookie or NULL if secret not available (wrong generation number)
 */
respcookie*
Statemodule::create_resp_cookie(const nli* querier_nli, const routingkey* key, uint32 gennumber, uint16 if_index, const NetMsg* transparent_data)
{

  //============================================
  // take our secrets
  //============================================
  
  uchar* secret=NULL;
  uint32 secretsize = param.secrets.get_secret_size();

  uint32 generationnumber= gennumber;
  // we generate a cookie with a previous generationnumber
  if (generationnumber) { 
    secret = param.secrets.get_secret(generationnumber);
  } else {
    // create a current one
    secret = param.secrets.get_secret();
    generationnumber = param.secrets.get_generation_number();
  }  

  if (secret == NULL)
  {
    // this should not happen
    ERRLog(param.name,"secret for generation number 0x" << hex << gennumber << dec << " is not available");
    return NULL;
  }


  //===================================================================
  // Now put our input data into a byte buffer for calculating the hash
  //===================================================================
  
         mri* mr= key->mr;
  sessionid* sid= key->sid;
   uint16 nslpid= key->nslpid;
   uint8  pi_len= querier_nli->get_pi()->get_length();
   uint16 transparent_data_len= (transparent_data && transparent_data->get_buffer()) ? transparent_data->get_size() : 0; 

  // compute size needed for data in NetMsg (note: just used for computing the hash)

  // generation number (4 byte) in clear text (not hashed)
   uint32 buffersize = sizeof(generationnumber);

  buffersize+= sizeof(if_index);

  // NLI (querier peer identity)
  if(querier_nli->get_pi())
    buffersize+= pi_len;

  // NLI (interface address)
  buffersize+= querier_nli->get_if_address().is_ipv6() ? 32 : 4;
  
  // MRI without header
  buffersize+= mr->get_serialized_size(IE::protocol_v1)-4;
  
  // SessionID without header
  buffersize+= sid->get_serialized_size(IE::protocol_v1)-4;

  // NSLPID
  buffersize+=sizeof(nslpid);
  
  // generation number (4 byte)
  buffersize+=sizeof(generationnumber);
  
  // secret
  buffersize+=secretsize;

  // if any transparent data given
  buffersize+= transparent_data_len;
    
  // build NetMsg of sufficient size -> used for calculating the hash
  NetMsg* msg = new NetMsg(buffersize);
  uint32 written= 0;

  // include interface index into hash
  msg->encode16(if_index);
  written+= 2;

  // serialize peer ID
  msg->copy_from(querier_nli->get_pi()->get_buffer(), msg->get_pos(), pi_len);
  msg->set_pos_r(pi_len);
  written+= pi_len;

  // serialize IP address
  struct in_addr ip4addr;
  if ( querier_nli->get_if_address().is_ipv6() ) 
  {
      const struct in6_addr *ip6addr=querier_nli->get_if_address().get_ip();
      if (ip6addr)
	msg->encode(*ip6addr);
      else
      {
	ERRCLog("nli::serialize()", "No valid IPv6 address in if_address");
	msg->encode(in6addr_any);
      }
  } 
  else 
  {
    querier_nli->get_if_address().get_ip(ip4addr);
    msg->encode(ip4addr);
  }

  // NSLPID
  msg->copy_from(reinterpret_cast<uchar*>(&nslpid), msg->get_pos(), sizeof(nslpid));
  msg->set_pos_r(sizeof(nslpid));

  // serialize MRI into NetMsg buffer
  mr->serializeNoHead(*msg, IE::protocol_v1, written);

  // serialize SessionID into NetMsg buffer
  sid->serializeNoHead(*msg, IE::protocol_v1, written);

  // generation number
  msg->copy_from(reinterpret_cast<uchar*>(&generationnumber), msg->get_pos(), sizeof(generationnumber));
  msg->set_pos_r(sizeof(generationnumber));
  
  // serialize secret into NetMsg buffer
  msg->copy_from(secret, msg->get_pos(), secretsize);
  msg->set_pos_r(secretsize);

  // put transparent_data into hash if present
  if (transparent_data_len > 0)
  {
	  memcpy(msg->get_buffer()+msg->get_pos(), transparent_data->get_buffer(), transparent_data_len);
  }

    
  //============================================
  // From here on we compute the Hash
  //============================================
  
  // 1st: define datastructures necessary for OpenSSL hashing functions
  EVP_MD_CTX mdctx;
  const EVP_MD *md;
  unsigned char md_value[EVP_MAX_MD_SIZE];
  unsigned int md_len;
    
  // 2nd: We want SHA1 (maybe we make this a command line option)
  md = EVP_get_digestbyname(gconf.getparref<string>(gistconf_cookie_digest).c_str());
  
  if(!md) {
	  ERRCLog(param.name, "Hash algorithm " << gconf.getparref<string>(gistconf_cookie_digest) << " not available. Please update your OpenSSL library!");
    abort();
  }
  
  // 3rd: Initialize hasher
  EVP_MD_CTX_init(&mdctx);
  EVP_DigestInit_ex(&mdctx, md, NULL);
  // 4th: give input to hasher
  EVP_DigestUpdate(&mdctx, msg->get_buffer(), msg->get_size());
    
  // 5th: Finalize message digest
  EVP_DigestFinal_ex(&mdctx, md_value, &md_len);

  // 6th: Cleanup datastructures
  EVP_MD_CTX_cleanup(&mdctx);

  //==============================================
  // From here on we put the cookie value together
  //==============================================

  // 7.1: Put together result buffer
  uint16 totallen= sizeof(generationnumber) + 2*sizeof(uint16) + transparent_data_len + md_len;
  uchar* buf = new uchar[totallen];
  uchar* pbuf= buf;

  // 7.2: Put in Generation Number first
  *((uint32*)pbuf) = htonl(generationnumber);
  pbuf+= sizeof(generationnumber);

  // 7.3: interface index next (16 bit)
  *((uint16*)pbuf) = htons(if_index);
  pbuf+= sizeof(uint16);

  // 7.4: byte offset pointing to HMAC data behind optional NTO
  *((uint16*)pbuf) = htons( transparent_data_len );
  pbuf+= sizeof(uint16);

  // put transparent data into buffer if present, currently it could be the NAT traversal object
  if (transparent_data && transparent_data->get_buffer() && transparent_data_len > 0)
  {
	  memcpy(pbuf, transparent_data->get_buffer(), transparent_data_len);
	  pbuf+= transparent_data_len;
  }

  // 7.4th: Copy Resulting Hash/MD into buffer
  memcpy(pbuf, md_value, md_len);
  
  // 8th: Put into Responder Cookie
  respcookie* cookie = new respcookie(buf, totallen);

  DLog(param.name, "Computed Responder Cookie, length "<< totallen*8 << " Bit: " << cookie->to_str());

  // delete temporary buffer now
  delete buf;
  // delete netmsg buffer
  delete msg;

  return cookie;
}

/**
 * Evaluate a Responder Cookie, regenerate it by using the included Generation number and the Key objects in pdu
 * @param querier_nli -- the NLI of the querier which must match
 * @param r_key -- the routing key specifying the routing entry (may NOT be NULL)
 * @param responder_cookie -- the responder cookie to check
 * @return true if responder_cookie
 */
bool 
Statemodule::evaluate_resp_cookie(const nli* querier_nli, const routingkey* r_key, const respcookie* responder_cookie)
{
  // if any argument is missing (probably in incoming pdu, we cannot recalc the responder cookie, so it is invalid)
  if (querier_nli==NULL || r_key==NULL || responder_cookie==NULL)
    return false;

  // First decode generation number
  
  uint32 generationnumber= responder_cookie->get_generationnumber();

  // decode interface id (16bit)
  uint16 if_index= responder_cookie->get_if_index();

  uint16 td_len= responder_cookie->get_transparent_data_len();
  // check for any transparent objects carried in the responder cookie
  // and make sure that we don't read behind the end of the buffer
  NetMsg* transparent_data_msgbuf= ( (td_len > 0) && ( (uint32)(td_len+4+2+2) < responder_cookie->get_size()) ) ? 
	  new NetMsg(responder_cookie->get_transparent_data(), td_len, false) : NULL;

  // Now generate fitting cookie with decoded generation number of secret
  const respcookie* ourcookie= create_resp_cookie(querier_nli, r_key, generationnumber, if_index, transparent_data_msgbuf);
  delete transparent_data_msgbuf;
  
  if (ourcookie==NULL)
  { // this will happen if the generation number is wrong
    return false;
  }

  DLog(param.name, "Recomputed Responder Cookie, length "<< ourcookie->get_size()*8 << " Bit: " << ourcookie->to_str()); 

  DLog(param.name, "Given Responder Cookie, length "<< responder_cookie->get_size()*8 << " Bit: " << responder_cookie->to_str());

  // perform actual comparison
  if (*responder_cookie != *ourcookie) {
      DLog(param.name, color[red] << "Responder Cookie did not match!" << color[off]);

      delete ourcookie;
      return false;
  } else {
    DLog(param.name, color[green] << "Responder Cookie matched" << color[off]);
  }

  delete ourcookie;
  return true;
}


/**
 * ExpireRNode Timer processing. Routing State gets deleted
 * @param r_key -- the routing key (may NOT be NULL)
 * @param timermsg -- the timer message
 */
void 
Statemodule::to_expire_rnode(const routingkey* r_key, TimerMsg* timermsg) 
{

  DLog(param.name, "to_expire_rnode(): looking up routing key");
    
  routingentry* r_entry = param.rt.lookup(r_key);
    
  if (r_entry) 
  {

    if (r_entry->get_timer_type_1() != expire_rnode) {
      DLog(param.name, "Deliberately stopped timer, ignoring");
      param.rt.unlock(r_key);
      return;
    }

    if (r_entry->get_timer_id_1() == timermsg->get_id()) 
    {
      DLog(param.name, "Timer slot 1, timer is valid, id: " << timermsg->get_id());

      // expire_rnode timer is to be applied from all states
      EVLog(param.name, color[magenta] << "ExpireRNode timer went off" << color[off]);
      r_entry->set_state(dead);
	
      // notify about Routing State down
      // Notify NSLP about Routing State Change
      networknotification(r_key, r_entry, APIMsg::route_changed_status_bad, true);

      if (param.rt.delete_entry(r_key))
      {
	DLog(param.name,"Invalidated Routing Entry");
      }

      // Look for inverse direction, if in QN_ESTABLISHED, stop their Refresh_QNode timer
      r_key->mr->invertDirection();
	    
      r_entry = param.rt.lookup(r_key);
	    
      if (r_entry) {

	  if (r_entry->get_state() == qn_established) {
	  EVLog(param.name, "Stopping RefreshQNode for downstream path as my upstream just timed out.");

	  stoptimer(r_entry, refresh_qnode);

	}
      }
	    
      if (r_key) {
	param.rt.unlock(r_key);
	return;
      }
    } // if timer is valid
    else
    {
      DLog(param.name, "Timer not valid, ignoring");
    }
  } // if r_entry

  if (r_key) param.rt.unlock(r_key);
}



/**
 * NoConfirm Timer processing. Repeat Responses if retry count is OK
 * @param r_key -- the routing key (may NOT be NULL)
 * @param timermsg -- the timer message
 */
void 
Statemodule::to_no_confirm(const routingkey* r_key, TimerMsg* timermsg) 
{
  DLog(param.name, "to_no_confirm(): looking up routing key");
    
  routingentry* r_entry = param.rt.lookup(r_key);
    
  if (r_entry) 
  {
      if (r_entry->get_timer_type_1() != noconfirm) {
      DLog(param.name, "Deliberately stopped timer, ignoring");

      if (r_key) param.rt.unlock(r_key);

      return;
    }

      if (r_entry->get_timer_id_1() == timermsg->get_id()) {
      DLog(param.name, "Timer slot 1, timer is valid, id: " << timermsg->get_id());
	    
      EVLog(param.name, color[magenta] << "NoConfirm timer went off" << color[off]);
	    
      // In stateful mode, NoConfirm timer triggers retransmit of Responses 
      // until state changes to rn_established and timer is stopped
      if ( !r_entry->retry_limit_reached() )
      {
	send_response(NULL, r_key, r_entry);

	DLog(param.name, "Response retry dispatched (after " << r_entry->get_retry_timeout() << " ms)");
		
	// exponential backoff timeout for next retry
	r_entry->increase_retry_timeout();
       	// restart noresponse timer
	starttimer(r_key, r_entry, noconfirm, 0, r_entry->get_retry_timeout(), false);

      } 
      else 
      {
	// leave noConf timer stopped, kill state
	DLog(param.name, "Response retries stopped");
	r_entry->set_state(dead);

	// notify about Routing State down
	// Notify NSLP about Routing State Change
	networknotification(r_key, r_entry, APIMsg::route_changed_status_bad, true);
		
		
	// notify about inverse direction Routing State 'tentative'
	if (r_key) {

	  routingkey* tmpkey = new routingkey;
		    
	  tmpkey->nslpid=r_key->nslpid;
	  tmpkey->mr=r_key->mr->copy();
	  tmpkey->sid=r_key->sid->copy();
	  tmpkey->mr->invertDirection();
		    
	  if (routingentry* tmpentry = param.rt.lookup(tmpkey)) {
	    DLog(param.name, color[red] << "Refresh (or connect) failed, this direction is BAD and we signal 'tentative' for the existing inverse direction");
	    networknotification(tmpkey, tmpentry, APIMsg::route_changed_status_tentative, true);
	    param.rt.unlock(tmpkey);
	  }
	  delete tmpkey;			      
	}
	param.rt.delete_entry(r_key);
      } // end else (retry limit reached)
    } // if timer valid
    else
    {
      DLog(param.name, "Timer is invalid, id: " << timermsg->get_id());
    }

    // since we got a routing entry here, we need to unlock it
    if (r_key) param.rt.unlock(r_key);
  } // if r_entry
}


//@}

} // end namespace ntlp
