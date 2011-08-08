/// ----------------------------------------*- mode: C++; -*--
/// @file natfw_create.cpp
/// Implementation of the NATFW CREATE Message.
/// ----------------------------------------------------------
/// $Id: natfw_create.cpp 2896 2008-02-21 00:54:43Z bless $
/// $HeadURL: https://svn.tm.kit.edu/nsis/natfw-nslp/trunk/src/msg/natfw_create.cpp $
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
#include <openssl/rand.h>

#include "logfile.h"

#include "msg/natfw_ie.h"
#include "msg/natfw_create.h"
#include "msg/natfw_object.h"


using namespace natfw::msg;
using namespace protlib::log;


/**
 * Constructor.
 *
 * Only basic initialization is done. No NATFW objects exist yet. All other
 * attributes are set to default values.
 *
 * @param proxy_mode set to true, if proxy mode is enabled
 */
natfw_create::natfw_create(bool proxy_mode)
		: natfw_msg(MSG_TYPE, proxy_mode) {

	// nothing to do
}


/**
 * Copy constructor.
 *
 * Makes a deep copy of the object passed as an argument.
 *
 * @param other the object to copy
 */
natfw_create::natfw_create(const natfw_create &other)
		: natfw_msg(other) {
	
	// nothing else to do
}


/**
 * Destructor.
 *
 * Deletes all objects this message contains.
 */
natfw_create::~natfw_create() {
	// Nothing to do, parent class handles this.
}


natfw_create *natfw_create::new_instance() const {
	natfw_create *inst = NULL;
	catch_bad_alloc(inst = new natfw_create());
	return inst;
}


natfw_create *natfw_create::copy() const {
	natfw_create *copy = NULL;
	catch_bad_alloc(copy = new natfw_create(*this));
	return copy;
}


/* TODO: implement the check() method.
bool natfw_msg::check() const {
	
	// Error: no objects available
	if ( get_num_objects() == 0 )
		return false;

	// Check all objects for errors.
	for ( obj_iter i = objects.begin(); i != objects.end(); i++ ) {
		const IE *obj = i->second;

		if ( obj->check() == false )
			return false;
	}

	return true; // no error found
}
*/

void natfw_create::register_ie(IEManager *iem) const {
	iem->register_ie(cat_natfw_msg, get_msg_type(), 0, this);
}


/**
 * Set the desired session lifetime.
 *
 * @param seconds the session lifetime in seconds
 */
void natfw_create::set_session_lifetime(uint32 seconds) {
	set_object(new session_lifetime(seconds, natfw_object::tr_mandatory));
}


/**
 * Return the desired session lifetime.
 *
 * @return the session lifetime in seconds
 */
uint32 natfw_create::get_session_lifetime() const {
	session_lifetime *lt = dynamic_cast<session_lifetime *>(
		get_object(session_lifetime::OBJECT_TYPE));

	if ( lt == NULL )
		return 0; // TODO: use a default constant here?
	else
		return lt->get_value();
}


/**
 * Sets the NAT/firewall policy.
 *
 * @param rule_action the action of the policy rule
 * @param sub_ports the number of subsequent ports affected (0 or 1 allowed)
 */
void natfw_create::set_extended_flow_info(
		extended_flow_info::rule_action_t action,
		uint16 sub_ports) {

	set_object(new extended_flow_info(action, sub_ports,
		natfw_object::tr_mandatory));
}


/**
 * Returns the action of the policy rule.
 * 
 * @return currently either allow or deny
 */
extended_flow_info::rule_action_t natfw_create::get_rule_action() const {
	extended_flow_info *fi = dynamic_cast<extended_flow_info *>(
		get_object(extended_flow_info::OBJECT_TYPE));

	if ( fi == NULL )
		return extended_flow_info::ra_deny; // TODO: correct?
	else
		return extended_flow_info::rule_action_t(fi->get_rule_action());
}


/**
 * Returns the number of subsequent port the policy rule applies.
 *
 * @return currently either 0 or 1
 */
uint16 natfw_create::get_subsequent_ports() const {
	extended_flow_info *fi = dynamic_cast<extended_flow_info *>(
		get_object(extended_flow_info::OBJECT_TYPE));

	if ( fi == NULL )
		return 0; // TODO: Not critical, but use default constant here? 
	else
		return fi->get_sub_ports();

}


/**
 * Sets the message sequence number.
 *
 * See the msg_sequence_number object for more information.
 *
 * @param msn the value of the message sequence number
 */
void natfw_create::set_msg_sequence_number(uint32 msn) {
	set_object(new msg_sequence_number(msn, natfw_object::tr_mandatory));
}


/**
 * Set the nonce.
 *
 * When in proxy mode, the nonce object's treatment is set to mandatory.
 *
 * @param value the nonce value
 */
void natfw_create::set_nonce(uint32 value) {
	if ( is_proxy_mode() )
		set_object(new nonce(value, natfw_object::tr_mandatory));
	else
		set_object(new nonce(value, natfw_object::tr_optional));
}


/**
 * Set the nonce to a randomly generated value.
 *
 * When in proxy mode, the nonce object's treatment is set to mandatory.
 */
void natfw_create::set_nonce() {
	uint32 value;
	int ret = RAND_bytes((unsigned char *) &value, sizeof(value));
	assert( ret == 1 );

	set_nonce(value);
}


/**
 * Return the nonce.
 *
 * A nonce is only present in proxy mode, so this method should only be
 * used in proxy mode. TODO: check if that's true!
 *
 * @return the nonce's value
 */
uint32 natfw_create::get_nonce() const {
	nonce *n = dynamic_cast<nonce *>(
		get_object(nonce::OBJECT_TYPE));

	if ( n == NULL )
		return 0; // TODO: use a default constant here?
	else
		return n->get_value();
}




/**
 * Sets the ICMP Types.
 *
 * The types parameter is copied.
 *
 * @param types a list of ICMP type IDs
 */
void natfw_create::set_icmp_types(const std::list<uint8> &types) {
	set_object(new icmp_types(types, natfw_object::tr_optional));
}

// EOF
