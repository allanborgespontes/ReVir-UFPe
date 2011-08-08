/// ----------------------------------------*- mode: C++; -*--
/// @file aggregate.h
/// QoS NSLP Aggregate
/// ----------------------------------------------------------
/// $Id: aggregate.h 5881 2011-02-16 13:16:03Z stud-mueller $
/// $HeadURL: https://svn.tm.kit.edu/nsis/qos-nslp/trunk/include/aggregate.h $
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
/** @ingroup ienslpaggregate
 * @file
 * NSLP NSLPaggregate
 */

#ifndef _NSLP__AGGREGATE_H_
#define _NSLP__AGGREGATE_H_

#include <vector>

#include "sessionid.h"
#include "qspec.h"
#include "qos_aggr_object.h"
#include "mri_pc.h"

using namespace protlib;
using namespace ntlp;

namespace qos_nslp {

class NSLP_Session_Context_Map;

/** @addtogroup ienslpaggregate NSLP Aggregate
 * @ingroup ienslpaggregate
 * @{
 */

/// NSLP Aggregate
class nslp_aggregate {

public:
        enum aggregator_type_t {
	  not_set      = 0,
	  AGGREGATOR   = 1,
	  DEAGGREGATOR = 2
	}; // end aggregator_type_t

        virtual nslp_aggregate* new_instance() const;
	/// constructor
	nslp_aggregate();
	nslp_aggregate(vector<sessionid*> sid, aggregator_type_t type, ntlp::mri_pathcoupled* my_mri);
	virtual ~nslp_aggregate();

	void set_s_id(sessionid* sid);
	void get_s_id(sessionid* sid);
	void increment_counter();
	void decrement_counter();
	uint32 get_counter();
	void set_counter(uint32 c);
	void add_qspec(qspec_object* q);
	bool insert_sid_into_aggregate(sessionid* sid);
	bool delete_sid_from_aggregate(sessionid* sid);
	bool check_for_sid_in_aggregate(sessionid* sid);
	void set_aggregator_type(nslp_aggregate::aggregator_type_t type);
	nslp_aggregate::aggregator_type_t get_aggregator_type();
	void set_mri(ntlp::mri_pathcoupled* set_mri);
	ntlp::mri_pathcoupled* get_mri();
	
	sidmap_t sid_hashmap;
private:
	sessionid* aggregate_sid;
	qspec_object* aggregate_qspec;
	aggregator_type_t aggr_type;
	ntlp::mri_pathcoupled*  aggregator_mri;
	qspec_object* aggregator_qspec;
	uint32 counter;
	// reference to nslp_session_context_map
	NSLP_Session_Context_Map* session_context_map;
}; // end nslp_aggregate

//@}

} // end namespace qos_nslp

#endif // _NSLP__AGGREGATE_H_
