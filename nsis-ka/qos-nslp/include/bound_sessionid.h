/// ----------------------------------------*- mode: C++; -*--
/// @file bound_sessionid.h
/// QoS NSLP Bound Session ID Object
/// ----------------------------------------------------------
/// $Id: bound_sessionid.h 5253 2010-04-26 13:41:09Z roehricht $
/// $HeadURL: https://svn.tm.kit.edu/nsis/qos-nslp/trunk/include/bound_sessionid.h $
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
/** @ingroup iebound_sessionid
 * @file
 * NSLP BOUND_SESSIONIDObject
 */

#ifndef _NSLP__BOUND_SESSIONID_H_
#define _NSLP__BOUND_SESSIONID_H_

#include "protlib_types.h"
#include "nslp_object.h"

using namespace protlib;

namespace qos_nslp {

/** @addtogroup iebound_sessionid Session Identification
 * @ingroup ienslpobject
 * @{
 */

/// NSLP BOUND_SESSIONID
class bound_sessionid : public known_nslp_object {
/***** inherited from IE ****/
public:
	virtual bound_sessionid* new_instance() const;
	virtual bound_sessionid* copy() const;
	virtual bound_sessionid* deserialize(NetMsg& msg, coding_t cod, IEErrorList& errorlist, uint32& bread, bool skip = true);
	virtual void serialize(NetMsg& msg, coding_t cod, uint32& wbytes) const;
	virtual bool check() const;
	virtual size_t get_serialized_size(coding_t coding) const;
	virtual bool operator==(const IE& ie) const;
	virtual const char* get_ie_name() const;
	virtual ostream& print(ostream& os, uint32 level, const uint32 indent, const char* name = NULL) const;
	virtual istream& input(istream& is, bool istty, uint32 level, const uint32 indent, const char* name = NULL);
/***** new members *****/
public:

	// specific flags for RESERVE message
        #define TUNNEL_SESSION           0x01   // 0000 0001
        #define BIDIRECT_SESSION         0x02   // 0000 0010
        #define AGGREGATE_SESSION        0x03   // 0000 0011
	#define DEPENDENT_SESSION        0x04   // 0000 0100

	static const uint128 bound_sessioniddefault;
	// @{
	/// constructor
	bound_sessionid();
	bound_sessionid(uint128 r);
	// @}
	/// copy constructor
	bound_sessionid(const bound_sessionid& n);
	/// assignment
	bound_sessionid& operator=(const bound_sessionid& n);
	/// set to default
	void set_default();
	/// get bound_sessionid
	void get(uint128& r) const;
	// @{
	/// set bound_sessionid
	void set(const uint128& r);
	// @}
	// @{
	/// tests
	bool is_bound_sid() const;
	// @}
	void generate_new();
	void set_bit_field(uint32 bits);
	uint8 get_binding_code();
	void set_binding_code(uint8 b_code);

	void set_tunnel_session();
	void set_bidirectional_session();
	void set_aggregate_session();
	void set_dependent_session();

	bool is_tunnel_session();
	bool is_bidirectional_session();
	bool is_aggregate_session();
	bool is_dependent_session();

	operator uint128() const { return bound_sessionidref; };

private:
	uint128 bound_sessionidref;
	uint32 bit_field;
	uint8 binding_code;
	static const char* const iename;
	static const uint16 contlen;
}; // end bound_sessionid

//@}

} // end namespace qos_nslp

#endif // _NSLP__BOUND_SESSIONID_H_
