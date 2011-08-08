/// ----------------------------------------*- mode: C++; -*--
/// @file routingtable.h
/// GIST Messaging Routing Table
/// ----------------------------------------------------------
/// $Id: routingtable.h 6282 2011-06-17 13:31:57Z bless $
/// $HeadURL: https://svn.tm.kit.edu/nsis/ntlp/trunk/src/routingtable.h $
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
#ifndef _NTLP__ROUTINGTABLE_H_
#define _NTLP__ROUTINGTABLE_H_
/* 
   
  Objects holding the data structure for Message routing, the big grand
  routing table and the adjacent data structures.

  Lookup is available via a key combined from MRI, sessionid and nslpid in routing table.
  
  Lookup is also available via SII-Handle, which is defined as key for the table.
  
  data is stored in hashmaps and can be reached via keys stored in an additional hashmap (one for MRI/sessionid/nslpid).
*/

#include <cerrno>
#include "hashmap"

#include "sessionid.h"
#include "mri.h"
#include "nli.h"

#include "capability.h"

#include "routingentry.h"

namespace ntlp 
{

using namespace protlib;


/// the key for the routing table index hashmap
/// comprises: MRI, SID, NSLPID
/// @note MRI and SID are pointers
struct routingkey 
{
  routingkey(mri* mr= NULL, sessionid* sid= NULL, uint32 nslpid= 0) :
    mr(mr),
    sid(sid),
    nslpid(nslpid)
  {
  }
    
  ~routingkey() 
  {
    //if (mr) delete mr;
    //if (sid) delete sid;
  }

  mri* mr;
  sessionid* sid;
  uint32 nslpid;
};
  
/// a hash function operating on the routing key
struct hash_routingkey 
{
   size_t operator()(const routingkey in)  const 
   {
     long ret = in.nslpid;
     long ret2 = in.sid->get_hash();
     return (size_t) ret^ret2;
   }
 };

/// a comparison function for routing hash map on the routing key
struct eqroutingkey
{
  bool operator()(const routingkey& s1, const routingkey& s2) const
  {
    if ((s1.sid) && (s2.sid) && (s1.mr) && (s2.mr))
      return ((s1.nslpid==s2.nslpid) && (*(s1.sid) == *(s2.sid))  && (*(s1.mr) == *(s2.mr)));
    else 
      return false;
  }
};

/// a comparison function for Message Association table key (NLI)
/// only peer_identity and if_address should be used for this
struct eqnli
{
  bool operator()(const nli& nliobj1, const nli& nliobj2) const
  {
    return (nliobj1.get_pi() != NULL && nliobj2.get_pi() != NULL) ? 
	    ((*nliobj1.get_pi() == *nliobj2.get_pi()) && nliobj1.get_if_address() == nliobj2.get_if_address()) : 
            (nliobj1.get_if_address() == nliobj2.get_if_address());
  }
};

/// a hash function operating on the routing key
struct hash_nli {
  size_t operator()(const nli nliobj)  const {
    const peer_identity* pi_p= nliobj.get_pi();
    return pi_p!=0 ? pi_p->get_hash() ^ nliobj.get_if_address().get_hash() : nliobj.get_if_address().get_hash();
  }
};


typedef enum ma_state_enum {
  ma_state_birth= 0,
  ma_state_awaiting_connection = 1,
  ma_state_connected = 2,
  ma_state_idle = 3,
  ma_state_death= 4,
  ma_state_max= 5
} ma_state_t;


const char* const ma_statestring[] = {
  "MA_BIRTH",
  "MA_AWAITING_CONNECTION",
  "MA_CONNECTED",
  "MA_IDLE",
  "MA_DEAD"
};
      
  
/// an entry for the message association hashmap
class ma_entry 
{
private:

  /// appladdress holding all necessary data for TP Modules
  appladdress peer;
  
  /// messaging association state
  ma_state_t state;
  
  /// hold time of peer (ms)
  uint32 peer_ma_hold_time;
  
  /// is this a secure MA
  bool secure_ma;

  /// is this a reliable MA? (DCCP is not)
  bool reliable_ma;

  /// Am I a responding node? This is important to know because then we cannot re-establish an MA
  bool responding_node;

  /// HelloID
  uint32 acc_hid;  

public:
  /// timer slot 1
  uint32 timer_id;
  timer_type_t timer_type;
  
  /// timer slot 2
  uint32 timer_id_2;
  timer_type_t timer_type_2;


  ma_entry() :
    state(ma_state_birth),
    peer_ma_hold_time(ma_hold_time_default),
    secure_ma(false),
    reliable_ma(true),
    responding_node(false),
    acc_hid(0)
  {
  }

  const char* get_state_name() const { return (state<ma_state_max) ? ma_statestring[state] : "state variable invalid - internal error"; }
  
  // sets peer address and sets transfer attributes appropriately
  void set_peer_address(const appladdress& peeraddr) { peer= peeraddr; peer.convert_to_ipv6(); set_transfer_attributes(); };

  const appladdress& get_peer_address() const { return peer;  };

  // sets peer MA hold time
  void set_ma_hold_time( uint32 ma_hold_time );
  // get MA hold time of remote peer
  uint32 get_ma_hold_time() const { return peer_ma_hold_time; };

  void set_transfer_attributes();

  void set_responder() { responding_node= true; }
  void set_querier() { responding_node= false; }

  bool is_responder() const { return responding_node; }
  bool is_querier() const { return responding_node; }

  ma_state_t get_state() const { return state; }
  // sets new state and returns previous state
  ma_state_t set_state( ma_state_t newstate ) { ma_state_t oldstate= state; state= newstate; return oldstate; }
  
  bool is_reliable() const { return reliable_ma; };
  bool is_secure() const { return secure_ma; };

  uint32 get_new_hello_id();
  uint32 get_hello_id() const { return acc_hid; };

};
  

/**
 * This class extends the TimerMsg from protlib with a routing key.
 */
class RoutingTableTimerMsg : public TimerMsg {
  public:
	/**
	 * Constructor.
	 */
	RoutingTableTimerMsg(const routingkey &r_key, timer_type_t timer_type,
		uint32 sec, uint32 msec)
		: TimerMsg(message::qaddr_coordination, true),
		  timer_type(timer_type) {

		routing_key.mr = (r_key.mr)->copy();
		routing_key.sid = (r_key.sid)->copy();
		routing_key.nslpid = r_key.nslpid;
		
		// This has to be allocated dynamically because the
		// statemodule::process_timer_msg() method will delete it.
		uint32 *t = new uint32();
		*t = timer_type;
		start_relative(sec, msec, t, (void *) &routing_key);
	}

	const routingkey *get_routing_key() const { return &routing_key; }

  private:
	routingkey routing_key;
	timer_type_t timer_type;
};



    
/// the object encapsulating the routing data structures, provides lookup methods and storage
class routingtable 
{
 public:
  typedef hashmap_t<routingkey, routingentry*, hash_routingkey, eqroutingkey> rkey_to_rentry_hashmap_t;
  typedef hashmap_t<nli, ma_entry, hash_nli, eqnli> ma_hashmap_t;
  typedef hashmap_t<nli, uint32, hash_nli, eqnli> peer_to_sii_hashmap_t;
  typedef hashmap_t<uint32, nli> sii_to_peer_hashmap_t;
  typedef hashmap_t<uint32, uint32> rao_to_nslpid_hashmap_t;

  /// peersii_iter
  typedef peer_to_sii_hashmap_t::iterator peersii_iter;
  typedef peer_to_sii_hashmap_t::const_iterator peersii_citer;
  
  /// ma_iter
  typedef ma_hashmap_t::iterator ma_iter;

  /// ma_citer
  typedef ma_hashmap_t::const_iterator ma_citer;
  
  /// rao_iter
  typedef rao_to_nslpid_hashmap_t::iterator rao_iter;

  /// rao_citer
  typedef rao_to_nslpid_hashmap_t::const_iterator rao_citer;

  /// rt_iter
  typedef rkey_to_rentry_hashmap_t::iterator rt_iter;
  /// rt_citer
  typedef rkey_to_rentry_hashmap_t::const_iterator rt_citer;
  
  /// sii_iter
  typedef sii_to_peer_hashmap_t::iterator siipeer_iter;
  /// sii_citer
  typedef sii_to_peer_hashmap_t::const_iterator siipeer_citer;
       
  routingtable(); 
      
  ~routingtable();
      
  /// adds routing information
  void add(const routingkey* key, routingentry* entry);
      
  /// lookup entry with key (and "lock")
  routingentry* lookup(const routingkey* key);
      
  /// check whether entry for key exists
  bool exists(const routingkey* key);
      
  ///  unlock entry
  void unlock(const routingkey* key);
      
  /// update and unlock
  void update_unlock(routingkey* key, routingentry* entry);
      
  /// delete entry, still accessible via sii-handle
  bool delete_entry(const routingkey* key, bool table_already_locked= false);
  /// delete entry completely
  bool destroy_entry(const routingkey* key);
      
  void invalidate_routing_state(const mri*, const uint32 nslpid, const APIMsg::status_t, const bool urgency);
      
  void set_state_lifetime(const mri*, const uint32 nslpid, const uint32 lifetime);
      
  /// prints some status information (rough number statistics) about the routing table and MA table
  void status() const; 

  /// prints some status information about the routing table
  string print_routingtable() const; 
      
  /// get SII-Handle by nli information
  uint32 get_sii_handle(const nli* peer) const;

  /// get SII-Handle by routing entry information
  uint32 get_sii_handle(const routingkey* key);

  /// get or create SII-Handle by nli information
  uint32 generate_sii_handle(const nli* peer);
      
  /// adds MA to table
  void add_ma(nli* key, const appladdress& addr, uint32 peer_ma_hold_time, bool responding_node);
      
  // returns copy of MA address (no const method because of locking)
  appladdress* lookup_ma_peeraddress(const nli* key);

  /// returns MA entry (no const method because of locking)
  const ma_entry*  lookup_ma(const nli* key);

  /// runs MA timers
  void process_timer_msg(TimerMsg* timermsg);
      
  /// gets indication of a received MA Hellp
  void hello_ind_ma(const appladdress* peer, bool rflag, uint32 hid);
      
  /// Indicates activity for a MA ->resets timers, called on GIST message receipt and on send_data()
  void activity_ind_ma(const nli* key);
      
  /// set MA hold time (in ms)
  void set_own_ma_hold_time(uint32 val) { own_ma_hold_time = val; }

  /// check if a MA is correct for a Routing Key
  bool checkMA(const routingentry* r_entry, const appladdress* peer, const capability& cap);
      
  /// find the nli for a peer
  const nli* findNLI(const appladdress* peer);

  /// find the nli for a peer by using the SII handle
  const nli* findNLI(uint32 sii_handle);
      
  /// sends HELLO Messages
  void sendhello(appladdress* addr, bool request_reply, uint32 hid);
      
  /// adds RAO to table
  inline void add_rao(uint32 nslpid, uint32 rao) 
  { 
    rao_table[nslpid] = rao; 
  }
      
  /// returns RAO for NSLPID
  uint32 get_rao(uint32 nslpid) { return rao_table[nslpid]; }
      
 private:
  const char* const classname;

  void locktable();
  void unlocktable();

  /// delete entry, still accessible via sii-handle
  void remove_entry(rt_iter &it);

  void inc_sii_handle() { sii_counter++;
    // do not use 0 as valid handle
    if (sii_counter==0) sii_counter++;
  }

  // mutex for lock
  pthread_mutexattr_t mutex_attr;
  pthread_mutex_t mutex;
       
  /// MA hold time (my own MA hold time)
  uint32 own_ma_hold_time;
      
  /// SII-Handle counter to generate unique SII-Handles, 0 is not a valid value
  // should better be uint64 to allow better robustness against rapid turnover
  uint32 sii_counter;

  /// this is the routing table
  rkey_to_rentry_hashmap_t rtable;
      
  /// hash map for key lookup from peerid,if (routingkey) to SII handle
  peer_to_sii_hashmap_t peer_to_sii_table;
      
  /// hashmap for mapping SII-Handle -> routingentry
  sii_to_peer_hashmap_t sii_to_peer_table;

  /// hash map for message associations: NLI -> MA
  ma_hashmap_t ma_table;
      
  /// hash map for lookup RAO -> NSLPID
  rao_to_nslpid_hashmap_t rao_table;
            
}; // end class routingtable



inline
void 
ntlp::routingtable::locktable()
{
  if (pthread_mutex_lock(&mutex))
    ERRCLog("routingtable::locktable()", "Mutex Lock failed. Error: " << strerror(errno));
}

inline
void 
ntlp::routingtable::unlocktable()
{
  if (pthread_mutex_unlock(&mutex))
    ERRCLog("routingtable::unlocktable()", "Mutex Unlock failed. Error: " << strerror(errno));
}

    
}// end namespace



#endif
