/// ----------------------------------------*- mode: C++; -*--
/// @file external_address.cpp
/// The External Address Object.
/// ----------------------------------------------------------
/// $Id: external_address.cpp 2558 2007-04-04 15:17:16Z bless $
/// $HeadURL: https://svn.tm.kit.edu/nsis/natfw-nslp/trunk/src/msg/external_address.cpp $
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
#include "msg/external_address.h"
#include "logfile.h"


using namespace natfw::msg;


const char *const external_address::ie_name = "external_address";


/**
 * Default constructor.
 */
external_address::external_address()
		: natfw_object(OBJECT_TYPE, tr_optional),
		  address(), port(0) {

	// nothing to do
}


/**
 * Constructor for manual use.
 *
 * Note: The hostaddress class also supports IPv6 addresses, but here only
 * IPv4 is allowed!
 *
 * @param addr an IPv4 IP address
 * @param port a port number
 */
external_address::external_address(hostaddress addr, uint16 port,
		treatment_t treatment)
		: natfw_object(OBJECT_TYPE, treatment),
		  address(addr), port(port) {

	// only IPv4 addresses may be used
	assert( addr.is_ipv4() );
}


external_address::~external_address() {
	// nothing to do
}


external_address *external_address::new_instance() const {
	external_address *q = NULL;
	catch_bad_alloc( q = new external_address() );
	return q;
}


external_address *external_address::copy() const {
	external_address *q = NULL;
	catch_bad_alloc( q = new external_address(*this) );
	return q;
}


bool external_address::deserialize_body(NetMsg &msg, uint16 body_length,
		IEErrorList &err, bool skip) {

	set_port(msg.decode16());
	msg.set_pos_r(2);	// skip 2 bytes of reserved space

	// parse and set IPv4 address
	struct in_addr in;
	msg.decode(in);

	hostaddress ha = hostaddress();
	ha.set_ip(in);
	set_address(ha);

	// TODO: check for errors

	return true; // IP address and port look valid.
}


void external_address::serialize_body(NetMsg &msg) const {
	struct in_addr in;
	get_address().get_ip(in);	// 'in' is set to the IP address

	msg.encode16(get_port());
	msg.encode16(0); // reserved part
	msg.encode(in); // IPv4 address
}


size_t external_address::get_serialized_size(coding_t coding) const {
	return HEADER_LENGTH + 8;
}


// All values are valid.
bool external_address::check_body() const {
	// Check for an unset port or uninitialized IP address.
	if ( get_address() == hostaddress() )
		return false;
	else
		return true;
}


bool external_address::equals_body(const natfw_object &obj) const {

	const external_address *other
		= dynamic_cast<const external_address *>(&obj);

	return other != NULL && get_port() == other->get_port()
		&& get_address() == other->get_address();
}


const char *external_address::get_ie_name() const {
	return ie_name;
}


ostream &external_address::print_attributes(ostream &os) const {
	return os << ", address=" << get_address().get_ip_str()
		<< ", port=" << get_port();
}


/**
 * Returns the port number.
 *
 * @return the port number.
 */
uint16 external_address::get_port() const {
	return port;
}


/**
 * Sets the port number.
 *
 * @param num the port number
 */
void external_address::set_port(uint16 num) {
	port = num;
}


/**
 * Returns the IPv4 address.
 *
 * @return the IPv4 address
 */
hostaddress external_address::get_address() const {
	return address;
}


/**
 * Sets the IPv4 address.
 *
 * Note: The hostaddress class also supports IPv6 addresses, but here only
 * IPv4 is allowed!
 *
 * @param num the IPv4 address
 */
void external_address::set_address(hostaddress addr) {
	// only IPv4 addresses may be used
	assert( addr.is_ipv4() );

	address = addr;
}


// EOF
