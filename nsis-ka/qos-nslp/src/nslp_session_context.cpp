/// ----------------------------------------*- mode: C++; -*--
/// @file nslp_session_context.cpp
/// QoS NSLP Session Context
/// ----------------------------------------------------------
/// $Id: nslp_session_context.cpp 5442 2010-06-16 08:42:37Z bless $
/// $HeadURL: https://svn.tm.kit.edu/nsis/qos-nslp/trunk/src/nslp_session_context.cpp $
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
#include "nslp_session_context.h"
#include "logfile.h"


using namespace protlib;

namespace qos_nslp {


/***** class NSLP_Session_Context *****/

NSLP_Session_Context::NSLP_Session_Context(const uint128 &sid, NSLP_Session_Context::qn_type_t qn_type, bool sender_init) :
	session_id(sid),
	predRSN_EST_MRM_set(false),
	qn(qn_type),
	is_sender_initiated(sender_init),
	bound_sid_exists(false),
	maybe_dead_end(false),
	in_an_aggregate(false),
	aggregate(NULL)
//	aggregator_type(nslp_aggregate::not_set)
  {
    pthread_mutexattr_init(&mutex_attr);
#ifdef _DEBUG
    pthread_mutexattr_settype(&mutex_attr, PTHREAD_MUTEX_ERRORCHECK);
#else
    pthread_mutexattr_settype(&mutex_attr, PTHREAD_MUTEX_NORMAL);
#endif
    pthread_mutex_init(&mutex, &mutex_attr);  
}


NSLP_Session_Context::~NSLP_Session_Context() {
    pthread_mutex_destroy(&mutex);
    pthread_mutexattr_destroy(&mutex_attr);
}



/** This function sets the session id of the current NSLP_Session_Context.
  * @param sid the value of the session id of the current NSLP_Session_Context will be set to the value of this variable.
  */
void NSLP_Session_Context::set_sid(const uint128 &sid) {
    session_id.w1 = sid.w1;
    session_id.w2 = sid.w2;
    session_id.w3 = sid.w3;
    session_id.w4 = sid.w4;
}

/** This function gets the session id of the current NSLP_Session_Context.
  * @param sid the value of the session id of the current NSLP_Session_Context will be written into this variable.
  */
void NSLP_Session_Context::get_sid(uint128 &sid) const {
    sid.w1 = session_id.w1;
    sid.w2 = session_id.w2;
    sid.w3 = session_id.w3;
    sid.w4 = session_id.w4;
}


/** This function sets the bound session id of the current NSLP_Session_Context.
  * @param b_sid the value of the bound session id of the current NSLP_Session_Context will be set to the value of this variable.
  */
void NSLP_Session_Context::set_bound_sid(const uint128 &b_sid) {
    bound_sid_exists = true;

    bound_sid.w1 = b_sid.w1;
    bound_sid.w2 = b_sid.w2;
    bound_sid.w3 = b_sid.w3;
    bound_sid.w4 = b_sid.w4;
}        	
             
/** This function gets the bound session id of the current NSLP_Session_Context.
  * @param b_sid the value of the bound session id of the current NSLP_Session_Context will be written into this variable.
  */
bool NSLP_Session_Context::get_bound_sid(uint128 &b_sid) const {
    if(bound_sid_exists) {
        b_sid.w1 = bound_sid.w1;
        b_sid.w2 = bound_sid.w2;
        b_sid.w3 = bound_sid.w3;
        b_sid.w4 = bound_sid.w4;

        return true;
    }
    else {
        return false;
    }
}

void NSLP_Session_Context::unset_bound_sid() {
    bound_sid_exists = false;
}



/** This method adds a per flow context for the flow specified by the variable mri to the current session context.
  * @param mri the MRI which describes the flow to add a (flow) context for.
  * @param context the flow context to add to the session context.
  */
void NSLP_Session_Context::add_flow_context(const ntlp::mri_pathcoupled mri, NSLP_Flow_Context *context) {
    DLog("NSLP_Session_Context", "add_flow_context()");

    if(context != NULL) {
        flowcontext[mri] = context;
    }

    DLog("NSLP_Session_Context", "END add_flow_context()");
}

/** This method removes the per flow context for the flow specified by the variable mri from the current session context.
  * @param mri the MRI that describes the flow which context has to be removed.
  */
void NSLP_Session_Context::remove_flow_context(const ntlp::mri_pathcoupled &mri) {
    DLog("NSLP_Session_Context", "remove_flow_context()");

    flowcontext.erase(mri);

    DLog("NSLP_Session_Context", "END remove_flow_context()");
}

/** This method searches for the per flow context of the flow specified by the variable mri.
  * @param mri the MRI that describes the flow for which the per flow context has to be searched.
  */
NSLP_Flow_Context *NSLP_Session_Context::find_flow_context(const ntlp::mri_pathcoupled &mri) const {
    DLog("NSLP_Session_Context", "find_flow_context()");

    flowcontext_const_it_t it = flowcontext.find(mri);
    NSLP_Flow_Context *context = (it != flowcontext.end()) ? it->second : NULL;

    DLog("NSLP_Session_Context", "END find_flow_context() - context " << ((context == NULL) ? "NOT found" : "found") );

    return context;
}


/** Set function for the predecessor SII handle of the flow specified by the MRI mri.
  * @param mri the MRI of the flow for which the predecessor SII should be set.
  * @param sii the value of the predecessor SII handle will be set to the value of this variable.
  */
void NSLP_Session_Context::set_predecessor_sii(const ntlp::mri_pathcoupled mri, uint32 sii) {
    DLog("NSLP_Session_Context", "set_predecessor_sii() - mri: (" << mri.get_sourceaddress() << ", " << mri.get_destaddress() << "), sii: " << sii);

    if(is_predecessor_sii_set_for_flow(mri)) {
        if(sii != predSIIs[mri]) {
            predSIIs_old[mri] = predSIIs[mri];
            predSIIs[mri] = sii;
        }
    }
    else {
        predSIIs_old[mri] = 0;
        predSIIs[mri] = sii;
    }

    DLog("NSLP_Session_Context", "END set_predecessor_sii()");
}

/** Function which removes the predecessor SII handle corresponding to the flow specified by
  * the MRI mri from the hash_map predSIIs.
  * @param mri the MRI of the flow for which the predecessor SII should removed.
  */
void NSLP_Session_Context::remove_predecessor_sii(const ntlp::mri_pathcoupled &mri) {
    DLog("NSLP_Session_Context", "remove_predecessor_sii() - mri: (" << mri.get_sourceaddress() << ", " << mri.get_destaddress() << ")");

    if (is_predecessor_sii_set_for_flow(mri)) {
        uint32 predSII;
        get_predecessor_sii(mri, predSII);

        predSIIs.erase(mri);

        // check if an other flow exists, which shares the same predecessor SII
        bool exists_flow_with_same_predSII = false;
        mri_to_sii_const_it_t it;
        for(it = predSIIs.begin(); it != predSIIs.end(); ++it) {
            if(it->second == predSII) {
                exists_flow_with_same_predSII = true;
                break;
            }
        }

        // if no other flow has the same predecessor SII,
        // delete the predecessor RSN also
        if(!exists_flow_with_same_predSII) {
            remove_predecessor_rsn(predSII);
        }
    }

    DLog("NSLP_Session_Context", "END remove_predecessor_sii()");
}

/** Get function for the predecessor SII handle of the flow specified by the MRI mri.
  * @param mri the MRI of the flow for which the predecessor SII should be retrieved.
  * @param sii the value of the predecessor SII handle will be written into the value of this variable.
  */
void NSLP_Session_Context::get_predecessor_sii(const ntlp::mri_pathcoupled &mri, uint32 &sii) {
    if(is_predecessor_sii_set_for_flow(mri)) {
        sii = predSIIs[mri];
    }
}

/** Get function for the old predecessor SII handle of the flow specified by the MRI mri.
  * @param mri the MRI of the flow for which the old predecessor SII should be retrieved.
  * @param old_sii the value of the old predecessor SII handle will be written into the value of this variable.
  */
void NSLP_Session_Context::get_predecessor_sii_old(const ntlp::mri_pathcoupled &mri, uint32 &old_sii) {
    if(predSIIs_old.find(mri) != predSIIs_old.end()) {
        old_sii = predSIIs_old[mri];
    }
}

/** Method to check whether a predecessor SII is saved in the hash_map predSIIs for the given MRI.
  * @param mri the MRI of the flow to check for a saved predecessor SII.
  * @return TRUE if a predecessor SII is saved in hash_map predSIIs for the given MRI.
  */
bool NSLP_Session_Context::is_predecessor_sii_set_for_flow(const ntlp::mri_pathcoupled &mri) const {
    return (predSIIs.find(mri) != predSIIs.end());
}

/** Method to check whether the given pair (MRI, SII) matches with those saved in the hash_map predSIIs.
  * @param mri the MRI which describes the flow to check for.
  * @param sii the source interface identifier (SII) to check for.
  * @return TRUE if the pair (MRI, SII) matches with those saved in the hash_map predSIIs.
  */
bool NSLP_Session_Context::matches_given_sii_with_saved_predecessor_sii(const ntlp::mri_pathcoupled &mri, uint32 sii) const {
    //precondition: for the given mri is a sii saved in the hash_map

    return (predSIIs.find(mri)->second == sii);
}

/** Method which determines if this node is a (flow) merging node. A node is called a (flow) merging node
  * if there are multiple reservations (with different MRIs) whereas at least two of them have different predecessor SIIs.
  * @return TRUE if this node is a (flow) merging node.
  */
bool NSLP_Session_Context::get_flows_merging() const {
    // If not all MRIs of this session have the same predecessor sii, different flows merge at our node.

    if(predSIIs.size() >= 1) {
        mri_to_sii_const_it_t it = predSIIs.begin();
        const uint32 sii = it->second;

        for(++it; it != predSIIs.end(); ++it) {
            if(it->second != sii) {
                return true;
            }
        }

        return false;
    }
    else {
        return false;
    }
}


/** Set function for the successor SII handle of the flow specified by the MRI mri.
  * @param mri the MRI of the flow for which the successor SII should be set.
  * @param sii the value of the successor SII handle will be set to the value of this variable.
  */
void NSLP_Session_Context::set_successor_sii(const ntlp::mri_pathcoupled mri, uint32 sii) {
    DLog("NSLP_Session_Context", "set_successor_sii() - mri: (" << mri.get_sourceaddress() << ", " << mri.get_destaddress() << "), sii: " << sii);

    if(is_successor_sii_set_for_flow(mri)) {
        if(sii != succSIIs[mri]) {
            succSIIs_old[mri] = succSIIs[mri];
            succSIIs[mri] = sii;
        }
    }
    else {
        succSIIs_old[mri] = 0;
        succSIIs[mri] = sii;
    }

    DLog("NSLP_Session_Context", "set_successor_sii()");
}


/** Method which determines if this node is a (flow) branching node. A node is called a (flow) branching node
  * if there are multiple reservations (with different MRIs) whereas at least two of them have different successor SIIs.
  * @return TRUE if this node is a (flow) branching node.
  */
bool NSLP_Session_Context::get_flows_branching() const {
    // If not all MRIs of this session have the same successor sii, different flows branch at our node.

    if(succSIIs.size() >= 1) {
        mri_to_sii_const_it_t it = succSIIs.begin();
        const uint32 sii = it->second;

        for(++it; it != succSIIs.end(); ++it) {
            if(it->second != sii) {
                return true;
            }
        }

        return false;
    }
    else {
        return false;
    }
}

/** This function gets the value of the liefetime.
  * @param sec the value of the lifetime of the current NSLP_Session_Context will be written into this variable.
  */
void NSLP_Session_Context::get_time_to_live(timer_t &sec) const {
    timer_t max_time_to_live = time_to_live;

    flowcontext_const_it_t it;
    for(it = flowcontext.begin(); it != flowcontext.end(); ++it) {
        NSLP_Flow_Context *context = it->second;
        if(context != NULL) {
            timer_t tmp_time;
            context->get_time_to_live(tmp_time);

            if(tmp_time > max_time_to_live) {
                max_time_to_live = tmp_time;
            }
        }
    }

    sec = max_time_to_live;
}



/** This function determines whether the current node is a merging one.
  * @return TRUE if the current node is a merging one.
  */
/*
bool NSLP_Session_Context::get_mergingNode()
{
//    return mergingNode;
}
*/

/** This function determines whether the current node is a branching one.
  * @return TRUE if the current node is a branching one.
  */
/*
bool NSLP_Session_Context::get_branchingNode()
{
//    return branchingNode;
}
*/



/** Set function for the RII object into the hashmap with RIIs of the current NSLP_Session_Context.
  * @param r the position in the hashmap for the given RII object.
  * @param temp_rii the RII object to be stored in the RII hashmap.
  */
/*
void NSLP_Session_Context::set_rii_res(uint32 r, rii *temp_rii) {
    rii_hashmap[r] = temp_rii;
}
*/

/** Get function for the RII object from the hashmap with RIIs of the current NSLP_Session_Context.
  * @param r the position in the hashmap for the RII object to be read.
  * @param temp_rii the RII object from the RII hashmap will be written into this variable.
  */
/*
void NSLP_Session_Context::get_rii_res(uint32 r, rii *temp_rii) {
    rii_hashmap_it_t hit;
    if ((hit=rii_hashmap.find(r))!=rii_hashmap.end())
    { 
      Log(INFO_LOG,LOG_NORMAL,"NSLP_CONTEXT","Found RII!");
      temp_rii = hit->second;
    }
    else {
      temp_rii = NULL;
    }
}
*/




} // end namespace qos_nslp

// vi: expandtab softtabstop=4 tabstop=4 shiftwidth=4
