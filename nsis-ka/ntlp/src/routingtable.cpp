/// ----------------------------------------*- mode: C++; -*--
/// @file routingtable.cpp
/// GIST Routing Table 
/// ----------------------------------------------------------
/// $Id: routingtable.cpp 6282 2011-06-17 13:31:57Z bless $
/// $HeadURL: https://svn.tm.kit.edu/nsis/ntlp/trunk/src/routingtable.cpp $
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
#include "routingtable.h"
#include "hello.h"
#include "ntlp_starter.h" // only for getting a parameter setting using global_ntlpstarterthread_p->get_param()
#include "ntlp_statemodule.h"
#include "sstream"

#include "gist_conf.h"

using namespace ntlp;
using namespace protlib;
using namespace protlib::log;

void
ma_entry::set_ma_hold_time(uint32 ma_hold_time)
{ 
  if (ma_hold_time != 0)
  {
    peer_ma_hold_time= ma_hold_time; 
  }
  else
  {
    ERRLog("ma_entry::set_ma_hold_time","Trying to set MA hold time to 0, ignoring.");
  }
}


void 
ma_entry::set_transfer_attributes()
{
  switch (peer.get_protocol())
  {
    // reliable and secure
    case protlib::prot_tls_tcp: 
      secure_ma= true; 
      reliable_ma= true;
      break;
    // reliable but not secure    
    case protlib::prot_tcp:
    case protlib::prot_sctp:
      reliable_ma= true;
      secure_ma=false;
      break;
    default:
      secure_ma= false;
      reliable_ma= false;
      break;
  }
}


uint32
ma_entry::get_new_hello_id()
{
  if (acc_hid == 0)
    acc_hid = (uint32)((double(random()) / RAND_MAX) * 0xffffffff);

  acc_hid++;

  while (acc_hid == 0)
    acc_hid = (uint32)((double(random()) / RAND_MAX) * 0xffffffff);

  return acc_hid;
}

/// give the routing table an initial size
routingtable::routingtable() :
  classname("GIST Routing"), sii_counter(0)
{
  // init mutex
  pthread_mutexattr_init(&mutex_attr);
  pthread_mutexattr_settype(&mutex_attr,PTHREAD_MUTEX_ERRORCHECK);
  pthread_mutex_init(&mutex,&mutex_attr);

#ifndef USE_UNORDERED_MAP
  peer_to_sii_table.resize(128);
  sii_to_peer_table.resize(128);
  rtable.resize(128);
  ma_table.resize(128);
  rao_table.resize(128);
#else
  peer_to_sii_table.rehash(128);
  sii_to_peer_table.rehash(128);
  rtable.rehash(128);
  ma_table.rehash(128);
  rao_table.rehash(128);
#endif
}
  
/// destruct
routingtable::~routingtable()
{
  sii_to_peer_table.clear();
  peer_to_sii_table.clear();
  rtable.clear();
  ma_table.clear();
  rao_table.clear();
  // destroy mutex
  pthread_mutexattr_destroy(&mutex_attr);
  pthread_mutex_destroy(&mutex);
}


/// we add routing data, and assure it is unlocked
void 
routingtable::add(const routingkey* key, routingentry* entry) 
{
  locktable(); //   >=>=>  LOCK  >=>=>

  entry->unlock();
  if (key)
  {
    // should add a key and an entry
    rtable[*key]= entry;
  }
  else
    ERRLog("routingtable::add()","key is not available (NULL)");

  unlocktable(); // <=<=< UNLOCK <=<=<
}
  
/** add MA
 * this adds a messaging association to the routing table at the corresponding key
 * or updates an existing one.
 * @param nlikey - pointer to NLI key
 * @param addr - a transport address for the MA
 * @param peer_ma_hold_time - the MA hold time for this peer
 * @param responding_node - true if this MA was initiated on the remote side
 */
void 
routingtable::add_ma(nli* nlikey, const appladdress& addr, uint32 peer_ma_hold_time, bool responding_node) 
{
  locktable(); //   >=>=>  LOCK  >=>=>

  ma_entry ma;
  
  const ma_entry* existing_ma= NULL;

  // check for an existing entry (e.g. we do not update the hold_time to smaller values)
  ma_iter ma_it= ma_table.find(*nlikey);
  if (ma_it != ma_table.end())
  { // existing entry found, so update it
    existing_ma= &(ma_it->second);
  }

  // override smaller peer_ma_hold_time by already longer one
  if ( existing_ma && (existing_ma->get_ma_hold_time() >= peer_ma_hold_time) )
  { 
    peer_ma_hold_time= existing_ma->get_ma_hold_time();
  }
  
  // we save MA addresses in IPv6 form only
  ma.set_peer_address(addr);
  if (peer_ma_hold_time != 0)
  {
    ma.set_ma_hold_time(peer_ma_hold_time);
  }
  else
  {
    ERRCLog(classname, "Tried to set MA hold time to 0, this should not happen. Override using own value:" << own_ma_hold_time);
    ma.set_ma_hold_time(own_ma_hold_time);
  }

  ma.set_state(ma_state_connected);
  if (responding_node) 
    ma.set_responder();
  else
    ma.set_querier();

  // Start timers, let them be sent to coordination, give them an NLI as SECOND object to identify

  // start SendHello timer on timer slot #1, use randomized interval around (retryfactor * Peer_MA_Hold_Time (milliseconds!))
		    
  TimerMsg* msg = new TimerMsg(message::qaddr_coordination,true);
      
  // start timer, type SendHello
     
  // change timer_type
  ma.timer_type = send_hello;
  uint32* timer_type = new uint32;
  *timer_type = ma.timer_type;
  
  int timerint= Statemodule::randomized(peer_ma_hold_time, gconf.getpar<float>(gistconf_retryfactor));
  msg->start_relative(0, timerint, (void*) timer_type, (void*)nlikey->copy());
  ma.timer_id = msg->get_id();
  
  DLog(classname, color[yellow] << "SendHello Timer started, set to: " << timerint << " ms" << color[off]);
  
  // send timer msg to Timer Module
  msg->send_to(message::qaddr_timer);
  
  // start timer, type NoActivity, with OWN MA_HOLD_TIME in sec
  
  msg = new TimerMsg(message::qaddr_coordination,true);
  
  // change timer_type_2
  ma.timer_type_2 = no_activity;
  timer_type = new uint32;
  *timer_type = ma.timer_type_2;
  msg->start_relative(0, own_ma_hold_time, (void*) timer_type, (void*)nlikey->copy());
  ma.timer_id_2 = msg->get_id();
  
  DLog(classname, color[yellow] << "NoActivity Timer started, set to: " << own_ma_hold_time << " ms" << color[off]);
  
  // send timer msg to Timer Module
  msg->send_to(message::qaddr_timer);

  // add to MA table (we must be given a copy of the key!) or update existing entry
  if (ma_it != ma_table.end())
  { // existing entry found, so update it
    ma_it->second= ma;
  }
  else
  { // new entry to be added
    ma_table.insert( make_pair(*nlikey,ma) );
  }

  unlocktable(); // <=<=< UNLOCK <=<=<
}

/// lookup a messaging association  
/// @note this function locks the table during lookup
const ma_entry* 
routingtable::lookup_ma(const nli* nlikey)
{
  locktable();   // >=>=>  LOCK  >=>=>

  ma_entry* ma_p = NULL;
  bool found= false;
  ma_iter cur = ma_table.find(*nlikey);
  if (cur != ma_table.end())
  {
    ma_p=&((*cur).second);
    if (ma_p)
    {
      found= true;
      DLog(classname, "Found MA for given NLI, peer address:" << ma_p->get_peer_address() << " state:" << ma_p->get_state_name());
      // check state of MA
      if (ma_p->get_state() != ma_state_connected && ma_p->get_state() != ma_state_idle)
      {
	WLog(classname, "MA not in a usable state:" << ma_p->get_state_name());
	ma_p= NULL;
      }
    } // if pointer not NULL
  }
  
  unlocktable(); // <=<=< UNLOCK <=<=<

  if (ma_p) 
    return ma_p;
  else
  {
	  if (nlikey->get_pi())
		  DLog(classname, "MA " << (!found ? "not found" : "not usable") 
		       << "for NLI " << *(nlikey->get_pi()) << ", IP:" << nlikey->get_if_address().get_ip_str() );
	  else
		  DLog(classname, "MA " << (!found ? "not found" : "not usable") 
		       << " for NLI <NO PeerID!>, IP:" << nlikey->get_if_address().get_ip_str() );


	  return NULL;
  }
}


/// return an address for routing 
/// @note a new fresh copied object is returned in order to avoid race conditions if MA gets deleted somehow
appladdress* 
routingtable::lookup_ma_peeraddress(const nli* nlikey)
{
  const ma_entry* ma_p = lookup_ma(nlikey);
  return (ma_p!=NULL) ? ma_p->get_peer_address().copy() : NULL;
}

// reset NoActivity Timer on Message Associations
void 
routingtable::activity_ind_ma(const nli* nlikey) 
{
  locktable();   // >=>=>  LOCK  >=>=>

  // Lookup MA

  ma_entry* ma_p = NULL;
  ma_iter cur = ma_table.find(*nlikey);
  
  if (cur != ma_table.end())
  {
    ma_p=&((*cur).second);    
  }

  unlocktable(); // <=<=< UNLOCK <=<=<

  // Maintain if one is found
  if (ma_p) 
  {
    if (ma_p->get_state() == ma_state_connected)
      DLog(classname, "MA activity detected: Resetting MA activity timer, staying in " << ma_p->get_state_name());
    else
    if (ma_p->get_state() == ma_state_idle)
    {
      // go back to connected
      ma_p->set_state(ma_state_connected);    
      DLog(classname, "MA activity detected: MA state going back to " << ma_p->get_state_name());
      // stop NoHello Timer
      if (ma_p->timer_type_2 == no_hello)
      {
	DLog(classname, "Stopping No Hello timer, mid "<<ma_p->timer_id_2);
	TimerMsg* msg = new TimerMsg(message::qaddr_coordination,true);

	// change timer_type_2
	ma_p->timer_type_2 = none;
	msg->stop(ma_p->timer_id_2);
	ma_p->timer_id_2=msg->get_id();
      }
    }
    
    // Restart NoActivity timer on MA and go to ma_connected
    
    // start timer, type NoActivity
    TimerMsg* msg = new TimerMsg(message::qaddr_coordination,true);

    // change timer_type_2
    ma_p->timer_type_2 = no_activity;
    msg->restart_relative(ma_p->timer_id_2, 0, own_ma_hold_time);
    ma_p->timer_id_2=msg->get_id();
    
    DLog(classname, color[yellow] << "NoActivity Timer restarted, set to: " << own_ma_hold_time << " ms, mid " << msg->get_id() << color[off]);
    
    // send timer msg to Timer Module
    msg->send_to(message::qaddr_timer);

    ma_p->set_state(ma_state_connected);    
  } 
  else 
  {
    DLog(classname, "There is no active Message Association to maintain");  
  }
}



uint32 
routingtable::get_sii_handle(const routingkey* key)
{
  uint32 sii_handle= 0;

  locktable();   // >=>=>  LOCK  >=>=>
  rt_citer cur = rtable.find(*key);
  if (cur != rtable.end()) 
  {
    const routingentry* rtentry= cur->second;
    if (rtentry)
	    sii_handle= rtentry->get_sii_handle();
  }
  unlocktable(); // <=<=< UNLOCK <=<=<

  return sii_handle;
}


/// lookup SII handle that corresponds to given nli
/// @return found SII handle or 0 if no SII handle found
uint32 
routingtable::get_sii_handle(const nli* nli) const
{
  if (nli)
  {
    peersii_citer psiiciter= peer_to_sii_table.find(*nli);
    if (psiiciter != peer_to_sii_table.end())
    {
      return psiiciter->second;
    }
  }
  
  return 0;
}


/// returns either already existing SII handle or creates a new SII handle
uint32 
routingtable::generate_sii_handle(const nli* nli)
{
  uint32 return_sii_handle= get_sii_handle(nli);
  
  // no handle found, create a new one
  if (return_sii_handle == 0)
  {
    locktable();   // >=>=>  LOCK  >=>=>
    // get a new value for an SII handle
    inc_sii_handle();
    peer_to_sii_table.insert(make_pair(*nli,sii_counter));
    sii_to_peer_table.insert(make_pair(sii_counter,*nli));
    return_sii_handle= sii_counter;

    unlocktable(); // <=<=< UNLOCK <=<=<
  }
   
  return return_sii_handle;
}


// we return true if the entry exists
bool 
routingtable::exists(const routingkey* key) 
{
  locktable();   // >=>=>  LOCK  >=>=>

  rt_citer cur  = rtable.find(*key);
  bool result= (cur != rtable.end());

  unlocktable(); // <=<=< UNLOCK <=<=<

  return result;
}
    

/// we return a pointer to the entry and "lock" the entry
/// @note DO NOT LOCK! Attention! Your pointer may be invalidated due to multithreading
routingentry* 
routingtable::lookup(const routingkey* key) 
{
  routingentry* entry = NULL;      
 
  locktable();   // >=>=>  LOCK  >=>=>
      
  rt_iter cur  = rtable.find(*key);
      
  if (cur != rtable.end()) 
  { 
    entry = (*cur).second;
	  
    if (entry->is_locked()) {
      ERRCLog(classname, "Tried to access LOCKED ENTRY!");
      unlocktable(); // <=<=< UNLOCK <=<=<
      return NULL; 
    }
    entry->set_lock();
      
    DLog(classname, color[yellow] << "******* Returning entry for Session-ID "<< ((key->sid) ? key->sid->to_string() : "NULL") << " with state " << color[red] << statestring[entry->get_state()] << color[off]);
      
  }
  else 
  {
    DLog(classname, "Lookup not successful");
  }
  unlocktable(); // <=<=< UNLOCK <=<=<

  return entry;   
}    
    

// we unlock a lock on routing data
void 
routingtable::unlock(const routingkey* key) 
{
  locktable();   // >=>=>  LOCK  >=>=>

  rt_iter rt_it= rtable.find(*key);
      
  if (rt_it != rtable.end()) 
  { 
    routingentry* tmp = rt_it->second;
    if (tmp) 
    {
	    DLog(classname, color[yellow] << "******* Unlocking Session-ID "<< ((key->sid) ? key->sid->to_string() : "NULL") << " with state " << color[red] << statestring[tmp->get_state()] << color[off]);
	    tmp->unlock();
    }
    else
      ERRCLog(classname,"while unlocking(): routing entry for Session-ID "<< ((key->sid) ? key->sid->to_string() : "NULL") << " exists, but routing entry is NULL");
  }
  unlocktable(); // <=<=< UNLOCK <=<=<
}
 

// we update routing data and unlock
void 
routingtable::update_unlock(routingkey* key, routingentry* entry) 
{
  locktable();   // >=>=>  LOCK  >=>=>

  entry->unlock();
  // lookup
  rt_iter rt_it= rtable.find(*key);
  if (rt_it != rtable.end())
    rt_it->second= entry;

  unlocktable(); // <=<=< UNLOCK <=<=<
}
  
/**
 * we invalidate routing data, return true if succesfully done
 * key is deleted from rkey_to_sii_table, but entry is still accessible via sii-handle
 * @param key                      routing entry to be deleted
 * @param table_already_locked     table will not be locked because already locked
 **/
bool 
routingtable::delete_entry(const routingkey* key, bool table_already_locked)
{
  if (!table_already_locked) 
    locktable();   // >=>=>  LOCK  >=>=>

  rt_iter rt_it= rtable.find(*key);

  routingentry* rt_entry= 0;
  if (rt_it != rtable.end()) 
  {
    rt_entry= rt_it->second;
  }
  else
  {
    ERRCLog(classname,"Trying to delete a non-existing entry (Session-ID "<< ((key->sid) ? key->sid->to_string() : "NULL") << ")!");
    if (!table_already_locked) 
      unlocktable(); // <=<=< UNLOCK <=<=<
    
    return false;
  }
  
  if (!rt_entry->is_locked()) {
    ERRCLog(classname,"You should ACQUIRE A LOCK PRIOR TO DELETING an entry!");
    if (!table_already_locked) 
      unlocktable(); // <=<=< UNLOCK <=<=<
    
    return false;
  }

  remove_entry(rt_it);

  unlocktable(); // <=<=< UNLOCK <=<=<
  
  return true;
}

void
routingtable::remove_entry(rt_iter &it)
{
  routingentry* rt_entry;
  if (it == rtable.end()) 
    return;

  rt_entry= it->second;

  // mark entry dead
  rt_entry->set_state(dead);
  rt_entry->unlock();    
  // switch to D-Mode, unsecure
  rt_entry->set_dmode();
  rt_entry->set_secure(false);

  const routingkey* key = &it->first;
  DLog(classname, "Deleted Routing Key w/ Session-ID "<< ((key->sid) ? key->sid->to_string() : "NULL") );

  // delete entry in routing table, note that this will probably make the iterator invalid
  rtable.erase(it);
  status();
}


// we completely delete routing data (key in map and entry), return true if OK
// 
bool routingtable::destroy_entry(const routingkey* key) 
{
  locktable();   // >=>=>  LOCK  >=>=>

  rt_iter rt_it= rtable.find(*key);
  routingentry* rt_entry= 0;
  if (rt_it != rtable.end()) 
  {
    rt_entry= rt_it->second;
  }
  else
  {
    ERRCLog(classname,"Trying to delete a non-existing entry (Session-ID "<< ((key->sid) ? key->sid->to_string() : "NULL") << ")!");
    unlocktable(); // <=<=< UNLOCK <=<=<
    
    return false;
  }
  
  if (!rt_entry->is_locked()) {
    ERRCLog(classname,"You should ACQUIRE A LOCK PRIOR TO DELETING an entry!");
    unlocktable(); // <=<=< UNLOCK <=<=<
    
    return false;
  }

  remove_entry(rt_it);

  delete key;
  DLog(classname, "Destroyed Routing Key, key is unusable");
  
  unlocktable(); // <=<=< UNLOCK <=<=<
  
  return true;
}

  
  
// dump some status about routing states to log file
void 
routingtable::status() const
{
  // routing holds size()-1 valid entries! (first is NULL)
  DLog(classname, color[red] << "Active Routing Entries: " << rtable.size() << " MAs: " << ma_table.size() << " SII handles: " << peer_to_sii_table.size() <<color[off]);
}

// dump some status about routing states to log file
string
routingtable::print_routingtable() const
{
  // routing holds size()-1 valid entries! (first is NULL)
  ostringstream os;

  const routingentry* entry= NULL;
  
  rt_citer entryit= rtable.begin();
  while(entryit !=rtable.end())
  {
    if ( (entry= (*entryit).second) != NULL )
    {

	    const routingkey& rkey= (*entryit).first;
	    os << "sid:" << (rkey.sid ? rkey.sid->to_string() : "-") 
	       << " NSLPID:" << rkey.nslpid << endl << " MRI/Flow:" << (rkey.mr ? rkey.mr->to_string() : "NULL") << endl;

	    os << entry->to_string() << endl << "---------------------------------------------" << endl;
    }
    entryit++;
  } // end while
 
  return os.str();  
}


// process timer messages
void 
routingtable::process_timer_msg(TimerMsg* timermsg) 
{
  DLog(classname, color[yellow] << "Timer Handler called for timer mid " << timermsg->get_id() << color[off]);

  // we HAVE a key object != NULL, take it
  nli* key = static_cast<nli*>(timermsg->get_param2());
  if (key == NULL) // if nli as key not present we may have an unexpected timer event here, log error!
  {
    ERRCLog(classname, "Internal error - invalid timer event: carried no NLI parameter, timer type:" << (timermsg->get_param1() ? *static_cast<uint32*>(timermsg->get_param1()) : 0));
    // bail out since we cannot continue
    return;
  }

  ma_entry* ma_p = NULL;
  
  locktable();   // >=>=>  LOCK  >=>=>

  ma_iter cur = ma_table.find(*key);
  if (cur != ma_table.end()){
    ma_p=&((*cur).second);    
  }

  unlocktable(); // <=<=< UNLOCK <=<=<

  if (ma_p) 
  {
    if (ma_p->timer_id == timermsg->get_id()) 
    {
      DLog(classname, "Timer slot 1, timer is valid, id: " << timermsg->get_id());
      
      // Timers on Slot #1:
      //
      // SendHello
      //
      //
      switch (ma_p->timer_type) 
      {
	
	case send_hello: 
	{
	  // send an MA_HELLO
	  if (gconf.getpar<bool>(gistconf_reqhelloecho)) {
		sendhello(ma_p->get_peer_address().copy(), true, ma_p->get_new_hello_id());
	  } else
	  	sendhello(ma_p->get_peer_address().copy(), false, 0);

	  // restart SendHello timer on timer slot #1, use 0.75 * MA_Hold_Time (milliseconds!)
	  TimerMsg* msg = new TimerMsg(message::qaddr_coordination,true);
	  
	  // start timer
	  ma_p->timer_type= send_hello;
	  
	  uint32* timer_type= new uint32;
	  *timer_type = ma_p->timer_type;
	  
	  int timerint= Statemodule::randomized(ma_p->get_ma_hold_time(), gconf.getpar<float>(gistconf_retryfactor));
	  msg->start_relative(0, timerint,(void*) timer_type, (void*)key->copy());
	  // change timer_type
	  ma_p->timer_id= msg->get_id();
	  
	  DLog(classname, color[yellow] << "SendHello Timer started, set to: " << timerint << " ms" << color[off]);
	  
	  // send timer msg to Timer Module
	  msg->send_to(message::qaddr_timer);
	  
	  break;
	}

	default: 
	{
	  if (ma_p->timer_type==none)
	    DLog(classname, "Ignoring timer that was STOPPED meanwhile: Stored type: " << ma_p->timer_type << " type in msg: " << *static_cast<uint32*>(timermsg->get_param1()));
	  else
	    ERRCLog(classname, "Unhandled Timer Type: " << ma_p->timer_type << " type in msg: " << *static_cast<uint32*>(timermsg->get_param1()));
	}
      }
    } 
    else
    {
      if (ma_p->timer_id_2 == timermsg->get_id())
      {
	DLog(classname, "Timer slot 2, timer is valid, id: " << timermsg->get_id());
	switch (ma_p->timer_type_2) 
	{
	  case no_activity: 
	  {
	    // stop SendHello
	    ma_p->timer_type= none;
	    
	    // start NoHello
	    ma_p->timer_type_2= no_hello;

	    // set state to MA_IDLE
	    ma_p->set_state(ma_state_idle);
	    DLog(classname, color[yellow] << "NoActivity timer went off, transitioning to " << ma_p->get_state_name() << color[off]);

	    TimerMsg* msg = new TimerMsg(message::qaddr_coordination,true);
	    
	    // start timer	    
	    uint32* timer_type= new uint32;
	    *timer_type = ma_p->timer_type_2;
	    
	    msg->start_relative(0, ma_p->get_ma_hold_time() * 2,(void*) timer_type, (void*)key->copy());
	    // change timer_type
	    ma_p->timer_id_2=msg->get_id();
	    
	    DLog(classname, color[yellow] << "NoHello Timer started, set to: " << ma_p->get_ma_hold_time() * 2 << " ms" << color[off]);
	    
	    // send timer msg to Timer Module
	    msg->send_to(message::qaddr_timer);
	    
	    break;
	  }
	  
	  case no_hello: 
	  { 
	    if (ma_p->get_state() != ma_state_idle)
	    {
	      ERRLog(classname, color[red] << "NoHello timer went off - but MA still in state " << ma_p->get_state_name() << " and not in MA_IDLE - this is an internal error, will ignore timer event" << color[off]);
	      return;
	    }

	    Log(EVENT_LOG, LOG_NORMAL, classname, color[yellow] << "NoHello timer went off - MA destroyed" << color[off]);
	    ma_p->set_state(ma_state_death);
	    // Now send a SigTrm to Signaling to tear down TPoverTCP (or whatever else)
	    SignalingMsgNTLP* sigmsg = new SignalingMsgNTLP;
	    sigmsg->set_trm(ma_p->get_peer_address().copy());	    
	    sigmsg->send_or_delete();
	    ma_table.erase(*key);
	    break;
	  }

	  default: 
	  {
	    Log(EVENT_LOG, LOG_NORMAL, classname, "Deliberately stopped timer: Stored type: " << ma_p->timer_type_2 << " type in msg: " << *static_cast<uint32*>(timermsg->get_param1()));
	  }
	}
      }
      else 
      {
	DLog(classname, "Obsolete timer received");
      }
    }
  } else 
  {
    DLog(classname, "Timer event received for already inexistant MA entry, ignoring.");
  }
}
//end of process_timer_messages
    

/** we must find the matching entry of the MA table and refresh its state
 * or indicate an error
 **/
void
routingtable::hello_ind_ma(const appladdress* peer, bool r_flag, uint32 hid)
{
  if (peer) 
  {
    appladdress* mypeer=peer->copy();
    
    mypeer->convert_to_ipv6();
    
    ma_entry* entry = NULL;
    
    DLog(classname, "Got a Notification about incoming Hello, must iterate MA table to find it's entry");

    locktable();   // >=>=>  LOCK  >=>=>
    
    ma_iter myiter=ma_table.begin();
    while ((!entry) && (myiter!=ma_table.end())) 
    {
      entry = &((*myiter).second);
      if (entry) 
      {
	if (entry->get_peer_address() != *mypeer) entry = NULL;
      }
      myiter++;
    } // end while
    
    unlocktable(); // <=<=< UNLOCK <=<=<

    if (entry) 
    {
      // if R flag is not set and it contains a hello ID, it's a reply to our request
      if (r_flag==false)
      { 
	      if (hid != 0 && entry->get_hello_id() != hid) {
		      WLog(classname,color[red] << "Mismatching HelloID of incoming Hello. Waited for: "<< entry->get_hello_id() << ", got: 0x" << hex << hid << dec << color[off]);
	      }
      }
      else
      {
	      DLog(classname, "Other side requests a reply to this hello with ID 0x" << hex << hid << dec);
      }

      DLog(classname, color[yellow] << "Found the MA table entry for this peer" << color[off]);
      if (entry->get_state() == ma_state_connected) 
      {
	DLog(classname, color[yellow] << "MA state still CONNECTED, no need to reset NoHello timer" << color[off]);
      } 
      else 
      {
	DLog(classname, color[yellow] << "MA in state IDLE, resetting NoHello timer" << color[off]);

	// restart NoHello
	entry->timer_type_2= no_hello;
		
	// remain in state ma_idle
	entry->set_state(ma_state_idle);
		
	TimerMsg* msg = new TimerMsg(message::qaddr_coordination,true);
	
	// start timer
	uint32* timer_type= new uint32;
	*timer_type = entry->timer_type_2;
	
	msg->restart_relative(entry->timer_id_2, 0, entry->get_ma_hold_time() * 2);
	
	DLog(classname, color[yellow] << "NoHello Timer started, set to: " << entry->get_ma_hold_time() * 2 << " ms" << color[off]);
	
	// send timer msg to Timer Module
	msg->send_to(message::qaddr_timer);
      }
    }
  } // endif peer
  else 
    ERRCLog(classname, "hello_ind_ma() called with NULL as peer address");
}


/**
 * Invalidate a routing state entry
 * @param mymr   MRI to invalidate
 * @param nslpid NSLP id
 * @param status - APIMsg::bad = Routing state must be set to 'BAD', which means killing state; APIMsg::tentative= GIST will notify on change
 * @param urgency - if true immediate re-installation is requested, starting new handshake
 */
void 
routingtable::invalidate_routing_state(const mri* mymr, const uint32 nslpid, const APIMsg::status_t status, const bool urgency) 
{

  DLog(classname, color[red] << "Iterating over Routing Key table to find the entries we must apply InvalidateRoutingState" << color[off]);

  if (!mymr) {
    ERRCLog(classname, "Attempt was undertaken to Invalidate Routing State with no MRI given");
    return;
  }

  const routingkey* key = NULL;

  locktable();   // >=>=>  LOCK  >=>=>

  rt_iter cur = rtable.begin();

  while (!(cur == rtable.end())) 
  {
    Log(DEBUG_LOG,LOG_NORMAL, classname, color[red] << "Inspecting key" << color[off]);
    
    key=&(*cur).first;
    
    if ((*(key->mr) == *mymr) && (key->nslpid == nslpid)) {
      DLog(classname, color[red] << "Found a matching entry, Invalidating Routing State" << color[off]);
      
      routingentry* entry = (*cur).second;
      
      if (entry)
      {
	// check for status and set
	
	// ****************** BAD ***********************************+++
	if (status==APIMsg::bad) 
	{ // Routing state must be set to 'BAD', which means killing state
	  DLog(classname, color[red] << "Routing state must be set to 'BAD', which means killing state" << color[off]);
	  
	  entry->set_errstate(APIMsg::route_changed_status_bad);
	  
	  if (urgency) 
	  { // Immediate re-installation is requested, starting new handshake
	    DLog(classname, color[red] << "Immediate re-installation is requested, starting new handshake" << color[off]);
			    
	    if (entry->get_state() == qn_established) 
	    {
			    
	      DLog(classname, "I am QN for this state, going ahead and starting new handshake in QN_AWAITING_RESPONSE (no communication possible)");

	      // Make a COPY of the routing entry, invalidate the old one by remove_entry()
	      routingentry* tmpentry = new routingentry(*entry);
	      remove_entry(cur);
	      // XXX: after removing the entry from the table the iterator is no longer valid!
	      //      starting from the begining again, though this is not 100% correct, as the
	      //      cur++ at the bottom will make us start from beginning+1
	      //      It seems unlikely, however, that we have more than one match in a hash_map
	      //      so this whole while-loop is probably bogus anyway.  For the moment this is
	      //      good enough.
	      cur = rtable.begin();

	      add(key, tmpentry);
	      if (entry) delete entry;
	      entry = lookup(key);
	      // unlock. this is not safe. shhhh...
	      entry->unlock();
				
	      // drop to AWAITING_RESPONSE
	      entry->set_state(qn_awaiting_response);
	      //start NoResponse timer
	      TimerMsg* msg = new TimerMsg(message::qaddr_coordination,true);
	      // starting timer, give pointer to routingkey
	      entry->set_timer_type_2(noresponse);
				    
	      uint32* timer_type = new uint32;
	      *timer_type = entry->get_timer_type_2();

	      // set retry timeout back to initial value
	      entry->reset_retry_timeout();
				    
	      // start for 500msec (or whatever retryperiod is set to)
	      msg->start_relative(0, entry->get_retry_timeout(),(void*) timer_type, (void*) key);
	      entry->set_timer_id_2(msg->get_id());
				    
	      DLog(classname, "NoResponse timer started");
				
	      // send timer msg to Timer Module
	      msg->send_to(message::qaddr_timer);
				
	    } // endif state==qn_established
			    
	    if (entry->get_state() == rn_established) 
	    {
	      DLog(classname, "I am RN, I cannot re-install state, even though it was requested simply going to 'DEAD'");
	      
	      remove_entry(cur);
	      // XXX: see above
	      cur = rtable.begin();
	    }
	  } // endif urgency
	  else 
	  { // No immediate re-installation is requested, just killing the state
	    DLog(classname, color[red] << "No immediate re-installation is requested, just killing the state" << color[off]);
	    
	    remove_entry(cur);
	    // XXX: see above
	    cur = rtable.begin();
	  }
	}


	//************************* TENTATIVE *********************

	if (status==APIMsg::tentative) 
	{	  
	  EVLog(classname, "Set Routingstate to 'tentative', GIST will notify on change");
	  entry->set_errstate(APIMsg::route_changed_status_tentative);
	  
	  // Drop only to qn_awaiting_refresh
	  if (urgency) 
	  {
	    DLog(classname, color[red] << "Immediate re-installation is requested, starting new handshake" << color[off]);
			    
	    // we can do so if we are Querier and there is nothing else going on
	    if (entry->get_state() == qn_established) 
	    {
			    
	      DLog(classname, "I am QN for this state, going ahead and starting new handshake in QN_AWAITING_REFRESH (communication possible)");
				
	      // drop to AWAITING_RESPONSE
	      entry->set_state(qn_awaiting_refresh);
	      //start NoResponse timer
	      TimerMsg* msg = new TimerMsg(message::qaddr_coordination,true);
	      // starting timer, give pointer to routingkey
	      entry->set_timer_type_2(noresponse);
				    
	      uint32* timer_type = new uint32;
	      *timer_type = entry->get_timer_type_2();
				    
	      // set retry timeout back to initial value
	      entry->reset_retry_timeout();

	      // start for 500msec (or whatever retryperiod is set to)
	      msg->start_relative(0, entry->get_retry_timeout(),(void*) timer_type, (void*) key);
	      entry->set_timer_id_2(msg->get_id());
				    
	      DLog(classname, "NoResponse timer started");
				
	      // send timer msg to Timer Module
	      msg->send_to(message::qaddr_timer);
				
	    }
			    
	    if (entry->get_state() == rn_established) {
	      DLog(classname, "I am RN, I cannot start a state refresh");
				

	    }
	  } // end if URGENT
	} // end if TENTATIVE

      } 
      else 
	ERRCLog(classname, "There is a key for which there is no routing state anymore?!?!");
    } // end if key matches

    // XXX: in case we just killed the last one
    if (cur == rtable.end())
    	break;

    cur++;
  } // end while

  unlocktable(); // <=<=< UNLOCK <=<=<

}


/// iterates over routing table and sets lifetime for every routing table entry
void 
routingtable::set_state_lifetime(const mri* mymr, const uint32 nslpid, const uint32 lifetime) 
{
  // we must iterate over the full table and cannot use a lookup since routing key contains
  // additionally the session ID which is not given here
  DLog(classname, color[red] << "Iterating over Routing Key table to find the entries we must apply SetStateLifetime" << color[off]);

  const routingkey* key = NULL;

  locktable();   // >=>=>  LOCK  >=>=>

  routingentry* rte= NULL;
  rt_iter rt_it= rtable.begin();

  while (rt_it != rtable.end()) 
  {
    Log(DEBUG_LOG,LOG_NORMAL, classname, color[red] << "Inspecting key" << color[off]);
    
    key=&(*rt_it).first;
    
    if ((*(key->mr) == *mymr) && (key->nslpid == nslpid)) 
    {
      DLog(classname, color[red] << "Found a matching entry, setting State Lifetime (RS Time) to " << lifetime << "sec" << color[off]);
      rte= (*rt_it).second;
      if (rte!=NULL) 
      {
	      rte->set_rs_validity_time(lifetime);
      } 
      else 
	ERRCLog(classname, "Found no appropriate Routing data entry for given Routing Key");
    }
      
    rt_it++;
  } // end while

  unlocktable(); // <=<=< UNLOCK <=<=<
}


/** checks whether this flow is allowed to be signaled via this MA
 *  transfer attributes (security) are also checked
 */
bool 
routingtable::checkMA(const routingentry* r_entry, const appladdress* peer, const capability& cap)
{
  // precondition: the message arrived via C-Mode
  const ma_entry* ma= r_entry->get_peer_nli() ? lookup_ma(r_entry->get_peer_nli()) : 0;
  if (ma)
  { // if an MA exists for the peer
    // check whether the MA is sufficient to carry the message
    if (ma->get_peer_address() == *peer)
    { // message arrived via existing MA
      // in this case the security requirements must be checked:
      // if routing entry requires is, the MA should be secure, too,
      // otherwise unsecure data can be sent via secure MAs
      return r_entry->is_secure() ? ma->is_secure() : true;
    } 
    //    else
    // message arrived via different MA
    // this may be a new MA that has been just setup and it will
    // override the current MA
  }
 
  // no MA exists so far, or we have a new transport not matching the existing MA
  // but we can check whether the source address matches
  // the requirements
  if ( !r_entry->is_secure() || (r_entry->is_secure() && cap.is_secure(peer)) )
  {
    // security was requested and message came via secure C-Mode transport
    return true;
  }
  else
    return false;
}


void 
routingtable::sendhello(appladdress* addr, bool request_reply, uint32 hid) 
{
  Log(EVENT_LOG, LOG_NORMAL, classname, "MA Hello dispatched");

  // Build a HELLO Message

  //addr->set_ip_ttl(0);
    
  // construct DATA
  hello* pdu = new hello;
  pdu->set_hops(1);
  pdu->set_S();
  // if require a response to the MA-Hello, set R-bit
  if (request_reply)
  {
    DLog(classname, "MA Hello requests response (R=1)");

    pdu->set_R();
  }
  helloid *my_hid = new helloid(hid);
  pdu->add_helloid(my_hid);
  
  // leave NSLPID = 0, the HELLO needs not to relate to a NSLPID, it is irrelevant
  //pdu->set_nslpid(r_key->nslpid);
  
  SignalingMsgNTLP* sigmsg = new SignalingMsgNTLP();
  
  // send it
  sigmsg->set_req(addr, pdu);
  // do not initiate connection again for sending hellos 
  // if there is no active MA anymore, we have nothing to maintain
  sigmsg->set_useexistconn();
  sigmsg->send_or_delete();
}


const nli* 
routingtable::findNLI(const appladdress* peer) 
{
  const nli* nl = NULL;

  if (peer) 
  {
    appladdress* mypeer=peer->copy();

    mypeer->convert_to_ipv6();
	
    ma_entry* entry = NULL;
	
    DLog(classname, "Must iterate MA table to find the NLI");

    locktable();   // >=>=>  LOCK  >=>=>
    ma_iter myiter= ma_table.begin();
    
    while ((!entry) && (myiter!=ma_table.end())) 
    {
      entry = &((*myiter).second);
      if (entry) 
      {
	if (entry->get_peer_address() != *mypeer) entry = NULL; else nl = &((*myiter).first);
      }
      myiter++;
    } // end while
    unlocktable(); // <=<=< UNLOCK <=<=<

    if (entry) 
    {
      DLog(classname, color[yellow] << "Found the MA table entry for this peer" << color[off]);
    } 
  } 
  else 
    ERRCLog(classname, "hello_ind_ma() called with NULL as peer address");
    
  return nl;
} // end findNLI



/// find a peer information using a given SII handle
/// @return NLI (peer id and interface) for SII handle or NULL if nothing could be found
const nli* 
routingtable::findNLI(uint32 sii_handle) 
{
  siipeer_citer siiciter= sii_to_peer_table.find(sii_handle);
  if (siiciter != sii_to_peer_table.end())
  {
    return &(*siiciter).second;
  }
  else
  {
    // no entry found, can only return NULL
    return NULL;
  }
}

