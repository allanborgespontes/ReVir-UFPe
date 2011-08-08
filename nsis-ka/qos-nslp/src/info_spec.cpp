/// ----------------------------------------*- mode: C++; -*--
/// @file info_spec.cpp
/// NSLP error objects
/// ----------------------------------------------------------
/// $Id: info_spec.cpp 5755 2010-11-19 12:35:42Z bless $
/// $HeadURL: https://svn.tm.kit.edu/nsis/qos-nslp/trunk/src/info_spec.cpp $
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

#include "info_spec.h"
#include "logfile.h"

#include <iomanip>

using namespace protlib;
using namespace protlib::log;

namespace qos_nslp {

/** @addtogroup ieinfo_spec Error Objects
 * @ingroup ienslpobject
 * @{
 */

    //using namespace qos_qos_nslp::log;

/***** class info_spec *****/

/// Info_Spec default error strings
/** These strings are used when setting error class and error code of an
 * error object by default. You may set your own error string, too.
 */
const char *const info_spec::defstring[num_of_classes][num_of_codes] = {
	// number 0 is not used for error class
	{"Undefined Info_Spec (0 is unused)"},

	// Informational:
	{"", "Unknown BOUND_SESSION_ID: the message refers to an unknown SESSION_ID in its BOUND_SESSION_ID object.",
		"Route Change: possible route change occurred on downstream path.",
		"Reduced refreshes not supported, full QSPEC required.",
		"Congestion situation: Possible congestion situation occurred on downstream path.",
		"Unknown SESSION ID in SESSION_ID_LIST",
		"Mismatching RSN in RSN LIST"
#ifdef USE_AHO
		,
		"InitQuery: Query message requested to send downstream.",
		"InitReserve: Reserve message requested to send downstream."
#endif
		,
		"TearDownBranch: Tear down the reservation of an other flow."
       	},

	// Success
	{"", "Reservation successful",
		"Tear down successful",
		"Acknowledgement",
		"Refresh successful" },

	// Protocol Error
	{"", "Illegal message type: the type given in the Message Type field of the common header is unknown.", 
		"Wrong message length: the length given for the message does not match the length of the message data.",
		"Bad flags value: an undefined flag or combination of flags was set in the generic flags",
		"Bad flags value: an undefined flag or combination of flags was set in the message-specific flags",
		"Mandatory object missing: an object required in a message of this type was missing.",
		"Illegal object present: an object was present which must not be used in a message of this type.",
		"Unknown object present: an object of an unknown type was present in the message.",
		"Wrong object length: the length given for the object did not match the length of the object data present.",
		"RESERVE received from wrong direction.",
		"Unknown object field value: a field in an object had an unknown value.",
		"Duplicate object present.",
		"Malformed QSPEC.",
		"Unknown MRI.",
		"Erroneous value in the TLV object's value field.",
       		"Incompatible QSPEC" },

	// Transient Failure
	{"", "No GIST reverse-path forwarding state",
		"No path state for RESERVE, when doing a receiveroriented reservation",
		"RII conflict",
		"Full QSPEC required",
		"Mismatch synchronization between end-to-end RESERVE and intra-domain RESERVE",
		"Reservation preempted",
		"Reservation failure",
		"Path truncated - Next peer dead" },

	// Permanent Failure
	{"", "Internal or system error",
		"Authorization failure" }
}; // end defstring[][]

/***** IE name *****/

const char* const info_spec::iename = "info_spec";

const char* info_spec::get_ie_name() const { return iename; }

/***** inherited from IE *****/

/** Generate new INFO_SPEC instance.
  */
info_spec* info_spec::new_instance() const {
	info_spec* n = NULL;
	catch_bad_alloc(n = new info_spec());
	return n;
} // end new_instance

/** Copy function for INFO_SPEC object. 
  */
info_spec* info_spec::copy() const {
	info_spec* n = NULL;
	catch_bad_alloc(n = new info_spec(*this));
	return n;
} // end copy 

/** Deserialize function for INFO_SPEC object.
  * @param msg Network message: from this message RSN object will be deserialized.
  * @param cod it is set to nslp_v1 for this version of QoS NSLP.
  * @param errorlist List to record errors.
  * @param bread reference to integer. Set to the number of bytes read by this call.
  * @param skip skip the IE in case of an error or restore NetMsg position.
  */
info_spec* info_spec::deserialize(NetMsg& msg, coding_t cod, IEErrorList& errorlist, uint32& bread, bool skip) {
	uint16 t = 0;
	uint16 len = 0;
	int strlen = 0;
	uint16 paddedlen = 0;
	uint32 ielen = 0;
	uint32 saved_pos = 0;
	uint32 resume = 0;
	uint8 temp_uint8;
	errorstring.clear();
	//DLog("DESERIALIZE", __PRETTY_FUNCTION__);

	// check arguments
	if (!check_deser_args(cod, errorlist, bread)) 
		return NULL;
	
	// decode header
	if (!decode_header_nslpv1(msg, t, len, ielen, saved_pos, resume, errorlist, bread, skip)) 
		return NULL;
	
	// check type
	if (t == INFO_SPEC) 
		type = INFO_SPEC;
	else {
		// wrong type
		error_wrong_type(cod, t, saved_pos, skip, errorlist, resume, msg);
		return NULL;
	} // end if type

	// reserved(8) | errorcode(8) | class(4) | esi_type(4) | esi_len(8)
	temp_uint8 = msg.decode8(); // ignore reserved bits
	errorcode = static_cast<info_spec::errorcode_t>(msg.decode8());
	temp_uint8 = msg.decode8(); // read class and esi_type together
	errorclass = static_cast<info_spec::errorclass_t>((temp_uint8 & 0xf0) >> 4);
	esi_type = static_cast<info_spec::esitype_t>(temp_uint8 & 0x0f);

	// esi (esi_len) 32bit words!
	esi_length = msg.decode8() * sizeof(uint32);
	switch (esi_type) {
	case ESI_none:
		if (esi_length != 0) {
			//TODO: emit error
			return NULL;
		}
		break;
	case ESI_IPv4:
		if (esi_length != sizeof(in_addr)) {
			//TODO: emit error
			return NULL; 
		}
		msg.decode(esi_addr.esi_v4);
		break;
	case ESI_IPv6:
		if (esi_length != sizeof(in6_addr)) {
			//TODO: emit error
			return NULL; 
		}
		msg.decode(esi_addr.esi_v6);
		break;
	case ESI_FQDN:
		msg.decode(esi_fqdn, esi_length);
		break;
	default:
		//TODO: emit error
		return NULL;
	}
	// decode string length
	strlen = len - 4 - esi_length;	// 4 = reserved(8) | errorcode(8) | class(4) | esi_type(4) | esi_len(8)
	if (strlen >= 0) {
		paddedlen = round_up4(strlen);
		// decode string
		msg.decode(errorstring,strlen);
		// padding
		msg.padding(paddedlen-strlen);
	}
	// OK
	bread = ielen;
	return this;
} // end deserialize

/** Serialize function for INFO_SPEC object.
  * @param netmsg Network message: INFO_SPEC object will be serialized and added to this message.
  * @param cod it is set to nslp_v1 for this version of QoS NSLP.
  * @param wbytes Written bytes: the length of added bytes.
  */
void info_spec::serialize(NetMsg& msg, coding_t cod, uint32& wbytes) const {
	uint16 len = 0;
	uint16 strlen = 0;
	uint16 paddedlen = 0;
	uint32 ielen = 0;
	uint8 tmp8;
	// check arguments and IE state
	check_ser_args(cod,wbytes);
	// claculate length and encode header
	strlen = errorstring.length();
	paddedlen = round_up4(strlen);
	len = paddedlen + 4;
	len += esi_length;
	ielen = len+header_length;
	encode_header_nslpv1(msg,len);
	// reserved(8) | errorcode(16) | class(4) | esi_type(4) | esi_len(8)
	msg.encode8(0); // reserved
	msg.encode8(errorcode);
	tmp8 = ((errorclass & 0x0f) << 4) | (esi_type & 0x0f);
	msg.encode8(tmp8);
	msg.encode8((esi_length / sizeof(uint32)));
	switch (esi_type) {
	case ESI_IPv4:
		msg.encode(esi_addr.esi_v4);
		break;
	case ESI_IPv6:
		msg.encode(esi_addr.esi_v6);
		break;
	case ESI_FQDN: {
		uint32 pad = esi_length - msg.encode(esi_fqdn);
		while (pad--)
			msg.encode8(0);
		break;
	}
	default:
		;	// stupid compilers
	}
	// encode string
	msg.encode(errorstring);
	// padding
	msg.padding(paddedlen-strlen);
	wbytes = ielen;
	return;
} // end serialize

/** Check function for INFO_SPEC object.
  * @return TRUE, if the object is of type INFO_SPEC and the given info class and info code are existing.
  */
bool info_spec::check() const {
	if (type != INFO_SPEC)
		return false;

	switch ((errorclass_t)errorclass) {
	case information: 
		return (errorcode>=0) && (errorcode<=num_of_codes);
	case success: 
		return ((errorcode>=0) && (errorcode<=num_of_codes));
	case protocol: 
		return ((errorcode>=0) && (errorcode<=num_of_codes));
	case transient: 
		return ((errorcode>=0) && (errorcode<=num_of_codes));
	case permanent: 
		return ((errorcode>=0) && (errorcode<=num_of_codes));
	case qos_model_error: 
		return (errorcode>=0);
	default: 
		return false;
	} // end switch errorclass
} // end check

/** Function to get the size of the INFO_SPEC object (incl. header length).
  */
size_t info_spec::get_serialized_size(coding_t cod) const {
	if (supports_coding(cod)) 
		return round_up4(errorstring.length())+ 4 + esi_length + header_length;
	else throw IEError(IEError::ERROR_CODING);
} // end get_serialized_size

/** Function to compare two objects of type INFO_SPEC: objects are equal if error class and error code are equal.
 */
bool info_spec::operator==(const IE& ie) const {
	const info_spec* n = dynamic_cast<const info_spec*>(&ie);
	if (n) 
		return ((errorclass==n->errorclass) && (errorcode==n->errorcode));
	else return false;
} // end operator==

/** Function to print nicely the object of type INFO_SPEC on the monitor.
  */
ostream& info_spec::print(ostream& os, uint32 level, const uint32 indent, const char* name) const {
	os << setw(level * indent) << "";
	ILog("InfoSpec", "<" << get_ie_name() << ">:");
	char str[INET6_ADDRSTRLEN];
	os << setw(level * indent) << ""; 
	os << setw(level * indent) << "";
	switch (esi_type) {
	case ESI_none:
		ILog("InfoSpec", "originator NOT SET");
		break;
	case ESI_IPv4:
		inet_ntop(AF_INET, &esi_addr.esi_v4, str, sizeof(str));
		ILog("InfoSpec", "originator " << str);
		break;
	case ESI_IPv6:
		inet_ntop(AF_INET6, &esi_addr.esi_v6, str, sizeof(str));
		ILog("InfoSpec", "originator " << str);
		break;
	case ESI_FQDN:
		ILog("InfoSpec", "originator " << esi_fqdn);
		break;
	default:
		ILog("InfoSpec", "originator UNKNOWN TYPE");
	}
	os << setw(level * indent) << ""; 
	os << setw(level * indent) << "";
	ILog("InfoSpec", "Errorclass: " << (errorclass_t) errorclass);
	os << setw(level * indent) << ""; 
	os << setw(level * indent) << "";
	ILog("InfoSpec", "Errorcode: " << (errorcode_t) errorcode);
	os << setw(level * indent) << ""; 
	os << setw(level * indent) << "";
	ILog("InfoSpec", "Errorstring: \"" << errorstring);

	return os;
} // end print


/***** new members in class info_spec *****/

/** Default constructor for INFO_SPEC object without any params. The values of the object will be set to the default.
  */
info_spec::info_spec() : known_nslp_object(INFO_SPEC, mandatory),
			 errorclass(success),
			 errorcode(ReservationSuccessful),
			 esi_type(ESI_none),
			 esi_length(0)
{
	errorstring.clear();
	esi_fqdn.clear();
} // end constructor

/** Constructor for INFO_SPEC object with given params error class and error code. If state is invalid, set state to
 * GeneralError/InternalError.
 * @param err_origin this error originator will be copied into new INFO_SPEC object.
 * @param ecl this error class will be copied into new INFO_SPEC object.
 * @param ecd this error code will be copied into new INFO_SPEC object.
 */
info_spec::info_spec(const in6_addr* err_origin, errorclass_t ecl, errorcode_t ecd, uint8 e_subcode) 
	: known_nslp_object(INFO_SPEC, mandatory)
{
	errorstring.clear();
	esi_type = ESI_none;
	esi_length = 0;
	esi_fqdn.clear();
	if (err_origin != NULL)
		set_errororigin(*err_origin);
	if (!set_error(ecl, ecd, e_subcode))
		set_error(permanent, InternalOrSystemError, 0);
} // end constructor

/** Constructor for INFO_SPEC object with given params error class, error code and error string. If state is invalid, set
 * state to GeneralError/InternalError and use default string.
 * @param err_origin this error originator will be copied into new INFO_SPEC object.
 * @param ecl this error class will be copied into new INFO_SPEC object.
 * @param ecd this error code will be copied into new INFO_SPEC object.
 * @param s this error string will be copied into new INFO_SPEC object.
 */
info_spec::info_spec(const in6_addr* err_origin, errorclass_t ecl, errorcode_t ecd, uint8 s_code, const string& s) 
	: known_nslp_object(INFO_SPEC, mandatory) 
{
	esi_type = ESI_none;
	esi_length = 0;
	esi_fqdn.clear();
	if (err_origin != NULL)
		set_errororigin(*err_origin);
	if (set_error(ecl,ecd, s_code))
		errorstring = s;
	else
		set_error(permanent,InternalOrSystemError, 0);
} // end constructor

/** Constructor for INFO_SPEC object with given params error class, error code and error string. If state is invalid, set
 * state to GeneralError/InternalError and use default string.
 * @param err_origin this error originator will be copied into new INFO_SPEC object.
 * @param ecl this error class will be copied into new INFO_SPEC object.
 * @param ecd this error code will be copied into new INFO_SPEC object.
 * @param s this error string will be appended to the string in the new INFO_SPEC object if append is set to TRUE.
 * @param append if this boolean variable set to TRUE then the string s will be appended to the string in the new INFO_SPEC object.
 */
info_spec::info_spec(const in6_addr* err_origin, errorclass_t ecl, errorcode_t ecd, uint8 s_code, const string& s, bool append) 
	: known_nslp_object(INFO_SPEC, mandatory) 
{
	esi_type = ESI_none;
	esi_length = 0;
	esi_fqdn.clear();
	if (err_origin != NULL)
        set_errororigin(*err_origin);
	if (set_error(ecl,ecd, s_code)) {
		if (append) append_to_errorstring(s);
		else errorstring = s;
	} else set_error(permanent, InternalOrSystemError, 0);
} // end constructor

/** Constructor for INFO_SPEC object with given INFO_SPEC object.
  * @param n the INFO_SPEC object to be copied into new INFO_SPEC object.
  */
info_spec::info_spec(const info_spec& n) : known_nslp_object(n),
					   errorclass(n.errorclass),
					   errorcode(n.errorcode),
					   esi_type(n.esi_type),
					   esi_length(n.esi_length),
					   errorstring(n.errorstring)
{
	switch (esi_type) {
	case ESI_IPv4:
		esi_addr.esi_v4 = n.esi_addr.esi_v4;
		break;
	case ESI_IPv6:
		esi_addr.esi_v6 = n.esi_addr.esi_v6;
		break;
	case ESI_FQDN:
		esi_fqdn = n.esi_fqdn;
		break;
	default:
		; // Stupid compilers
	}
} // end copy constructor

/** Destructor for INFO_SPEC object.
  */
info_spec::~info_spec() {
} // end destructor

/** Asign operator for INFO_SPEC object.
  * @param n the INFO_SPEC object to be asigned to the current INFO_SPEC object.
  */
info_spec& info_spec::operator=(const info_spec& n) {
	known_nslp_object::operator=(n);
	errorclass = n.errorclass;
	errorcode = n.errorcode;
	esi_type = n.esi_type;
	esi_length = n.esi_length;
	switch (esi_type) {
	case ESI_IPv4:
		esi_addr.esi_v4 = n.esi_addr.esi_v4;
		break;
	case ESI_IPv6:
		esi_addr.esi_v6 = n.esi_addr.esi_v6;
		break;
	case ESI_FQDN:
		esi_fqdn = n.esi_fqdn;
		break;
	default:
		; // Stupid compilers
	}
	errorstring = n.errorstring;
	return *this;
} // end operator=

/** Get function for the error string of the current INFO_SPEC object.
  * @return the the error string of the current INFO_SPEC object.
  */
const string& info_spec::get_errorstring() const { return errorstring; }

/** Set function for error string of the current INFO_SPEC object.
  * @param s the the error string of the current INFO_SPEC object will be set to the value of this variable.
  */
info_spec& info_spec::set_errorstring(const string& s) {
	errorstring = s;
	return *this;
} // end set_errorstring

/** Append function for error string of the current INFO_SPEC object.
  * @param s the the error string of the current INFO_SPEC object will be appended to the existing error string.
  */
info_spec& info_spec::append_to_errorstring(const string& s) {
	if (errorstring.length() == 0) 
		errorstring = s;
	else if (s.length() != 0) 
		(errorstring += ' ') += s;
	return *this;
} // end set_errorstring

/** Function to set the error string to a default based on error class and error code of the current INFO_SPEC object.
  */
bool info_spec::set_defaultstring() {
	const char *dstr;
	errorstring.clear();
	if (check() && (errorclass < num_of_classes) && (errorcode < num_of_codes)) {
		dstr = defstring[errorclass][errorcode];
		if (dstr) {
			errorstring = dstr;
			return true;
		} else return false;
	} else return false;
} // end set_defaultstring

/** Set function for error class and error code of the current INFO_SPEC object.
  * @param ecl this error class of the current INFO_SPEC object will be set to this value.
  * @param ecd this error code of the current INFO_SPEC object will be set to this value.
  */
bool info_spec::set_error(errorclass_t ecl, errorcode_t ecd, uint8 e_subcode) {
	errorclass_t oldecl = errorclass;
	errorcode_t oldecd = errorcode;
	errorclass = ecl;
	errorcode = ecd;
	if (check())
		return set_defaultstring();
	else {
		ERRLog("InfoSpec", "set_error(): new error not set!");
		errorclass = oldecl;
		errorcode = oldecd;
		return false;
	} // end if check
} // end set_error

/** Get function for the error class of the current INFO_SPEC object.
  * @return the the error class of the current INFO_SPEC object.
  */
info_spec::errorclass_t info_spec::get_errorclass() const {
	return errorclass;
} // end get_errorclass

/** Set function for error class of the current INFO_SPEC object.
  * @param ecl the error class of the current INFO_SPEC object will be set to this value.
  */
void info_spec::set_errorclass(errorclass_t ecl) {
	errorclass = ecl;
} // end set_errorclass

/** Get function for the error class of the current INFO_SPEC object as a string.
  * @return the the error class as a string of the current INFO_SPEC object.
  */
const char* info_spec::get_errorclass_string() const {
	return defstring[errorclass][0];
} // end get_errorclass_string()

/** Get function for the error code of the current INFO_SPEC object.
  * @return the the error code of the current INFO_SPEC object.
  */
info_spec::errorcode_t info_spec::get_errorcode() const {
	return errorcode;
} // end get_errorcode

/** Set function for error code of the current INFO_SPEC object.
  * @param ecd the error code of the current INFO_SPEC object will be set to this value.
  */
void  info_spec::set_errorcode(errorcode_t ecd) {
	errorcode = ecd;
} // end set_errorcode

/** Get function for the error originator of the current INFO_SPEC object.
  * @param origin the error originator of the current INFO_SPEC object will be copid into this variable.
  */
void info_spec::get_errororigin(in6_addr& origin) const {
	if (esi_type == ESI_IPv6)
		origin = esi_addr.esi_v6;
} // end get_errororigin

/** Set function for error origin of the current INFO_SPEC object.
  * @param origin the error origin of the current INFO_SPEC object will be set to this value.
  */
void  info_spec::set_errororigin(in6_addr origin) {
	if (esi_type == ESI_FQDN)
		esi_fqdn.clear();
	esi_length = sizeof(in6_addr);
	esi_type = ESI_IPv6;
	esi_addr.esi_v6 = origin;
} // end set_errororigin

/** Check function for INFO_SPEC object.
  * @return TRUE if the current object is of type INFO_SPEC.
  */
bool info_spec::is_eo() const {
	return (type == INFO_SPEC);
} // end is_eo


//@}

} // end namespace qos_nslp
