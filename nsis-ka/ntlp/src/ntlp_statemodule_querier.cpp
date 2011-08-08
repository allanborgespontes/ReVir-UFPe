/// ----------------------------------------*- mode: C++; -*--
/// @file ntlp_statemodule_querier.cpp
/// GIST state module - implements querier specific GIST protocol logic
/// ----------------------------------------------------------
/// $Id: ntlp_statemodule_querier.cpp 5778 2010-12-03 12:32:02Z roehricht $
/// $HeadURL: https://svn.tm.kit.edu/nsis/ntlp/trunk/src/ntlp_statemodule_querier.cpp $
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
 * This module implements querier specific protocol logic as described in 
 * NSIS protocol draft.
 * 
 * Messages from network are processed and routing state is managed.
 *
 */
#include "ntlp_statemodule.h"
#include "query.h"
#include "confirm.h"
#include <sstream>

#include "gist_conf.h"

namespace ntlp {

/** @defgroup querier Querier Actions
 *  @ingroup statemodule
 * @{
 */

using namespace protlib;


/**
 * Send a GIST Query
 * @param apimsg -- The apimsg initiating the communication (if available, may be NULL)
 * @param r_key -- the routing key specifying the routing entry (may NOT be NULL)
 * @param r_entry -- the routing entry itself (may NOT be NULL)
 * @param handshake -- specify if a handshake for C-Mode has to be sent
 */
void 
Statemodule::send_query(const APIMsg* apimsg, const routingkey* r_key, routingentry* r_entry, bool handshake) 
{
  // Get a pointer to MRI in apimsg. We intend to support generic MRMs, the determine_dest_address() function
  // of class mri will provide us with the IP address of the next hop. We override port and protocol.
  //
  // This function is able to transfer even without apimsg. if apimsg=NULL, needed data is taken from routingtable
  //
    
  mri* refmri=NULL;
    
  if (apimsg) 
  { 
    refmri=apimsg->get_mri();

    
    DLog(param.name,"send_query() - api requested transfer " 
	 << "with " << ( (refmri->get_mrm() == mri::mri_t_pathcoupled) ? "path coupled" : 
			 ( (refmri->get_mrm()==mri::mri_t_looseend) ? "loose end" : "explicit signaling target"))
	 << " MRI (" << (int) refmri->get_mrm() << ") being " 
                    << (apimsg->get_tx_attr().secure ? "secured" : "unsecured") << " and "
	            << (apimsg->get_tx_attr().reliable ? "reliable" : "unreliable" ));
  } 
  else 
  {
    if (r_key==0)
    {
      ERRCLog(param.name, "Encountered a call to send_query without valid Routingentry (NULL), aborting.");
      abort();
    }
    else
      refmri=r_key->mr;
  }

  if (!refmri) 
  {
    ERRCLog(param.name, "Encountered a call to send_query without either Routingentry or API call - would lead to NULL pointer exception, aborting.");
    abort();
  }


  // if we have both apimsg AND r_entry, we save NSLP Msg Handle for MessageStatus call if handshake/transmission fails
  if (apimsg) 
  {
    if (r_entry) 
    {
	    r_entry->set_msg_id(apimsg->get_nslpmsghandle());
	    r_entry->set_gist_hop_count(apimsg->get_ghc());
    }    
  }


  // fill appladdress for QueryEncapsulation Mode with target address
  appladdress* peer= refmri->determine_dest_address();

  uint16 outgoinginterface =0;
  if (refmri->get_mrm() == mri::mri_t_explicitsigtarget)
  { // for explicit signaling target MRI, we know the final signaling destination
    // nothing to do
    DLog(param.name, "Explicit Signaling Target MRM, signaling destination is " << *peer);
  }
  else
  {
    if ( gconf.getpar<bool>(gistconf_send_rao) )
    {
	    // set RAO value on peer address
	    peer->set_rao(param.rt.get_rao(r_key->nslpid));
	    DLog(param.name, "Set request for RAO value of " << param.rt.get_rao(r_key->nslpid) << " for GIST Query"); 
    }

/*
 * XXXMOB: This is mostly useless
 *
    outgoinginterface = refmri->get_local_if();
    if (!outgoinginterface) 
    {
      DLog(param.name, "Lookup of outgoing interface failed (this would certainly also affect the flow's routing). TP modules may use any interface.");
    } 
    else 
    {
      peer->set_if_index(outgoinginterface);
      DLog(param.name, "Set request for outgoing interface index " << outgoinginterface << " for GIST Query");
    }
 */
  }


  // set IP TTL on peer address
  if (r_entry->get_ip_ttl())
  {
	  peer->set_ip_ttl(r_entry->get_ip_ttl());
	
	  DLog(param.name, "Set request for IP TTL value of " << r_entry->get_ip_ttl() << " for GIST Query");
  }

  DLog(param.name, "Copying the MRI (type: " << (int) refmri->get_mrm() << ") for including in Query");

  // make a local copy of the mri
  mri* mymri=refmri->copy();
  DLog(param.name, "Copied MRI (type: " << (int) mymri->get_mrm() << ") for including in Query");

  DLog(param.name, "Creating a SignalingMsgNTLP");

  SignalingMsgNTLP* sigmsg = new SignalingMsgNTLP();
  sigmsg->set_local_addr(r_entry->get_local_src()->copy());
    
  DLog(param.name, "Setting Port and Protocol");

  // set well-known-port for GIST and dummy prot_query_encap protocol to force the message to tp_queryencap
  peer->set_port( gconf.getpar<uint16>(gistconf_udpport) );
  peer->set_protocol(refmri->get_mrm() == mri::mri_t_explicitsigtarget ? param.udp : param.qenc);

  // Build a Query Message, NLI describing how to reach us, query cookie, set R-Flag

  DLog(param.name, "Copying SessionID");

  sessionid* mysid= apimsg ? new sessionid(*(apimsg->get_sessionid())) :  new sessionid(*(r_key->sid));

  // take eventual payload from apimsg
  nslpdata* data=NULL;

  // payload is only transferred if no C-Mode setup is requested
  if (!handshake) 
  {
    // only if we have an apimessage
    if (apimsg) 
    {
      if (apimsg->get_data())
      {
	if (gconf.getpar<bool>(gistconf_senddatainquery))
	{
	  data= new nslpdata(*(apimsg->get_data()));
	
	  // store it for repetitions (D-Mode)
	  if (r_entry) 
	  {
	      r_entry->set_mod_data(data);
	  }
	}
	else
	{
	  // store this data for later sending in a DATA pdu after a handshake
	  enqueuedata(r_key, r_entry, apimsg->get_data(), apimsg->get_timeout(), apimsg->get_nslpmsghandle());
	}
      }
    }
    else // or if we have some data saved for us
    {
      if (r_entry) 
      {
	  if (r_entry->get_mod_data()) 
		  data= r_entry->get_mod_data()->copy();
      }
    }
  } // endif !handshake

  querycookie* myqc= query_cookie(r_key);

  r_entry->set_querycookie(myqc);

  // let dedicated function build our NLI
  nli* mynli = build_local_nli(mymri->get_ip_version(),
      refmri->get_downstream()?refmri->get_destaddress():refmri->get_sourceaddress(), r_entry);

  DLog(param.name, "Built Local NLI");
    
  DLog(param.name, "Setting IP TTL of NLI to value given to us in initiating API call");

  if (!r_entry) 
    ERRCLog(param.name, "r_entry = NULL!!!");
  else
  {
    // set IP TTL as it was saved for us from APIMsg processing
    if (r_entry->get_ip_ttl())
    {
	    mynli->set_ip_ttl(r_entry->get_ip_ttl());
	    DLog(param.name, "Success!");
    } 
  }

  // if handshake is used build a stack proposal, stack configuration
  stackprop* sp= NULL;
  stack_conf_data* sc= NULL;
  if (handshake) 
  {
    bool secure= apimsg ? apimsg->get_tx_attr().secure :  r_entry->is_secure();

    sp= cap.query_stackprop(secure);
    sc= cap.query_stackconf(secure);
    sc->set_ma_hold_time(gconf.getpar<uint32>(gistconf_ma_hold_time));

    if (capability::is_consistent(sp,sc))
    {
	DLog(param.name, "Successfully constructed Stack Proposal/Stack Configuration data for C-Mode handshake");
    }
    else
    {
      ERRCLog(param.name, "Constructed inconsistent Stack Proposal/Stack Configuration data for C-Mode handshake, not sending any query. Programming error, please fix the code.");
      delete mysid;
      delete mymri;
      delete data;
      delete mynli;
      delete sp;
      delete sc;
      return;
    }
  }

  // construct query
  query* pdu=new query(mymri, mysid, mynli, myqc, sp, sc, data);

  DLog(param.name, "Constructed Query");

  // set IP TTL in address
  peer->set_ip_ttl(mynli->get_ip_ttl());

  DLog(param.name, "Set IP TTL in target address");

  // set NSLPID
  if (apimsg) 
  {
    pdu->set_nslpid(apimsg->get_nslpid());
  } 
  else 
  {
    pdu->set_nslpid(r_key->nslpid);
  }

  DLog(param.name, "NSLPID set.");

  // set R-Flag
  pdu->set_R();
  DLog(param.name, "R-Flag set.");

  // set S-Flag
  pdu->set_S();
  DLog(param.name, "S-Flag set.");

  // set IP-TTL 255 if 'upstream'
  if (!mymri->get_downstream()) 
    peer->set_ip_ttl(255);

  // set GIST Hop Count
  pdu->set_hops(r_entry->get_gist_hop_count());

  DLog(param.name, "Set GIST Hop Count to " << (int) pdu->get_hops());

  // Send it away, give outgoinginterface, so SignalingModule will put it into TPMsg
  sigmsg->set_req(peer, pdu, 0, outgoinginterface);
  sigmsg->send_or_delete();

  if (handshake) 
  {
    DLog(param.name, color[blue] << "Query sent to Request C-Mode setup" << color[off]);
  } 
  else 
  {
    DLog(param.name, color[blue] << "Query sent in D-Mode" << color[off]);
  }
     
  if (data) 
    DLog(param.name, color[blue] << "Data payload included" << color[off]);
}


/**
 * Send a GIST Query with C-Mode handshake
 * @param apimsg -- The apimsg initiating the communication (if available, may be NULL)
 * @param r_key -- the routing key specifying the routing entry (may NOT be NULL)
 * @param r_entry -- the routing entry itself (may NOT be NULL)
 * 
 * @note Obsolete interface, directly calls send_query()
 */
void Statemodule::send_query_handshake(const APIMsg* apimsg, const routingkey* r_key, routingentry* r_entry) 
{

  send_query(apimsg, r_key, r_entry, true);

}

void
Statemodule::handle_response_in_awaiting_response(known_ntlp_pdu* incoming_pdu, routingkey* r_key, routingentry* r_entry, const appladdress* peer)
{
  // Here we install finally routing state for our initiated Query
  // look if we ought to send back a confirm!
  // look if we want C-Mode and if so, initiate it by sending over new (!) C-Mode assoc

  // since draft-12 the NLI is no longer optional and must be always present in all Responses
  // usually this one here will not happen, because the parsing method will throw an error 
  // earlier
  if (!incoming_pdu->get_nli()) 
  {
      ERRCLog(param.name, "No NLI in Response present. Sending back error message.");
      senderror(incoming_pdu, peer, GIST_Error::error_gist_missing_object, known_ntlp_object::NLI);

      return;
  } // endif no NLI available

  // print some info
  DLog(param.name, "GIST Response received, inspecting (QN Awaiting Response)");

  // if response came via C-Mode we check for correct MA re-use
  // this is probably redundant with checkMA
  if (peer->get_protocol() != param.udp)
  {
    if ( r_entry->is_secure() && cap.is_secure(peer)==false )
    {
      // Response came via non-secure connection
      ERRLog(param.name, color[red] << "Response came via unsecure connection although we required secure MA" << color[off]);
      senderror(incoming_pdu, peer, GIST_Error::error_gist_incorrect_encapsulation);
      return;
    }
  }

  if (!r_entry->get_querycookie()) 
  {
    ERRCLog(param.name, "No Local stored Query Cookie, dropping");
    return;
  }

  if (!incoming_pdu->get_querycookie()) {
    ERRCLog(param.name, "No Query Cookie in Response, dropping");
    return;
  }
  
  // Check whether  Query Cookie matches, if so we accept the response
  if (*(incoming_pdu->get_querycookie())==*(r_entry->get_querycookie())) 
  {
    DLog(param.name, "Query Cookie matched, accepting");

    // delete the Query Cookie, it has fulfilled its purpose
    r_entry->set_querycookie(NULL);

    // delete old NLI use new one (new one is present due to checked precondition above)
    r_entry->set_peer_nli(incoming_pdu->get_nli());

    r_entry->set_state(qn_established);
    r_entry->set_rs_validity_time(gconf.getpar<uint32>(gistconf_rs_validity_time));

    // create/reuse and store SII handle for this peer
    r_entry->set_sii_handle(param.rt.generate_sii_handle(r_entry->get_peer_nli()));

    DLog(param.name, "Installed NLI in routing state, SII handle = " << r_entry->get_sii_handle());

    // set the timers
		
    // start Inactive_QNode timer on timer slot #1
    DLog(param.name, "Installed peer's RS-Validity time of " << r_entry->get_peer_nli()->get_rs_validity_time() << "ms");

    // only start Inactive QNode if not running yet
    if (r_entry->get_timer_type_1() != inactive_qnode)
	    starttimer(r_key, r_entry, inactive_qnode, 0, r_entry->get_rs_validity_time());
		
    // start Refresh_QNode timer on timer slot #2
    starttimer(r_key, r_entry, refresh_qnode, 0, randomized(r_entry->get_peer_nli()->get_rs_validity_time(), gconf.getpar<float>(gistconf_retryfactor)));

    // check whether other side did MA re-use and promoted our D-mode setup to C-mode
    if (peer->get_protocol() != param.udp)
    {
      // if we requested D-Mode only, other side may decide to re-use existing MA
      // we must update our routing entry accordingly in order to send back the right confirm
      if (r_entry->is_dmode())
      {
	DLog(param.name, "We wanted D-mode setup, but other side wants MA re-use.");

	r_entry->set_ma_reuse_requested(incoming_pdu->get_nli());
      }	    
    }

    if (r_entry->is_dmode())
      DLog(param.name, "Connection Mode was not locally requested, setting up in D-Mode");

    // send confirms/queued data

    // Now look if we were requested to send a confirm
    bool confirm_success= true;

    // send always back a Confirm if Request confirmation flag is set in response
    if (incoming_pdu->get_R()) 
    {
      if ( r_entry->is_dmode() && (r_entry->is_ma_reuse_requested(incoming_pdu->get_nli()) == false) )
      {
	DLog(param.name, "Sending back confirm in D-Mode");
	send_confirm_dmode(incoming_pdu, r_key, r_entry, incoming_pdu->get_nli());
      } 
      else 
      {
	if (r_entry->is_secure())
	{
	  DLog(param.name, "Sending back confirm in TLS secured C-Mode");
	  confirm_success= send_confirm_cmode(incoming_pdu, r_key, r_entry, incoming_pdu->get_nli());
	} 
	else 
	{
	  DLog(param.name, "Sending back confirm in C-Mode");
	  confirm_success= send_confirm_cmode(incoming_pdu, r_key, r_entry, incoming_pdu->get_nli());
	}
      }
    } // endif R-Flag set

    if (confirm_success)	    
    {
      DLog(param.name, "Confirm dispatched. Triggering to send enqueued data.");

      sendqueue(r_key, r_entry);
      // So, confirm is dispatched, but we need to set up timers on our routing entry!

      // Notify NSLP about Routing State Change
      networknotification(r_key, r_entry, APIMsg::route_changed_status_good, false);
    }
    else
    { // confirm could not be sent, e.g. if stack proposal doesn't match, responder cookie not valid etc.
      ERRLog(param.name, "No Confirm sent. Invalidating routing state." );
      // probably tear down MRS and notify application, otherwise ...
      param.rt.invalidate_routing_state(r_key->mr, incoming_pdu->get_nslpid(), APIMsg::bad, false);
      // Notify NSLP about Routing State Change
      networknotification(r_key, r_entry, APIMsg::route_changed_status_bad, false);

      /*
2007-05-11 11:17:19.410-18284- EVENT /4: GIST Processing Enqueued data for later sending. Count of enqueued data for this target: 1846
2007-05-11 11:17:19.410-18284- DEBUG /4: GIST Processing Started timer: NO_RESPONSE 500 msec mid 3707
2007-05-11 11:17:19.410-18284- DEBUG /4: GIST Routing    MA not found / not usable
2007-05-11 11:17:19.410-18284- EVENT /4: GIST Processing No Messaging Association was found to send -> trying to re-establish
2007-05-11 11:17:19.410-18284**ERROR**4: GIST Processing I am Querying Node, I will re-establish Routing State, enqueuing data, will be dropped after 120sec
      */
    }
		    
  }
  else 
  {
    EVLog(param.name, "Got a Response with non-matching Query Cookie, dropping.");

    // throw error
    senderror(incoming_pdu, peer, GIST_Error::error_gist_invalid_qcookie);
  }
}


void
Statemodule::handle_response_in_awaiting_refresh(const known_ntlp_pdu* incoming_pdu, routingkey* r_key, routingentry* r_entry)
{
  DLog(param.name, "GIST Response received, processing (QN Awaiting Refresh)");

  bool rerouted = false;
		
  // freshen NLI
		
  //*****************************************************************************
  //
  // Detect Routing Changes Compare Peer Identity of NLI local and Remote
  //
  //*****************************************************************************
		
  const peer_identity* mypi = r_entry->get_peer_nli()->get_pi();

  const peer_identity* peerpi = incoming_pdu->get_nli()->get_pi();
		
  if (!(*mypi == *peerpi)) 
  { // new peer
    DLog(param.name, color[red] << "Routing Change detected" << color[off]);
    DLog(param.name, color[red] << "Discarding old Routing State, establishing new one, notifying NSLP" << color[off]);
		  
    // create new entry as a copy (copy connection settings, objects which are stored while issuing refresh query)
    routingentry* new_entry = new routingentry(*r_entry);

    // fill it with all the new data delivered from peer
    new_entry->set_peer_nli(incoming_pdu->get_nli());
    
    // check for new SII handle and update it
    uint32 old_sii_handle=  new_entry->get_sii_handle();
    new_entry->set_sii_handle(param.rt.generate_sii_handle(new_entry->get_peer_nli()));
    if (new_entry->get_sii_handle() == old_sii_handle)
    {
	    ERRLog(param.name, "Consistency check failure for SII handle: I have a new peer, but SII handle did not change, it is still: " << new_entry->get_sii_handle());
    }
    DLog(param.name, color[red] << "New SII handle = " << new_entry->get_sii_handle() << color[off]);

    if (incoming_pdu->get_stackconf()) 
	    new_entry->set_ma_hold_time(incoming_pdu->get_stackconf()->get_ma_hold_time());
		  
    // look for inverse direction
    routingkey* tmpkey = new routingkey;
    tmpkey->nslpid=r_key->nslpid;
    tmpkey->mr=r_key->mr->copy();
    tmpkey->sid=r_key->sid->copy();
    tmpkey->mr->invertDirection();

    if (routingentry* tmpentry = param.rt.lookup(tmpkey)) 
    {
      DLog(param.name, color[red] << "Re-routing occured, this direction is GOOD, but we signal 'tentative' for inverse direction");
      networknotification(tmpkey, tmpentry, APIMsg::route_changed_status_tentative, true);
      param.rt.unlock(tmpkey);
    }

    delete tmpkey;

    // delete old entry
    param.rt.delete_entry(r_key);
		  
    // add new entry to Routing Data
    param.rt.add(r_key, new_entry);
		  
    // look it up again, it should return the new one
    r_entry = param.rt.lookup(r_key);
  } // new peer detected
		
  // send Confirm if one was requested
  // Now check if other side solicited a confirm		
  if (incoming_pdu->get_R()) 
  {
    if (r_entry->is_dmode() && !r_entry->is_ma_reuse_requested()) 
    {
      DLog(param.name, "Sending back confirm in D-Mode");
      send_confirm_dmode(incoming_pdu, r_key, r_entry, incoming_pdu->get_nli());
    } 
    else 
    {
      if (r_entry->is_secure())
      {
	send_confirm_cmode(incoming_pdu, r_key, r_entry, incoming_pdu->get_nli());
	DLog(param.name, "Sending back confirm in TLS secured C-Mode");
      } 
      else 
      {
	send_confirm_cmode(incoming_pdu, r_key, r_entry, incoming_pdu->get_nli());
	DLog(param.name, "Sending back confirm in unsecure C-Mode");
      }
    }
  } 
  else 
  {
    DLog(param.name, "No Confirm was requested");
  }
		
  // So, confirm is dispatched, but we need to set up timers on our routing entry!
		
  // freshen NLI
  if (!rerouted) 
	  r_entry->set_peer_nli(incoming_pdu->get_nli());

  DLog(param.name, "Installed peer's RS-Validity time of " << r_entry->get_peer_nli()->get_rs_validity_time() <<"ms");

  // (re)Start refresh_QNode timer
  starttimer(r_key, r_entry, refresh_qnode, 0, randomized(r_entry->get_peer_nli()->get_rs_validity_time(), gconf.getpar<float>(gistconf_retryfactor)));

  // Start inactive_QNode timer if not running
  if (r_entry->get_timer_type_1() != inactive_qnode)
	  starttimer(r_key, r_entry, inactive_qnode, 0, r_entry->get_rs_validity_time());

  r_entry->set_state(qn_established);

  networknotification(r_key, r_entry, APIMsg::route_changed_status_good, rerouted);
}


void
Statemodule::handle_response_in_established(const known_ntlp_pdu* incoming_pdu, routingkey* r_key, routingentry* r_entry)
{
  if (r_key == NULL or r_entry == NULL)
  {
    ERRCLog(param.name, "Handle Response in Established: no valid routing state or routing key given, ignoring");
    return;
  }

  // if confirm was request we have to send it back
  if (incoming_pdu->get_R()) {

    DLog(param.name, "Established, but however sending back a GIST Confirm, as it was requested");
    if (r_entry->is_dmode()) {
	    send_confirm_dmode(incoming_pdu, r_key, r_entry, incoming_pdu->get_nli()); 
    } else {
	    send_confirm_cmode(incoming_pdu, r_key, r_entry, incoming_pdu->get_nli()); 
    }
    
  }
	    
  // start Refresh_QNode again
  starttimer(r_key, r_entry, refresh_qnode, 0, randomized(r_entry->get_peer_nli()->get_rs_validity_time(), gconf.getpar<float>(gistconf_retryfactor)));

  // Inactive_QNode should be running already (was probably restarted by incoming piggybacked NSLP data earlier)
  if (r_entry->get_timer_type_1() != inactive_qnode)
  {
    ERRCLog(param.name, "Response in established: expected that " << timername(inactive_qnode) << " timer is already running - internal error");
    //starttimer(r_key, r_entry, inactive_qnode, 0, r_entry->get_rs_validity_time());
  }

  // send stored messages
  sendqueue(r_key, r_entry);

  // update RS_Validity Time (we update the whole NLI)
  if (incoming_pdu->get_nli()) {
    r_entry->set_peer_nli(incoming_pdu->get_nli());
  }
}


/**
 * Generate a Query Cookie by using OpenSSL random generator
 * @param key -- the routing key specifying the routing entry (may NOT be NULL)
 */
querycookie* 
Statemodule::query_cookie(const routingkey* key)
{
	uint32 len = param.secrets.get_secret_size() / 8;

	uchar* tmp = new uchar[len];

	if (!RAND_bytes(tmp, len)) {
		ERRCLog(param.name, "OpenSSL Random number generator could not get a random number");
		abort();
	}

	// 8th: Put into Query Cookie
	// content of tmp will be copied
	querycookie* cookie = new querycookie(tmp, len);

	// Debug output
	ostringstream tmpostr;
	for (unsigned int i = 0; i < len; i++)
		tmpostr << hex << (int) tmp[i];
	DLog(param.name, "Computed Query Cookie, length " << len * 8 << " Bit: " << tmpostr.str().c_str());

	delete [] tmp;

	return cookie;
}


/**
 * Send a GIST Confirm in D-Mode
 * @param pdu -- The pdu which elicits the confirm (may not be NULL, need responder cookie)
 * @param r_key -- the routing key specifying the routing entry (may NOT be NULL)
 * @param r_entry -- the routing entry itself (may NOT be NULL)
 * @param peer_nli -- the peer to which the confirm should be sent (may NOT be NULL)
 */
void
Statemodule::send_confirm_dmode(const known_ntlp_pdu* pdu, const routingkey* r_key, routingentry* r_entry, const nli* peer_nli) {

  assert(pdu != NULL);
  assert(r_key != NULL);
  assert(r_entry != NULL);
  assert(peer_nli != NULL);

  // construct signaling message
  SignalingMsgNTLP* sigmsg = new SignalingMsgNTLP();
    
  // construct target address
  appladdress* target= new appladdress;
  target->set_port(gconf.getpar<uint16>(gistconf_udpport));
  target->set_ip(peer_nli->get_if_address());
  target->set_protocol(param.udp);

  respcookie* rspcook= pdu->get_respcookie();
  if (!rspcook) 
  {
    DLog(param.name, "No Respcookie available to send.");
  }
  // construct CONFIRM with copies of objects in response, but no stackprop and NLI of US
    
  nli* mynli = build_local_nli(r_key->mr->get_ip_version(), peer_nli->get_if_address(), r_entry);
  mynli->set_ip_ttl(r_entry->get_ip_ttl());


  confirm* conf=new confirm(r_key->mr->copy(), r_key->sid->copy(), mynli, rspcook ? rspcook->copy() : 0, NULL, NULL, NULL);

  // set IP TTL in target address
  target->set_ip_ttl(mynli->get_ip_ttl());

  conf->set_nslpid(r_key->nslpid);

  conf->set_hops(1);

  //set S-flag
  conf->set_S();

  // put into signaling message
  sigmsg->set_req(target, conf);
  sigmsg->set_local_addr(r_entry->get_local_src()->copy());
  sigmsg->send_or_delete();
}


/**
 * Send a GIST Confirm in C-Mode
 * @param pdu -- The pdu which elicits the confirm (if available, may be NULL)
 * @param r_key -- the routing key specifying the routing entry (may NOT be NULL)
 * @param r_entry -- the routing entry itself (may NOT be NULL)
 * @param peer_nli -- the peer to which the confirm should be sent (may NOT be NULL)
 * @return returns false if a failure occured, true if confirm was issued at least
 */
bool
Statemodule::send_confirm_cmode(const known_ntlp_pdu* pdu, const routingkey* r_key, routingentry* r_entry, const nli* peer_nli) 
{
  assert(pdu != NULL);
  assert(r_key != NULL);
  assert(r_entry != NULL);
  assert(peer_nli != NULL);

  // construct signaling message
  SignalingMsgNTLP* sigmsg = new SignalingMsgNTLP();
  sigmsg->set_local_addr(r_entry->get_local_src()->copy());

  bool use_existing_ma= false;
  appladdress* target = NULL;
  // construct target address for sending back any error messages
  appladdress*  errortarget= new appladdress;
  errortarget->set_port( gconf.getpar<uint16>(gistconf_udpport) );
  errortarget->set_ip(peer_nli->get_if_address());
  errortarget->set_protocol(param.udp);

  if (pdu->get_stackprop() && pdu->get_stackconf()) 
  { // if stack proposal and stack configuration data were present
    // construct target address
    target= cap.get_address(pdu->get_stackprop(), pdu->get_stackconf(), peer_nli, r_entry->is_secure());
    if (target == NULL || target->get_port() == 0)
    {
      ERRCLog(param.name, "Could not find suitable proposal for C-Mode. Cannot send a confirm. Doing Nothing.");     
      return false;
    }
  } 
  else 
  {
    // no stack proposal and stuff, this could be the case for receiving responses sent over existing MAs for MA re-use
    // state may already have been changed to established before calling this method
    if ( (r_entry->is_cmode() || r_entry->is_ma_reuse_requested(peer_nli)) && 
	(r_entry->get_state() == qn_awaiting_response || r_entry->get_state() == qn_awaiting_refresh || 
	 r_entry->get_state() == qn_established) )
    {
      // try ma re-use
      target= param.rt.lookup_ma_peeraddress(peer_nli);
      if (target == NULL)
      {
	      ERRCLog(param.name, "No existing MA found and no Stack proposal/config given, cannot figure out where to send Confirm in C-Mode. This is probably an internal error.");
	      // this should actually not happen as we have gotten the response via an existing MA

	      // MA re-use not possible
	      r_entry->set_ma_reuse_requested(peer_nli,false);
	      // send back missing object error
	      senderror(pdu, errortarget, GIST_Error::error_gist_missing_object, known_ntlp_object::StackProposal);
	      
	      return false;
      }
      else
      {
	      use_existing_ma= true;
      }
    }
    else
    {
      ERRCLog(param.name, "No stack proposal in PDU, but we wanted to set up C-Mode. Doing Nothing.");

      // send back missing object error
      senderror(pdu, errortarget, GIST_Error::error_gist_missing_object, known_ntlp_object::StackProposal);

      return false;
    }
  }

  // if we need to set up a new MA, we must have a responder cookie
  if (!use_existing_ma)
  {
    if ( !pdu->get_respcookie()) 
    {
      ERRCLog(param.name, "No Responder Cookie available to send, but required because C-mode setup requested. Cannot proceed.");
      // send back missing object error
      senderror(pdu, errortarget, GIST_Error::error_gist_missing_object, known_ntlp_object::RespCookie);
      
      return false;
    }
  }

  // An already existing MA is either re-used or a new one is created
  // If the attributes of an existing connection are the same as the one returned by our "capability lookup" above, it will automatically
  // overwrite the same Association (NLI is the key to the MA table), if they do not fit, the MA gets destroyed and another one is created.

  // save Message Association, start refresh-timer (done by routingtable object)
  // if no stack conf available, we should have an MA anyway
  if (pdu->get_stackconf())
	  param.rt.add_ma(peer_nli->copy(), *target, pdu->get_stackconf()->get_ma_hold_time(),false);

  // construct CONFIRM with copies of objects in response, but no data and NLI of US
  nli* mynli = build_local_nli(r_key->mr->get_ip_version(), peer_nli->get_if_address(), r_entry);
  mynli->set_ip_ttl(r_entry->get_ip_ttl());

  // construct an empty ma_protocol_option object, only holding MA_HOLD_TIME, repetition for delayed state installation on peer
  // but this is only required for the very first confirmation message during
  // initial MA setup
  stack_conf_data* mysc = (use_existing_ma == false)  ? new stack_conf_data(gconf.getpar<uint32>(gistconf_ma_hold_time)) : NULL;
    
  // echo responder cookie
  respcookie* r_cook = NULL;
  if (pdu->get_respcookie()) r_cook=pdu->get_respcookie()->copy();
    
  // echo stack proposal
  stackprop* s_prop = NULL;
  if (pdu->get_stackprop()) s_prop = pdu->get_stackprop()->copy();

  assert(r_key->mr != NULL);
  assert(r_key->sid != NULL);
    
  // construct Confirm PDU
  confirm* conf= new confirm(r_key->mr->copy(), r_key->sid->copy(), mynli, r_cook, s_prop, mysc, NULL);

  conf->set_nslpid(r_key->nslpid);
    
  // set S-flag
  conf->set_S();

  // set Hops
  conf->set_hops(1);

  // put into signaling message
  sigmsg->set_req(target, conf);
  sigmsg->send_or_delete();  

  // no error occured, its safe to delete the whole thing, because 
  // errors would have returned already to caller
  delete errortarget;

  return true;
}


/**
 * InactiveQNode Timer processing. Routing State gets deleted
 * @param r_key -- the routing key (may NOT be NULL)
 * @param timermsg -- the timer message
 */
void 
Statemodule::to_inactive_qnode(const routingkey* r_key, TimerMsg* timermsg) 
{
  DLog(param.name, "to_inactive_qnode(): looking up routing key");
    
  routingentry* r_entry = param.rt.lookup(r_key);
    
  if (r_entry) 
  {
	  if (r_entry->get_timer_type_1() != inactive_qnode) {
		  DLog(param.name, "Deliberately stopped timer, ignoring");
		  param.rt.unlock(r_key);
		  return;
	  }

	  if (r_entry->get_timer_id_1() == timermsg->get_id()) 
	  {
		  DLog(param.name, "Timer slot 1, timer is valid, id: " << timermsg->get_id());
	
		  // inactive_qnode timer is to be applied from all states
		  
		  r_entry->set_state(dead);
		  EVLog(param.name, color[magenta] << "InactiveQNode timer went off" << color[off]);
		  // notify about Routing State down
		  // Notify NSLP about Routing State Change
	    
		  networknotification(r_key, r_entry, APIMsg::route_changed_status_bad, true);
      
		  // we should stop any running timers on slot 2, refresh qnode
		  // slot 3 should not be running at this point in time!
		  canceltimers(r_entry);

		  param.rt.delete_entry(r_key);
		  r_key= 0;
	  }
	  if (r_key) param.rt.unlock(r_key);
  }
}

/**
 * NoResponse Timer processing. Repeat Queries if retry count is OK
 * @param r_key -- the routing key (may NOT be NULL)
 * @param timermsg -- the timer message
 */
void Statemodule::to_no_response(const routingkey* r_key, TimerMsg* timermsg) 
{
  assert( r_key != NULL );

  DLog(param.name, "to_no_response(): looking up routing key");
    
  routingentry* r_entry = param.rt.lookup(r_key);
    
  if (r_entry) 
  {
    if (r_entry->get_timer_type_2() != noresponse) {
      DLog(param.name, "Deliberately stopped timer, ignoring");
      param.rt.unlock(r_key);
      return;
    }

    if (r_entry->get_timer_id_2() == timermsg->get_id()) 
    {
      DLog(param.name, "Timer slot 2, timer is valid, id: " << timermsg->get_id());


      EVLog(param.name, color[magenta] << "NoResponse Timer went off" << color[off]);
      // NoResponse timer retransmits Queries until state changes to rn_established and timer is stopped
      if (! r_entry->retry_limit_reached() &&
          (param.addresses.addr_is(*r_entry->get_local_src(), AddressList::LocalAddr_P) ||
	   param.addresses.addr_is(*r_entry->get_local_src(), AddressList::HomeAddr_P)))
      {
	if (r_entry->is_dmode()) { 
	  send_query(NULL, r_key, r_entry); 
	} else {
	  send_query_handshake(NULL, r_key, r_entry);
	}
		
	DLog(param.name, "Query retry dispatched (after " << r_entry->get_retry_timeout() << " ms)");
		
	// perform exponential backoff for next try
	r_entry->increase_retry_timeout();
	// restart noresponse timer
	starttimer(r_key, r_entry, noresponse, 0, r_entry->get_retry_timeout(), false);        

      } 
      else 
      {
	// leave NoResponse timer stopped
		
	r_entry->set_state(dead);
		
	// clear data for repeats
	r_entry->set_mod_data(NULL);
		
	// notify about Routing State INVALID (was never UP!)
	// Notify NSLP about Routing State Change
		
	networknotification(r_key, r_entry, APIMsg::route_changed_status_invalid, true);
		
	// notify about lost Message because no node found (handshake went wrong)
	messagestatus(r_key, r_entry, r_entry->get_msg_id(), APIMsg::error_msg_node_not_found);
				
	// notify about inverse direction Routing State 'tentative'
	routingkey* tmpkey = new routingkey;
		
	if (r_key) {
		    
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
		
	// delete the original state
	if (r_key) param.rt.delete_entry(r_key);
      }
	    
      if (r_key) param.rt.unlock(r_key);
    } // end if timer valid
  } // if r_entry
}


/**
 * RefreshQNode Timer processing. Start a refresh cycle
 * @param r_key -- the routing key (may NOT be NULL)
 * @param timermsg -- the timer message
 */
void 
Statemodule::to_refresh_qnode(const routingkey* r_key, TimerMsg* timermsg) 
{
  DLog(param.name, "to_refresh_qnode(): looking up routing key");
    
  routingentry* r_entry = param.rt.lookup(r_key);
    
  if (r_entry) 
  {
    if (r_entry->get_timer_type_2() != refresh_qnode) {
      DLog(param.name, "Deliberately stopped timer, ignoring");
      param.rt.unlock(r_key);
      return;
    }

    if (!(param.addresses.addr_is(*r_entry->get_local_src(), AddressList::LocalAddr_P) ||
	  param.addresses.addr_is(*r_entry->get_local_src(), AddressList::HomeAddr_P))) {
      DLog(param.name, "Local Address no longer owned - let it die!");
      param.rt.unlock(r_key);
      return;
    }

    if (r_entry->get_timer_id_2() == timermsg->get_id()) 
    {
      DLog(param.name, "Timer slot 2, timer is valid, id: " << timermsg->get_id());

      EVLog(param.name, color[magenta] << "RefreshQNode timer went off" << color[off]);

      // send Query, start NoResponse Timer, transit to awaiting refresh, stop RefreshQNode 

      // delete query cookie before initiating the Query Sequence ->new cookies are used each handshake
      r_entry->set_querycookie(NULL);
	    
      // send Query
      if (r_entry->is_dmode())
      {
	send_query(NULL, r_key, r_entry); 
      } 
      else 
      {
	send_query_handshake(NULL, r_key, r_entry);
      }
	    
      // transition to qn_awaiting_refresh
      r_entry->set_state(qn_awaiting_refresh);
	    
      // kickstart NoResponse timer
      starttimer(r_key, r_entry, noresponse, 0, r_entry->reset_retry_timeout(), false);
	    
      param.rt.unlock(r_key);	    
    } else {
      param.rt.unlock(r_key);
    }
  }
}

//@}

} // end namespace ntlp
