/// ----------------------------------------*- mode: C++; -*--
/// @file nslp_flow_context.cpp
/// QoS NSLP Flow Context
/// ----------------------------------------------------------
/// $Id: nslp_flow_context.cpp 5377 2010-06-09 20:42:12Z bless $
/// $HeadURL: https://svn.tm.kit.edu/nsis/qos-nslp/trunk/src/nslp_flow_context.cpp $
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

#include "nslp_pdu.h"
#include "nslp_flow_context.h"
#include "logfile.h"


using namespace protlib;

namespace qos_nslp {


/***** class NSLP_Flow_Context *****/

/** Default constructor for NSLP_Flow_Context without any params. All values will be set to the default.
  */
NSLP_Flow_Context::NSLP_Flow_Context(const ntlp::mri_pathcoupled &mri, const qspec_object &qspec) : flow_mri(mri), flow_qspec(qspec)
{
    pthread_mutexattr_init(&mutex_attr);
#ifdef _DEBUG
    pthread_mutexattr_settype(&mutex_attr, PTHREAD_MUTEX_ERRORCHECK);
#else
    pthread_mutexattr_settype(&mutex_attr, PTHREAD_MUTEX_NORMAL);
#endif
    pthread_mutex_init(&mutex, &mutex_attr);


    reserved = false;
    replace_set = false;
    reduced_refresh = false;

    merging_node = false;
    branching_node = false;

#ifdef USE_AHO
    anticipated_reservation = false;
#endif


    // TODO: initialization
/*
    time_t t = time(NULL) + getpar<uint32>(qosnslpconf_refresh_period);
    set_time_to_live(t);
    set_ReducedRefresh(false);
    set_retry_counter_for_query(0);
    set_replace(false);
    set_mergingNode(false);
    set_branchingNode(false);
    set_is_maybe_dead_end(false);
    set_is_in_an_aggregate(false);
    //set_aggregator_type(nslp_aggregate::not_set);
    context_mri = logical_mri = old_mri = 0;
*/
} // end constructor


/** Destructor for NSLP_Flow_Context.  */
NSLP_Flow_Context::~NSLP_Flow_Context() {
    pthread_mutex_destroy(&mutex);
    pthread_mutexattr_destroy(&mutex_attr);

/*
  context_mri = NULL;
  logical_mri = NULL;
  context_qspec = NULL;
*/
}



/** This function sets the MRI of the current NSLP_Flow_Context.
  * @param mri the MRI of the current NSLP_Flow_Context.
  */
// XXX: LEAK LEAK LEAK ...
void NSLP_Flow_Context::set_mri(const ntlp::mri_pathcoupled &mri)
{
    /*
    if (mri) {
        if (flow_mri && *mri != *flow_mri) {
            if ((old_mri && flow_mri->get_downstream() == old_mri->get_downstream()) || !old_mri) {
                ILog("NSLP_Flow_Context", "Changing MRI: " << *flow_mri << *mri);
                old_mri = flow_mri;
            }
        }
        flow_mri = mri->copy();
    }
    else {
        flow_mri = NULL;
    }
    */

    flow_mri = mri;
}

/** This function gets the MRI of the current NSLP_Flow_Context.
  * @return MRI of the current NSLP_Flow_Context.
  */
const ntlp::mri_pathcoupled &NSLP_Flow_Context::get_mri() const {
    return flow_mri;
}



/** This function sets the value of the liefetime.
  * @param sec the value of the lifetime.
  */
void NSLP_Flow_Context::set_time_to_live(timer_t sec) {
    time_to_live = sec;
}

/** This function increases the value of the lifetime with the given value.
  * @param sec the value of the lifetime of the current NSLP_Flow_Context will be increased with the value of this variable.
  */
void NSLP_Flow_Context::increase_time_to_live(int32 sec) {
    time_to_live = time_to_live + sec;
}

/** This function gets the value of the liefetime.
  * @param sec the value of the lifetime of the current NSLP_Flow_Context will be written into this variable.
  */
void NSLP_Flow_Context::get_time_to_live(timer_t &sec) const {
    sec = time_to_live;
}



/** This function sets the value of the timer id for the liefetime.
  * @param t the value of the timer id for the lifetime.
  */
void NSLP_Flow_Context::set_timer_id_for_lifetime(id_t t) {
    timer_id_for_lifetime = t;
}

/** This function gets the value of the timer id for the liefetime.
  * @param t the value of the timer id for the lifetime of the current NSLP_Flow_Context will be written into this variable.
  */
void NSLP_Flow_Context::get_timer_id_for_lifetime(id_t &t) const {
    t = timer_id_for_lifetime;
}



/** This function sets the QSPEC of the current NSLP_Flow_Context.
  * @param qspec the QSPEC for the current NSLP_Flow_Context.
  */
void NSLP_Flow_Context::set_qspec(const qspec_object &qspec) {
//    flow_qspec = qspec->copy();
    flow_qspec = qspec;
}

/** This function gets the QSPEC of the current NSLP_Flow_Context.
  * @return the QSPEC for the current NSLP_Flow_Context.
  */
// XXX: LEAK LEAK LEAK ...
const qspec_object &NSLP_Flow_Context::get_qspec() const {
//    return flow_qspec->copy();
    return flow_qspec;
}



/** Set function for the REFRESH_PERIOD of the current NSLP_Flow_Context.
  * @param refr_per the value of the REFRESH_PERIOD of the current NSLP_Flow_Context will be set to the value of this variable.
  */
void NSLP_Flow_Context::set_refresh_period(uint32 refr_per) {
    refresh_period = refr_per;
}

/** Get function for the REFRESH_PERIOD of the current NSLP_Flow_Context.
  * @param refr_per the value of the REFRESH_PERIOD of the current NSLP_Flow_Context will be written into this variable.
  */
void NSLP_Flow_Context::get_refresh_period(uint32 &refr_per) const {
    refr_per = refresh_period;
}



/** This function sets the value that indicates whether the reduced refresh is activated.
  * @param red_refr the value that indicates whether the reduced refresh is activated.
  */
void NSLP_Flow_Context::set_reduced_refresh(bool red_refr) {
    reduced_refresh = red_refr;
}
	
/** This function gets the value that indicates whether the reduced refresh is activated.
  * @return TRUE if the reduced refresh is activated.
  */
bool NSLP_Flow_Context::get_reduced_refresh() const {
    return reduced_refresh;
}



/** This function sets the value of the refresh timer.
  * @param sec the value of the refresh timer.
  */
/*
void NSLP_Flow_Context::set_refresh_timer(timer_t sec) {
    refresh_timer = sec;
}
*/

/** This function gets the value of the refresh timer.
  * @param sec the value of the refresh timer of the current NSLP_Flow_Context will be written into this variable.
  */
/*
void NSLP_Flow_Context::get_refresh_timer(timer_t &sec) const {
    sec = refresh_timer;
}
*/



/** This function sets the value that indicates whether the current NSLP_Flow_Context has reserved ressources or not.
  * @param res the value that indicates whether the current NSLP_Flow_Context has reserved ressources or not.
  */
void NSLP_Flow_Context::set_reserved(bool res) {
    reserved = res;
}

/** This function gets the value that indicates whether the current NSLP_Flow_Context has reserved ressources or not.
  * @return TURE if the current NSLP_Flow_Context has reserved ressources.
  */
bool NSLP_Flow_Context::get_reserved() const {
    return reserved;
}



/** This function sets the value that indicates whether the reservation on the old path should be torn down explicitly.
  * @param down the value that indicates whether the reservation on the old path should be torn down explicitly.
  */
void NSLP_Flow_Context::set_replace(bool rep) {
    replace_set = rep;
}

/** This function gets the value that indicates whether the reservation on the old path should be torn down explicitly.
  * @return TRUE if the reservation on the old path should be torn down explicitly.
  */	
bool NSLP_Flow_Context::get_replace() const {
    return replace_set;
}



/** This function sets whether a node is the sender of the flow.
  * @param is_sender true if sender of the flow.
  */
void NSLP_Flow_Context::set_is_flow_sender(bool is_sender) {
	  is_flow_sender = is_sender;
}

/** This function returns whether a node is the sender of the flow.
  * @return true if sender of the flow.
  */
bool NSLP_Flow_Context::get_is_flow_sender() const {
	  return is_flow_sender;
}

/** This function sets whether a node is the receiver of the flow.
  * @param is_receiver true if receiver of the flow
  */
void NSLP_Flow_Context::set_is_flow_receiver(bool is_receiver) {
	  is_flow_receiver = is_receiver;
}

/** This function returns whether a node is the receiver of the flow.
  * @return true if receiver of the flow.
  */
bool NSLP_Flow_Context::get_is_flow_receiver() const {
	  return is_flow_receiver;
}



/** This function sets the value of the timer id for the RESERVE or QUERY.
  * @param t the value of the timer id for the RESERVE or QUERY.
  */
//void NSLP_Flow_Context::set_timer_id_for_reserve_or_query(id_t t) {
//    timer_id_for_reserve_or_query = t;
//}

/** This function gets the value of the timer id for the RESERVE or QUERY.
  * @param t the value of the timer id for the RESERVE or QUERY of the current NSLP_Flow_Context will be written into this variable.
  */
//void NSLP_Flow_Context::get_timer_id_for_reserve_or_query(id_t &t) const {
//    t = timer_id_for_reserve_or_query;
//}



/** This function sets the retry counter for QUERY message.
  * @param counter the value for the retry counter for QUERY message.
  */
//void NSLP_Flow_Context::set_retry_counter_for_query(uint32 counter) {
//    retry_counter_for_query = counter;
//}

/** This function gets the retry counter for QUERY message.
  * @param counter the value for the retry counter for QUERY message will be written into this variable.
  */
//void NSLP_Flow_Context::get_retry_counter_for_query(uint32 &counter) const {
//    counter = retry_counter_for_query;
//}



/** This function sets the value that indicates whether our node node is a merging node for this flow (same MRI ==> rerouting).
  * Note that a flow merging node is not the same (different MRIs)!
  * @param mn the value that indicates whether our node is a merging node for this flow.
  */
void NSLP_Flow_Context::set_merging_node(bool mn) {
    merging_node = mn;
}

/** This function gets the value that indicates whether our node is a merging node for this flow (same MRI ==> rerouting).
  * Note that a flow merging node is not the same (different MRIs)!
  * @return TRUE if our node is a merging node for this flow.
  */	
bool NSLP_Flow_Context::get_merging_node() const {
    return merging_node;
}

/** This function sets the value that indicates whether our node node is a branching node for this flow (same MRI ==> rerouting).
  * Note that a flow branching node is not the same (different MRIs)!
  * @param bn the value that indicates whether our node is a branching node for this flow.
  */
void NSLP_Flow_Context::set_branching_node(bool bn) {
    branching_node = bn;
}

/** This function gets the value that indicates whether our node is a branching node for this flow (same MRI ==> rerouting).
  * Note that a flow branching node is not the same (different MRIs)!
  * @return TRUE if our node is a branching node for this flow.
  */	
bool NSLP_Flow_Context::get_branching_node() const {
    return branching_node;
}



#ifdef USE_AHO
/** This function sets the value that indicates whether this reservation is an anticipated reservation.
  * @param flag TRUE if this reservation is an anticipated reservation.
  */
void NSLP_Flow_Context::set_anticipated_reservation(bool flag) {
    DLog("NSLP_Flow_Context", "set_anticipated_reservation(): " << (flag ? "true" : "false") << " for flow ("
            << flow_mri.get_sourceaddress() << ", "
            << flow_mri.get_destaddress() << ")");

    anticipated_reservation = flag;
}

/** This function gets the value that indicates whether this reservation is an anticipated reservation.
  * @return TRUE if this reservation is an anticipated reservation.
  */	
bool NSLP_Flow_Context::get_anticipated_reservation() const {
    return anticipated_reservation;
}
#endif // use AHO


//==================== Methods of which I'm unsure if appropriate here, after completely rewriting the context class ====================
/** This function sets the logical MRI of the current NSLP_Flow_Context.
  * @param mri the MRI of the current NSLP_Flow_Context.
  */
void NSLP_Flow_Context::set_logical_mri(const ntlp::mri_pathcoupled* mri) {
    if(mri) {
        logical_mri = mri->copy();
    }
    else {
        logical_mri = NULL;
    }
}

/** This function gets the logical MRI of the current NSLP_Flow_Context.
  * @return MRI of the current NSLP_Flow_Context.
  */
const ntlp::mri_pathcoupled* NSLP_Flow_Context::get_logical_mri() const {
    if(logical_mri) {
        return logical_mri->copy();
    }
    else {
        return NULL;
    }
}



/** This function gets the old MRI of the current NSLP_Flow_Context (if one exists).
  * @return old MRI of the current NSLP_Flow_Context.
  */
ntlp::mri_pathcoupled* NSLP_Flow_Context::get_old_mri()
{
    if(old_mri) {
        return old_mri->copy();
    }
    else {
        return NULL;
    }
}



void NSLP_Flow_Context::set_timer_id_for_tunnel(id_t t) {
        timer_id_for_tunnel = t;
}

void NSLP_Flow_Context::get_timer_id_for_tunnel(id_t &t) const {
        t = timer_id_for_tunnel;
}
//=======================================================================================================================================

} // end namespace qos_nslp

// vi: expandtab softtabstop=4 tabstop=4 shiftwidth=4
