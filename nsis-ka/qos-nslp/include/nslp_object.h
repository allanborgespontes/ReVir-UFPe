/// ----------------------------------------*- mode: C++; -*--
/// @file nslp_object.h
/// NSLP object base classes and types
/// ----------------------------------------------------------
/// $Id: nslp_object.h 6159 2011-05-18 15:24:52Z bless $
/// $HeadURL: https://svn.tm.kit.edu/nsis/qos-nslp/trunk/include/nslp_object.h $
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
/** @addtogroup ienslpobject NSLP Objects
 * @ingroup ie
 * @{
 */

/** @ingroup ienslpobject
 * @file
 * NSLP object base classes
 */

#ifndef _NSLP__NSLPOBJECT_H_
#define _NSLP__NSLPOBJECT_H_

#include "protlib_types.h"
#include "nslp_ie.h"

using namespace protlib;

namespace qos_nslp {

/// NSLP object

/** Base class for (known and unknown) objects which can be part of a NSLP PDU.
 * Note that service specific data have their own category and base
 * class.
 *
 * Common Header:
 *  0                   1                   2                   3
 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |A|B|r|r|         Type          |r|r|r|r|        Length         |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 */
class nslp_object : public NSLP_IE {
/***** inherited from IE *****/
public:
	virtual nslp_object* new_instance() const = 0;
	virtual nslp_object* copy() const = 0;
public:
	/// max object content size
	static const uint16 max_size;
	/// object header length
	static const uint32 header_length;
#define	NSLP_OBJ_ACTION_MASK	0xC000	// ABrr Type(12) (Upper Two Bits)
#define	NSLP_OBJ_ACTION_SHIFT	14
        /// set action
	virtual void set_action(action_type_t a);
        /// get action
	virtual action_type_t get_action() const;

protected:
	/// constructor
	nslp_object(bool known, action_type_t a);
protected:
	/// action type
	action_type_t action;

public:
	/// number of object types
	static const uint32 num_of_types;

	/// decode header for NSLP_v1
        static void decode_header_nslpv1(NetMsg& m, action_type_t &a, uint16& t, uint16& dlen, uint32& ielen);
	/// get type
	virtual uint16 get_type() const = 0;
}; // end class nslp_object


/** @set NSLP object action. */
inline
void
nslp_object::set_action(action_type_t a) { action = a; }

/** @set NSLP object action. */
inline
nslp_object::action_type_t
nslp_object::get_action() const { return action; }


/// Known NSLP object

/** Base class for known objects which can be part of a NSLP PDU.
 */
class known_nslp_object : public nslp_object {
/***** inherited from IE *****/
protected:
	/// register this IE
	virtual void register_ie(IEManager *iem) const;
/***** new members *****/
public:
	/// object type
	/** All NSLP object types have to be listed here and their values should
	 * correspond with those sent over the wire. Otherwise they have to be
	 * mapped when (de)serializing.
	 * Type values must not exceed 12 bits in NSLP_V1.
	 * Look for IANA assignments
	 * http://www.iana.org/assignments/nslp-parameters/nslp-parameters.xml
	 */
#define	NSLP_OBJ_TYPE_MASK	0x0FFF	// ABrr Type(12) [rrrr Length(12)]
#define	NSLP_OBJ_LENGTH_MASK	0x0FFF	// [ABrr Type(12)] rrrr Length(12)
	// @{
	enum type_t {
		RII			= 0x01,
		RSN			= 0x02,
		REFRESH_PERIOD		= 0x03,
		BOUND_SESSION_ID	= 0x04,
		PACKET_CLASSIFIER	= 0x05,
		INFO_SPEC		= 0x06,
		SESSION_ID_LIST		= 0x07,
		RSN_LIST		= 0x08,
		MSG_ID                  = 0x09,
		BOUND_MSG_ID            = 0x0A,
		QSPEC			= 0x0B,
		SESSION_AUTH_OBJECT     = 0x16,
		VLSP_OBJECT		= 0xfe,	// experimental
		reserved          = 0xff
	}; // end type_t

	static bool is_known(uint16 t);
protected:
	/// constructor
	known_nslp_object(type_t t, action_type_t a);
	/// copy constructor
	known_nslp_object(const known_nslp_object& n);
	/// assignment
	known_nslp_object& operator=(const known_nslp_object& n);
public:
	/// get type
	virtual uint16 get_type() const;
	///check if type is rsn
	virtual bool is_rsn() const;
        ///check if type is packet_classifier
	virtual bool is_packet_classifier() const;
	///check if type is rii
	virtual bool is_rii() const;
	///check if type is rp
	virtual bool is_rp() const;
        ///check if type is sid
	virtual bool is_bound_sid() const;
        ///check if type is eo
	virtual bool is_eo() const;
        ///check if type is qspec
	virtual bool is_qspec() const;
	///check if type is sessionauth 
	virtual bool is_sessionauth() const;
	// check if type is vlsp object
	virtual bool is_vlsp_object() const;
	/// set type
	virtual void set_type(type_t t);
protected:
	/// object type
	type_t type;
	/// decode header for nslp_v1
	bool decode_header_nslpv1(NetMsg& msg, uint16& t, uint16& len, uint32& ielen, uint32& saved_pos, uint32& resume, IEErrorList& errorlist, uint32& bread, bool skip = true);
	/// encode header for nslp_v1
	void encode_header_nslpv1(NetMsg& msg, uint16 len) const;
	// @{
	/// report error
	void error_wrong_type(coding_t cod, uint16 t, uint32 msgpos, bool skip, IEErrorList& elist, uint32 resume, NetMsg& msg);
	void error_wrong_length(coding_t cod, uint16 len, uint32 msgpos, bool skip, IEErrorList& elist, uint32 resume, NetMsg& msg);
	// @}
public:

}; // end class known_nslp_object

inline
bool
known_nslp_object::is_rsn() const { return type == RSN; }

inline
bool
known_nslp_object::is_packet_classifier() const { return type == PACKET_CLASSIFIER; }

inline
bool
known_nslp_object::is_rii() const { return type == RII; }

inline
bool
known_nslp_object::is_rp() const { return type == REFRESH_PERIOD; }

inline
bool
known_nslp_object::is_bound_sid() const { return type == BOUND_SESSION_ID; }

inline
bool
known_nslp_object::is_eo() const { return type == INFO_SPEC; }

inline
bool
known_nslp_object::is_qspec() const { return type == QSPEC; }

inline 
bool 
known_nslp_object::is_sessionauth() const { return type == SESSION_AUTH_OBJECT; }

inline
bool
known_nslp_object::is_vlsp_object() const { return type == VLSP_OBJECT; }

/** @return NSLP object type. */
inline
uint16
known_nslp_object::get_type() const { return type; }

/** @set NSLP object type. */
inline
void
known_nslp_object::set_type(type_t t) { type = t; }

//@}

} // end namespace qos_nslp

#endif // _NSLP__NSLPOBJECT_H_

