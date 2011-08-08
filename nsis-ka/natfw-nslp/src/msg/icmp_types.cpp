/// ----------------------------------------*- mode: C++; -*--
/// @file icmp_types.cpp
/// The ICMP Types Object.
/// ----------------------------------------------------------
/// $Id: icmp_types.cpp 2558 2007-04-04 15:17:16Z bless $
/// $HeadURL: https://svn.tm.kit.edu/nsis/natfw-nslp/trunk/src/msg/icmp_types.cpp $
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
#include <cstring>

#include "logfile.h"

#include "msg/icmp_types.h"


using namespace natfw::msg;


const char *const icmp_types::ie_name = "icmp_types";


/**
 * Default constructor.
 */
icmp_types::icmp_types()
		: natfw_object(OBJECT_TYPE, tr_optional), types() {

	// nothing to do
}


/**
 * Constructor for manual use.
 *
 * The list of ICMP type IDs is copied.
 *
 * @param types a list of ICMP type IDs
 * @param treatment the object's treatment
 */
icmp_types::icmp_types(const std::list<uint8> &types, treatment_t treatment)
		: natfw_object(OBJECT_TYPE, treatment), types(types) {

	// nothing to do
}


icmp_types::~icmp_types() {
	// nothing to do
}


icmp_types *icmp_types::new_instance() const {
	icmp_types *q = NULL;
	catch_bad_alloc( q = new icmp_types() );
	return q;
}


icmp_types *icmp_types::copy() const {
	icmp_types *q = NULL;
	catch_bad_alloc( q = new icmp_types(*this) );
	return q;
}


/*
 * We don't accept invalid values at all, so skip isn't used here.
 */
bool icmp_types::deserialize_body(NetMsg &msg, uint16 body_length,
		IEErrorList &err, bool skip) {

	types.clear();

	// read the count field
	uint8 count = msg.decode8();

	// check if length derived from count and body_length match
	if ( (((count+1)+3) / 4) != body_length / 4 ) {
		catch_bad_alloc( err.put(
			new PDUSyntaxError(CODING, get_category(),
			get_object_type(), 0, msg.get_pos(), "length mismatch"))
		);
		return false;
	}

	for ( int i = 0; i < count; i++ )
		types.push_back(msg.decode8());

	// skip padding
	msg.set_pos_r( body_length - (count+1) );

	return true; // success, all values are syntactically valid
}


void icmp_types::serialize_body(NetMsg &msg) const {

	// write the count field unless we have too many types
	if ( types.size() <= 255 )
		msg.encode8(types.size());
	else
		throw IEError(IEError::ERROR_INVALID_STATE);

	for ( const_iterator i = types.begin(); i != types.end(); i++ )
		msg.encode8(*i);

	// add padding
	uint8 padding = get_serialized_size(CODING)
			- HEADER_LENGTH - types.size()-1;
	for ( int i = 0; i < padding; i++ )
		msg.encode8(0);
}


size_t icmp_types::get_serialized_size(coding_t coding) const {
	uint8 fields = types.size() + 1; // count and all types fields

	return HEADER_LENGTH + ( ((fields + 3) / 4) * 4 );
}


// All values are valid.
bool icmp_types::check_body() const {
	return true;
}


bool icmp_types::equals_body(const natfw_object &obj) const {

	const icmp_types *other
		= dynamic_cast<const icmp_types *>(&obj);

	return other != NULL && types == other->types;
}


const char *icmp_types::get_ie_name() const {
	return ie_name;
}


ostream &icmp_types::print_attributes(ostream &os) const {
	os << ", ids=[";

	for ( const_iterator i = types.begin(); i != types.end(); i++ )
		os << *i << " ";

	return os << "]";
}


/**
 * Returns the icmp_types.
 *
 * @return the list of ICMP type IDs
 */
const std::list<uint8> &icmp_types::get_icmp_types() const {
	return types;
}


// EOF
