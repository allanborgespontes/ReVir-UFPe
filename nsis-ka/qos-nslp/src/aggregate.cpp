/// ----------------------------------------*- mode: C++; -*--
/// @file aggregate.cpp
/// NSLP Aggregate
/// ----------------------------------------------------------
/// $Id: aggregate.cpp 6282 2011-06-17 13:31:57Z bless $
/// $HeadURL: https://svn.tm.kit.edu/nsis/qos-nslp/trunk/src/aggregate.cpp $
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

#include "hashmap"
#include "llhashers.h"
#include "nslp_pdu.h"
#include "logfile.h"
#include "aggregate.h"
#include "nslp_session_context_map.h"
#include "nslp_session_context.h"
#include "context_manager.h"

using namespace protlib;

namespace qos_nslp {


/** @addtogroup ienslpaggregate NSLP Aggregate
 * @{
 */

/***** class nslp_aggregate *****/

/** Generate new nslp_aggregate instance.
  */
nslp_aggregate* nslp_aggregate::new_instance() const {
        nslp_aggregate* n = new nslp_aggregate();
	return n;
} // end new_instance

/** Default constructor for nslp_aggregate without any params. The values of nslp_aggregate will be set to the default.
  */
  nslp_aggregate::nslp_aggregate() : aggregate_sid(NULL), aggregate_qspec(NULL), aggr_type(nslp_aggregate::not_set), aggregator_mri(NULL), aggregator_qspec(NULL), counter(0)
{
      session_context_map = Context_Manager::instance()->get_scm();
}

/** Constructor for nslp_aggregate.
  * @param sid vector with session ids for new nslp_aggregate.
  * @param type aggregator type: AGGREGATOR or DEAGGREGATOR.
  */
  nslp_aggregate::nslp_aggregate(vector<sessionid*> sid, aggregator_type_t type, ntlp::mri_pathcoupled* my_mri) :
  aggr_type(type)
{
  session_context_map = Context_Manager::instance()->get_scm();
  NSLP_Session_Context *session_context = NULL;
  qspec_object *q;
  sessionid* my_sid = new sessionid();
  my_sid->generate_random();
  set_s_id(my_sid);
  aggregator_qspec = new qspec_object();
  counter = 0;
  for (uint32 i = 0; i < sid.size(); i++) {
    const sessionid* temp_sid = sid[i];
    sid_hashmap[temp_sid->get_hash()] = temp_sid->copy();
    counter++;
    session_context = session_context_map->find_session_context(*temp_sid) ;
    if (session_context != NULL) {
      if (type == nslp_aggregate::AGGREGATOR) {
        session_context->lock();
	NSLP_Flow_Context *flow_context = session_context->find_flow_context(*my_mri);
	if (flow_context != NULL) {
	  q = flow_context->get_qspec().copy();
	  add_qspec(q);
	  session_context->set_is_in_an_aggregate(true);
	  session_context->set_aggregate_sid(*my_sid);
	  q = NULL;
	}
	session_context->unlock();
      }
    }
    temp_sid = NULL;
  }

  // XXX: What does this piece of code? I think rather nothing
  // since the instantiated object "context" is never used!
  /*
  context = new NSLP_Context();
  context->set_s_id(*my_sid);
  context->set_mri(my_mri);
  //context->set_aggregator_type(type);
  context->set_aggregate(this);
  */
}

/** Destructor for nslp_aggregate.  */
nslp_aggregate::~nslp_aggregate()
{
  aggregator_qspec = NULL;
  aggregate_sid = NULL;
  aggregator_mri = NULL;
}

/** Set SESSIONID for the current nslp_aggregate.
  * @param sid the SESSIONID for the current nslp_aggregate.
  */
void nslp_aggregate::set_s_id(sessionid* sid) {
  delete aggregate_sid;
  aggregate_sid= sid;
}

/** Get SESSIONID of the current nslp_aggregate.
  * @param sid the SESSIONID of the current nslp_aggregate will be written into this variable.
  */
void nslp_aggregate::get_s_id(sessionid* sid) {
  *sid= *aggregate_sid;
}

/** Increment counter of SESSIONIDs of the current nslp_aggregate. */
void nslp_aggregate::increment_counter()
{
  counter++;
}
	
/** Decrement counter of SESSIONIDs of the current nslp_aggregate. */
void nslp_aggregate::decrement_counter()
{
  counter--;
}
	
/** Get counter of SESSIONIDs of the current nslp_aggregate.
  * @return sid counter of SESSIONIDs of the current nslp_aggregate.
  */
uint32 nslp_aggregate::get_counter()
{
  return counter;
}
	
/** Set counter of SESSIONIDs of the current nslp_aggregate.
  * @param c counter of SESSIONIDs for the current nslp_aggregate.
  */
void nslp_aggregate::set_counter(uint32 c)
{
  counter = c;
}
	
/** Insert SESSIONID into current nslp_aggregate.
  * @param sid SESSIONID to be inserted into current nslp_aggregate.
  * @return TRUE if the insertion was successfull.
  */
bool nslp_aggregate::insert_sid_into_aggregate(sessionid* sid)
{
  return true;
}

/** Delete SESSIONID from current nslp_aggregate.
  * @param sid SESSIONID to be deleted from current nslp_aggregate.
  * @return TRUE if the deletion was successfull.
  */	
bool nslp_aggregate::delete_sid_from_aggregate(sessionid* sid)
{
  return true;
}
	
/** Check for SESSIONID in current nslp_aggregate.
  * @param sid SESSIONID to be found in the current nslp_aggregate.
  * @return TRUE if the sid was found.
  */
bool nslp_aggregate::check_for_sid_in_aggregate(sessionid* sid)
{
  return true;
}

/** Add QSPEC to current nslp_aggregate.
  * @param q QSPEC to be added to the current nslp_aggregate.
  */
void nslp_aggregate::add_qspec(qspec_object* q) 
{
  // add the given QSPEC to the local QSPEC
}

/** Set aggregator type of the current nslp_aggregate.
  * @param type aggregator type: AGGREGATOR or DEAGGREGATOR.
  */
void nslp_aggregate::set_aggregator_type(nslp_aggregate::aggregator_type_t type)
{
  aggr_type = type;
}

/** Get aggregator type of the current nslp_aggregate.
  * @return aggregator type of the current nslp_aggregate.
  */	
nslp_aggregate::aggregator_type_t nslp_aggregate::get_aggregator_type()
{
  return aggr_type;
}

/** Set MRI for the current nslp_aggregate.
  * @param set_mri MRI for the current nslp_aggregate.
  */
void nslp_aggregate::set_mri(ntlp::mri_pathcoupled* set_mri)
{
  aggregator_mri = set_mri->copy();
}
	
/** Get MRI of the current nslp_aggregate.
  * @return MRI of the current nslp_aggregate.
  */
ntlp::mri_pathcoupled* nslp_aggregate::get_mri()
{
  return aggregator_mri->copy();
}

//@}

} // end namespace qos_nslp
