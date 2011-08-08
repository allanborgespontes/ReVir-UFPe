/// ----------------------------------------*- mode: C++; -*--
/// @file refresh_period.h
/// QoS NSLP Refresh Period Object
/// ----------------------------------------------------------
/// $Id: refresh_period.h 5253 2010-04-26 13:41:09Z roehricht $
/// $HeadURL: https://svn.tm.kit.edu/nsis/qos-nslp/trunk/include/refresh_period.h $
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
/** @ingroup ierp
 * @file
 * NSLP RPObject
 */

#ifndef _NSLP__RP_H_
#define _NSLP__RP_H_

#include "protlib_types.h"
#include "nslp_object.h"

using namespace protlib;

namespace qos_nslp {

/** @addtogroup ierp Refresh Period
 * @ingroup ienslpobject
 * @{
 */

/// NSLP RP
class rp : public known_nslp_object {
/***** inherited from IE ****/
public:
	virtual rp* new_instance() const;
	virtual rp* copy() const;
	virtual rp* deserialize(NetMsg& msg, coding_t cod, IEErrorList& errorlist, uint32& bread, bool skip = true);
	virtual void serialize(NetMsg& msg, coding_t cod, uint32& wbytes) const;
	virtual bool check() const;
	virtual size_t get_serialized_size(coding_t coding) const;
	virtual bool operator==(const IE& ie) const;
	virtual const char* get_ie_name() const;
	virtual ostream& print(ostream& os, uint32 level, const uint32 indent, const char* name = NULL) const;
	virtual istream& input(istream& is, bool istty, uint32 level, const uint32 indent, const char* name = NULL);
/***** new members *****/
public:
	static const uint32 rpdefault;
	// @{
	/// constructor
	rp();
	rp(uint32 r);
	// @}
	/// copy constructor
	rp(const rp& n);
	/// assignment
	rp& operator=(const rp& n);
	/// set to default
	void set_default();
	/// get rp
	void get(uint32& r) const;
	// @{
	/// set rp
	void set(uint32 r);
	// @}
	// @{
	/// tests
	bool is_rp() const;
	void get_rand_wait(uint32 period, uint32& new_period);
	void get_rand_lifetime(uint32 period, uint32& new_lifetime);
	// @}
private:
	uint32 rpref;
	static const char* const iename;
	static const uint16 contlen;
}; // end rp

//@}

} // end namespace qos_nslp

#endif // _NSLP__RP_H_
