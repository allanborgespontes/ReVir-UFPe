/// ----------------------------------------*- mode: C++; -*--
/// @file session_lifetime.cpp
/// The Session Lifetime Object.
/// ----------------------------------------------------------
/// $Id: session_lifetime.cpp 2558 2007-04-04 15:17:16Z bless $
/// $HeadURL: https://svn.tm.kit.edu/nsis/natfw-nslp/trunk/src/msg/session_lifetime.cpp $
// ===========================================================
//                      
// Copyright (C) 2005-2007, all rights reserved by
// - Institute of Telematics, Universitaet Karlsruhe (TH)
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
#include "logfile.h"

#include "msg/session_lifetime.h"


using namespace natfw::msg;


const char *const session_lifetime::ie_name = "session_lifetime";


/**
 * Default constructor.
 */
session_lifetime::session_lifetime()
		: natfw_object(OBJECT_TYPE, tr_mandatory), value(0) {

	// nothing to do
}


/**
 * Constructor for manual use.
 *
 * @param value the session lifetime in seconds
 */
session_lifetime::session_lifetime(uint32 value, treatment_t treatment)
		: natfw_object(OBJECT_TYPE, treatment), value(value) {

	// nothing to do
}


session_lifetime::~session_lifetime() {
	// nothing to do
}


session_lifetime *session_lifetime::new_instance() const {
	session_lifetime *q = NULL;
	catch_bad_alloc( q = new session_lifetime() );
	return q;
}


session_lifetime *session_lifetime::copy() const {
	session_lifetime *q = NULL;
	catch_bad_alloc( q = new session_lifetime(*this) );
	return q;
}


bool session_lifetime::deserialize_body(NetMsg &msg, uint16 body_length,
		IEErrorList &err, bool skip) {

	set_value(msg.decode32());

	return true; // success, all values are syntactically valid
}


void session_lifetime::serialize_body(NetMsg &msg) const {
	msg.encode32(get_value());
}


size_t session_lifetime::get_serialized_size(coding_t coding) const {
	return HEADER_LENGTH + 4;
}


// All values are valid.
bool session_lifetime::check_body() const {
	return true;
}


bool session_lifetime::equals_body(const natfw_object &obj) const {

	const session_lifetime *other
		= dynamic_cast<const session_lifetime *>(&obj);

	return other != NULL && get_value() == other->get_value();
}


const char *session_lifetime::get_ie_name() const {
	return ie_name;
}


ostream &session_lifetime::print_attributes(ostream &os) const {
	return os << ", value=" << get_value();
}


/**
 * Returns the session lifetime in seconds.
 *
 * @return the session lifetime in seconds.
 */
uint32 session_lifetime::get_value() const {
	return value;
}


/**
 * Set the session lifetime in seconds.
 *
 * @param val the session lifetime in seconds.
 */
void session_lifetime::set_value(uint32 val) {
	value = val;
}


// EOF
