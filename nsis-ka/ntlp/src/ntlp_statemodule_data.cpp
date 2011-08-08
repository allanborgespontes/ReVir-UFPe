/// ----------------------------------------*- mode: C++; -*--
/// @file ntlp_statemodule_data.cpp
/// GIST state module - implements GIST functions for handling data pdus
/// ----------------------------------------------------------
/// $Id: ntlp_statemodule_data.cpp 5840 2011-01-20 14:02:25Z bless $
/// $HeadURL: https://svn.tm.kit.edu/nsis/ntlp/trunk/src/ntlp_statemodule_data.cpp $
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
/** @defgroup nslpdatahandling NSLP Data handling
 * @ingroup statemodule
 * Statemodule
 * @{
 * This module implements data PDU handling methods for GIST
 *
 */
#include "ntlp_statemodule.h"
#include "data.h"

#include "gist_conf.h"

namespace ntlp {

using namespace protlib;


/**
 * Send a GIST data with explicit routing
 * @param apimsg -- the API message containing payload and parameters
 */
void 
Statemodule::send_data_explicit(const APIMsg* apimsg) 
{

  if (!apimsg) 
  {
    ERRCLog(param.name, "Tried to do explicit routing without API Message?!?!");
    return;
  } 

  if (!apimsg->get_sii_handle())
  {
    ERRCLog(param.name, "Tried to do explicit routing without SII Handle?!?!");
    return;
  } 

  // explicit routing: message must find its way to the peer that is referred to by the SII handle
  const nli* dest_nli = param.rt.findNLI(apimsg->get_sii_handle());

  if (!dest_nli) 
  {
    ERRCLog(param.name, "Tried to do explicit routing but SII-Handle is not valid any more, i.e., if have no idea about the peer it once reffered to");
    return;
  } 


  SignalingMsgNTLP* sigmsg = new SignalingMsgNTLP();

  // Build a DATA Message
  
  sessionid* mysid = new sessionid(*(apimsg->get_sessionid()));

  // take eventual payload from apimsg
  nslpdata* mydata=NULL;
  if (apimsg->get_data())
    mydata= new nslpdata(*(apimsg->get_data()));

  mri* mymri= apimsg->get_mri()->copy();
  
  nli* own_nli= build_local_nli(mymri->get_ip_version(), dest_nli->get_if_address());
  own_nli->set_ip_ttl(apimsg->get_ip_ttl());
    
  // construct DATA
  data* pdu = new data(mymri, mysid, own_nli, mydata);

  pdu->set_nslpid(apimsg->get_nslpid());
  pdu->set_hops(apimsg->get_ghc());
  
  // explicit routing took place (usually sending for DEAD state)
  pdu->set_E();

  // set S-flag
  pdu->set_S();


  appladdress* peer = NULL;
  // get routing state
  const routingkey myrkey(mymri,mysid,apimsg->get_nslpid());
  routingentry* r_entry= param.rt.lookup(&myrkey);
  if (r_entry)
  {
    if (r_entry->get_state() == qn_established || r_entry->get_state() == qn_awaiting_refresh) 
    {
	    // Restart InactiveQNode timer (running on slot 1)
	    starttimer(&myrkey, r_entry, inactive_qnode, 0, r_entry->get_rs_validity_time());
    }
    // as a responder we are not allowed to re-setup MAs
    if (r_entry->is_responding_node())
	    sigmsg->set_useexistconn();

    // check if we have to use an existing MA
    // if reliable or secure transport is requested, we have to use it
    // otherwise its better to send it via D-mode since the connection may have stopped working
    if (apimsg->get_tx_attr().reliable || apimsg->get_tx_attr().secure)
    {
	    peer= param.rt.lookup_ma_peeraddress(dest_nli);
	    if (peer) 
	    {
		    // indicate activity to MA maintainance
		    param.rt.activity_ind_ma(dest_nli);
	    }

	    sigmsg->set_local_addr(r_entry->get_local_src()->copy());
	    param.rt.unlock(&myrkey);
    } // endif reliable or secure transfer required
  }
  else
	  WLog(param.name,"explicit routing requested, but could not determine querier/responder role, because no routing entry found");
  
  // if peer not set yet, we should use D-mode
  if (peer == NULL)
	  peer = new appladdress(dest_nli->get_if_address(), param.udp, gconf.getpar<uint16>(gistconf_udpport));

  // set IP TTL
  DLog(param.name, "Set IP TTL in target address");
  peer->set_ip_ttl(own_nli->get_ip_ttl());

  // send it
  sigmsg->set_req(peer, pdu);

  sigmsg->send_or_delete();
}



/**
 * Send a GIST Data in D-Mode
 * @param nslp_data -- the data payload
 * @param r_key -- the routing key specifying the routing entry
 * @param r_entry -- the routing entry itself
 * @param msghandle -- the NSLP MsgHandle, given here for eventual error messages
 */
void 
Statemodule::send_data_dmode(nslpdata*  nslp_data, routingkey* r_key, routingentry* r_entry,  msghandle_t msghandle) 
{
  if (r_entry->get_state()==qn_established || r_entry->get_state()==qn_awaiting_refresh) 
  {
    // Restart InactiveQNode timer (running on slot 1)
    starttimer(r_key, r_entry, inactive_qnode, 0, r_entry->get_rs_validity_time());
  }

  // save Message ID for this Message (in case there will be an error)
  r_entry->set_msg_id(msghandle);
  
  appladdress* peer= new appladdress;
  peer->set_port( gconf.getpar<uint16>(gistconf_udpport) );
  peer->set_protocol(param.udp);
  
  peer->set_ip(r_entry->get_peer_nli()->get_if_address());
  
  SignalingMsgNTLP* sigmsg = new SignalingMsgNTLP();
  sigmsg->set_local_addr(r_entry->get_local_src()->copy());

  // Build a DATA Message
  sessionid* mysid = new sessionid(*r_key->sid);

  // take eventual payload from apimsg
  nslpdata* mydata=NULL;
  if (nslp_data) 
  {
    mydata=new nslpdata(*nslp_data);
  }

  mri* mymri=r_key->mr->copy();
    
  nli* own_nli=build_local_nli(mymri->get_ip_version(), r_entry->get_peer_nli()->get_if_address(), r_entry);
    
  own_nli->set_ip_ttl(r_entry->get_ip_ttl());
    

  // construct DATA
  data* pdu = new data(mymri, mysid, own_nli, mydata);

  pdu->set_nslpid(r_key->nslpid);
  pdu->set_hops(r_entry->get_gist_hop_count());
  
  // if explicit routing took place (sending for DEAD state)
  if (r_entry->get_state() == dead) 
  {  
    pdu->set_E();  
  }

  // set S-flag
  pdu->set_S();

  // set IP TTL
  DLog(param.name, "Set IP TTL in target address");
  peer->set_ip_ttl(own_nli->get_ip_ttl());

  // set hops
  pdu->set_hops(1);
  
  
  // send it
  sigmsg->set_req(peer, pdu);
  sigmsg->send_or_delete();
} //end send_data_dmode



/**
 * Send a GIST Data in C-Mode
 * @param payload -- the data payload
 * @param r_key -- the routing key specifying the routing entry
 * @param r_entry -- the routing entry itself
 * @param msghandle -- the NSLP MsgHandle, given here for eventual error messages
 */
void 
Statemodule::send_data_cmode(nslpdata* payload, routingkey* r_key, routingentry* r_entry, msghandle_t msghandle) 
{

  if (r_entry->get_state() == qn_established || r_entry->get_state() == qn_awaiting_refresh) 
  {
    // Restart InactiveQNode timer (running on slot 1)
    starttimer(r_key, r_entry, inactive_qnode, 0, r_entry->get_rs_validity_time());
  }
    
  appladdress* peer= param.rt.lookup_ma_peeraddress(r_entry->get_peer_nli());
  if (peer) 
  {
    // indicate activity to MA maintainance
    param.rt.activity_ind_ma(r_entry->get_peer_nli());

    SignalingMsgNTLP* sigmsg = new SignalingMsgNTLP();
    sigmsg->set_local_addr(r_entry->get_local_src()->copy());
    
    // Build a DATA Message
    sessionid* mysid = new sessionid(*(r_key->sid));
	
    // take eventual payload from apimsg
    nslpdata* mydata=NULL;
    if (payload) mydata=new nslpdata(*(payload));
	
    mri* mymri=r_key->mr->copy();
	
    // construct DATA
    data* pdu = new data(mymri, mysid, NULL, mydata);
	
    pdu->set_nslpid(r_key->nslpid);
	
    // set S-flag
    pdu->set_S();

    // set Hops
    pdu->set_hops(1);

    // send it
    sigmsg->set_req(peer, pdu);
    sigmsg->send_or_delete();
    
  } 
  else 
  {
    EVLog(param.name, color[green] << "No Messaging Association was found to send -> trying to re-establish" << color[off]);
    
    if (r_entry->is_responding_node()) {
      
      ERRLog(param.name, color[green] << "I am Responder Node, I cannot establish a Messaging Association. Dropping data");
    } 
    else 
    {
      ERRLog(param.name, color[green] << "I am Querying Node, I will re-establish Routing State, enqueuing data, will be dropped after " << data_keep_timeout << "sec");
      
      enqueuedata(r_key, r_entry, payload, data_keep_timeout, msghandle);
      
      r_entry->set_state(qn_awaiting_response);

      // start NoResponse timer    
      starttimer(r_key, r_entry, noresponse, 0, r_entry->reset_retry_timeout());

    }
  }
} //end send_data_cmode

/**
 * Enqueue data in Message Queue for a specified entry, start Queue Poll Timer
 * @param key -- the routing key (may NOT be NULL)
 * @param entry -- the routing entry itself (may NOT be NULL)
 * @param data -- the Data Payload
 * @param timeout -- the timeout given for this data
 * @param msghandle -- the msghandle given for this data
 */
void 
Statemodule::enqueuedata(const routingkey* key, routingentry* entry, nslpdata* data, uint32 timeout, msghandle_t msghandle) 
{
  // enqueue a COPY, as the apimsg from which it is derived gets deleted
  queueentry* tmp = new queueentry;
    
  tmp->timeout = timeout;
  tmp->msghandle = msghandle;
  tmp->data= new nslpdata(*data);

  entry->dataqueue.push_back(tmp);

  // Look if this is the initial entry, so we must start the Queue Poll Timer for this entry
  if (entry->dataqueue.size()==1) {
    starttimer(key, entry, queue_poll, queue_poll_timeout_default, 0);
  }
   
  EVLog(param.name, "Enqueued data for later sending. Count of enqueued data for this target: " << entry->dataqueue.size());
}

/**
 * Send Message Queue contents
 * @param key -- the routing key (may NOT be NULL)
 * @param entry -- the routing entry itself (may NOT be NULL)
 */
void 
Statemodule::sendqueue(routingkey* key, routingentry* entry) 
{
  nslpdata* mydata=NULL;
  DLog(param.name, "SendQueue called, queued data objects: " << entry->dataqueue.size());
  for (unsigned int i=0; i < entry->dataqueue.size(); i++) 
  {
    // if the data is not yet timed out, else it is 0 and data object NULL!
    if (entry->dataqueue[i]->timeout) 
    {
      mydata=entry->dataqueue[i]->data;    
      // this cannot be explicitly routed, as we would not use C-Mode
      if (entry->is_dmode() && !entry->is_ma_reuse_requested()) {
	send_data_dmode(mydata, key, entry, entry->dataqueue[i]->msghandle);
      }
      else 
	send_data_cmode(mydata, key, entry, entry->dataqueue[i]->msghandle);
    }
    // notify about sent message!
    // TODO?
  }
  entry->dataqueue.clear();
}


/**
 * QueuePoll Timer processing. Maintain timeouts in a Message Queue, clean it up, if all timed out, restart until Queue empty
 * @param r_key -- the routing key (may NOT be NULL)
 * @param timermsg -- the timer message
 */
void 
Statemodule::to_queue_poll(const routingkey* r_key, TimerMsg* timermsg) 
{
  DLog(param.name, "to_queue_poll(): looking up routing key");
    
  routingentry* r_entry = param.rt.lookup(r_key);
    
  if (r_entry) 
  {

      if (r_entry->get_timer_type_3() != queue_poll) {
      DLog(param.name, "Deliberately stopped timer, ignoring");
      param.rt.unlock(r_key);
      return;
    }

    if (r_entry->get_timer_id_3() == timermsg->get_id()) 
    {
      DLog(param.name, "Timer slot 3, timer is valid, id: " << timermsg->get_id());

      // Queue Poll Timer
	    
      if (r_entry->dataqueue.size()!=0) {
		
	// Reorganize Queue (count timeouts down by queue poll timer interval (2 sec))
	bool anyone_alive = false;
	for (uint32 o = 0; o< r_entry->dataqueue.size(); o++) 
	{
	  // process only not yet timed out timers
	  if (r_entry->dataqueue[o]->timeout != 0) 
	  {
	    if (r_entry->dataqueue[o]->timeout > queue_poll_timeout_default) 
	    {
	      r_entry->dataqueue[o]->timeout -= queue_poll_timeout_default;
	      // count as alive
	      anyone_alive= true;
	    } 
	    else 
	    { // remaining timeout value is smaller than queue_poll_timeout_default
	      // or exactly queue_poll_timeout_default and would be decremented to zero now
	      // this entry just timed out
			    
	      // notify NSLP
	      messagestatus(r_key, r_entry, r_entry->dataqueue[o]->msghandle, APIMsg::error_msg_node_not_found);
    
	      r_entry->dataqueue[o]->timeout = 0;
	      delete r_entry->dataqueue[o]->data;
	      r_entry->dataqueue[o]->data= 0;
	    }
	  }
	} // end for
		
	// if only dead entries, delete them
	if (!anyone_alive) r_entry->dataqueue.clear();
		
	// Notify about timed out data
		
	// restart timer
	starttimer(r_key, r_entry, queue_poll, queue_poll_timeout_default, 0, false);
		
      } else {
	// Do not restart the timer
	DLog(param.name, color[green] << "Queue Poll timer encountered empty data queue, not restarting" << color[off]);
	
	// avoid stopping a non-running timer when queue poll timer is restarted
	r_entry->set_timer_type_3(none);
	r_entry->set_timer_id_3(0);
      }
	    
      param.rt.unlock(r_key);
    }
  }
}


} // end namespace ntlp

//@}
