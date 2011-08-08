/// ----------------------------------------*- mode: C++; -*--
/// @file qos_aggr_object.h
/// QoS NSLP Aggregation Object
/// ----------------------------------------------------------
/// $Id: qos_aggr_object.h 6282 2011-06-17 13:31:57Z bless $
/// $HeadURL: https://svn.tm.kit.edu/nsis/qos-nslp/trunk/include/qos_aggr_object.h $
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
/** @ingroup ieqosaggrobject
 * @file
 * NSLP QoSaggregatorObject
 */

#ifndef NSLP_QOS_AGGREGATOR_H
#define NSLP_QOS_AGGREGATOR_H

#include "sessionid.h"

using namespace protlib;

namespace qos_nslp {

/** @addtogroup ieqosaggrobject QoS AGGREGATOR hash maps
 * @{
 */
	/// nslp_aggregate hash_maps
	typedef hashmap_t<uint64, ntlp::sessionid*> sidmap_t;
	typedef sidmap_t::iterator sid_hashmap_it_t;
//@}
} // end namespace qos_nslp

#endif // NSLP_QOS_AGGREGATOR_H
