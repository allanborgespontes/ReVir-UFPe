/// ----------------------------------------*- mode: C++; -*--
/// @gist_exceptions.h
/// GIST exceptions
/// ----------------------------------------------------------
/// $Id: gist_exceptions.h 5249 2010-04-26 08:40:59Z roehricht $
/// $HeadURL: https://svn.tm.kit.edu/nsis/ntlp/trunk/src/gist_exceptions.h $
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
#ifndef NTLP__GIST_EXCEPTIONS_H
#define NTLP__GIST_EXCEPTIONS_H

#include "protlib_types.h"


namespace ntlp {
  using protlib::uint8;
  using protlib::uint16;


// forward declarations
class errorobject;
class ntlp_pdu;
class mri;
class sessionid;


/**
 * Base class for all GIST exceptions.
 *
 * All subclasses consist at least of three error codes (class, code, subcode)
 * and a human readable message.
 */
class GistError : public protlib::ProtLibException {
  public:
	enum mode_t {
		q_mode	= 0,
		d_mode	= 1,
		c_mode	= 2
	};

	virtual ~GistError() throw ();

	virtual const char *what() const throw();

	errorobject *get_error_object() throw ();
	uint8 get_error_class() const throw () { return error_class; }
	uint16 get_error_code() const throw () { return error_code; }
	uint8 get_error_subcode() const throw () { return error_subcode; }

  protected:
	enum error_class_t {
		ec_informational	= 0,
		ec_success		= 1,
		ec_protocol_error	= 2,
		ec_transient_error	= 3,
		ec_permanent_failure	= 4
	};

	GistError(const ntlp_pdu &pdu, mode_t mode,
		uint8 error_class, uint16 error_code, uint8 error_subcode,
		const std::string &msg) throw ();

	virtual errorobject *create_error_object(
		const ntlp_pdu &pdu, mode_t mode,
		uint8 error_class, uint16 error_code, uint8 error_subcode,
		const std::string &msg) const throw ();

  private:
	uint8 error_class;
	uint16 error_code;
	uint8 error_subcode;
	std::string msg;
	std::string full_msg;

	errorobject *error_obj;
};



class GistCommonHeaderParseError : public GistError {
  public:
	enum subcode_t {
		sc_unknown_version	= 0,
		sc_unknown_type		= 1,
		sc_invalid_r_flag	= 2,
		sc_incorrect_msg_length	= 3,
		sc_invalid_e_flag	= 4,
		sc_invalid_c_flag	= 5
	};

	GistCommonHeaderParseError(const ntlp_pdu &pdu, mode_t mode,
		uint8 subcode, uint16 msg_length=0) throw ()
		: GistError(pdu, mode, ec_protocol_error, 1, subcode,
		  "Common Header Parse Error"), msg_length(msg_length) { }

	virtual ~GistCommonHeaderParseError() throw () { }

	virtual errorobject *create_error_object(
		const ntlp_pdu &pdu, mode_t mode,
		uint8 error_class, uint16 error_code, uint8 error_subcode,
		const std::string &msg) const throw ();

	//virtual const char *what() const throw();

	uint16 get_msg_length() const throw () { return msg_length; }

  private:
	uint16 msg_length;
};



class GistHopLimitExceeded : public GistError {
  public:
	GistHopLimitExceeded(const ntlp_pdu &pdu, mode_t mode) throw ()
		: GistError(pdu, mode, ec_permanent_failure, 2, 0,
		"Hop Limit Exceeded") { }

	virtual ~GistHopLimitExceeded() throw () { }
};


class GistIncorrectEncapsulation : public GistError {
  public:
	GistIncorrectEncapsulation(const ntlp_pdu &pdu, mode_t mode) throw ()
		: GistError(pdu, mode, ec_protocol_error, 3, 0,
		"Incorrect Encapsulation") { }

	virtual ~GistIncorrectEncapsulation() throw () { }
};


class GistIncorrectlyDeliveredMessage : public GistError {
  public:
	GistIncorrectlyDeliveredMessage(const ntlp_pdu &pdu, mode_t mode)
		throw ()
		: GistError(pdu, mode, ec_protocol_error, 4, 0,
		"Incorrectly Delivered Message") { }

	virtual ~GistIncorrectlyDeliveredMessage() throw () { }
};


class GistNoRoutingState : public GistError {
  public:
	GistNoRoutingState(const ntlp_pdu &pdu, mode_t mode) throw ()
		: GistError(pdu, mode, ec_protocol_error, 5, 0,
		"No Routing State") { }

	virtual ~GistNoRoutingState() throw () { }
};


class GistUnknownNSLPID : public GistError {
  public:
	GistUnknownNSLPID(const ntlp_pdu &pdu, mode_t mode) throw ()
		: GistError(pdu, mode, ec_permanent_failure, 6, 0,
		"Unknown NSLPID") { }

	virtual ~GistUnknownNSLPID() throw () { }
};


class GistEndpointFound : public GistError {
  public:
	GistEndpointFound(const ntlp_pdu &pdu, mode_t mode) throw ()
		: GistError(pdu, mode, ec_informational, 7, 0,
		"Endpoint Found") { }

	virtual ~GistEndpointFound() throw () { }
};


class GistMessageTooLarge : public GistError {
  public:
	GistMessageTooLarge(const ntlp_pdu &pdu, mode_t mode, uint16 mtu_info)
		throw ()
		: GistError(pdu, mode, ec_permanent_failure, 8, 0,
		"Message Too Large"), mtu_info(mtu_info) { }

	virtual ~GistMessageTooLarge() throw () { }

	virtual errorobject *create_error_object(
		const ntlp_pdu &pdu, mode_t mode,
		uint8 error_class, uint16 error_code, uint8 error_subcode,
		const std::string &msg) const throw ();

	uint16 get_mtu_info() const throw () { return mtu_info; }

  private:
	uint16 mtu_info;
};


class GistObjectTypeError : public GistError {
  public:
	enum subcode_t {
		sc_duplicate_object		= 0,
		sc_unrecognised_object		= 1,
		sc_missing_object		= 2,
		sc_invalid_object_type		= 3,
		sc_untranslated_object		= 4,
		sc_invalid_extensibility_flags	= 5
	};

	GistObjectTypeError(const ntlp_pdu &pdu, mode_t mode, uint16 subcode,
		uint16 object_type) throw ()
		: GistError(pdu, mode, ec_protocol_error, 9, subcode,
		"Object Type Error"), object_type(object_type) { }

	virtual ~GistObjectTypeError() throw () { }

	virtual errorobject *create_error_object(
		const ntlp_pdu &pdu, mode_t mode,
		uint8 error_class, uint16 error_code, uint8 error_subcode,
		const std::string &msg) const throw ();

	uint16 get_object_type() const throw () { return object_type; }

  private:
	uint16 object_type;	// type of the object which caused the problem
};


class GistObjectValueError : public GistError {
  public:
	enum subcode_t {
		sc_incorrect_length			= 0,
		sc_value_not_supported			= 1,
		sc_invalid_flag_field_combination	= 2,
		sc_empty_list				= 3,
		sc_invalid_cookie			= 4,
		sc_stack_proposal			= 5
	};

	GistObjectValueError(const ntlp_pdu &pdu, mode_t mode, uint16 subcode)
		throw ()
		: GistError(pdu, mode, ec_protocol_error, 10, subcode,
		"Object Value Error") { }

	virtual ~GistObjectValueError() throw () { }
};


class GistInvalidIPLayerTTL : public GistError {
  public:
	GistInvalidIPLayerTTL(const ntlp_pdu &pdu, mode_t mode) throw ()
		: GistError(pdu, mode, ec_permanent_failure, 11, 0,
		"Invalid IP layer TTL") { }

	virtual ~GistInvalidIPLayerTTL() throw () { }
};


class GistMriValidationFailure : public GistError {
  public:
	enum subcode_t {
		sc_mri_too_wild			= 0,
		sc_ip_version_mismatch		= 1,
		sc_ingress_filter_failure	= 2
	};

	GistMriValidationFailure(const ntlp_pdu &pdu, mode_t mode,
		uint16 subcode) throw ()
		: GistError(pdu, mode, ec_protocol_error, 12, subcode,
		"Object Type Error") { }

	virtual ~GistMriValidationFailure() throw () { }
};



} // namespace ntlp

#endif // NTLP__GIST_EXCEPTIONS_H
