/// ----------------------------------------*- mode: C++; -*--
/// @file nslp_flow_context.h
/// QoS NSLP Flow Context
/// ----------------------------------------------------------
/// $Id: nslp_flow_context.h 6282 2011-06-17 13:31:57Z bless $
/// $HeadURL: https://svn.tm.kit.edu/nsis/qos-nslp/trunk/include/nslp_flow_context.h $
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
#ifndef _NSLP_FLOW_CONTEXT_H_
#define _NSLP_FLOW_CONTEXT_H_

//#include "sessionid.h"
//#include "rsn.h"
//#include "bound_sessionid.h"
//#include "timer_module.h"
//#include "aggregate.h"

#include "protlib_types.h"
#include "logfile.h"
//#include <pthread.h>
//#include "hashmap"
//#include "llhashers.h"
//#include "rii.h"
#include "mri_pc.h"
#include "qspec.h"

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
//using namespace protlib::log;


namespace qos_nslp {


typedef int32 timer_t;    // timer type


// This class represents the information which is saved on a per flow
// basis.

class NSLP_Flow_Context {
public:
    // constructor
    NSLP_Flow_Context(const ntlp::mri_pathcoupled &mri, const qspec_object &qspec);

    // destructor
    virtual ~NSLP_Flow_Context();
    
    
    // methods to handle the MRI of this flow
    void set_mri(const ntlp::mri_pathcoupled &mri);
    const ntlp::mri_pathcoupled &get_mri() const;


    // methods to handle timers
    void set_time_to_live(timer_t sec);
    void increase_time_to_live(int32 sec);
    void get_time_to_live(timer_t &sec) const;

    void set_timer_id_for_lifetime(id_t t);
    void get_timer_id_for_lifetime(id_t &t) const;


    // methods to handle the QSPEC of a reservation
    void set_qspec(const qspec_object &qspec);
    const qspec_object &get_qspec() const;


    // methods to handle the refreshing of reservations
    void set_refresh_period(uint32 refr_per);
    void get_refresh_period(uint32 &refr_per) const;

    void set_reduced_refresh(bool red_refr);
    bool get_reduced_refresh() const;
    
    // Stored value never used. So this methods aren't useful anymore.
    //void set_refresh_timer(timer_t sec);
    //void get_refresh_timer(timer_t &sec) const;


    // methods to ease the processing of messages
    void set_reserved(bool res);
    bool get_reserved() const;

    void set_replace(bool rep);
    bool get_replace() const;

    void set_is_flow_sender(bool is_sender);
    bool get_is_flow_sender() const;

    void set_is_flow_receiver(bool is_receiver);
    bool get_is_flow_receiver() const;

    //void set_timer_id_for_reserve_or_query(id_t t);
    //void get_timer_id_for_reserve_or_query(id_t &t) const;

    //void set_retry_counter_for_query(uint32 counter);
    //void get_retry_counter_for_query(uint32 &counter) const;

    // methods to handle rerouting events
    void set_merging_node(bool mn);
    bool get_merging_node() const;
    void set_branching_node(bool bn);
    bool get_branching_node() const;

#ifdef USE_AHO
    // methods for the anticipated handover
    void set_anticipated_reservation(bool flag);
    bool get_anticipated_reservation() const;
#endif

    
    //==================== Methods of which I'm unsure if appropriate here, after completely rewriting the context class ====================
    void set_logical_mri(const ntlp::mri_pathcoupled* mri);
    const ntlp::mri_pathcoupled* get_logical_mri() const;

    ntlp::mri_pathcoupled* get_old_mri();

    void set_timer_id_for_tunnel(id_t t);
    void get_timer_id_for_tunnel(id_t &t) const;
    //=======================================================================================================================================

    
    // lock mutex
    void lock();
    
    // unlock mutex
    void unlock();

private:
    // MRI of this flow
    ntlp::mri_pathcoupled flow_mri;

    // Time when NSLP_Flow_Context expires
    time_t time_to_live;

    // Timer ID for lifetime timer
    id_t timer_id_for_lifetime;

    // The QSPEC for the flow corresponding to this context
    qspec_object flow_qspec;

    // Refresh period for the particular flow
    uint32 refresh_period;

    // Determines whether reduced refreshes are used or not
    bool reduced_refresh;

    // Stored value is never used.
    //time_t refresh_timer;

    // TRUE if the reservation for this flow is already set up
    bool reserved;

    // If an outgoing reserve message has the replace bit set
    // and is after this waiting for a response which tears
    // down old reservations, this flag is set.
    bool replace_set;

    // needs to be saved in context since in case of a handover
    // a mobile node cannot determine if sender or receiver by
    // means of the node address
    bool is_flow_sender;
    bool is_flow_receiver;

    // TODO: check if really useful
    id_t timer_id_for_reserve_or_query;

    // TODO: move handling of maximum retransmissions into class querymsg
    uint32 retry_counter_for_query;

    // TRUE if this node is a merging node for this flow (same MRI ==> rerouting). Note
    // that a flow merging node is not the same (different MRIs)!
    bool merging_node;

    // TRUE if this node is a branching node for this flow (same MRI ==> rerouting). Note
    // that a flow branching node is not the same (different MRIs)!
    bool branching_node;

#ifdef USE_AHO
    // TRUE if the reservation of this flow is an anticipated reservation.
    bool anticipated_reservation;
#endif


    //==================== Instance variables of which I'm unsure if appropriate here, after completely rewriting the context class ====================
    ntlp::mri_pathcoupled* logical_mri;
    ntlp::mri_pathcoupled* old_mri;

    id_t timer_id_for_tunnel;
    //==================================================================================================================================================

    
    // This mutex is used to lock the NSLP_Session_context
    pthread_mutex_t mutex;
    
    // mutex attribute for setting kind of mutex
    pthread_mutexattr_t mutex_attr;
}; // end NSLP_Flow_Context


inline void NSLP_Flow_Context::lock() {
    if(pthread_mutex_lock(&mutex)) {
        ERRLog("NSLP_Flow_Context", "Error while locking mutex");
    }
}


inline void NSLP_Flow_Context::unlock() {
    pthread_mutex_unlock(&mutex);
}

} // end namespace qos_nslp

#endif // _NSLP_FLOW_CONTEXT_H_

// vi: expandtab softtabstop=4 tabstop=4 shiftwidth=4
