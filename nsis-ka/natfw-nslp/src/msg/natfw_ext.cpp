/// ----------------------------------------*- mode: C++; -*--
/// @file natfw_ext.cpp
/// Implementation of the NATFW EXT Message.
/// ----------------------------------------------------------
/// $Id: natfw_ext.cpp 2896 2008-02-21 00:54:43Z bless $
/// $HeadURL: https://svn.tm.kit.edu/nsis/natfw-nslp/trunk/src/msg/natfw_ext.cpp $
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
#include "msg/natfw_ext.h"
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
natfw_ext::natfw_ext(bool proxy_mode)
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
natfw_ext::natfw_ext(const natfw_ext &other)
		: natfw_msg(other) {
	
	// nothing else to do
}


/**
 * Destructor.
 *
 * Deletes all objects this message contains.
 */
natfw_ext::~natfw_ext() {
	// Nothing to do, parent class handles this.
}


natfw_ext *natfw_ext::new_instance() const {
	natfw_ext *inst = NULL;
	catch_bad_alloc(inst = new natfw_ext());
	return inst;
}


natfw_ext *natfw_ext::copy() const {
	natfw_ext *copy = NULL;
	catch_bad_alloc(copy = new natfw_ext(*this));
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

void natfw_ext::register_ie(IEManager *iem) const {
	iem->register_ie(cat_natfw_msg, get_msg_type(), 0, this);
}


/**
 * Set the desired session lifetime.
 *
 * @param seconds the session lifetime in seconds
 */
void natfw_ext::set_session_lifetime(uint32 seconds) {
	set_object(new session_lifetime(seconds, natfw_object::tr_mandatory));
}


/**
 * Return the desired session lifetime.
 *
 * @return the session lifetime in seconds
 */
uint32 natfw_ext::get_session_lifetime() const {
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
void natfw_ext::set_extended_flow_info(
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
extended_flow_info::rule_action_t natfw_ext::get_rule_action() const {
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
uint16 natfw_ext::get_subsequent_ports() const {
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
void natfw_ext::set_msg_sequence_number(uint32 msn) {
	set_object(new msg_sequence_number(msn, natfw_object::tr_mandatory));
}


/**
 * Set the nonce.
 *
 * When in proxy mode, the nonce object's treatment is set to mandatory.
 *
 * @param value the nonce value
 */
void natfw_ext::set_nonce(uint32 value) {
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
void natfw_ext::set_nonce() {
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
uint32 natfw_ext::get_nonce() const {
	nonce *n = dynamic_cast<nonce *>(
		get_object(nonce::OBJECT_TYPE));

	if ( n == NULL )
		return 0; // TODO: use a default constant here?
	else
		return n->get_value();
}


/**
 * Set the ICMP Types.
 *
 * The types parameter is copied.
 *
 * @param types a list of ICMP type IDs
 */
void natfw_ext::set_icmp_types(const std::list<uint8> &types) {
	set_object(new icmp_types(types, natfw_object::tr_optional));
}


/**
 * Set the data sender's host address and address prefix.
 *
 * @param address the data sender's address (must be IPv4!)
 * @param prefix prefix length of the data sender's address in bits (0 to 32)
 */
void natfw_ext::set_data_terminal_info(hostaddress address, uint8 prefix) {
	assert( address.is_ipv4() );
	assert( prefix <= 32 );
	set_object(new data_terminal_info(address, prefix,
		natfw_object::tr_optional));
}


/**
 * Set a valid subset of the NATFW_DTINFO_IPv4 object.
 *
 * Any of the three parameters may be set to 0, which acts as a wildcard.
 *
 * @param protocol the IP protocol (TCP, UDP, etc.)
 * @param dr_port the data receiver's port
 * @param ds_port the data sender's port
 */
void natfw_ext::set_data_terminal_info(
		uint8 protocol, uint16 dr_port, uint16 ds_port) {

	data_terminal_info *dti = new data_terminal_info();

	if ( protocol != 0 )
		dti->set_protocol(protocol);

	dti->set_destination_port(dr_port);
	dti->set_source_port(ds_port);

	set_object(dti);
}


/**
 * Return the data sender's address.
 *
 * @return the sender address (an IPv4 host address)
 */
hostaddress natfw_ext::get_ds_address() const {
	data_terminal_info *dti = dynamic_cast<data_terminal_info *>(
		get_object(data_terminal_info::OBJECT_TYPE));

	if ( dti == NULL )
		return hostaddress("0.0.0.0"); // TODO: use a default constant?
	else
		return dti->get_address();
}


/**
 * Return the data receiver's port number.
 *
 * @return the port number (may be 0, in this case it's a wildcard)
 */
uint16 natfw_ext::get_dr_port() const {
	data_terminal_info *dti = dynamic_cast<data_terminal_info *>(
		get_object(data_terminal_info::OBJECT_TYPE));

	if ( dti == NULL || ! dti->has_port_numbers() )
		return 0; // 0 means wildcarded, best we can do
	else
		return dti->get_destination_port();
}


/**
 * Return the data sender's port number.
 *
 * @return the port number (may be 0, in this case it's a wildcard)
 */
uint16 natfw_ext::get_ds_port() const {
	data_terminal_info *dti = dynamic_cast<data_terminal_info *>(
		get_object(data_terminal_info::OBJECT_TYPE));

	if ( dti == NULL || ! dti->has_port_numbers() )
		return 0; // 0 means wildcarded, best we can do
	else
		return dti->get_source_port();
}


/**
 * Return the protocol used for the data flow.
 *
 * @return the IP protocol number (may be 0, in this case it's a wildcard)
 */
uint16 natfw_ext::get_protocol() const {
	data_terminal_info *dti = dynamic_cast<data_terminal_info *>(
		get_object(data_terminal_info::OBJECT_TYPE));

	if ( dti == NULL || ! dti->has_protocol() )
		return 0; // 0 means wildcarded, best we can do
	else
		return dti->get_protocol();
}


// EOF
