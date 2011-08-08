/// ----------------------------------------*- mode: C++; -*--
/// @file nonce.cpp
/// The Nonce Object.
/// ----------------------------------------------------------
/// $Id: nonce.cpp 2558 2007-04-04 15:17:16Z bless $
/// $HeadURL: https://svn.tm.kit.edu/nsis/natfw-nslp/trunk/src/msg/nonce.cpp $
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

#include "msg/nonce.h"


using namespace natfw::msg;


const char *const nonce::ie_name = "nonce";


/**
 * Default constructor.
 */
nonce::nonce()
		: natfw_object(OBJECT_TYPE, tr_mandatory), value(0) {

	// nothing to do
}


/**
 * Constructor for manual use.
 *
 * @param value the nonce value
 */
nonce::nonce(uint32 value, treatment_t treatment)
		: natfw_object(OBJECT_TYPE, treatment), value(value) {

	// nothing to do
}


nonce::~nonce() {
	// nothing to do
}


nonce *nonce::new_instance() const {
	nonce *q = NULL;
	catch_bad_alloc( q = new nonce() );
	return q;
}


nonce *nonce::copy() const {
	nonce *q = NULL;
	catch_bad_alloc( q = new nonce(*this) );
	return q;
}


bool nonce::deserialize_body(NetMsg &msg, uint16 body_length,
		IEErrorList &err, bool skip) {

	set_value(msg.decode32());

	return true; // success, all values are syntactically valid
}


void nonce::serialize_body(NetMsg &msg) const {
	msg.encode32(get_value());
}


size_t nonce::get_serialized_size(coding_t coding) const {
	return HEADER_LENGTH + 4;
}


// All values are valid.
bool nonce::check_body() const {
	return true;
}


bool nonce::equals_body(const natfw_object &obj) const {

	const nonce *other
		= dynamic_cast<const nonce *>(&obj);

	return other != NULL && get_value() == other->get_value();
}


const char *nonce::get_ie_name() const {
	return ie_name;
}


ostream &nonce::print_attributes(ostream &os) const {
	return os << ", value=" << get_value();
}


/**
 * Returns the nonce value.
 *
 * @return the nonce value
 */
uint32 nonce::get_value() const {
	return value;
}


/**
 * Set the nonce value.
 *
 * @param val the nonce value
 */
void nonce::set_value(uint32 val) {
	value = val;
}


// EOF
