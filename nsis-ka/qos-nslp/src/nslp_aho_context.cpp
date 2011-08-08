/// ----------------------------------------*- mode: C++; -*--
/// @file nslp_aho_context.cpp
/// QoS NSLP Anticipated Handover Context
/// ----------------------------------------------------------
/// $Id: nslp_aho_context.cpp 5253 2010-04-26 13:41:09Z roehricht $
/// $HeadURL: https://svn.tm.kit.edu/nsis/qos-nslp/trunk/src/nslp_aho_context.cpp $
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

#include "nslp_aho_context.h"

namespace qos_nslp {

NSLP_AHO_Context::NSLP_AHO_Context() : node_role(NSLP_AHO_Context::OTHER)
{
  pthread_mutexattr_init(&mutex_attr);
#ifdef _DEBUG
  pthread_mutexattr_settype(&mutex_attr, PTHREAD_MUTEX_ERRORCHECK);
#else
  pthread_mutexattr_settype(&mutex_attr, PTHREAD_MUTEX_NORMAL);
#endif
  pthread_mutex_init(&mutex, &mutex_attr);  

  is_flow_sender = false;
  is_flow_receiver = false;

  rii_obj = NULL;
  qspec_obj = NULL;
  refresh_period = NULL;
  bs = NULL;
} // end constructor


NSLP_AHO_Context::~NSLP_AHO_Context()
{
  pthread_mutex_destroy(&mutex);
  pthread_mutexattr_destroy(&mutex_attr);


  if(rii_obj != NULL) {
    delete rii_obj;
  }

  if(qspec_obj != NULL) {
    delete qspec_obj;
  }

  if(refresh_period != NULL) {
    delete refresh_period;
  }

  if(bs != NULL) {
    delete bs;
  }
}


/** This function sets the role of a node during an anticipated handover.
  * @param role the role of the node.
  */
void NSLP_AHO_Context::set_aho_node_role(const NSLP_AHO_Context::node_role_t role)
{
  node_role = role;
}


/** This function gets the role of a node during an anticipated handover.
  * @return the role of the node.
  */
NSLP_AHO_Context::node_role_t NSLP_AHO_Context::get_aho_node_role()
{
  return node_role;
}


/** This function is used by the new access router and sets the address of
  * the mobile node, which is needed later in an anticipated handover.
  * @param the address of the mobile node.
  */
void NSLP_AHO_Context::set_addr_mobile_node(const hostaddress &mn) {
  this->mn = mn;
}


/** This function gets the address of the mobile node, which is needed during
  * an anticipated handover.
  * @return the address of the mobile node.
  */
const hostaddress &NSLP_AHO_Context::get_addr_mobile_node() const {
  return mn;
}


/** This function is used by the mobile node and the new access router and sets
  * the address of the new access router, which is needed later in an
  * anticipated handover.
  * @param the address of the new access router.
  */
void NSLP_AHO_Context::set_addr_new_access_router(const hostaddress &ar_n) {
  this->ar_n = ar_n;
}


/** This function gets the address of the new access router, which is
  * needed during an anticipated handover.
  * @return the address of the new access router.
  */
const hostaddress &NSLP_AHO_Context::get_addr_new_access_router() const {
  return ar_n;
}


/** This function is used by the new access router and adds the mri of a new
  * flow to the NSLP_AHO_Context, which is needed to match RESERVE / RESPONSE
  * messages, that need to be forwarded towards the mobile node.
  * @param the mri of a new flow.
  */
void NSLP_AHO_Context::add_flow_to_context(const ntlp::mri_pathcoupled mri) {
  DLog("NSLP_AHO_Context", "add_new_flow_to_context(): ("
      << mri.get_sourceaddress() << ", "
      << mri.get_destaddress() << ")");

  mri_new_flow_set.insert(mri);

  DLog("NSLP_AHO_Context", "END add_new_flow_to_context()");
}


/** This function is used by the new access router and removes the mri of a
  * flow from the NSLP_AHO_Context.
  * @param the mri of a flow.
  */
void NSLP_AHO_Context::remove_flow_from_context(const ntlp::mri_pathcoupled &mri) {
  DLog("NSLP_AHO_Context", "remove_flow_from_context(): ("
      << mri.get_sourceaddress() << ", "
      << mri.get_destaddress() << ")");

  mri_new_flow_set.erase(mri);

  DLog("NSLP_AHO_Context", "END remove_flow_from_context()");
}


/** This function is used by the new access router and returns TRUE if the given
  * mri is saved in the NSLP_AHO_Context.
  * @return TRUE if the given mri is saved in the NSLP_AHO_Context.
  */
bool NSLP_AHO_Context::is_flow_in_context(const ntlp::mri_pathcoupled &mri) const {
  DLog("NSLP_AHO_Context", "is_new_flow_in_context(): ("
      << mri.get_sourceaddress() << ", "
      << mri.get_destaddress() << ")");

  mri_new_flow_set_const_it_t it = mri_new_flow_set.find(mri);
  bool is_flow_in_context = (it != mri_new_flow_set.end());

  DLog("NSLP_AHO_Context", "END is_new_flow_in_context()");

  return is_flow_in_context;
}


/** This function returns whether the set of flows is empty. The functionality
  * is needed to delete the NSLP_AHO_Context if there is no flow saved in
  * mri_new_flow_set.
  * @return TRUE if the set of flows is empty.
  */
bool NSLP_AHO_Context::is_set_of_flows_empty() const {
  return (mri_new_flow_set.size() < 1);
}


void NSLP_AHO_Context::set_is_flow_sender(bool is_flow_sender) {
  this->is_flow_sender = is_flow_sender;
}

bool NSLP_AHO_Context::get_is_flow_sender() const {
  return is_flow_sender;
}

void NSLP_AHO_Context::set_is_flow_receiver(bool is_flow_receiver) {
  this->is_flow_receiver = is_flow_receiver;
}

bool NSLP_AHO_Context::get_is_flow_receiver() const {
  return is_flow_receiver;
}


/** This function is used by the mobile node in the case where the MN is QNI and
  * saves the RII object of the sent RESERVE message to match a received
  * RESPONSE message.
  * @param the sent RII object.
  */
void NSLP_AHO_Context::set_rii(const rii *rii_obj) {
  if(rii_obj != NULL) {
    this->rii_obj = rii_obj->copy();
  }
}


/** This function is used by the mobile node in the case where the MN is QNI and
  * returns the saved RII object of the sent RESERVE message to match a received
  * RESPONSE message.
  * @return the previously sent RII object.
  */
const rii *NSLP_AHO_Context::get_rii() const {
  return rii_obj;
}


/** This function is used by the new access router and sets the QSPEC to use in
  * a subsequent RESERVE message.
  * @param the QSPEC to save in context.
  */
void NSLP_AHO_Context::set_qspec(qspec_object *qspec_obj) {
  if(qspec_obj != NULL) {
    this->qspec_obj = qspec_obj->copy();
  }
}


/** This function is used by the new access router and returns the QSPEC to use
  * for the RESERVE message of an anticipated reservation.
  * @return the saved QSPEC.
  */
qspec_object *NSLP_AHO_Context::get_qspec() const {
  return qspec_obj;
}


/** This function is used by the new access router in case it is QNI and flow
  * receiver, and saves the refresh period of a reservation in the context.
  * @param the refresh period to save in the context.
  */
void NSLP_AHO_Context::set_refresh_period(const rp *refresh_period) {
  if(refresh_period != NULL) {
    this->refresh_period = refresh_period->copy();
  }
}


/** This function is used by the new access router in case it is QNI and flow
  * receiver, and returns the refresh period of a reservation saved in the
  * context.
  * @return the refresh period saved in the context.
  */
const rp *NSLP_AHO_Context::get_refresh_period() const {
  return refresh_period;
}


/** This function is used by the new access router in case it is QNI and flow
  * receiver, and saves the id of a bound session in the context.
  * @param the id of a bound session to save in the context.
  */
void NSLP_AHO_Context::set_bound_sessionid(const bound_sessionid *bs) {
  if(bs != NULL) {
    this->bs = bs->copy();
  }
}


/** This function is used by the new access router in case it is QNI and flow
  * receiver, and saves the id of a bound session in the context.
  * @return the id of a bound session to save in the context.
  */
const bound_sessionid *NSLP_AHO_Context::get_bound_sessionid() const {
  return bs;
}

} // end namespace qos_nslp

// vi: expandtab softtabstop=2 tabstop=2 shiftwidth=2
