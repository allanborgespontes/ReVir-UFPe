/// ----------------------------------------*- mode: C++; -*--
/// @file information_code.cpp
/// The Information Code Object
/// ----------------------------------------------------------
/// $Id: information_code.cpp 2558 2007-04-04 15:17:16Z bless $
/// $HeadURL: https://svn.tm.kit.edu/nsis/natfw-nslp/trunk/src/msg/information_code.cpp $
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

#include "msg/information_code.h"


using namespace natfw::msg;


const char *const information_code::ie_name = "information_code";


/**
 * Default constructor.
 */
information_code::information_code()
		: natfw_object(OBJECT_TYPE, tr_mandatory),
		  severity_class(0), response_code(0), response_object_type(0) {

	// nothing to do
}


/**
 * Constructor for manual use.
 *
 * To obtain valid severity classes, the severity_class_t enum can be used.
 * The response code depends on the severity class; there are enums defined
 * for each class.
 *
 * Note: Don't confuse the two object_type parameters. There is one in the
 * object header and one in the object body.
 *
 * @param severity the severity class (4 bit)
 * @param response_code a response code, depending on the severity class
 * @param object_type the object type the response is about (12 bit)
 */
information_code::information_code(uint8 severity, uint8 response_code,
			uint16 object_type, treatment_t treatment)
		: natfw_object(OBJECT_TYPE, treatment),
		  severity_class(severity), response_code(response_code),
		  response_object_type(object_type) {

	// nothing to do
}


information_code::~information_code() {
	// nothing to do
}


information_code *information_code::new_instance() const {
	information_code *q = NULL;
	catch_bad_alloc( q = new information_code() );
	return q;
}


information_code *information_code::copy() const {
	information_code *q = NULL;
	catch_bad_alloc( q = new information_code(*this) );
	return q;
}


bool information_code::deserialize_body(NetMsg &msg, uint16 body_length,
		IEErrorList &err, bool skip) {

	uint32 value = msg.decode32();

	set_severity_class( (value >> 24) & 0xF );
	set_response_code( (value >> 16) & 0xFF );
	set_response_object_type( value & 0xFFF );

	// Check for invalid values.
	if ( ! check() ) {
		catch_bad_alloc( err.put( 
			new PDUSyntaxError(CODING, get_category(),
				get_object_type(), 0, msg.get_pos()-4)) );

		if ( ! skip )
			return false;
	}

	return true; // success
}


void information_code::serialize_body(NetMsg &msg) const {
	msg.encode8(get_severity_class() & 0xF);
	msg.encode8(get_response_code());
	msg.encode16(get_response_object_type() & 0xFFF);
}


size_t information_code::get_serialized_size(coding_t coding) const {
	return HEADER_LENGTH + 4;
}


bool information_code::check_body() const {
	// make sure that only 4 or 12 bit are used, respectively
	if ( (get_severity_class() & 0xF0)
			|| (get_response_object_type() & 0xF000) )
		return false;
	else
		return true;
}


bool information_code::equals_body(const natfw_object &obj) const {

	const information_code *other
		= dynamic_cast<const information_code *>(&obj);

	return other != NULL
		&& get_severity_class() == other->get_severity_class()
		&& get_response_code() == other->get_response_code()
		&& get_response_object_type()
			== other->get_response_object_type();
}


const char *information_code::get_ie_name() const {
	return ie_name;
}


ostream &information_code::print_attributes(ostream &os) const {
	return os << ", severity=" << int(get_severity_class())
		<< ", response_code=" << int(get_response_code())
		<< ", obj_type=" << int(get_response_object_type());
}


/**
 * Returns the severity class.
 *
 * The severity_class_t enum can be used to test the returned values.
 *
 * @return the severity class (4 bit)
 */
uint8 information_code::get_severity_class() const {
	return severity_class;
}


/**
 * Sets the severity class.
 *
 * You can use the severity_class_t enum to generate valid values.
 *
 * @param severity the severity class (4 bit)
 */
void information_code::set_severity_class(uint8 severity) {
	severity_class = severity;
}


/**
 * Returns the response code.
 *
 * The response codes depend on the severity class. To test the return values,
 * use the enums defined for each severity class.
 *
 * @return the response code
 */
uint8 information_code::get_response_code() const {
	return response_code;
}


/**
 * Sets the response code.
 *
 * This depends on the severity class. Use the enums defined for each class to
 * obtain valid values.
 *
 * @param code the response code
 */
void information_code::set_response_code(uint8 code) {
	response_code = code;
}


/**
 * Returns the object type this response is about.
 *
 * This may be 0 if no object is concerned.
 *
 * @return a NATFW object type (12 bit)
 */
uint16 information_code::get_response_object_type() const {
	return response_object_type;
}


/**
 * Sets the object type this response is about.
 *
 * This may be set to 0 if no object is concerned.
 *
 * @param obj_type a NATFW object type (12 bit)
 */
void information_code::set_response_object_type(uint16 obj_type) {
	response_object_type = obj_type;
}


/**
 * Check if this object indicates success.
 *
 * @return true if the severity field indicates success 
 */
bool information_code::is_success() const {
	return get_severity_class() == sc_success
		&& get_response_code() == suc_successfully_processed;
}

// EOF
