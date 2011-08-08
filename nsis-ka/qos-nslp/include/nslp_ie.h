/// ----------------------------------------*- mode: C++; -*--
/// @file nslp_ie.h
/// NSLP Information Elements (PDU objects)
/// ----------------------------------------------------------
/// $Id: nslp_ie.h 6159 2011-05-18 15:24:52Z bless $
/// $HeadURL: https://svn.tm.kit.edu/nsis/qos-nslp/trunk/include/nslp_ie.h $
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

#ifndef _NSLP__IE_H_
#define _NSLP__IE_H_

#include "ie.h"
//#include "marsp_ie.h" // for IEErrorWithString

//using namespace protlib;

namespace qos_nslp {
    using namespace protlib;

/** @addtogroup ie Information Elements
 * @{
 */

class nslp_pdu;
class nslp_object;

/** Abstract Information Element (IE) interface
 */
class NSLP_IE : public protlib::IE {
public:
/// IE category
	/** IE types and subtype values are not unique in the protocol.
	 * Therefore IEs are categorized.
	 * There is no cathegory hierarchy as type-subtype patterns.
	 *
	 * Categories must have contiguous values starting at 0 because
	 * the IE Manager holds an array of hash_maps - one for each category.
	 */
	enum category_t {
		cat_known_pdu           = 0,
		cat_unknown_pdu         = 1,
		cat_known_pdu_object    = 2,
		cat_known_nslp_object	= 3,
		cat_unknown_nslp_object	= 4,
		cat_known_nslp_pdu	= 5,
		cat_unknown_nslp_pdu	= 6,
		cat_ulp_data            = 7,
		cat_raw_protocol_object = 8,
		/// number of known IE categories
		num_of_categories = 9
	};
  
  enum action_type_t {
    mandatory  = 0,	// 00
    ignore     = 1,	// 01
    forward    = 2,	// 10
    refresh    = 3	// 11
  };

protected:
	NSLP_IE(uint16 cat) : IE(cat) {};
	/// copy constructor
	NSLP_IE(const NSLP_IE& n) : IE(n) {} ;
public:
	/// get a new instance of the IE
	virtual IE* new_instance() const = 0;
	/// copy an IE
	virtual IE* copy() const = 0;
	/// deserialization
	virtual IE* deserialize(NetMsg& msg, coding_t coding, IEErrorList& errorlist, uint32& bread, bool skip = true) = 0;
	/// serialize
	virtual void serialize(NetMsg& msg, coding_t coding, uint32& wbytes) const = 0;
public:
	/// check if IE supports coding sheme
	virtual bool supports_coding(coding_t c) const;
	/// IE consistency check
	virtual bool check() const = 0;
	/// IE serialized size
	virtual size_t get_serialized_size(coding_t coding) const= 0;
	/// equality
	virtual bool operator==(const IE& ie) const = 0;
	/** This is mainly for error reporting. */
	virtual const char* get_ie_name() const = 0;
}; // end class IE

/** Check if an IE supports the given coding sheme. */
/* currently all elements are v1 only */
inline
bool
NSLP_IE::supports_coding(coding_t c) const {
	return (c==nslp_v1);
} // end supports_coding

/// unexpected object
  /*
class UnexpectedNSLPObject : public IEErrorWithString {
public:
	/// constructor
	UnexpectedNSLPObject(IE::coding_t cod, const nslp_object& obj, const nslp_pdu& pdu, uint32 pos);
	const IE::coding_t coding;
	const uint8 objecttype;
	const uint8 objectsubtype;
	const uint8 pdutype;
	const uint16 pdusubtype;
	const uint32 errorpos;
}; // end class UnexpectedObject
  */

/// IE Manager singleton
/** Each IE has to register at the IE Manager. The IEManager then provides
 * methods to deserialize IEs from a NetMsg object.
 */
class NSLP_IEManager : public IEManager {
public:
  virtual IE*  deserialize(NetMsg& msg, uint16 cat, IE::coding_t coding, IEErrorList& errorlist, uint32& bread, bool skip);

  /// return IEManager singleton instance
  static NSLP_IEManager* instance();
  /// clear IEManager
  static void clear();

private:
        NSLP_IEManager(); // private to prevent instantiation
        static NSLP_IEManager *qos_nslp_inst;
	IE* deserialize_known_nslp_pdu(NetMsg& msg, IE::coding_t coding, uint32 saved_pos, uint32 bleft, IEErrorList& errorlist, uint32& bread, bool skip = true);
	IE* deserialize_known_nslp_object(NetMsg& msg, IE::coding_t coding, uint32 saved_pos, uint32 bleft, IEErrorList& errorlist, uint32& bread, bool skip = true);
	//IE* deserialize_raw_nslp_object(NetMsg& msg, IE::coding_t coding, uint32 saved_pos, uint32 bleft, IEErrorList& errorlist, uint32& bread, bool skip = true);
	//IE* deserialize_service(NetMsg& msg, IE::coding_t coding, uint32 saved_pos, uint32 bleft, IEErrorList& errorlist, uint32& bread, bool skip = true);

	/// lookup IE
        IE* lookup(uint16 cat, uint16 type, uint16 subtype = 0);

}; // end class NSLP_IEManager

//@}

} // end namespace protlib

#endif // _NSLP__IE_H_
