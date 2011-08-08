/// ----------------------------------------*- mode: C++; -*--
/// @file gist_exceptions.cpp
/// GIST Exceptions 
/// ----------------------------------------------------------
/// $Id: gist_exceptions.cpp 5249 2010-04-26 08:40:59Z roehricht $
/// $HeadURL: https://svn.tm.kit.edu/nsis/ntlp/trunk/src/gist_exceptions.cpp $
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
#include <string>
#include <sstream>

#include "ie.h"
#include "gist_exceptions.h"
#include "ntlp_errorobject.h"
#include "ntlp_pdu.h"
#include "mri.h"
#include "sessionid.h"

using namespace ntlp;


#define safe_copy(ie) ( ie != NULL ? ie->copy() : NULL )


/**
 * Constructor.
 *
 * The pdu points to a ntlp_pdu subclass. All we need from it is the GIST
 * Common Header, the session ID and the MRI.
 */
GistError::GistError(const ntlp_pdu &pdu, mode_t mode,
		uint8 error_class, uint16 error_code, uint8 error_subcode,
		const std::string &msg) throw () 
		: ProtLibException(msg),
		  error_class(error_class), error_code(error_code),
		  error_subcode(error_subcode), error_obj(NULL) {

	error_obj = create_error_object(pdu, mode, error_class,
		error_code, error_subcode, msg);

	std::ostringstream buf;

	buf << ProtLibException::what() << " (" << error_class << ", "
		<< error_code << ", " << error_subcode << ")";

	full_msg = buf.str();
}


/**
 * Destructor.
 */
GistError::~GistError() throw () {
	delete error_obj;
}


const char *GistError::what() const throw() {
	return full_msg.c_str();
}


/**
 * Get the errorobject created for this exception.
 *
 * This method may only be used once and returns NULL afterwards.
 */
errorobject *GistError::get_error_object() throw () {
	errorobject *tmp = error_obj;
	error_obj = NULL;
	return tmp;
}


/**
 * Create a GIST error object based on the given parameters.
 *
 * The implementation in GistError doesn't set any of the Additional
 * Information fields. Subclasses have to override create_error_object()
 * to do this.
 */
errorobject *GistError::create_error_object(const ntlp_pdu &pdu, mode_t mode,
		uint8 error_class, uint16 error_code,
		uint8 error_subcode, const std::string &msg) const throw () {

	return new errorobject(
		safe_copy(pdu.get_mri()), safe_copy(pdu.get_sessionid()),
		error_class, error_code, error_subcode,
		pdu.get_version(), pdu.get_hops(), pdu.get_length(),
		pdu.get_nslpid(), pdu.get_type(), pdu.get_flags(),
		(mode == d_mode), (mode == q_mode), NULL, errorobject::oti_not_set, 0, 0
	);
}


errorobject *GistCommonHeaderParseError::create_error_object(
		const ntlp_pdu &pdu, mode_t mode,
		uint8 error_class, uint16 error_code,
		uint8 error_subcode, const std::string &msg) const throw () {

	return new errorobject(
		safe_copy(pdu.get_mri()), safe_copy(pdu.get_sessionid()),
		error_class, error_code, error_subcode,
		pdu.get_version(), pdu.get_hops(), pdu.get_length(),
		pdu.get_nslpid(), pdu.get_type(), pdu.get_flags(),
		(mode == d_mode), (mode == q_mode), NULL,  errorobject::oti_not_set, 0, msg_length
	);
}


errorobject *GistMessageTooLarge::create_error_object(const ntlp_pdu &pdu,
		mode_t mode, uint8 error_class, uint16 error_code,
		uint8 error_subcode, const std::string &msg) const throw () {

	return new errorobject(
		safe_copy(pdu.get_mri()), safe_copy(pdu.get_sessionid()),
		error_class, error_code, error_subcode,
		pdu.get_version(), pdu.get_hops(), pdu.get_length(),
		pdu.get_nslpid(), pdu.get_type(), pdu.get_flags(),
		(mode == d_mode), (mode == q_mode), NULL, errorobject::oti_not_set, mtu_info, 0
	);
}


errorobject *GistObjectTypeError::create_error_object(const ntlp_pdu &pdu,
		mode_t mode, uint8 error_class, uint16 error_code,
		uint8 error_subcode, const std::string &msg) const throw () {

	return new errorobject(
		safe_copy(pdu.get_mri()), safe_copy(pdu.get_sessionid()),
		error_class, error_code, error_subcode,
		pdu.get_version(), pdu.get_hops(), pdu.get_length(),
		pdu.get_nslpid(), pdu.get_type(), pdu.get_flags(),
		(mode == d_mode), (mode == q_mode), NULL, object_type, 0, 0
	);
}


// EOF
