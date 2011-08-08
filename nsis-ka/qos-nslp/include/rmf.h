/// ----------------------------------------*- mode: C++; -*--
/// @file rmf.h
/// QoS NSLP Resource Management Function
/// ----------------------------------------------------------
/// $Id: rmf.h 6159 2011-05-18 15:24:52Z bless $
/// $HeadURL: https://svn.tm.kit.edu/nsis/qos-nslp/trunk/include/rmf.h $
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
/** @ingroup iermf
 * @file
 * NSLP RMF
 */

#ifndef _NSLP__RMF_H_
#define _NSLP__RMF_H_

#include "protlib_types.h"
#include "qspec.h"
#include "qspec_pdu.h"
#include "vlsp_object.h"

using namespace protlib;
using namespace qspec;

namespace qos_nslp {

/** @addtogroup iermf Resource Management Function
 * @{
 */

/// NSLP RMF
class rmf {

public:
	static const uint32 max_avail_bandwidth_default;
	static const uint32 reserved_default;
	long static AF_available_bandwidth;
	long static EF_available_bandwidth;
	long static EF_res_avg_rate;
	long static BE_available_bandwidth;
	// @{
	/// constructor
	rmf();
	rmf(uint32 qspec_id, uint32 required_bandwidth);
	// @}
	// @{
	/// manage bandwidth
	/// set max available bandwidth to default
	void set_default_max();
	/// set reserved bandwidth to default
	void set_default_reserved();
	/// get available bandwidth
	void get_avail_bandwidth(uint32& r);
	/// calculate available bandwidth
	void calculate_avail_bandwidth(uint32& r) const;
	/// set max available bandwidth
	void set_max_avail_bandwidth(uint32 r);
	/// get reserved bandwidth
	void get_reserved_bandwidth(uint32& r) const;
	/// reserve bandwidth
	bool reserve_bandwidth(uint32 r);
	/// free some bandwidth
	void release_bandwidth(uint32 r);
	// @}

	/// process qspec from QUERY msg
	void process_qspec_in_query(qspec_pdu* q);
	bool reserve_resources(const qspec_pdu* q);
	void release_resources(const qspec_pdu* q);
//	void set_AF_available_bandwidth(long a);

	/// process VLSP object
        void process_vlsp_object_setup_req(const vlsp_object* vlspobj);
        void process_vlsp_object_setup_resp(const vlsp_object* vlspobj);
	void process_vlsp_object_teardown(const vlsp_object* vlspobj);

private:
	uint32 max_available_bandwidth;
	uint32 reserved_bandwidth;
	uint32 available_bandwidth;
		
	
}; // end rmf

//@}

} // end namespace qos_nslp

#endif // _NSLP__RMF_H_
