/// ----------------------------------------*- mode: C++; -*--
/// @file data_terminal_info.cpp
/// The Data Terminal Information Object.
/// ----------------------------------------------------------
/// $Id: data_terminal_info.cpp 2558 2007-04-04 15:17:16Z bless $
/// $HeadURL: https://svn.tm.kit.edu/nsis/natfw-nslp/trunk/src/msg/data_terminal_info.cpp $
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

#include "msg/data_terminal_info.h"


using namespace natfw::msg;


const char *const data_terminal_info::ie_name = "data_terminal_info";


/**
 * Default constructor.
 */
data_terminal_info::data_terminal_info()
		: natfw_object(OBJECT_TYPE, tr_optional),
		  sender_address("0.0.0.0"), sender_prefix(0),
		  protocol_present(false), port_numbers_present(false),
		  ipsec_spi_present(false) {

	// nothing to do
}


/**
 * Constructor for manual use.
 *
 * @param address the data sender's address (must be IPv4!)
 * @param prefix prefix length of the data sender's address in bits (0 to 32)
 * @param treatment the NATFW object treatment
 */
data_terminal_info::data_terminal_info(hostaddress address, uint8 prefix,
		treatment_t treatment)
		: natfw_object(OBJECT_TYPE, treatment), sender_address(address),
		  sender_prefix(prefix), protocol_present(false),
		  port_numbers_present(false), ipsec_spi_present(false) {

	assert( address.is_ipv4() );
	assert( prefix <= 32 );
}


data_terminal_info::~data_terminal_info() {
	// nothing to do
}


data_terminal_info *data_terminal_info::new_instance() const {
	data_terminal_info *inst = NULL;
	catch_bad_alloc( inst = new data_terminal_info() );
	return inst;
}


data_terminal_info *data_terminal_info::copy() const {
	data_terminal_info *inst = NULL;
	catch_bad_alloc( inst = new data_terminal_info(*this) );
	return inst;
}


bool data_terminal_info::deserialize_body(NetMsg &msg, uint16 body_length,
		IEErrorList &err, bool skip) {

	uint32 tmp = msg.decode16();

	protocol_present = (tmp >> 15) & 1;
	port_numbers_present = (tmp >> 14 ) & 1;
	ipsec_spi_present = (tmp >> 13 ) & 1;

	set_prefix(msg.decode8());
	if ( sender_prefix > 32 )
		catch_bad_alloc( err.put(
			new PDUSyntaxError(CODING, get_category(), OBJECT_TYPE,
			0, msg.get_pos()-1, "IP prefix out of bounds") )
		);

	// only meaningful if the protocol flag is set
	if ( protocol_present )
		set_protocol(msg.decode8());
	else
		msg.set_pos_r(1); // skip one byte

	if ( has_port_numbers() ) {
		set_destination_port(msg.decode16());
		set_source_port(msg.decode16());
	}

	if ( has_ipsec_spi() )
		set_ipsec_spi(msg.decode32());

	// parse and set IPv4 address
	struct in_addr in;
	msg.decode(in);

	hostaddress ha = hostaddress();
	ha.set_ip(in);
	set_address(ha);

	return true; // success, all values are syntactically valid
}


void data_terminal_info::serialize_body(NetMsg &msg) const {

	uint16 flags = (has_protocol() << 15) | (has_port_numbers() << 14)
			| (has_ipsec_spi() << 13);

	msg.encode16(flags);

	msg.encode8(get_prefix());

	msg.encode8( has_protocol() ? get_protocol() : 0 );

	if ( has_port_numbers() ) {
		msg.encode16(get_destination_port());
		msg.encode16(get_source_port());
	}

	if ( has_ipsec_spi() )
		msg.encode32(get_ipsec_spi());

	// last field: IPv4 address
	struct in_addr in;
	get_address().get_ip(in);	// 'in' is set to the IP address
	msg.encode(in); // IPv4 address
}


size_t data_terminal_info::get_serialized_size(coding_t coding) const {
	return HEADER_LENGTH + 4 + ( has_port_numbers() ? 4 : 0 )
		+ ( has_ipsec_spi() ? 4 : 0 ) + 4;
}


// All values are valid.
bool data_terminal_info::check_body() const {

	// 'Protocol error' (0x3), 'Invalid Flag-Field combination' (0x09)
	if ( has_port_numbers() && has_ipsec_spi() )
		return false;

	// 'Protocol error' (0x3), 'Invalid Flag-Field combination' (0x09)
	if ( ( has_port_numbers() || has_ipsec_spi() ) && ! has_protocol() )
		return false;

	return sender_prefix <= 32;
}


bool data_terminal_info::equals_body(const natfw_object &obj) const {

	const data_terminal_info *other
		= dynamic_cast<const data_terminal_info *>(&obj);

	if ( other == NULL )
		return false;

	if ( has_protocol() != other->has_protocol()
			|| ( has_protocol() && other->has_protocol()
				&& get_protocol() != other->get_protocol() ) )
		return false;

	if ( has_port_numbers() != other->has_port_numbers()
			|| ( has_port_numbers() && other->has_port_numbers()
		&& ( get_destination_port() != other->get_destination_port()
			|| get_source_port() != other->get_source_port() ) ) )
		return false;

	if ( has_ipsec_spi() != other->has_ipsec_spi()
			|| ( has_ipsec_spi() && other->has_ipsec_spi()
				&& get_ipsec_spi() != other->get_ipsec_spi() ) )
		return false;

	return get_address() == other->get_address()
		&& get_prefix() == other->get_prefix();
}


const char *data_terminal_info::get_ie_name() const {
	return ie_name;
}


ostream &data_terminal_info::print_attributes(ostream &os) const {
	os << ", address=" << get_address().get_ip_str()
		<< "/" << int(get_prefix());

	if ( has_protocol() )
		os << ", protocol=" << int(get_protocol());

	if ( has_port_numbers() )
		os << ", dst_port=" << get_destination_port()
			<< ", src_port=" << get_source_port();

	if ( has_ipsec_spi() )
		os << ", ipsec_spi=" << get_ipsec_spi();

	return os;
}


/**
 * Return the data sender's address.
 *
 * @return the sender address (an IPv4 host address)
 */
hostaddress data_terminal_info::get_address() const {
	return sender_address;
}


/**
 * Set the sender address.
 *
 * @param address an IPv4 host address
 */
void data_terminal_info::set_address(hostaddress address) {
	assert( address.is_ipv4() );
	sender_address = address;
}


/**
 * Return the prefix length of the sender address.
 *
 * @return the prefix length (0 <= prefix <= 32)
 */
uint8 data_terminal_info::get_prefix() const {
	return sender_prefix;
}


/**
 * Set the prefix length of the sender address.
 *
 * @param length the prefix length (0 <= prefix <= 32)
 */
void data_terminal_info::set_prefix(uint8 length) {
	assert( length <= 32 );
	sender_prefix = length;
}


/**
 * Checks if the protocol field is present.
 *
 * @return true if the protocol field is present.
 */
bool data_terminal_info::has_protocol() const {
	return protocol_present;
}


/**
 * Returns the protocol ID.
 *
 * This may only be used if has_protocol() is true. Otherwise the value of
 * this method is undefined.
 *
 * @return the protocol ID
 */
uint8 data_terminal_info::get_protocol() const {
	return ( has_protocol() ? protocol : ~0 );
}


/**
 * Set the protocol ID.
 *
 * After using this method, has_protocol() will always return true.
 *
 * @param value the protocol ID
 */
void data_terminal_info::set_protocol(uint8 value) {
	protocol_present = true;
	protocol = value;
}


/**
 * Checks if the port numbers are present.
 *
 * @return true if the port numbers are present.
 */
bool data_terminal_info::has_port_numbers() const {
	return port_numbers_present;
}


/**
 * Return the source port.
 *
 * This may only be used if has_port_numbers() is true. Otherwise the value of
 * this method is undefined.
 *
 * @return the port number
 */
uint16 data_terminal_info::get_source_port() const {
	return ( has_port_numbers() ? src_port : ~0 );
}


/**
 * Set the source port.
 *
 * After using this method, has_port_numbers() will always return true. After
 * calling this method, you also have to set the destination port!
 *
 * @param value the port number
 */
void data_terminal_info::set_source_port(uint16 value) {
	port_numbers_present = true;
	src_port = value;
}


/**
 * Return the destination port.
 *
 * This may only be used if has_port_numbers() is true. Otherwise the value of
 * this method is undefined.
 *
 * @return the port number
 */
uint16 data_terminal_info::get_destination_port() const {
	return ( has_port_numbers() ? dest_port : ~0 );
}


/**
 * Set the destination port.
 *
 * After using this method, has_port_numbers() will always return true. After
 * calling this method, you also have to set the source port!
 *
 * @param value the port number
 */
void data_terminal_info::set_destination_port(uint16 value) {
	port_numbers_present = true;
	dest_port = value;
}


/**
 * Checks if the IPsec SPI field is present.
 *
 * @return true if the IPsec SPI field is present.
 */
bool data_terminal_info::has_ipsec_spi() const {
	return ipsec_spi_present;
}


/**
 * Returns the IPsec SPI.
 *
 * This may only be used if has_ipsec_spi() is true. Otherwise the value of
 * this method is undefined.
 *
 * @return the IPsec SPI
 */
uint32 data_terminal_info::get_ipsec_spi() const {
	return ( has_ipsec_spi() ? ipsec_spi : ~0 );
}


/**
 * Set the IPsec SPI.
 *
 * After using this method, has_ipsec_spi() will always return true.
 *
 * @param value the IPsec SPI
 */
void data_terminal_info::set_ipsec_spi(uint32 value) {
	ipsec_spi_present = true;
	ipsec_spi = value;
}


// EOF
