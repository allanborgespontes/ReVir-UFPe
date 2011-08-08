/// ----------------------------------------*- mode: C++; -*--
/// @file session_id_list.h
/// QoS NSLP Session ID List
/// ----------------------------------------------------------
/// $Id: session_id_list.h 5253 2010-04-26 13:41:09Z roehricht $
/// $HeadURL: https://svn.tm.kit.edu/nsis/qos-nslp/trunk/include/session_id_list.h $
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
/** @ingroup sid_list
 * @file
 * NSLP SESSION_ID_LIST object
 */

#ifndef SESSION_ID_LIST_H_
#define SESSION_ID_LIST_H_

#include "protlib_types.h"
#include "nslp_object.h"
#include "messages.h"

#include <list>

using namespace protlib;

namespace qos_nslp {

/** @ingroup iesid_list
 * @ingroup ienslpobject
 * @{
 */

/// NSLP SESSION_ID_LIST
class sid_list : public known_nslp_object {
/***** inherited from IE ****/
public:
	virtual sid_list* new_instance() const;
	virtual sid_list* copy() const;
	virtual sid_list* deserialize(NetMsg& msg, coding_t cod, IEErrorList& errorlist, uint32& bread, bool skip = true);
	virtual void serialize(NetMsg& msg, coding_t cod, uint32& wbytes) const;
	virtual bool check() const;
	virtual size_t get_serialized_size(coding_t coding) const;
	virtual bool operator==(const IE& ie) const;
	virtual const char* get_ie_name() const;
	virtual ostream& print(ostream& os, uint32 level, const uint32 indent, const char* name = NULL) const;
	virtual istream& input(istream& is, bool istty, uint32 level, const uint32 indent, const char* name = NULL);
/***** new members *****/
public:
	// @{
	sid_list();
	sid_list(uint128 *sid_arr, int count);
	// @}
	/// copy constructor
	sid_list(const sid_list& n);
	/// assignment
	sid_list& operator=(const sid_list& n);
	// @{
	void add_sid(uint128 sid);
	list<uint128> get_sid_list();
	int get_sid_count();
	// @}
	// @{
	/// tests
	bool is_sid_list() const;
	// @}
private:
	list<uint128> sid_list_list;		// better name?!?
	static const char* const iename;
}; // end rii

//@}

} // end namespace qos_nslp

#endif /*SESSION_ID_LIST_H_*/
