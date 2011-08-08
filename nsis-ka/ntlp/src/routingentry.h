/// ----------------------------------------*- mode: C++; -*--
/// @file routingentry.h
/// GIST Messaging Routing Entry
/// ----------------------------------------------------------
/// $Id: routingentry.h 5249 2010-04-26 08:40:59Z roehricht $
/// $HeadURL: https://svn.tm.kit.edu/nsis/ntlp/trunk/src/routingentry.h $
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
#ifndef _NTLP__ROUTINGENTRY_H_
#define _NTLP__ROUTINGENTRY_H_

#include "protlib_types.h"
#include "nli.h"
#include "nattraversal.h"
#include "query_cookie.h"
#include "ntlp_pdu.h"
#include "msghandle.h"
#include "timer_module.h"
#include "apimessage.h"

namespace ntlp 
{

using namespace protlib;

/// Timers
typedef enum time_type_enum {
  invalid        = 0,
  expire_rnode   = 1,
  noconfirm      = 2,
  noresponse     = 3,
  inactive_qnode = 4,
  refresh_qnode  = 5,
  send_hello     = 6,
  no_activity    = 7,
  no_hello       = 8,
  refresh_secrets= 9,
  queue_poll     = 10,
  garbage_collect= 11,
  timer_stopped  = 12,
  last_timer_type= 13,
  none           = 255
} timer_type_t;


/// States for the running of state machine transitions
typedef enum routingstate_enum {
  initial = 0,
  qn_awaiting_response = 1,
  qn_established = 2,
  qn_awaiting_refresh = 3,
  rn_awaiting_confirm = 4,
  rn_established = 5,
  rn_awaiting_refresh =6,
  rn_querynslp = 7,
  dead = 8,
  rt_state_max= 9
} routingstate_t;

const char* const statestring[] = {
  "Initial",
  "QN_AWAITING_RESPONSE",
  "QN_ESTABLISHED",
  "QN_AWAITING_REFRESH",
  "QN_AWAITING_CONFIRM",
  "RN_ESTABLISHED",
  "RN_AWAITING_REFRESH",
  "RN_QUERYNSLP",
  "DEAD"
}; // end statestring

const char* const timerstring[] = {
  "INVALID_TYPE",
  "EXPIRE_R_NODE",
  "NO_CONFIRM",
  "NO_RESPONSE",
  "INACTIVE_Q_NODE",
  "REFRESH_Q_NODE",
  "SEND_HELLO",
  "NO_ACTIVITY",
  "NO_HELLO",
  "REFRESH_SECRETS",
  "QUEUE_POLL",
  "GARBAGE_COLLECT",
  "STOPPED",
  "INVALID_TYPE_LAST_TIMER"
}; // end timerstring

const char* timername(timer_type_t timertype);

struct queueentry {
  msghandle_t msghandle;
  uint32 timeout;
  nslpdata* data;
};

/// an entry for the routing data vector
class routingentry {

public:
  // constructor
  routingentry(bool is_responder);

  /// destructor
  ~routingentry() 
  {
    delete nattrav;
    delete translated_nli;
    delete peer_nli;
    delete qrc;
    delete tmppeer;
    delete tmppdu;
    delete mod_data;
    delete sp;
    delete sc;	
    delete local_src;
  } // end destructor
    
  routingentry(routingentry& n) : 
	  nattrav(n.nattrav ? n.nattrav->copy() : NULL),
	  translated_nli(n.translated_nli ? n.translated_nli->copy() : NULL),
	  is_responder(n.is_responder),
	  dmode(n.dmode),
	  secure(n.secure),
	  peer_nli(n.peer_nli ? n.peer_nli->copy() : NULL),
	  ma_reuse(false),
	  local_src(n.local_src ? n.local_src->copy() : NULL),
	  state(n.state),
	  lock(false), // NOTE: not a true copy
	  qrc(NULL),   // NOTE: not a true copy
	  timer_type(none), // NOTE: not a true copy
	  timer_type_2(none), // NOTE: not a true copy
	  timer_type_3(none), // NOTE: not a true copy
	  timer_id(0), // NOTE: not a true copy
	  timer_id_2(0), // NOTE: not a true copy
	  timer_id_3(0), // NOTE: not a true copy
	  rs_validity_time(n.rs_validity_time),
	  ma_hold_time(n.ma_hold_time),
	  in_if(n.in_if),
	  out_if(n.out_if),
	  tmppeer(NULL), // NOTE: not a true copy
	  tmppdu(NULL),  // NOTE: not a true copy
	  ghc(n.ghc),
	  ip_ttl(n.ip_ttl),
	  mod_data(n.mod_data ? n.mod_data->copy() :  NULL),
	  rtstate(APIMsg::error_ok), // NOTE: not a true copy
	  msg_id(0),  // NOTE: not a true copy
	  sp(n.sp ? n.sp->copy() :  NULL),
	  sc(n.sc ? n.sc->copy() :  NULL),
	  sii_handle(n.sii_handle),
	  retry_timeout(n.retry_timeout)
  {
    dataqueue.clear();
  } // end copy constructor


	/// print routing entry as string
	string to_string() const;

	// get and set methods
	void set_nattraversal(const nattraversal* newnattrav) { delete (nattrav); nattrav= newnattrav ? newnattrav->copy() : NULL; }
	const nattraversal* get_nattraversal() const { return nattrav; }

	void set_translated_nli(const nli* newnli) { delete (translated_nli); translated_nli= newnli ? newnli->copy() : NULL; }
	const nli* get_translated_nli() const { return translated_nli; }

	void set_is_responding_node(bool is_responding_node= true) { is_responder= is_responding_node; }
	void set_is_querying_node(bool is_querying_node= true) { is_responder= !is_querying_node; }

	/// returns whether node is responding node
	bool is_responding_node() const { return is_responder; };
	bool is_querying_node() const { return !is_responder; };

	void set_dmode(bool d_mode= true) { dmode= d_mode; }
	void set_cmode(bool c_mode= true) { dmode= !c_mode; }
	bool is_dmode() const { return dmode; }
	bool is_cmode() const { return !dmode; }

	bool is_ma_reuse_requested(const nli* nli= NULL) const { return ma_reuse; }
	void set_ma_reuse_requested(const nli* nli= NULL, bool reuse_ma= true) { ma_reuse= reuse_ma; }

	void set_secure(bool securereq= true) { secure= securereq; }
	bool is_secure() const { return secure; }

	void set_state(routingstate_t rs) { state= rs; }
	routingstate_t get_state() const { return state; }

	void set_peer_nli(const nli* newnli) { delete (peer_nli); peer_nli= newnli ? newnli->copy() :  NULL; }
	const nli* get_peer_nli() const { return peer_nli; }

	void set_querycookie(const querycookie* newqrc) { delete(qrc); qrc= newqrc ? newqrc->copy() : NULL; }
	querycookie* get_querycookie() const { return qrc; }

	void set_rs_validity_time(uint32 routing_state_validity_time) { rs_validity_time= routing_state_validity_time; }
	uint32 get_rs_validity_time() const { return rs_validity_time; }

	void set_ma_hold_time(uint32 ma_holdtime) { ma_hold_time= ma_holdtime; }
	uint32 get_ma_hold_time() const { return ma_hold_time; }
	
	void set_tmppeer(const appladdress* tmp_peer) { delete tmppeer; tmppeer= tmp_peer ? tmp_peer->copy() : NULL; }
	const appladdress* get_tmppeer() const { return tmppeer; }
        appladdress* deliver_tmppeer() { appladdress* dtmp_peer=tmppeer; tmppeer= NULL; return dtmp_peer; }

	void set_tmppdu(const known_ntlp_pdu* tmp_pdu) { delete tmppdu; tmppdu= tmp_pdu ? tmp_pdu->copy() : NULL; }
	const known_ntlp_pdu* get_tmppdu() const { return tmppdu; }
	known_ntlp_pdu* deliver_tmppdu() { known_ntlp_pdu* dtmp_pdu=tmppdu; tmppdu= NULL; return dtmp_pdu; }

	void set_gist_hop_count(uint32 newghc) { ghc= newghc; }
	uint32 get_gist_hop_count() const { return ghc; }

	void set_ip_ttl(uint32 newip_ttl) { ip_ttl= newip_ttl; }
	uint32 get_ip_ttl() const { return ip_ttl; }

	void set_sii_handle(uint32 newsii_handle) { sii_handle= newsii_handle; }
	uint32 get_sii_handle() const { return sii_handle; }


	void set_msg_id(msghandle_t new_msg_id) { msg_id= new_msg_id; }
	msghandle_t get_msg_id() const { return msg_id; }


	void set_errstate(APIMsg::error_t rs) { rtstate= rs; }
	APIMsg::error_t get_errstate() const { return rtstate; }

	void set_sp(const stackprop* newsp) { delete sp; sp= newsp ? newsp->copy() :  NULL; }
	const stackprop* get_sp() const { return sp; }

	void set_sc(const stack_conf_data* newsc) { delete sc; sc= newsc ? newsc->copy() :  NULL; }
	const stack_conf_data* get_sc() const { return sc; }
	
	// get max retry timeout
	static uint32 get_retry_limit();
	// get default initial retry timeout
	static uint32 get_retry_period();
	
	/// get current retry_timeout
	uint32 get_retry_timeout() const { return retry_timeout; }
	/// reset timeout to local default
	uint32 reset_retry_timeout() { return retry_timeout= get_retry_period(); }
	/// perform exponential backoff
	void increase_retry_timeout() { if (!retry_limit_reached()) retry_timeout= 2 * retry_timeout; }
	/// returns true if max retry timeout is reached
	bool retry_limit_reached() const { return (retry_timeout > (get_retry_limit() - retry_timeout));  }
	
	
	/// get outgoing interface index
	uint16 get_outgoing_if() const { return out_if; }
	/// set outgoing interface index
	void set_outgoing_if(uint16 out_if_index) { out_if= out_if_index; }
	
	/// get incoming interface index
	uint16 get_incoming_if() const { return in_if; }
	/// set incoming interface index
	void set_incoming_if(uint16 incoming_if_index) { in_if= incoming_if_index; }
	
	timer_type_t get_timer_type_1() const { return timer_type; }
        void set_timer_type_1(timer_type_t type) { timer_type= type; }
	timer_type_t get_timer_type_2() const { return timer_type_2; }
        void set_timer_type_2(timer_type_t type) { timer_type_2= type; }
	timer_type_t get_timer_type_3() const { return timer_type_3; }
        void set_timer_type_3(timer_type_t type) { timer_type_3= type; }

	uint64 get_timer_id_1() const { return timer_id; }
	void set_timer_id_1(uint64 id) { timer_id= id; }
	uint64 get_timer_id_2() const { return timer_id_2; }
	void set_timer_id_2(uint64 id) { timer_id_2= id; }
	uint64 get_timer_id_3() const { return timer_id_3; }
	void set_timer_id_3(uint64 id) { timer_id_3= id; }

	const char* get_state_name() const { return (state<rt_state_max) ? statestring[state] : "state variable invalid - internal error"; };
	
	void set_local_src(const hostaddress& src) {
		delete local_src;
		local_src = src.copy();
		// must use plain IPv4 if possible
		local_src->convert_to_ipv4();
	}

	const hostaddress* get_local_src() const  { return local_src; }

	void set_mod_data(const nslpdata* data) { delete mod_data; mod_data= data ? data->copy() : NULL; }
	const nslpdata* get_mod_data() const { return mod_data; }
	nslpdata* deliver_mod_data() { 	nslpdata* tmp_data= mod_data; mod_data= NULL; return tmp_data; }

	void set_lock() { lock= true; }
	void unlock() { lock= false; }
	bool is_locked() const { return lock; }

  /// enqueued DATA PAYLOAD
  vector<queueentry*> dataqueue;

private:
    
  /// saved NAT Traversal object
  nattraversal* nattrav;
  /// translated NLI (external visible NLI i/f address)
  nli* translated_nli;
  
  /// role of the node
  bool is_responder;
  /// running D-Mode or C-Mode
  bool dmode;
  /// running secure C-Mode;
  bool secure;
    
  /// NLI object is used to reference messaging association in MA_table
  /// and provides DMode Adressing (Port is Well-Known)
  nli* peer_nli;

  // MA re-use requested by peer
  bool ma_reuse;

  // The local address that was used as source to create this entry
  hostaddress *local_src;
    
  /// store different states
  routingstate_t state;

  /// lock
  bool lock;
    
  /// pointer used to store query cookie for reference
  querycookie* qrc;
    
  /// which timer is currently set (timer 1 and timer 2)
  timer_type_t timer_type;
  timer_type_t timer_type_2;
  timer_type_t timer_type_3;
    
  /// the ID of the timers 1 and 2
  uint64 timer_id;
  uint64 timer_id_2;
  uint64 timer_id_3;    
    
  /// RSV time (ms)
  uint32 rs_validity_time;
    
  /// MA Hold Time (ms)
  uint32 ma_hold_time;
    
  /// local incoming interface (on responder side) (determined and updated by incoming queries), 0 is not valid
  uint16 in_if; 
  /// local outgoing interface (only used on Querier Side, send_query() will use if it it is a repeated message, saved by first call of send_query())
  uint16 out_if;

  /// temporary fields for saving in initial state setup on responder side
  appladdress* tmppeer;
  known_ntlp_pdu* tmppdu;
    
  /// save GIST hop count for repeating Query/Response
  uint32 ghc;
    
  /// save IP_TTL for repeating Query/Response
  uint32 ip_ttl;
    
  /// save NSLP data for exchanging with original one
  nslpdata* mod_data;
    
  /// save APIMsg::error_t Routingstate which is sent to NSLP
  APIMsg::error_t rtstate;
    
  /// for saving of Message ID of Message which initiated handshake
  msghandle_t msg_id;
    
  /// for temporary saving sc and sp of Response retries
  stackprop* sp;
  stack_conf_data* sc;  

  // SII handle for downstream/upstream peer (depending on querier/responder role)
  uint32 sii_handle;

  /// Retry timeout for Message Retransmits in ms
  uint32 retry_timeout;

}; // end class routingentry
   


inline
routingentry::routingentry(bool is_responder) :
    nattrav(NULL),
    translated_nli(NULL),
    is_responder(is_responder),
    dmode(true),
    secure(false),
    peer_nli(NULL),
    ma_reuse(false),
    local_src(NULL),
    state(initial),
    lock(false),
    qrc(NULL),
    timer_type(none),
    timer_type_2(none),
    timer_type_3(none),
    timer_id(0),
    timer_id_2(0),
    timer_id_3(0),
    rs_validity_time(0),
    ma_hold_time(0),
    in_if(0),
    out_if(0),
    tmppeer(NULL),
    tmppdu(NULL),
    ghc(0),
    ip_ttl(0),
    mod_data(NULL),
    rtstate(APIMsg::error_ok),
    msg_id(0),
    sp(NULL),
    sc(NULL),
    sii_handle(0),
    retry_timeout(get_retry_limit())
{
  dataqueue.clear();
} // end constructor

}// end namespace



#endif
