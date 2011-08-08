/// ----------------------------------------*- mode: C++; -*--
/// @file nslp_session_context.h
/// NSLP Session Context
/// ----------------------------------------------------------
/// $Id: nslp_session_context.h 6282 2011-06-17 13:31:57Z bless $
/// $HeadURL: https://svn.tm.kit.edu/nsis/qos-nslp/trunk/include/nslp_session_context.h $
// ===========================================================
//                      
// Copyright (C) 2005-2010, all rights reserved by
// - Institute of Telematics, Karlsruhe Institute of Technology
//
// More information and contact:
// https://projekte.tm.uka.de/trac/NSIS
//                      
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; version 2 of the License
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
// 02110-1301 USA.
//
// ===========================================================
#ifndef _NSLP_SESSION_CONTEXT_H_
#define _NSLP_SESSION_CONTEXT_H_

#include <stdexcept>

#include "protlib_types.h"
#include "logfile.h"
#include <pthread.h>
#include "hashmap"
#include "llhashers.h"
#include "nslp_flow_context.h"
#include "mri_pc.h"
#include "aggregate.h"
#include "rsn.h"

#ifdef NSIS_OMNETPP_SIM
#define pthread_mutexattr_init(a)
#define pthread_mutexattr_settype(a,b)
#define pthread_mutex_init(a,b)
#define pthread_mutexattr_destroy(a)
#define pthread_mutex_destroy(a)
#define pthread_mutex_lock(a) false
#define pthread_mutex_unlock(a) false
#endif


using namespace protlib;
using namespace protlib::log;


namespace qos_nslp {


typedef int64 simple_sid; // shortened sessionid in order to fit into a single 64-bit integer
typedef int32 timer_t;    // timer type


// This class represents the information which is global to a session.
// State which is saved on a per flow basis will be stored in an object
// of class NSLP_Flow_Context.
//
// If there is at least one NSLP_Flow_Context corresponding to the
// session of NSLP_Session_Context, the life time of this
// NSLP_Session_Context is as long as the longest per flow state.
// Otherwise the NSLP_Session_Context will be deletet after time_to_live.

class NSLP_Session_Context {
public:
    enum qn_type_t {
        QNI,
        QNE,
        QNR
    };

    // a hash function for the path-coupled MRI
    struct hash_pc_mri {
        size_t operator()(const ntlp::mri_pathcoupled &mri) const {
            size_t hash1 = mri.get_sourceaddress().get_hash();
            size_t hash2 = mri.get_destaddress().get_hash();

            return (size_t)(hash1 ^ hash2);
        }
    };

    // a comparison function for the path-coupled MRI used in conjunction with hash maps
    struct equal_to_pc_mri {
        bool operator()(const ntlp::mri_pathcoupled &a, const ntlp::mri_pathcoupled &b) const {
            return ((a.get_sourceaddress() == b.get_sourceaddress()) && (a.get_destaddress() == b.get_destaddress()));
        }
    };
  

    // constructor
    NSLP_Session_Context(const uint128 &sid, NSLP_Session_Context::qn_type_t qn_type, bool sender_init);

    // destructor
    virtual ~NSLP_Session_Context();


    // methods to handle session IDs
    void set_sid(const uint128 &sid);
    void get_sid(uint128 &sid) const;

    void set_bound_sid(const uint128 &b_sid);
    bool get_bound_sid(uint128 &b_sid) const;
    void unset_bound_sid();


    // methods to handle multiple per flow contexts
    void add_flow_context(const ntlp::mri_pathcoupled mri, NSLP_Flow_Context *context);
    void remove_flow_context(const ntlp::mri_pathcoupled &mri);
    NSLP_Flow_Context *find_flow_context(const ntlp::mri_pathcoupled &mri) const;


    // methods to handle the type of a node
    void set_qn_type(const NSLP_Session_Context::qn_type_t qn_type);
    NSLP_Session_Context::qn_type_t get_qn_type() const;


    // methods to handle the type of a reservation
    void set_sender_initiated(const bool sender_init);
    bool get_sender_initiated() const;

    // methods to handle RSNs

    // methods using only the RSN number
    void set_own_rsn_no(const uint32 rsn);
    uint32 get_own_rsn_no() const;

    // whole RSN object including epoch ID
    void set_own_rsn(const rsn& rsnobj);
    const rsn& get_own_rsn() const;
    void increment_own_rsn();
    void set_own_rsn_last_sent_for_flow(const ntlp::mri_pathcoupled mri, const rsn& rsnobj);
    bool is_own_rsn_last_sent_set_for_flow(const ntlp::mri_pathcoupled &mri) const;
    const rsn& get_own_rsn_last_sent_for_flow(const ntlp::mri_pathcoupled &mri) const;

    void set_predecessor_rsn(const uint32 sii, const rsn& rsnobj);
    void remove_predecessor_rsn(const uint32 sii);
    const rsn& get_predecessor_rsn(const uint32 sii) const;
    bool is_predecessor_rsn_set_for_sii(const uint32 sii) const;
    void set_predecessor_rsn_for_est_msg(const rsn& rsnobj);
    void unset_predecessor_rsn_for_est_msg();
    const rsn& get_predecessor_rsn_for_est_msg() const;
    bool is_predecessor_rsn_set_for_est_msg() const;


    // methods to handle SIIs
    void set_predecessor_sii(const ntlp::mri_pathcoupled mri, uint32 sii);
    void remove_predecessor_sii(const ntlp::mri_pathcoupled &mri);
    void get_predecessor_sii(const ntlp::mri_pathcoupled &mri, uint32 &sii);
    void get_predecessor_sii_old(const ntlp::mri_pathcoupled &mri, uint32 &old_sii);
    bool is_predecessor_sii_set_for_flow(const ntlp::mri_pathcoupled &mri) const;
    bool matches_given_sii_with_saved_predecessor_sii(const ntlp::mri_pathcoupled &mri, uint32 sii) const;
    bool get_flows_merging() const;

    void set_successor_sii(const ntlp::mri_pathcoupled mri, uint32 sii);
    void remove_successor_sii(const ntlp::mri_pathcoupled &mri);
    void get_successor_sii(const ntlp::mri_pathcoupled &mri, uint32 &sii);
    void get_successor_sii_old(const ntlp::mri_pathcoupled &mri, uint32 &sii);
    bool is_successor_sii_set_for_flow(const ntlp::mri_pathcoupled &mri) const;
    bool matches_given_sii_with_saved_successor_sii(const ntlp::mri_pathcoupled &mri, uint32 sii) const;
    bool get_flows_branching() const;


    // methods to handle timers
    void set_time_to_live(timer_t sec);
    void increase_time_to_live(int32 sec);
    void get_time_to_live(timer_t &sec) const;

    void set_timer_id_for_lifetime(id_t t);
    void get_timer_id_for_lifetime(id_t &t) const;


    // methods to ease the processing of messages
    //bool get_mergingNode();
    //bool get_branchingNode();


    // methods aren't used so far ???
    //void set_rii_res(uint32 r, rii *temp_rii);
    //void get_rii_res(uint32 r, rii *temp_rii);


    //==================== Methods of which I'm unsure if appropriate here, after completely rewriting the context class ====================
    void set_is_maybe_dead_end(bool dead_end);
    bool get_is_maybe_dead_end() const;

    // functions to manage an aggregate
    void set_is_in_an_aggregate(bool aggr);
    bool is_in_an_aggregate() const;
    void set_aggregate_sid(const uint128 &sid);
    void get_aggregate_sid(uint128 &sid) const;
    
    /*
    void set_aggregator_type(nslp_aggregate::aggregator_type_t aggr);
    nslp_aggregate::aggregator_type_t get_aggregator_type() const;
    */
    
    void set_aggregate(nslp_aggregate *set_aggr);
    nslp_aggregate* get_aggregate() const;
    //=======================================================================================================================================


    // lock mutex
    void lock();

    // unlock mutex
    void unlock();


    // NSLP_Flow_Context
    typedef hashmap_t<const ntlp::mri_pathcoupled, NSLP_Flow_Context *, hash_pc_mri, equal_to_pc_mri> flowcontext_t;
    typedef flowcontext_t::iterator flowcontext_it_t;
    typedef flowcontext_t::const_iterator flowcontext_const_it_t;
    flowcontext_t flowcontext;

private:
    // Session ID
    uint128 session_id;

    // Own RSN used for outgoing RESERVE messages.
    // Unused if a node is QNR.
    rsn ownRSNobj;

    typedef hashmap_t<const ntlp::mri_pathcoupled, rsn, hash_pc_mri, equal_to_pc_mri> mri_to_last_sent_rsn_t;
    typedef mri_to_last_sent_rsn_t::iterator mri_to_last_sent_rsn_it_t;
    typedef mri_to_last_sent_rsn_t::const_iterator mri_to_last_sent_rsn_const_it_t;
    mri_to_last_sent_rsn_t lastSentRSN;

    // Maps the SII handle (of a received RESERVE message) to the saved RSN of the predecessor node.
    // In case of a sender initiated reservation the predecessor node is upstream of us, otherwise
    // the predecessor node is downstream of us.
    // Unused if a node is QNI.
    typedef hashmap_t<uint32, rsn> predRSN_t;
    predRSN_t predRSNs;

    // Predecessor RSN for EST-MRM messages
    rsn predRSN_EST_MRM;
    bool predRSN_EST_MRM_set;


    typedef hashmap_t<const ntlp::mri_pathcoupled, uint32, hash_pc_mri, equal_to_pc_mri> mri_to_sii_t;
    typedef mri_to_sii_t::iterator mri_to_sii_it_t;
    typedef mri_to_sii_t::const_iterator mri_to_sii_const_it_t;

    // predecessor SIIs
    mri_to_sii_t predSIIs;
    mri_to_sii_t predSIIs_old;

    // successor SIIs
    mri_to_sii_t succSIIs;
    mri_to_sii_t succSIIs_old;


    // Time when NSLP_Session_Context expires
    time_t time_to_live;

    // Timer ID for lifetime timer
    id_t timer_id_for_lifetime;

    // Type of this node for this session
    NSLP_Session_Context::qn_type_t qn;

    // TRUE if reservations for this session
    // are sender initiated. This variable
    // was previously called "downstream".
    bool is_sender_initiated;

    // Flag, which tells if a bound Session ID exists or not
    bool bound_sid_exists;

    // Bound Session ID, if existing
    uint128 bound_sid;


    //==================== Instance variables of which I'm unsure if appropriate here, after completely rewriting the context class ====================
    bool maybe_dead_end;

    bool in_an_aggregate;
    uint128 aggr_sid;
    //nslp_aggregate::aggregator_type_t aggregator_type;
    qos_nslp::nslp_aggregate* aggregate;
    //==================================================================================================================================================
    

    // This mutex is used to lock the NSLP_Session_context
    pthread_mutex_t mutex;

    // mutex attribute for setting kind of mutex
    pthread_mutexattr_t mutex_attr;
}; // end NSLP_Session_Context


inline 
void 
NSLP_Session_Context::lock() {
    if (pthread_mutex_lock(&mutex)) {
        ERRLog("NSLP_Session_Context", "Error while locking mutex");
    }
}


inline 
void 
NSLP_Session_Context::unlock() { 
    pthread_mutex_unlock(&mutex);
}


/** Set function for the QN type of the current NSLP_Session_Context.
  * @param qn_type type of the current NSLP_Session_Context will be set to the value of this variable: QNI, QNE or QNR.
  */
inline
void 
NSLP_Session_Context::set_qn_type(const NSLP_Session_Context::qn_type_t qn_type)
{
    qn = qn_type;
}

/** Get function for the QN type of the current NSLP_Session_Context.
  * @return r type of the current NSLP_Session_Context: QNI, QNE or QNR.
  */
inline
NSLP_Session_Context::qn_type_t 
NSLP_Session_Context::get_qn_type() const {
    return qn;
}


/** This function sets the value that indicates whether the reservation is
  * sender initiated (== TRUE) or receiver initiated (== FALSE).
  * @param sender_init the value that indicates the kind of reservation.
  */
inline
void 
NSLP_Session_Context::set_sender_initiated(const bool sender_init)
{
    is_sender_initiated = sender_init;
}


/** This function gets the value that indicates whether the reservation is
  * sender initiated (== TRUE) or receiver initiated (== FALSE).
  * @return TRUE if the direction is sender initiated.
  */
inline
bool 
NSLP_Session_Context::get_sender_initiated() const {
    return is_sender_initiated;
}



/** Set function for RSN number of the current NSLP_Session_Context.
  * @param rsn the value of the RSN of the current NSLP_Session_Context will be set to the value of this variable.
  */
inline
void 
NSLP_Session_Context::set_own_rsn_no(const uint32 rsn) {
	ownRSNobj.set(rsn);
}

/** Get function for RSN object of the current NSLP_Session_Context.
  * @param rsn into this variable the value of RSN of the current NSLP_Session_Context will be written.
  */
inline
uint32 
NSLP_Session_Context::get_own_rsn_no() const {
    assert(qn != NSLP_Session_Context::QNR);

    return ownRSNobj.get();
}


/** Set function for RSN object of the current NSLP_Session_Context.
  * @param rsn the value of the RSN of the current NSLP_Session_Context will be set to the value of this variable.
  */
inline
void 
NSLP_Session_Context::set_own_rsn(const rsn& rsnobj) {
    ownRSNobj = rsnobj;
}

/** Get function for RSN object of the current NSLP_Session_Context.
  * @param rsn into this variable the value of RSN of the current NSLP_Session_Context will be written.
  */
inline
const rsn& 
NSLP_Session_Context::get_own_rsn() const {
    assert(qn != NSLP_Session_Context::QNR);

    return ownRSNobj;
}


/** Increment function for the own RSN number of the current NSLP_Session_Context.
  */
inline
void 
NSLP_Session_Context::increment_own_rsn() {
	ownRSNobj.increment();
}


/** Method saves the last sent (own) RSN. This is needed to control whether we have to add a
  * QSPEC object into an outgoing RESERVE message, because meantime the own RSN has been
  * incremented.
  */
inline
void 
NSLP_Session_Context::set_own_rsn_last_sent_for_flow(const ntlp::mri_pathcoupled mri, const rsn& rsnobj) {
    lastSentRSN[mri] = rsnobj;
}

/** Method returns whether the last sent (own) RSN has been saved in the context.
  * @return TRUE if the last sent (own) RSN has been saved in the context.
  */
inline
bool 
NSLP_Session_Context::is_own_rsn_last_sent_set_for_flow(const ntlp::mri_pathcoupled &mri) const {
    return (lastSentRSN.find(mri) != lastSentRSN.end());
}

/** Method returns the last sent (own) RSN. This is needed to control whether we have to add a
  * QSPEC object into an outgoing RESERVE message, because meantime the own RSN has been
  * incremented.
  * @return the last sent (own) RSN.
  */
inline
const rsn& 
NSLP_Session_Context::get_own_rsn_last_sent_for_flow(const ntlp::mri_pathcoupled &mri) const {
	mri_to_last_sent_rsn_const_it_t mri_rsn_it= lastSentRSN.find(mri);
	if (mri_rsn_it != lastSentRSN.end())
	{
		return mri_rsn_it->second;
	}
	else {
		throw new std::invalid_argument("Invalid Flow");
	}
}

/** Set function for the predecessor RSN object of the current NSLP_Session_Context.
  * @param sii the source interface identifier which defines the interface for which the predecessor RSN should be set.
  * @param rsn the predecessor RSN to store in the context.
  */
inline
void 
NSLP_Session_Context::set_predecessor_rsn(const uint32 sii, const rsn& rsnobj) {
    DLog("NSLP_Session_Context", "set_predecessor_rsn() - sii: " << sii << ", rsn: " << rsnobj);

    assert(qn != NSLP_Session_Context::QNI);

    predRSNs[sii] = rsnobj;

    DLog("NSLP_Session_Context", "END set_predecessor_rsn()");
}


inline
void 
NSLP_Session_Context::remove_predecessor_rsn(const uint32 sii) {
    DLog("NSLP_Session_Context", "remove_predecessor_rsn() - sii: " << sii);

    predRSNs.erase(sii);

    DLog("NSLP_Session_Context", "END remove_predecessor_rsn()");
}

/** Get function for the predecessor RSN object of the current NSLP_Session_Context. Method MUST NOT be called if
  * the specified SII doesn't exist in the hash_map predRSNs!
  * @param sii the source interface identifier which defines the interface for which the predecessor RSN should be retrieved.
  * @param rsn the value of the predecessor RSN, coressponding to the interface with SII sii, will be written into this variable.
  */
inline
const rsn& 
NSLP_Session_Context::get_predecessor_rsn(const uint32 sii) const {
    assert(qn != NSLP_Session_Context::QNI);

    predRSN_t::const_iterator predRSNit= predRSNs.find(sii);
    if (predRSNit != predRSNs.end()) {
        return predRSNit->second;
    }
    else {
        throw new std::invalid_argument("Invalid SII handle");
    }
}

/** Method to check whether a RSN is saved in the hash_map predRSNs for the given SII.
  * @param sii the source interface identifier
  * @return TRUE if a RSN is saved in predRSNs for the given SII.
  */
inline
bool 
NSLP_Session_Context::is_predecessor_rsn_set_for_sii(const uint32 sii) const {
    assert(qn != NSLP_Session_Context::QNI);

    return (predRSNs.find(sii) != predRSNs.end());
}

/** Set function for the predecessor RSN of EST-MRM messages.
  * @param rsn the predecessor RSN of an EST-MRM message to store in the context.
  */
inline
void 
NSLP_Session_Context::set_predecessor_rsn_for_est_msg(const rsn& rsnobj) {
    predRSN_EST_MRM = rsnobj;
    predRSN_EST_MRM_set = true;
}

/** Function which resets the predecessor RSN of EST-MRM messages.
  */
inline
void 
NSLP_Session_Context::unset_predecessor_rsn_for_est_msg() {
    predRSN_EST_MRM_set = false;
}

/** Get function for the predecessor RSN of EST-MRM messages. Method MUST NOT be called if
  * no predecessor RSN is saved for EST-MRM messages!
  * @param rsn the value of the predecessor RSN for EST-MRM messages will be written into this variable.
  */
inline
const rsn& 
NSLP_Session_Context::get_predecessor_rsn_for_est_msg() const {
    if (is_predecessor_rsn_set_for_est_msg()) {
        return predRSN_EST_MRM;
    }
    else {
        throw new std::invalid_argument("Predecessor RSN not yet set for EST-MRM messages.");
    }
}

/** Method to check whether a predecessor RSN is saved for EST-MRM messages.
  * @return TRUE if a predecessor RSN is saved for EST-MRM messages.
  */
inline
bool 
NSLP_Session_Context::is_predecessor_rsn_set_for_est_msg() const {
    return predRSN_EST_MRM_set;
}


/** Function which removes the successor SII handle corresponding to the flow specified by
  * the MRI mri from the hash_map succSIIs.
  * @param mri the MRI of the flow for which the successor SII should removed.
  */
inline
void 
NSLP_Session_Context::remove_successor_sii(const ntlp::mri_pathcoupled &mri) {
    DLog("NSLP_Session_Context", "remove_successor_sii() - mri: (" << mri.get_sourceaddress() << ", " << mri.get_destaddress() << ")");

    succSIIs.erase(mri);

    DLog("NSLP_Session_Context", "remove_successor_sii()");
}

/** Get function for the successor SII handle of the flow specified by the MRI mri.
  * @param mri the MRI of the flow for which the successor SII should be retrieved.
  * @param sii the value of the successor SII handle will be written into the value of this variable.
  */
inline
void 
NSLP_Session_Context::get_successor_sii(const ntlp::mri_pathcoupled &mri, uint32 &sii) {
    if (is_successor_sii_set_for_flow(mri)) {
        sii = succSIIs[mri];
    }
}

/** Get function for the old successor SII handle of the flow specified by the MRI mri.
  * @param mri the MRI of the flow for which the old successor SII should be retrieved.
  * @param sii the value of the successor SII handle will be written into the value of this variable.
  */
inline
void 
NSLP_Session_Context::get_successor_sii_old(const ntlp::mri_pathcoupled &mri, uint32 &old_sii) {
    if (succSIIs_old.find(mri) != succSIIs_old.end()) {
        old_sii = succSIIs_old[mri];
    }
}

/** Method to check whether a successor SII is saved in the hash_map succSIIs for the given MRI.
  * @param mri the MRI of the flow to check for a saved successor SII.
  * @return TRUE if a successor SII is saved in hash_map succSIIs for the given MRI.
  */
inline
bool 
NSLP_Session_Context::is_successor_sii_set_for_flow(const ntlp::mri_pathcoupled &mri) const {
    return (succSIIs.find(mri) != succSIIs.end());
}

/** Method to check whether the given pair (MRI, SII) matches with those saved in the hash_map succSIIs.
  * @param mri the MRI which describes the flow to check for.
  * @param sii the source interface identifier (SII) to check for.
  * @return TRUE if the pair (MRI, SII) matches with those saved in the hash_map succSIIs.
  */
inline
bool 
NSLP_Session_Context::matches_given_sii_with_saved_successor_sii(const ntlp::mri_pathcoupled &mri, uint32 sii) const {
    //precondition: for the given mri is a sii saved in the hash_map

    return (succSIIs.find(mri)->second == sii);
}


/** This function sets the value of the liefetime.
  * @param sec the value of the lifetime.
  */
inline
void 
NSLP_Session_Context::set_time_to_live(timer_t sec) {
    time_to_live = sec;
}

/** This function increases the value of the lifetime with the given value.
  * @param sec the value of the lifetime of the current NSLP_Session_Context will be increased by the value of this variable.
  */
inline
void 
NSLP_Session_Context::increase_time_to_live(int32 sec) {
    time_to_live = time_to_live + sec;
}


/** This function sets the value of the timer id for the liefetime.
  * @param t the value of the timer id for the lifetime.
  */
inline
void 
NSLP_Session_Context::set_timer_id_for_lifetime(id_t t) {
    timer_id_for_lifetime = t;
}

/** This function gets the value of the timer id for the liefetime.
  * @param t the value of the timer id for the lifetime of the current NSLP_Session_Context will be written into this variable.
  */
inline
void 
NSLP_Session_Context::get_timer_id_for_lifetime(id_t &t) const {
    t = timer_id_for_lifetime;
}

//==================== Methods of which I'm unsure if appropriate here, after completely rewriting the context class ====================
inline
void 
NSLP_Session_Context::set_is_maybe_dead_end(bool dead_end) {
    maybe_dead_end = true;
}


inline
bool 
NSLP_Session_Context::get_is_maybe_dead_end() const {
    return maybe_dead_end;
}



/** This function sets the value that indicates whether the current context is in an aggregate.
  * @param down the value that indicates whether the current context is in an aggregate.
  */
inline
void 
NSLP_Session_Context::set_is_in_an_aggregate(bool aggr) {
    in_an_aggregate = aggr;
}

/** This function gets the value that indicates whether the current context is in an aggregate.
  * @return TRUE if the current context is in an aggregate.
  */
inline
bool 
NSLP_Session_Context::is_in_an_aggregate() const {
    return in_an_aggregate;
}



/** This function sets the session id of the aggregate of the current NSLP_Session_Context.
  * @param sid the value of the session id of the aggregate of the current NSLP_Session_Context will be set to the value of this variable.
  */
inline
void 
NSLP_Session_Context::set_aggregate_sid(const uint128 &sid) {
    aggr_sid.w1 = sid.w1;
    aggr_sid.w2 = sid.w2;
    aggr_sid.w3 = sid.w3;
    aggr_sid.w4 = sid.w4;
}

/** This function gets the session id of the aggregate of the current NSLP_Session_Context.
  * @param sid the value of the session id of the aggregate of the current NSLP_Session_Context will be written into this variable.
  */
inline
void 
NSLP_Session_Context::get_aggregate_sid(uint128 &sid) const {
    sid.w1 = aggr_sid.w1;
    sid.w2 = aggr_sid.w2;
    sid.w3 = aggr_sid.w3;
    sid.w4 = aggr_sid.w4;
}



/** This function sets the aggregator type of the current NSLP_Session_Context in case of aggregation.
  * @param aggr the aggregator type of the current NSLP_Session_Context.
  */
/*
void NSLP_Session_Context::set_aggregator_type(nslp_aggregate::aggregator_type_t aggr) {
    aggregator_type = aggr;
}
*/	

/** This function gets the aggregator type of the current NSLP_Session_Context in case of aggregation.
  * @return the aggregator type of the current NSLP_Session_Context.
  */
/*
nslp_aggregate::aggregator_type_t NSLP_Session_Context::get_aggregator_type() const {
    return aggregator_type;
}
*/



/** This function sets the aggregate if the current NSLP_Session_Context is an aggregator or deaggregator.
  * @param set_aggr the aggregator of the current aggregation.
  */
inline
void NSLP_Session_Context::set_aggregate(nslp_aggregate *set_aggr) {
    aggregate = set_aggr;
}

/** This function gets the aggregate if the current NSLP_Session_Context is an aggregator or deaggregator.
  * @return the aggregator of the current aggregation.
  */
inline
nslp_aggregate* 
NSLP_Session_Context::get_aggregate() const {
    return aggregate;
}
//=======================================================================================================================================


} // end namespace qos_nslp

#endif // _NSLP_SESSION_CONTEXT_H_

// vi: expandtab softtabstop=4 tabstop=4 shiftwidth=4
