/// ----------------------------------------*- mode: C++; -*--
/// @file natfw_response.cpp
/// Implementation of the NATFW RESPONSE Message.
/// ----------------------------------------------------------
/// $Id: natfw_response.cpp 2558 2007-04-04 15:17:16Z bless $
/// $HeadURL: https://svn.tm.kit.edu/nsis/natfw-nslp/trunk/src/msg/natfw_response.cpp $
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

#include "msg/natfw_ie.h"
#include "msg/natfw_response.h"
#include "msg/natfw_object.h"


using namespace natfw::msg;
using namespace protlib::log;


/**
 * Constructor.
 *
 * Only basic initialization is done. No NATFW objects exist yet. All other
 * attributes are set to default values.
 */
natfw_response::natfw_response() : natfw_msg(MSG_TYPE, false) {
	// nothing to do
}


/**
 * Copy constructor.
 *
 * Makes a deep copy of the object passed as an argument.
 *
 * @param other the object to copy
 */
natfw_response::natfw_response(const natfw_response &other)
		: natfw_msg(other) {
	
	// nothing else to do
}


/**
 * Create a response for the given CREATE message.
 *
 * Note that the passed message has to be fully initialized and valid.
 *
 * @param c a valid CREATE message
 * @param severity the severity class
 * @param response_code the response code, which depends on the severity class
 */
natfw_response::natfw_response(natfw_create *c, uint8 severity,
		uint8 response_code)
		: natfw_msg(MSG_TYPE, false) {

	assert( c != NULL );

	set_msg_sequence_number(c->get_msg_sequence_number());
	set_information_code(severity, response_code, c->get_msg_type());
}


/**
 * Create a response for the given EXT message.
 *
 * Note that the passed message has to be fully initialized and valid.
 *
 * @param ext a valid EXT message
 * @param severity the severity class
 * @param response_code the response code, which depends on the severity class
 */
natfw_response::natfw_response(natfw_ext *ext, uint8 severity,
		uint8 response_code)
		: natfw_msg(MSG_TYPE, false) {

	assert( ext != NULL );

	set_msg_sequence_number(ext->get_msg_sequence_number());
	set_information_code(severity, response_code, ext->get_msg_type());
}


/**
 * Destructor.
 *
 * Deletes all objects this message contains.
 */
natfw_response::~natfw_response() {
	// Nothing to do, parent class handles this.
}


natfw_response *natfw_response::new_instance() const {
	natfw_response *inst = NULL;
	catch_bad_alloc(inst = new natfw_response());
	return inst;
}


natfw_response *natfw_response::copy() const {
	natfw_response *copy = NULL;
	catch_bad_alloc(copy = new natfw_response(*this));
	return copy;
}


/* TODO: implement the check() method.
bool natfw_msg::check() const {
	if ( is_proxy_mode() )
		return false;
}
*/

void natfw_response::register_ie(IEManager *iem) const {
	iem->register_ie(cat_natfw_msg, get_msg_type(), 0, this);
}


/**
 * Set the desired session lifetime.
 *
 * @param seconds the session lifetime in seconds
 */
void natfw_response::set_session_lifetime(uint32 seconds) {
	set_object(new session_lifetime(seconds, natfw_object::tr_mandatory));
}


/**
 * Return the desired session lifetime.
 *
 * @return the session lifetime in seconds
 */
uint32 natfw_response::get_session_lifetime() const {
	session_lifetime *lt = dynamic_cast<session_lifetime *>(
		get_object(session_lifetime::OBJECT_TYPE));

	if ( lt == NULL )
		return 0; // TODO: use a default constant here?
	else
		return lt->get_value();
}


/**
 * Sets the message sequence number.
 *
 * See the msg_sequence_number object for more information.
 *
 * @param msn the value of the message sequence number
 */
void natfw_response::set_msg_sequence_number(uint32 msn) {
	set_object(new msg_sequence_number(msn, natfw_object::tr_mandatory));
}


/**
 * Sets the information code.
 *
 * This is used to indicate success or various degrees of failure.
 *
 * @param severity the severity class (4 bit)
 * @param response_code a response code, depending on the severity class
 * @param object_type the object type the response is about (12 bit)
 */
void natfw_response::set_information_code(uint8 severity, uint8 response_code,
                uint16 object_type) {

	set_object(new information_code(severity, response_code, object_type,
		natfw_object::tr_mandatory));
}


/**
 * Return the object type that triggered this response message.
 *
 * @return a NATFW object type (12 bit)
 */
uint16 natfw_response::get_response_object_type() const {
	information_code *ic = dynamic_cast<information_code *>(
		get_object(information_code::OBJECT_TYPE));

	if ( ic == NULL )
		return 0; // TODO: use a default constant here?
	else
		return ic->get_response_object_type(); // Note: response!
}


/**
 * Return the severity class code.
 *
 * @return the severity class as defined in information_code
 */
uint8 natfw_response::get_severity_class() const {
	information_code *ic = dynamic_cast<information_code *>(
		get_object(information_code::OBJECT_TYPE));

	if ( ic == NULL )
		return 0; // TODO: use a default constant here?
	else
		return ic->get_severity_class();
}


/**
 * Set the external address and port.
 *
 * @param addr the external host address
 * @param port the external port number
 */
void natfw_response::set_external_address(hostaddress addr, uint16 port) {
	set_object(new external_address(addr, port, natfw_object::tr_optional));
}


/**
 * Return the allocated external address at an edge NAT.
 *
 * This is where data should be sent to if the receiver is behind a NAT.
 *
 * @return an IPv4 hostaddress object
 */
hostaddress natfw_response::get_external_address() const {
	external_address *ea = dynamic_cast<external_address *>(
		get_object(external_address::OBJECT_TYPE));

	if ( ea == NULL )
		// return 0.0.0.0
		return hostaddress(); // TODO: use a default constant here?
	else
		return ea->get_address();
}


/**
 * Return the allocated external port at an edge NAT.
 *
 * This is where data should be sent to if the receiver is behind a NAT.
 *
 * @return a port number
 */
uint16 natfw_response::get_external_port() const {
	external_address *ea = dynamic_cast<external_address *>(
		get_object(external_address::OBJECT_TYPE));

	if ( ea == NULL )
		return 0; // TODO: use a default constant here?
	else
		return ea->get_port();
}


/**
 * Check if this response indicates success.
 *
 * @return true if the response indicates success
 */
bool natfw_response::is_success() const {
	information_code *io = dynamic_cast<information_code *>(
		get_object(information_code::OBJECT_TYPE));

	if ( io != NULL && io->is_success() )
		return true;
	else
		return false;
}


/**
 * Check if this is a response to the given CREATE message.
 *
 * This method can be used to find out if this RESPONSE message is really
 * the response to a previously sent CREATE message.
 *
 * @param msg a previously sent CREATE message
 * @return true if it is
 */
bool natfw_response::is_response_to(const natfw_create *msg) const {
	return get_msg_sequence_number() == msg->get_msg_sequence_number();
}


/**
 * Check if this is a response to the given EXT message.
 *
 * This method can be used to find out if this RESPONSE message is really
 * the response to a previously sent EXT message.
 *
 * @param msg a previously sent EXT message
 * @return true if it is
 */
bool natfw_response::is_response_to(const natfw_ext *msg) const {
	return get_msg_sequence_number() == msg->get_msg_sequence_number();
}

// EOF
