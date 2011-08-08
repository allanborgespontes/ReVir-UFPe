/// ----------------------------------------*- mode: C++; -*--
/// @file info_spec.h
/// NSLP error and info specification objects
/// ----------------------------------------------------------
/// $Id: info_spec.h 5721 2010-10-16 16:52:52Z bless $
/// $HeadURL: https://svn.tm.kit.edu/nsis/qos-nslp/trunk/include/info_spec.h $
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
/** @ingroup ieinfo_spec 
 * @file
 * NSLP error objects
 */

#ifndef _NSLP__INFO_SPEC_H_
#define _NSLP__INFO_SPEC_H_

#include "protlib_types.h"
#include "nslp_object.h"
#include <netinet/in.h>
#include <arpa/inet.h>

using namespace protlib;

namespace qos_nslp {

/** @addtogroup ieinfo_spec Info Specification
 * @ingroup ienslpobject
 * @{
 */

/** Error object with error string.
 * Be sure to add a default error string to the defstring array when
 * introducing a new error code are error class.
 */
class info_spec : public known_nslp_object {
/***** inherited from IE ****/
public:
	virtual info_spec* new_instance() const;
	virtual info_spec* copy() const;
	virtual info_spec* deserialize(NetMsg& msg, coding_t cod, IEErrorList& errorlist, uint32& bread, bool skip = true);
	virtual void serialize(NetMsg& msg, coding_t cod, uint32& wbytes) const;
	virtual bool check() const;
	virtual size_t get_serialized_size(coding_t coding) const;
	virtual bool operator==(const IE& ie) const;
	virtual const char* get_ie_name() const;
	virtual ostream& print(ostream& os, uint32 level, const uint32 indent, const char* name = NULL) const;

/***** new members *****/
public:
	/// error class
	enum errorclass_t {
		/// number of error classes
		class_not_set  = 0x00, // note that coding starts with 1
		information    = 0x01,
		success        = 0x02,
		protocol       = 0x03,
		transient      = 0x04,
		permanent      = 0x05,
		qos_model_error= 0x06,
		num_of_classes = 0x07
	}; // end errorclass_t

	/// error code
	enum errorcode_t {
		/// number of error codes
		code_not_set			= 0x00,
		/// information
		UnknownBOUND_SESSION_ID		= 0x01,
		RouteChange			= 0x02,
		NoReducedRefreshes		= 0x03,
		Congestion			= 0x04,
		UnknownSESSION_ID		= 0x05,
		MismatchingRSN			= 0x06,
#ifdef USE_AHO
		InitQuery			= 0x07,
		InitReserve			= 0x08,
#endif
		TearDownBranch			= 0x09,
		/// success
		ReservationSuccessful		= 0x01,
		TeardownSuccessful		= 0x02,
		Acknowledgement			= 0x03,
		RefreshSuccessful		= 0x04,
		/// protocol
		IllegalMessageType		= 0x01,
		WrongMessageLength		= 0x02,
		BadGenericFlagsValue		= 0x03,
		BadSpecificFlagsValue		= 0x03,
		MandatoryObjectMissing		= 0x05,
		IllegalObjectPresent		= 0x06,
		UnknownObjectPresent		= 0x07,
		WrongObjectLength		= 0x08,
		RESERVEfromWrongDirection	= 0x09,
		UnknownObjectFieldValue		= 0x0a,
		DuplicateObject			= 0x0b,
		MalformedQSPEC			= 0x0c,
		UnknownMRI			= 0x0d,
		ErroneousTLV			= 0x0e,
		IncompatibleQSPEC		= 0x0f,
		/// transient
		NoReversePath			= 0x01,
		NoPathStateForReserve		= 0x02,
		RIIconflict			= 0x03,
		FullQSPECrequired		= 0x04,
		MismatchEndToEnd_Intry		= 0x05,
		ReservationPreempted		= 0x06,
		ReservationFailure		= 0x07,
		PathTruncated			= 0x08,
		/// permanent
		InternalOrSystemError		= 0x01,
		AuthenticationFailure		= 0x02,
		/// number of codes
		num_of_codes			= 16
	}; // end error code

	enum esitype_t {
		ESI_none = 0,
		ESI_IPv4 = 1,
		ESI_IPv6 = 2,
		ESI_FQDN = 3
	};

	// @{
	/// constructor
	info_spec();
	info_spec(const in6_addr* err_origin, errorclass_t ecl, errorcode_t ecd, uint8 e_subcode);
	info_spec(const in6_addr* err_origin, errorclass_t ecl, errorcode_t ecd, uint8 s_code, const string& s);
	info_spec(const in6_addr* err_origin, errorclass_t ecl, errorcode_t ecd, uint8 s_code, const string& s, bool append = true);
	// @}
	/// copy constructor
	info_spec(const info_spec& n);
	/// destructor
	virtual ~info_spec();
	/// assignment
	info_spec& operator=(const info_spec& n);
	/// get string
	const string& get_errorstring() const;
	/// set string
	info_spec& set_errorstring(const string& s);
	/// append to error string
	info_spec& append_to_errorstring(const string& s);
	/// set default string
	bool set_defaultstring();
	/// set error class and code
	bool set_error(errorclass_t ecl, errorcode_t ecd, uint8 e_subcode);
	/// get error class
	errorclass_t get_errorclass() const;
        /// set error class
	void set_errorclass(errorclass_t ecl);
	/// get error class as string
	const char* get_errorclass_string() const;
	/// get error code
	errorcode_t get_errorcode() const;
        /// set error code
	void set_errorcode(errorcode_t ecd);
        /// get error origin
	void get_errororigin(in6_addr& origin) const;
        /// set error origin
	void set_errororigin(in6_addr origin);

        bool is_eo() const;

private:
	errorclass_t errorclass;
	errorcode_t errorcode;
	esitype_t esi_type;
	uint16 esi_length;	// Note: uint8 on the wire, but counts 32bit words
	union {
		in_addr esi_v4;
		in6_addr esi_v6;
	} esi_addr;
	string esi_fqdn;
	string errorstring;
	static const char* const iename;
	uint16 contlen;
	static const char *const defstring[num_of_classes][num_of_codes];
}; // end class info_spec

//@}

} // end namespace qos_nslp

#endif // _NSLP__INFO_SPEC_H_
