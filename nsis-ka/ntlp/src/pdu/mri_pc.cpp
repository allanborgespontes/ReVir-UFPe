/// ----------------------------------------*- mode: C++; -*--
/// @file mri_pc.cpp
/// GIST Path coupled MRI (Message Routing Information)
/// ----------------------------------------------------------
/// $Id: mri_pc.cpp 5249 2010-04-26 08:40:59Z roehricht $
/// $HeadURL: https://svn.tm.kit.edu/nsis/ntlp/trunk/src/pdu/mri_pc.cpp $
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
#include <iostream>
#include <sstream>
#include <iomanip>
#include <string>

#include "logfile.h"
#include "mri_pc.h"

using namespace ntlp;
using namespace protlib;

/**
 * @ingroup mri
 * @{
 */

const char *const mri_pathcoupled::iename = "mri_pathcoupled";

const char *mri_pathcoupled::get_ie_name() const {
	return iename;
}


mri_pathcoupled *mri_pathcoupled::new_instance() const {
	mri_pathcoupled* sh = NULL;
	catch_bad_alloc(sh = new mri_pathcoupled());
	return sh;
}


mri_pathcoupled *mri_pathcoupled::copy() const {
	mri_pathcoupled* sh = NULL;
	catch_bad_alloc(sh = new mri_pathcoupled(*this));
	return sh;
}


// Default version of deserialize(): Expects and parses the object header.
mri_pathcoupled *mri_pathcoupled::deserialize(NetMsg &msg, coding_t cod,
		IEErrorList &errorlist, uint32 &bytes_read, bool skip) {

	return deserializeEXT(msg, cod, errorlist, bytes_read, skip, true);
}


mri_pathcoupled *mri_pathcoupled::deserializeNoHead(NetMsg &msg, coding_t cod,
		IEErrorList &errorlist, uint32 &bytes_read, bool skip) {

	return deserializeEXT(msg, cod, errorlist, bytes_read, skip, false);
}


mri_pathcoupled *mri_pathcoupled::deserializeEXT(NetMsg &msg, coding_t cod,
		IEErrorList &errorlist, uint32 &bytes_read, bool skip,
		bool read_header) {
	
	uint32 ielen = 0;

	// check arguments (sets bytes_read to 0)
	if ( ! check_deser_args(cod, errorlist, bytes_read) ) 
		return NULL;

	// decode header
	if ( read_header ) {
		uint16 len = 0;
		uint32 saved_pos = 0;
		uint32 resume = 0;

		if ( ! decode_header_ntlpv1(msg, len, ielen, saved_pos, resume,
				errorlist, bytes_read, skip) ) 
			return NULL;
		//std::cout << "bytes_read: " << bytes_read << std::endl;
		// TODO: bytes_read returns 0. Bug in decode_header_ntlpv1()!
		bytes_read += 4;
	}


	// Next 16 bits: MRM-ID (7 bits), N-Flag (1 bit), Reserved (8 bits)
	mrm = msg.decode8();
	nat_flag = msg.decode8() >> 7;
	bytes_read += 2;
    
	// Next 16 bits: IP Version (4 bits), Flags (7 bits), Reserved (9 bits)
	uint16 flagfield = msg.decode16();
	ip_version = flagfield >> 12;
	flags = flagfield & 0x0fe0;	// clear higest 4 and lowest 5 bits
	bytes_read += 2;


	/*
	 * The following fields contain either two IPv4 or two IPv4 addresses;
	 * first source address, then destination address.
	 */
	if ( ip_version == 4 ) {
		struct in_addr tempaddr4;

		msg.decode(tempaddr4);
		sourceaddress.set_ip(tempaddr4);
		msg.decode(tempaddr4);
		destaddress.set_ip(tempaddr4);

		bytes_read += 8;
	} 
	else if ( ip_version == 6 ) {
		struct in6_addr tempaddr6;

		msg.decode(tempaddr6);
		sourceaddress.set_ip(tempaddr6);
		msg.decode(tempaddr6);
		destaddress.set_ip(tempaddr6);

		bytes_read += 32;
	}
	else {
		/*
		 * Error: Neither IPv4 nor IPv6. This is fatal, as we have no
		 *        chance to figure out the binary format of this object.
		 */
		ERRCLog("mri_pc", "Unknown IP version: " << (int) ip_version);
		// TODO: Is this the correct error?
		errorlist.put(new GIST_InvalidFlagFieldCombination(
			protocol_v1, this));
		
		return NULL;
	}

	// Next 32 bits: source prefix (8 bits), destination prefix (8 bits),
	// IP protocol (8 bits), DSCP/Traffic Class (6 bits), Reserved (2 bits)
	sourceprefix = msg.decode8();
	destprefix = msg.decode8();
	protocol = msg.decode8();
	ds_field = msg.decode8() >> 2;
	bytes_read += 4;

	// Test if a flow label is present. If it is, decode it.
	if (flags & Flow_Label) {
		// Next 32 bits: Reserved (12 bits), Flow Label (20 bits)
		flowlabel = msg.decode32() & 0xffffff;
		bytes_read += 4;
	}

	// Decode the SPI in case it is present.
	if (flags & SPI) {
		// Next 32 bits: Security Parameter Index (32 bits)
		spi = msg.decode32();
		bytes_read += 4;
	}

	// Decode ports if either the 'A' or 'B' flag is set.
	if ( (flags & Destination_Port) || (flags & Source_Port) ) {
		// Next 32 bits: source port (16 bits), dest. port (16 bits)
		uint16 srcport = msg.decode16();
		uint16 dstport = msg.decode16();

		/*
		 * As per spec: If the corresponding flag for a port isn't set,
		 * the port number is considered a full wildcard. If the flag
		 * is set and the port is 0, then it should be interpreted as
		 * an "unknown port".
		 */
		sourceport = (flags & Source_Port) ? srcport : 0;
		destport = (flags & Destination_Port) ? dstport : 0;

		bytes_read += 4;
	}

	// Check for correct length.
	if ( read_header && (ielen != get_serialized_size(cod)) ) {
		ERRCLog("mri_pc", "Incorrect Object Length Error: MRI says: "
			<< ielen << ", read: " << bytes_read
			<< " but should be:" << get_serialized_size(cod));
		errorlist.put(
			new GIST_IncorrectObjectLength(protocol_v1, this));
	}


	// Check for incorrect Flag-Field Combinations.

	// For IPv4, the Flow Label is not allowed.
	if ( (ip_version == 4) && (flags & Flow_Label) ) {
		ERRCLog("mri_pc", "Flow Label is present but IP version is 4");
		errorlist.put(new GIST_InvalidFlagFieldCombination(
			protocol_v1, this));
	}


	// The 'S' flag is only allowed if the 'P' flag is set, too.
	if ( (flags & SPI) && ! (flags & IP_protocol) ) {
		ERRCLog("mri_pc", "SPI is set, but no IP protocol is defined");
		errorlist.put(new GIST_InvalidFlagFieldCombination(
			protocol_v1, this));
	}

	// The destination port may only be present if the L4 protocol is set.
	if ( (flags & Destination_Port) && ! (flags & IP_protocol) ) {
		ERRCLog("mri_pc", "destination port is set, but no IP "
			<< "protocol is defined");
		errorlist.put(new GIST_InvalidFlagFieldCombination(
				protocol_v1, this));
	}

	// The source port may only be present if the L4 protocol is set.
	if ( (flags & Source_Port) && ! (flags & IP_protocol) ) {
		ERRCLog("mri_pc", "source port is set, but no IP Protocol "
			<< "is defined");
		errorlist.put(new GIST_InvalidFlagFieldCombination(
			protocol_v1, this));
	}

	/*
	 * Sanity checking on source/destination prefixes.
	 */
	if ( ip_version == 4 ) {
		if ( sourceprefix > 32 || destprefix > 32 ) {
			ERRCLog("mri_pc", "invalid prefix for IPv4:"
				<< " sourceprefix=" << (int) sourceprefix
				<< " destprefix=" << (int) destprefix);
			errorlist.put(new GIST_InvalidFlagFieldCombination(
				protocol_v1, this));
		}
	}
	else if ( ip_version == 6 ) {
		if ( sourceprefix > 128 || destprefix > 128 ) {
			ERRCLog("mri_pc", "invalid prefix for IPv4:"
				<< " sourceprefix=" << (int) sourceprefix
				<< " destprefix=" << (int) destprefix);
			errorlist.put(new GIST_InvalidFlagFieldCombination(
				protocol_v1, this));
		}
	}

	return this;
}



// Default version of serialize(): This also writes the object header.
void mri_pathcoupled::serialize(NetMsg &msg, coding_t cod,
		uint32 &bytes_written) const {

	serializeEXT(msg, cod, bytes_written, true);
}


// Serializes the object, but leave out the object header.
void mri_pathcoupled::serializeNoHead(NetMsg &msg, coding_t cod,
		uint32 &bytes_written) const {

	serializeEXT(msg, cod, bytes_written, false);
}



void mri_pathcoupled::serializeEXT(NetMsg &msg, coding_t cod,
		uint32 &bytes_written, bool write_header) const {

	uint32 ielen = get_serialized_size(cod); // includes object TLV header
	if ( ! write_header)
		ielen -= header_length;

	// Check arguments and IE state. Side effect: bytes_written = 0
	check_ser_args(cod, bytes_written);
	
	uint32 savedpos= msg.get_pos();

	if ( write_header )
		encode_header_ntlpv1(msg, ielen-header_length);
	
	// Next 16 bits: MRM-ID (8 bits), N-Flag (1 bit), Reserved (7 bits)
	msg.encode8(mrm);
	msg.encode8(nat_flag << 7);

	// Next 16 bits: IP Version (4 bits), Flags (7 bits), Reserved (9 bits)
	msg.encode16( (ip_version << 12) | flags );

	// Next 32/128 bits: IPv4/IPv6 source address
	if ( sourceaddress.is_ipv6() ) {
		// Note: No new memory is allocated by hostaddress::get_ip().
		const struct in6_addr *ip6addr = sourceaddress.get_ip();

		if ( ip6addr != NULL )
			msg.encode(*ip6addr);
		else {
			// TODO: Could this really happen?
			ERRCLog("mri_pathcoupled::serialize()",
				"No valid IPv6 address in sourceaddress");
			msg.encode(in6addr_any);
		}
	}
	else if ( sourceaddress.is_ipv4() ) {
		struct in_addr ip4addr;
		sourceaddress.get_ip(ip4addr);
		msg.encode(ip4addr);
	}
	else
		assert( false );	// programming error


	// Next 32/128 bits: IPv4/IPv6 destination address
	if ( destaddress.is_ipv6() ) {
		const struct in6_addr *ip6addr = destaddress.get_ip();

		if ( ip6addr != NULL )
			msg.encode(*ip6addr);
		else {
			// TODO: See above. Can't happen.
			ERRCLog("mri_pathcoupled::serialize()",
				"No valid IPv6 address in destaddress");
		    msg.encode(in6addr_any);
		}
	}
	else if (destaddress.is_ipv4()) {
		struct in_addr ip4addr;
		destaddress.get_ip(ip4addr);
		msg.encode(ip4addr);
	}
	else
		assert( false );	// programming error

	// Next 32 bits: source prefix (8 bits), destination prefix (8 bits),
	// IP protocol (8 bits), DSCP/Traffic Class (6 bits), Reserved (2 bits)
	msg.encode8(sourceprefix);
	msg.encode8(destprefix);
	msg.encode8(protocol);
	msg.encode8(ds_field << 2); 

	if ( flags & Flow_Label )
		// Next 32 bits: Reserved (12 bits), Flow Label (20 bits)
		msg.encode32(flowlabel);

	if ( flags & SPI )
		// Next 32 bits: Security Parameter Index (32 bits)
		msg.encode32(spi);

	/*
	 * If the 'A' or 'B' flag is set, we have to encode both port numbers.
	 * In case only one port is set, the other port field has to be padded 
	 * with zeroes.
	 */
	if ( (flags & Source_Port) || (flags & Destination_Port) ) {
		// Next 16 bits: source port
		if ( flags & Source_Port )
			msg.encode16(sourceport);
		else
			msg.encode16(0);

		// Next 16 bits: destination port
		if ( flags & Destination_Port )
			msg.encode16(destport);
		else
			msg.encode16(0);
	}

	bytes_written = msg.get_pos()-savedpos;
}


// TODO: Actually check for invalid states, like IPv4 addresses and a set
//       flow label and the like.
bool mri_pathcoupled::check() const {
	return true;
}


size_t mri_pathcoupled::get_serialized_size(coding_t cod) const {

	if ( ! supports_coding(cod) )
		throw IEError(IEError::ERROR_CODING);
    
	// The first 32 bits are header and flags/ip_version.
	size_t size = 32;

	// Add 2 address fields of different size (IPv6 or IPv4).
	if ( ip_version == 6 )
		size += 256; 
	else if ( ip_version == 4 )
		size += 64;
	else
		assert( false );	// Invalid IP version.


	// Always present: 32 bits for prefixes, protocol etc.
	size+=32;

	// if flow label is there
	if ( flags & Flow_Label )
		size += 32;

	// if SPI is there
	if ( flags & SPI )
		size += 32;

	// if Ports are there
	if ( (flags & Source_Port) || (flags & Destination_Port) )
		size += 32;
  
	return (size/8) + header_length; // convert from bits to bytes + header
}


// We need a compare function for storing instances in hash maps.
bool mri_pathcoupled::operator==(const IE& ie) const {
	const mri_pathcoupled* sh = dynamic_cast<const mri_pathcoupled*>(&ie);
    
	return sh != NULL && (mrm == sh->mrm) && (nat_flag == sh->nat_flag)
		&& (ip_version == sh->ip_version) && (protocol == sh->protocol)
		&& (sourceaddress == sh->sourceaddress)
		&& (sourceport == sh->sourceport)
		&& (sourceprefix == sh->sourceprefix)
		&& (destaddress == sh->destaddress)
		&& (destport == sh->destport)
		&& (destprefix == sh->destprefix)
		&& (flowlabel == sh->flowlabel)
		&& (ds_field == sh->ds_field)
		&& (spi == sh->spi) && (flags == sh->flags);
}


ostream &mri_pathcoupled::print(ostream& os, uint32 level, const uint32 indent,
		const char* name) const {

	using std::endl;

	static const char *actions[] = { "Mandatory", "Ignore", "Forward" };

	if ( name != NULL && (name[0] != '\0') )
		os << name << " ";

	os << '<' << get_ie_name() << ">: (" << actions[action] << ")" << endl;

	os << "MRM-ID                : " << (int) mrm << endl; 
	os << "IP-Version            : " << (int) ip_version << endl;

	os << "Flags:                : "; 
	if (has_nat_flag())      	os << "N"; 
	if (flags & IP_protocol)      os << "P"; 
	if (flags & Source_Port)      os << "A";
	if (flags & Flow_Label)       os << "F";
	if (flags & Destination_Port) os << "B";
	if (flags & DS_Field)         os << "T";
	if (flags & SPI)              os << "S";
	if (flags & Direction)        os << "D";
	os << endl;

	os << "Source Address        : " << sourceaddress << endl;
	os << "Destination Address   : " << destaddress << endl;
	os << "Source Prefixlen      : " << (int) sourceprefix << endl;
	os << "Destination Prefixlen : " << (int) destprefix << endl;

	if ( flags & IP_protocol )
		os << "IP-Protocol           : "
			<< tsdb::getprotobynumber(protocol) << endl;

	if ( flags & SPI )
		os << "SPI                   : " << spi << endl;

	if ( flags & Flow_Label )
		os << "Flow Label            : " << (int) flowlabel << endl;

	if ( flags & Destination_Port )
		os << "Destination Port      : " << (int) destport << endl;

	if ( flags & Source_Port )
		os << "Source Port           : " << (int) sourceport << endl;

	if ( flags & DS_Field )
		os << "DSCP                  : " << (int) ds_field << endl;

	os << "Direction             : "
		<< ((flags & Direction) ? "Upstream" : "Downstream") << endl;

	return os;
}


istream& mri_pathcoupled::input(istream& is, bool istty, uint32 level,
		const uint32 indent, const char* name) {

	string s;
	uint32 i;

	nat_flag = false; // spec doesn't allow 'true' for the PC MRI
    

	if ( istty )
		cout << "IP-Version: ";
	is >> i;
	ip_version=i;


	if ( istty )
		cout << "Source Address: ";
	is >> s;
	sourceaddress = appladdress();
	sourceaddress.set_ip(s);


	if ( istty )
		cout << "Source Port: ";
	is >> i;
	sourceport = i;


	if ( istty )
		cout << "Destination Address: ";
    
	is >> s;
	destaddress = appladdress();
	destaddress.set_ip(s);


	if ( istty )
		cout << "Destination Port: ";
	is >> i;
	destport = i;


	if ( istty )
		cout << "Protocol (name): ";

	is >> s;
	protocol = tsdb::getprotobyname(s.c_str());
    

	if ( istty )
		cout << "Source Prefix Length: ";
	is >> i;
	sourceprefix =i;

	if ( istty )
		cout << "Destination Prefix Length: ";
	is >> i;
	destprefix = i;
	rebuild_flags();

	cout << "Upstream? <[d]/u>:";
	is >> s;
	if ( s=="u" )
		flags = flags | Direction;

	flowlabel=0;
	ds_field=0;
	spi=0;

	return is;
}


/**
 * Returns the address of the final IP hop for this MRI's direction.
 *
 * This an appladdress and no hostaddress, because it is used for underlying
 * transport connections
 */
appladdress *
mri_pathcoupled::determine_dest_address() const
{
  appladdress* returnaddr = new appladdress();

  if ( flags & Direction )
  {
    returnaddr->set_ip(sourceaddress);
  } else 
  {
    returnaddr->set_ip(destaddress);
  }
  
  returnaddr->set_protocol(prot_query_encap);
  
  return returnaddr;
}



// Invert direction flag; responder nodes stores MRI with inverted direction
void mri_pathcoupled::invertDirection() {

	if ( flags & Direction )
		flags = flags ^ Direction;
	else
		flags = flags | Direction;
}

//@}

// EOF
