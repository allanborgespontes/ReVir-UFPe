/// ----------------------------------------*- mode: C++; -*--
/// @file ntlp_ie.h
/// NTLP/GIST specific information elements (PDU objects)
/// ----------------------------------------------------------
/// $Id: ntlp_ie.h 6195 2011-05-25 13:31:46Z bless $
/// $HeadURL: https://svn.tm.kit.edu/nsis/ntlp/trunk/include/ntlp_ie.h $
// ===========================================================
//                      
// Copyright (C) 2005-2010, all rights reserved by
// - Institute of Telematics, Karlsruhe Institute of Technology
//
// More information and contact:
// https://projekte.tm.uka.de/trac/NSIS
//                      
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; version 2 of the License
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
//
// ===========================================================
#ifndef _NTLP__IE_H_
#define _NTLP__IE_H_

#include "ie.h"


namespace ntlp {
using namespace protlib;



// maximum NSLP payload MTU 576 is used as upper limit for UDP messages.
//
// Should be calculated as follows: 576 - UDPhdr - GIST Common Header - max. possible GIST TLV object lengths
//
// until correct calculation is possible, use 400Byte as upper limit
//
static const uint32 MAX_NSLP_PAYLOAD = 400;



/** @addtogroup ie Information Elements
 * @{
 */

class ntlp_pdu;
class ntlp_object;

/** Abstract Information Element (IE) interface
 */
class NTLP_IE : public protlib::IE {
public:
	enum category_t {
		cat_known_pdu		= 0,
		cat_unknown_pdu		= 1,
		cat_known_pdu_object	= 2,
		cat_raw_protocol_object	= 3,
		cat_ulp_data		= 4
	};
protected:
	NTLP_IE(NTLP_IE::category_t cat) : IE(cat) {};
	/// copy constructor
	NTLP_IE(const NTLP_IE& n) : IE(n) {} ;
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
	/// check if IE supports coding scheme
	virtual bool supports_coding(coding_t c) const;
	/// IE consistency check
	virtual bool check() const = 0;
	/// IE serialized size
	virtual size_t get_serialized_size(coding_t coding) const= 0;
	/// equality
	virtual bool operator==(const IE& ie) const = 0;
	/** This is mainly for error reporting. */
	virtual const char* get_ie_name() const = 0;
}; // end class NTLP_IE

/** Check if an IE supports the given coding sheme. */
/* currently all elements are v1 only */
inline
bool 
NTLP_IE::supports_coding(coding_t c) const {
	return (c==protocol_v1);
} // end supports_coding



// IEError

class IEHmacError : public IEError {
  public:
	IEHmacError() : IEError(IEError::ERROR_INVALID_STATE) { }
	virtual const char *getstr() const { return "Hmac check failed or Hmac preconditions are not observed."; }
	virtual ~IEHmacError() throw () { }
};



/// IE Manager singleton
/** Each IE has to register at the IE Manager. The IEManager then provides
 * methods to deserialize IEs from a NetMsg object.
 */
class NTLP_IEManager : public IEManager {
  public:
	/// return NTLP_IEManager singleton instance
	static NTLP_IEManager *instance();
	/// clear IEManager 
	static void clear();

	virtual IE *deserialize(NetMsg& msg, uint16 cat, IE::coding_t coding,
			IEErrorList& errorlist, uint32& bread, bool skip);

  protected:
	virtual IE *lookup_ie(uint16 category, uint16 type, uint16 subtype);

  private:
	NTLP_IEManager(); // private to prevent instantiation

	static NTLP_IEManager *ntlp_inst;

	IE* deserialize_known_ntlp_pdu(NetMsg& msg, IE::coding_t coding, uint32 saved_pos, uint32 bleft, IEErrorList& errorlist, uint32& bread, bool skip = true);
	IE* deserialize_known_ntlp_object(NetMsg& msg, IE::coding_t coding, uint32 saved_pos, uint32 bleft, IEErrorList& errorlist, uint32& bread, bool skip = true);
	IE* deserialize_raw_ntlp_object(NetMsg& msg, IE::coding_t coding, uint32 saved_pos, uint32 bleft, IEErrorList& errorlist, uint32& bread, bool skip = true);
	IE* deserialize_service(NetMsg& msg, IE::coding_t coding, uint32 saved_pos, uint32 bleft, IEErrorList& errorlist, uint32& bread, bool skip = true);

}; // end class NTLP_IEManager


class GIST_Error : public IEError {
public:
  // GIST error codes/classes during deserialization
  enum gist_error_code_t {	
		error_gist_incorrect_msg_length,
		error_gist_message_too_large,
		error_gist_duplicate_object,
		error_gist_unrecognised_object,
		error_gist_missing_object,
		error_gist_invalid_object,
		error_gist_incorrect_object_length,
		error_gist_value_not_supported,
		error_gist_invalid_flag_field_combination,
		error_gist_empty_list,
		error_gist_sp_scd_mismatch,
		error_gist_unknown_version,
		error_gist_unknown_type,
		error_gist_invalid_r_flag,
		error_gist_invalid_e_flag,
		error_gist_invalid_c_flag,
		error_gist_missing_magic_number,
		error_gist_no_routing_state,
		error_gist_incorrect_encapsulation,
		error_gist_hop_limit_exceeded,
		error_gist_endnode_found,
		error_gist_unknown_nslpid,
		error_gist_invalid_ip_ttl,
		error_gist_invalid_qcookie,
		error_gist_invalid_rcookie,
		error_gist_incorrectly_delivered_message,
		error_gist_untranslated_objects,
		error_gist_mri_too_wild,
		error_gist_MRI_ip_version_mismatch
  };

  GIST_Error() : IEError(IEError::ERROR_PROT_SPECIFIC) {};
  virtual gist_error_code_t errorcode() const = 0;
  virtual const char*getdescription() const = 0;
};


/// IncorrectMsgLength
class GIST_IncorrectMsgLength : public GIST_Error {
public:
    /// constructor
    GIST_IncorrectMsgLength(IE::coding_t cod, uint8 version, uint8 hops, uint16 length, uint16 nslpid, uint8 type, uint8 flags);
    const IE::coding_t coding;
    const uint8 version;
    const uint8 hops;
    const uint16 length;
    const uint16 nslpid;
    const uint8 type;
    const uint8 flags;

  gist_error_code_t errorcode() const { return error_gist_incorrect_msg_length; }
  const char*getdescription() const { return "Incorrect Message Length"; }

}; // end class IncorrectMsgLength

/// MessageTooLarge
class GIST_MessageTooLarge : public GIST_Error {
public:
	/// constructor
	GIST_MessageTooLarge(IE::coding_t cod, const ntlp_object& obj, const ntlp_pdu& pdu, uint32 pos); // 
	const IE::coding_t coding;
	const uint16 objecttype;
	const uint8 objectsubtype;
	const uint8 pdutype;
	const uint16 pdusubtype;
	const uint32 errorpos;

  gist_error_code_t errorcode() const { return error_gist_message_too_large; }
  const char*getdescription() const { return "Message Too Large"; }
}; // end class MessageTooLarge


/// Duplicate object
class GIST_DuplicateObject : public GIST_Error {
public:
	/// constructor
	GIST_DuplicateObject(IE::coding_t cod, uint8 pdutype, uint16 objecttype);
	const IE::coding_t coding;
	const uint16 objecttype;
	const uint8 pdutype;

  gist_error_code_t errorcode() const { return error_gist_duplicate_object; }
  const char*getdescription() const { return "Duplicate Object"; }
}; // end class Duplicate


/// Unrecognised object
class GIST_UnrecognisedObject : public GIST_Error {
public:
	/// constructor
	GIST_UnrecognisedObject(IE::coding_t cod, uint16 objecttype);
	const IE::coding_t coding;
	const uint16 objecttype;

  gist_error_code_t errorcode() const { return error_gist_unrecognised_object; }
  const char*getdescription() const { return "Unrecognised Object"; }
}; // end class Unrecognised


/// Missing object
class GIST_MissingObject : public GIST_Error {
public:
	/// constructor
	GIST_MissingObject(IE::coding_t cod, uint8 pdutype, uint16 objecttype);
	const IE::coding_t coding;
	const uint16 objecttype;
	const uint8 pdutype;

  gist_error_code_t errorcode() const { return error_gist_missing_object; }
  const char*getdescription() const { return "Missing Object"; }
}; // end class Missing


/// Invalid object
class GIST_InvalidObject : public GIST_Error {
public:
    /// constructor
    GIST_InvalidObject(IE::coding_t cod, uint8 pdutype, uint16 objecttype);
    const IE::coding_t coding;
    const uint16 objecttype;
    const uint8 pdutype;

  gist_error_code_t errorcode() const { return error_gist_invalid_object; }
  const char*getdescription() const { return "Invalid Object"; }
}; // end class 


/// Incorrect object length object
class GIST_IncorrectObjectLength : public GIST_Error {
public:
    /// constructor
    GIST_IncorrectObjectLength(IE::coding_t cod, const ntlp_object* obj);
    const IE::coding_t coding;
    ntlp_object* obj;

  gist_error_code_t errorcode() const { return error_gist_incorrect_object_length; }
  const char*getdescription() const { return "Incorrect Object Length"; }
}; // end class UnexpectedObject

/// Value not supported object
class GIST_ValueNotSupported : public GIST_Error {
public:
	/// constructor
	GIST_ValueNotSupported(IE::coding_t cod, const ntlp_object* object);
	const IE::coding_t coding;
	ntlp_object* obj;

  gist_error_code_t errorcode() const { return error_gist_value_not_supported; }
  const char*getdescription() const { return "Value Not Supported"; }
}; // end class 

/// unexpected InvalidFlagFieldCombination
class GIST_InvalidFlagFieldCombination : public GIST_Error {
public:
	/// constructor
	GIST_InvalidFlagFieldCombination(IE::coding_t cod, const ntlp_object* object);
	const IE::coding_t coding;
	ntlp_object* obj;

  gist_error_code_t errorcode() const { return error_gist_invalid_flag_field_combination; }
  const char*getdescription() const { return "Invalid Flag Field Combination"; }
}; // end class


/// Empty List object
class GIST_EmptyList : public GIST_Error {
public:
	/// constructor
	GIST_EmptyList(IE::coding_t cod, const ntlp_object* object);
	const IE::coding_t coding;
	ntlp_object* obj;

  gist_error_code_t errorcode() const { return error_gist_empty_list; }
  const char*getdescription() const { return "Empty Object List"; }
}; // end class 


/// InvalidIPTTL object
class GIST_InvalidIPTTL : public GIST_Error {
public:
	/// constructor
	GIST_InvalidIPTTL(IE::coding_t cod);
	const IE::coding_t coding;

  gist_error_code_t errorcode() const { return error_gist_invalid_ip_ttl; }
  const char*getdescription() const { return "Invalid IP TTL"; }
}; // end class 

/// IncorrectEncapsulation object
class GIST_IncorrectEncapsulation : public GIST_Error {
public:
	/// constructor
	GIST_IncorrectEncapsulation(IE::coding_t cod);
	const IE::coding_t coding;

  gist_error_code_t errorcode() const { return error_gist_incorrect_encapsulation; }
  const char*getdescription() const { return "Incorrect Encapsulation"; }
}; // end class IncorrectEncapsulationObject



/// SpScdMismatchobject
class GIST_SpScdMismatch : public GIST_Error {
public:
	/// constructor
	GIST_SpScdMismatch(IE::coding_t cod, const ntlp_object& obj, const ntlp_pdu& pdu, uint32 pos);
	const IE::coding_t coding;
	const uint16 objecttype;
	const uint8 objectsubtype;
	const uint8 pdutype;
	const uint16 pdusubtype;
	const uint32 errorpos;

  gist_error_code_t errorcode() const { return error_gist_sp_scd_mismatch; }
  const char*getdescription() const { return "Stack Proposal/Stack Configuration"; }
}; // end class SpScdMismatch

/// MRI_IPVerMismatch object
class GIST_MRI_IPVerMismatch : public GIST_Error {
public:
	/// constructor
	GIST_MRI_IPVerMismatch(IE::coding_t cod) : coding(cod) {} ;
	const IE::coding_t coding;

  gist_error_code_t errorcode() const { return error_gist_MRI_ip_version_mismatch; }
  const char*getdescription() const { return "MRI IP Version Mismatch"; }
}; // end class MRI_IPVerMismatch



/// UnknownVersion object
class GIST_UnknownVersion : public GIST_Error {
public:
    /// constructor
    GIST_UnknownVersion(IE::coding_t cod, uint8 version, uint8 hops, uint16 length, uint16 nslpid, uint8 type, uint8 flags);
    const IE::coding_t coding;
    const uint8 version;
    const uint8 hops;
    const uint16 length;
    const uint16 nslpid;
    const uint8 type;
    const uint8 flags;

  gist_error_code_t errorcode() const { return error_gist_unknown_version; }
  const char*getdescription() const { return "Unknown Version"; }
}; // end class UnknownVersion


/// unknown type object
class GIST_UnknownType : public GIST_Error {
public:
    /// constructor
    GIST_UnknownType(IE::coding_t cod, uint8 version, uint8 hops, uint16 length, uint16 nslpid, uint8 type, uint8 flags);
    const IE::coding_t coding;
    const uint8 version;
    const uint8 hops;
    const uint16 length;
    const uint16 nslpid;
    const uint8 type;
    const uint8 flags;

  gist_error_code_t errorcode() const { return error_gist_unknown_type; }
  const char*getdescription() const { return "Unknown Type"; };
}; // end class 


// Invalid R flag object
class GIST_InvalidRFlag : public GIST_Error {
public:
    /// constructor
    GIST_InvalidRFlag(IE::coding_t cod, uint8 version, uint8 hops, uint16 length, uint16 nslpid, uint8 type, uint8 flags);
    const IE::coding_t coding;
    const uint8 version;
    const uint8 hops;
    const uint16 length;
    const uint16 nslpid;
    const uint8 type;
    const uint8 flags;

  gist_error_code_t errorcode() const { return error_gist_invalid_r_flag; }
  const char*getdescription() const { return "Invalid R Flag"; }
}; // end class InvalidRFlag


// Invalid E flag  object
class GIST_InvalidEFlag : public GIST_Error {
public:
    /// constructor
    GIST_InvalidEFlag(IE::coding_t cod, uint8 version, uint8 hops, uint16 length, uint16 nslpid, uint8 type, uint8 flags);
    const IE::coding_t coding;
    const uint8 version;
    const uint8 hops;
    const uint16 length;
    const uint16 nslpid;
    const uint8 type;
    const uint8 flags;

  gist_error_code_t errorcode() const { return error_gist_invalid_e_flag; }
  const char*getdescription() const { return "Invalid E Flag"; }
}; // end class InvalidEFlag


// Invalid C flag  object
class GIST_InvalidCFlag : public GIST_Error {
public:
    /// constructor
    GIST_InvalidCFlag(IE::coding_t cod, uint8 version, uint8 hops, uint16 length, uint16 nslpid, uint8 type, uint8 flags);
    const IE::coding_t coding;
    const uint8 version;
    const uint8 hops;
    const uint16 length;
    const uint16 nslpid;
    const uint8 type;
    const uint8 flags;

  gist_error_code_t errorcode() const { return error_gist_invalid_c_flag; }
  const char*getdescription() const { return "Invalid C Flag"; }
}; // end class InvalidCFlag



//@}

} // end namespace protlib

#endif // _NTLP__IE_H_
