/// ----------------------------------------*- mode: C++; -*--
/// ntlp_statemodule.h
/// GIST State Module
/// ----------------------------------------------------------
/// $Id: ntlp_statemodule.h 5720 2010-10-14 11:55:52Z bless $
/// $HeadURL: https://svn.tm.kit.edu/nsis/ntlp/trunk/src/ntlp_statemodule.h $
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
 * @file ntlp_statemodule.h
 * the state machine managing module
 */

#ifndef _NTLP_STATEMODULE_H
#define _NTLP_STATEMODULE_H

#include "messages.h"
#include "threads.h"
#include "signalingmodule_ntlp.h"
#include "secretmanager.h"
#include "ntlp_errorobject.h"
#include "routingtable.h"
#include "nli.h"
#include "apimessage.h"
#include "capability.h"
#include "authorized_peer_db.h"
#include "flowinfo.h"
#ifdef USE_AHO
#include "network_notification_module.h"
#endif

// forward declaration from timer_module.h
namespace protlib
{
class TimerMsg;
}



namespace ntlp {

using namespace protlib;

/** @ingroup statemodule State Machine Managing Module
 * @{
 */



/// typedef transfert_t
typedef enum transfer_t_enum {
  tx_queryencap,
  tx_dmode,
  tx_cmode,
  tx_cmode_sec
} transfer_t;
  
/// statemodule parameters
struct StatemoduleParam : public ThreadParam {
  StatemoduleParam(routingtable& rt,
		   nli &nli_tmpl,
		   AddressList &addresses,
		   Flowinfo *fi_service,
		   secretmanager& secrets,
		   NSLPtable& nslptable,
		   vector<uint32>& raovec,
		   uint32 sleep = ThreadParam::default_sleep_time
		   );
  const message::qaddr_t qaddr_ext;
  const message::qaddr_t qaddr_int;
  routingtable& rt;
  nli &nli_tmpl;
  AddressList &addresses;
  Flowinfo *fi_service;
  uint32 retrylimit;
  uint32 retryperiod;
  float retryfactor;
  uint32 rs_validity_time;
  uint32 refreshtime;
  uint32 ma_hold_time;
  secretmanager& secrets;

  NSLPtable& nslptable;
  vector<uint32>& raovec;
  const message::qaddr_t timermsgsource;
  const bool requesttimererrors;
  
  protocol_t qenc; // query encapsulation
  protocol_t udp;
  protocol_t tcp;
  protocol_t sctp;
  
}; // end StatemoduleParam
  
/// state module class
class Statemodule : public Thread {
public:
  /// constructor
  Statemodule(const StatemoduleParam& p);
  /// destructor
  virtual ~Statemodule();
  /// module main loop
  virtual void main_loop(uint32 nr);


  /// fill a query cookie, use local secrets and OpenSSL hash function
  querycookie* query_cookie(const routingkey* r_key);   

  /// fill a query cookie, use local secrets and OpenSSL hash function
  respcookie* create_resp_cookie(const nli* querier_nli, const routingkey* r_key, uint32 gennumber, uint16 if_index, const NetMsg* transparent_data);

  /// check a responder cookie, use local secrets and resp_cookie generation function
  bool evaluate_resp_cookie(const nli* querier_nli, const routingkey* r_key, const respcookie* resp);

  ///calculation of refresh times. Give maximum time allowed and the factor to apply
  static uint32 randomized(uint32 value, float factor);

  const StatemoduleParam& getParam() const { return param; }

private:
  /// used to store available protocol profiles and to validate connection properties
  capability cap;
  authorized_peer_db auth_peer_db;

  enum action_t {
    action_bad_state,
    action_next_module,
    action_finish_request,
    action_forward_request,
    action_abort_context,
    action_user_abort
  }; // end action_t
	
  /// module parameters
  const StatemoduleParam& param;
  /// additional queue
  FastQueue* external_fq;
  //@{ message processing

protected:
  /// process internal messages
  void handleInternalMessage(message* msg);
  
  /// process timouts
  void handleTimeout();

private: 
  /// get messages from queue
  void process_queue(uint32 nr);

  /// process messages from signaling module, incorporate Node State Machine
  void process_sig_msg(SignalingMsgNTLP* msg);

  /// process messages from signaling module, incorporate Node State Machine
  void process_api_msg(APIMsg* msg);

  void handle_error_pdu(const known_ntlp_pdu* errpdu);

  /// got initial QUERY
  void handle_initial_query(const known_ntlp_pdu* query_pdu, routingkey* r_key, const appladdress* peer, const appladdress* own_addr);

  /// handle refreshing QUERY
  void handle_query_in_established(const known_ntlp_pdu* query_pdu, routingkey* r_key, routingentry* r_entry, const appladdress* peer);

  /// handle QUERY in awaiting confirm or in awaiting refresh
  void handle_query_in_awaiting_confirm_or_refresh(const known_ntlp_pdu* cnfpdu, routingkey* r_key, routingentry* r_entry, const appladdress* peer, const appladdress* own_addr);

  /// got CONFIRM when expected
  void handle_confirm_in_awaiting_confirm(const known_ntlp_pdu* cnfpdu, routingkey* r_key, routingentry* r_entry, const appladdress* peer, const appladdress* own_addr);

  /// got CONFIRM in delayed state mode
  void handle_confirm_delayed_state_install(const known_ntlp_pdu* cnfpdu, routingkey* r_key,  const appladdress* peer, const appladdress* own_addr);

  /// got CONFIRM in awaiting refresh
  void handle_confirm_in_awaiting_refresh(const known_ntlp_pdu* cnfpdu, routingkey* r_key, routingentry* r_entry, const appladdress* peer, const appladdress* own_addr);

  /// got RESPONSE when expected
  void handle_response_in_awaiting_response(known_ntlp_pdu* incoming_pdu, routingkey* r_key, routingentry* r_entry, const appladdress* peer);

  /// got RESPONSE in awaiting refresh
  void handle_response_in_awaiting_refresh(const known_ntlp_pdu* incoming_pdu, routingkey* r_key, routingentry* r_entry);

  /// got RESPONSE in qn_established
  void handle_response_in_established(const known_ntlp_pdu* incoming_pdu, routingkey* r_key, routingentry* r_entry);

  /// send a RESPONSE
  void send_response(const known_ntlp_pdu* pdu, const routingkey* rk, routingentry* re, const appladdress* peer= 0, bool forcerequestconfirm= false);
	
  /// pass data payload to NSLP (r_entry can be omitted)
  void deliver(const appladdress* peer, const appladdress* own_addr, const known_ntlp_pdu* pdu, const routingkey* r_key, const routingentry* r_entry = NULL);

  /// pass data payload to NSLP (r_entry can be omitted)
  void deliver_adjacency_flag(const appladdress* peer, const appladdress* own_addr, const known_ntlp_pdu* pdu, const routingentry* r_entry = NULL);

  /// send a GIST Query without asking for C-Mode handshake
  void send_query(const APIMsg* apimsg, const routingkey* r_key, routingentry* r_entry, bool handshake = false);
	
  /// send a GIST Query with asking for C-Mode handshake
  void send_query_handshake(const APIMsg* apimsg, const routingkey* r_key, routingentry* r_entry);

  /// send a GIST Confirm in D-Mode
  void send_confirm_dmode(const known_ntlp_pdu* pdu, const routingkey* r_key, routingentry* r_entry, const nli* peer_nli);
	
  /// send a GIST Confirm in C-Mode
  bool send_confirm_cmode(const known_ntlp_pdu* pdu, const routingkey* r_key, routingentry* r_entry, const nli* peer_nli);

  /// send GIST Data using explicit routing
  void send_data_explicit(const APIMsg* apimsg); 

  /// send GIST Data in D-Mode
  void send_data_dmode(nslpdata* nslp_data, routingkey* r_key, routingentry* r_entry, msghandle_t msghandle);

  /// send GIST Data in C-Mode
  void send_data_cmode(nslpdata* nslp_data, routingkey* r_key, routingentry* r_entry, msghandle_t msghandle);

  /// process timers
  void process_timer_msg(TimerMsg* timermsg);

  /// process mobility events
  void process_mobility_msg(MobilityMsg* mobmsg);

#ifdef USE_AHO
  /// process handover events
  void process_handover_msg(HandoverMsg* homsg);
#endif
	
  /// check a responder cookie, use local secrets and resp_cookie generation function
  bool evaluate_query_cookie(routingkey* r_key, respcookie* resp);

  /// check for untranslated mandatory objects in an NTO
  bool checkNATTraversal(const nattraversal* nattravobj, const known_ntlp_pdu* incoming_pdu, const appladdress* peer);
             
  /// construct a local NLI information object describing the node
  nli* build_local_nli(uint8 ip_version, const netaddress &dest, routingentry* re= NULL);

  /// enqueue data in a message queue and start Queue Poll timer
  void enqueuedata(const routingkey* key, routingentry* entry, nslpdata* data, uint32 timeout, msghandle_t msghandle);

  /// send enqueued Data
  void sendqueue(routingkey* key, routingentry* entry);

  /// Notify NSLP via NetworkNotification
  void networknotification(const routingkey* r_key, routingentry* r_entry, APIMsg::error_t status, bool force);
	
  /// Notify on timeout or something else concerning a Message
  void messagestatus(const routingkey* r_key, routingentry* r_entry, msghandle_t nslpmsgid, APIMsg::error_t status);

  /// Send Error Message to peer, if the error occured in StateModule processing
  void senderror(const known_ntlp_pdu* pdu, const appladdress* peer, GIST_Error::gist_error_code_t error, uint16 oti = 0, const ntlp_object* ovi_object= NULL);

  /// Start Timers on Routing Entries
  void starttimer(const routingkey* r_key, routingentry* r_entry, timer_type_t timer, uint32 sec, uint32 msec, bool stop = true);

  /// Stop Timers on Routing Entries
  void stoptimer(routingentry* r_entry, timer_type_t timer);

  /// Cancel Timers on Routing Entries
  void canceltimers(routingentry* r_entry);

  /// Expire RNode timeout
  void to_expire_rnode(const routingkey* r_key, TimerMsg* timermsg);

  /// InactiveQNode timeout
  void to_inactive_qnode(const routingkey* r_key, TimerMsg* timermsg);
	
  /// NoConfirm timeout
  void to_no_confirm(const routingkey* r_key, TimerMsg* timermsg);


  /// NoResponse timeout
  void to_no_response(const routingkey* r_key, TimerMsg* timermsg);

  /// NoResponse timeout
  void to_refresh_qnode(const routingkey* r_key, TimerMsg* timermsg);

  /// QueuePoll timeout
  void to_queue_poll(const routingkey* r_key, TimerMsg* timermsg);

  /// SecretsRefresh timeout
  void to_secrets_refresh();

  /// SendMessage API Call processing
  void tg_send_message(APIMsg* apimsg);

  /// RecvMessageAnswer API call processing
  void tg_recv_message_answer(APIMsg* apimsg);
	
  ///check refresh timer
  void check_refresh_timer(routingkey* r_key);

  bool is_local_address(const hostaddress &addr);

}; // end class Statemodule
    
//@}
    
} // end namespace

#endif // _NTLP_STATEMODULE_H
