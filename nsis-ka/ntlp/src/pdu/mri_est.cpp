/// ----------------------------------------*- mode: C++; -*--
/// @file mri_est.cpp
/// GIST Explicit Signaling Target MRI (Message Routing Information)
/// ----------------------------------------------------------
/// $Id: mri_est.cpp 5249 2010-04-26 08:40:59Z roehricht $
/// $HeadURL: https://svn.tm.kit.edu/nsis/ntlp/trunk/src/pdu/mri_est.cpp $
// ===========================================================
//                      
// Copyright (C) 2007-2010, all rights reserved by
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
#include <cerrno>
#include <net/if.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>

#include "logfile.h"
#include "mri_est.h"

#include "readnl.h"

using namespace ntlp;
using namespace protlib;

/**
 * @addtogroup mri Message Routing Information Objects
 * @{
 */

const char *const mri_explicitsigtarget::iename = "mri_explicitsigtarget";

const char *mri_explicitsigtarget::get_ie_name() const {
	return iename;
}


mri_explicitsigtarget *mri_explicitsigtarget::new_instance() const {
	mri_explicitsigtarget* sh = NULL;
	catch_bad_alloc(sh = new mri_explicitsigtarget());
	return sh;
}


mri_explicitsigtarget *mri_explicitsigtarget::copy() const {
	mri_explicitsigtarget* sh = NULL;
	catch_bad_alloc(sh = new mri_explicitsigtarget(*this));
	return sh;
}


// Default version of deserialize(): Expects and parses the object header.
mri_explicitsigtarget *mri_explicitsigtarget::deserialize(NetMsg &msg, coding_t cod,
		IEErrorList &errorlist, uint32 &bytes_read, bool skip) {

	return deserializeEXT(msg, cod, errorlist, bytes_read, skip, true);
}


mri_explicitsigtarget *mri_explicitsigtarget::deserializeNoHead(NetMsg &msg, coding_t cod,
		IEErrorList &errorlist, uint32 &bytes_read, bool skip) {

	return deserializeEXT(msg, cod, errorlist, bytes_read, skip, false);
}


mri_explicitsigtarget *mri_explicitsigtarget::deserializeEXT(NetMsg &msg, coding_t cod,
		IEErrorList &errorlist, uint32 &bytes_read, bool skip,
		bool read_header) {
	
	uint32 ielen = 0;

	// check arguments (sets bytes_read to 0)
	if ( ! check_deser_args(cod, errorlist, bytes_read) ) 
		return NULL;

	// decode common object header
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
	
	uint32 savedpos= msg.get_pos();
	// Next 16 bits: MRM-ID (7 bits), N-Flag (1 bit), Reserved (8 bits)
	mrm = msg.decode8();
	nat_flag = msg.decode8() >> 7;

	// must go back because PC MRI needs also this information
	msg.set_pos(savedpos);
	// write correct mrm type for path coupled MRI object
	msg.encode8(mri_t_pathcoupled,false);
	uint32 mripc_bytes= 0;
	// deserialize PC MRI without object header
	mri_pathcoupled* mri_pcp= mri_pc.deserializeNoHead(msg, cod, errorlist, mripc_bytes, skip);
	bytes_read += mripc_bytes;
	// now revert buffer change
	uint32 mri_endpos= msg.get_pos();
	msg.set_pos(savedpos);
	msg.encode8(mrm,false);
	msg.set_pos(mri_endpos);

	if (mri_pcp == NULL)
	{
	  ERRCLog("mri_est", "Failure during parsing PathCoupled-MRI");
	  errorlist.put(new GIST_InvalidObject(protocol_v1,0,get_type()));
	  return NULL;
	}
    
	// Next 16 bits: IP Version (4 bits), Reserved (12 bits)
	uint16 flagfield = msg.decode16();
	uint8 dsig_ip_version = flagfield >> 12;
	bytes_read += 2;

	// Next 16 bits: IP Version (4 bits), Reserved (12 bits)
	flagfield = msg.decode16();
	uint16 ingress_ip_version = flagfield >> 12;
	bytes_read += 2;

	/*
	 * The following fields contain either two IPv4 or two IPv4 addresses;
	 * first source address, then destination address.
	 */
	if ( dsig_ip_version == 4 ) {
		struct in_addr tempaddr4;

		msg.decode(tempaddr4);
		origin_signaling_addr.set_ip(tempaddr4);
		msg.decode(tempaddr4);
		dest_signaling_addr.set_ip(tempaddr4);

		bytes_read += 8;
	} 
	else if ( dsig_ip_version == 6 ) {
		struct in6_addr tempaddr6;

		msg.decode(tempaddr6);
		origin_signaling_addr.set_ip(tempaddr6);
		msg.decode(tempaddr6);
		dest_signaling_addr.set_ip(tempaddr6);

		bytes_read += 32;
	}
	else {
		/*
		 * Error: Neither IPv4 nor IPv6. This is fatal, as we have no
		 *        chance to figure out the binary format of this object.
		 */
		ERRCLog("mri_est", "Unknown IP version: " << (int) dsig_ip_version);
		// TODO: Is this the correct error?
		errorlist.put(new GIST_InvalidFlagFieldCombination(
			protocol_v1, this));
		
		return NULL;
	}


	/*
	 * check whether an ingress address is present
	 */
	if ( ingress_ip_version == 0 ) {
	  // this means that no ingress address is given
	  // do nothing, simply skip
	} 
	else if ( ingress_ip_version == 4 ) {
		struct in_addr tempaddr4;

		msg.decode(tempaddr4);
		ingress_node_addr.set_ip(tempaddr4);

		bytes_read += 4;
	} 
	else if ( ingress_ip_version == 6 ) {
		struct in6_addr tempaddr6;

		msg.decode(tempaddr6);
		ingress_node_addr.set_ip(tempaddr6);

		bytes_read += 4;
	}
	else {
		/*
		 * Error: Neither IPv4 nor IPv6. This is fatal, as we have no
		 *        chance to figure out the binary format of this object.
		 */
		ERRCLog("mri_est", "Unknown IP version: " << (int) ingress_ip_version);
		// TODO: Is this the correct error?
		errorlist.put(new GIST_InvalidFlagFieldCombination(
			protocol_v1, this));
		
		return NULL;
	}


	// Check for correct length.
	if ( read_header && (ielen != get_serialized_size(cod)) ) {
		ERRCLog("mri_est", "Incorrect Object Length Error: MRI says: "
			<< ielen << ", read: " << bytes_read
			<< " but should be:" << get_serialized_size(cod));
		errorlist.put(
			new GIST_IncorrectObjectLength(protocol_v1, this));
	}

	return this;
}



// Default version of serialize(): This also writes the object header.
void mri_explicitsigtarget::serialize(NetMsg &msg, coding_t cod,
		uint32 &bytes_written) const {

	serializeEXT(msg, cod, bytes_written, true);
}


// Serializes the object, but leave out the object header.
void mri_explicitsigtarget::serializeNoHead(NetMsg &msg, coding_t cod,
		uint32 &bytes_written) const {

	serializeEXT(msg, cod, bytes_written, false);
}



void mri_explicitsigtarget::serializeEXT(NetMsg &msg, coding_t cod,
		uint32 &bytes_written, bool write_header) const {

        uint32 startpos= msg.get_pos();
	uint32 ielen = get_serialized_size(cod); // includes object TLV header
	if ( ! write_header)
		ielen -= header_length;

	// Check arguments and IE state. Side effect: bytes_written = 0
	check_ser_args(cod, bytes_written);

	if ( write_header )
	{
	  encode_header_ntlpv1(msg, ielen-header_length); 
	}

	uint32 mrmidpos= msg.get_pos();

	// Next 16 bits: MRM-ID (8 bits), N-Flag (1 bit), Reserved (7 bits)
	// note: this will be overwritten by the PC-MRI, so we will encode it later	

	// write PC-MRI
	uint32 pc_bytes= 0;
	mri_pc.serializeNoHead(msg, cod, pc_bytes);

	uint32 savedpos= msg.get_pos();
	// overwrite MRM-ID and N-Flag
	msg.set_pos(mrmidpos);
	msg.encode8(mrm);
	msg.encode8(nat_flag << 7);
	msg.set_pos(savedpos);

	uint8 dest_sig_ip_version= dest_signaling_addr.is_ipv6() ? 6 : 4;
	
	// Next 16 bits: IP Version (4 bits), Reserved (12 bits)
	msg.encode16( dest_sig_ip_version << 12 );


	// unspecified IP addresses are not transmitted
	uint8 ingress_ip_version= ingress_node_addr.is_ip_unspec() ? 0 : (ingress_node_addr.is_ipv6() ? 6 : 4);
	// Next 16 bits: IP Version (4 bits), Reserved (12 bits)
	msg.encode16( ingress_ip_version << 12 );

	
	// Next 32/128 bits: IPv4/IPv6 source address
	if ( origin_signaling_addr.is_ipv6() ) {
		// Note: No new memory is allocated by hostaddress::get_ip().
		const struct in6_addr *ip6addr = origin_signaling_addr.get_ip();

		if ( ip6addr != NULL )
			msg.encode(*ip6addr);
		else {
			// TODO: Could this really happen?
			ERRCLog("mri_explicitsigtarget::serialize()",
				"No valid IPv6 address in origin_signaling_addr");
			msg.encode(in6addr_any);
		}

	}
	else if ( origin_signaling_addr.is_ipv4() ) {
		struct in_addr ip4addr;
		origin_signaling_addr.get_ip(ip4addr);
		msg.encode(ip4addr);
	}
	else
		assert( false );	// programming error


	// Next 32/128 bits: IPv4/IPv6 destination address
	if ( dest_signaling_addr.is_ipv6() ) {
		const struct in6_addr *ip6addr = dest_signaling_addr.get_ip();

		if ( ip6addr != NULL )
			msg.encode(*ip6addr);
		else {
			// TODO: See above. Can't happen.
			ERRCLog("mri_explicitsigtarget::serialize()",
				"No valid IPv6 address in dest_signaling_addr");
		    msg.encode(in6addr_any);
		}
	}
	else if (dest_signaling_addr.is_ipv4()) {
		struct in_addr ip4addr;
		dest_signaling_addr.get_ip(ip4addr);
		msg.encode(ip4addr);
	}
	else
		assert( false );	// programming error

	// Next 32/128 bits: IPv4/IPv6 node ingress if not unknown
	if ( ingress_ip_version != 0 )
	{
	  if ( ingress_node_addr.is_ipv6() ) {
	    const struct in6_addr *ip6addr = ingress_node_addr.get_ip();
	  
	    if ( ip6addr != NULL )
	      msg.encode(*ip6addr);
	    else {
	      // TODO: See above. Can't happen.
	      ERRCLog("mri_explicitsigtarget::serialize()",
		      "No valid IPv6 address in dest_signaling_addr");
	      msg.encode(in6addr_any);
	    }
	  }
	  else if (ingress_node_addr.is_ipv4()) {
	    struct in_addr ip4addr;
	    ingress_node_addr.get_ip(ip4addr);
	    msg.encode(ip4addr);
	  }
	  else
	    assert( false );	// programming error
	}
	bytes_written = msg.get_pos() - startpos;
}


// TODO: Actually check for invalid states, like IPv4 addresses and a set
//       flow label and the like.
bool mri_explicitsigtarget::check() const {
  return origin_signaling_addr.is_ipv6() == dest_signaling_addr.is_ipv6() || 
         origin_signaling_addr.is_ipv4() == dest_signaling_addr.is_ipv4() ;
}


size_t mri_explicitsigtarget::get_serialized_size(coding_t cod) const {

	if ( ! supports_coding(cod) )
		throw IEError(IEError::ERROR_CODING);
    
	// header and stuff is the same as for the normal PC MRI
	size_t size = mri_pc.get_serialized_size(cod);

	// add two 16-bit words containing the IP versions
	size+= 4;

	// Add 2 address fields of variable size (IPv6 or IPv4).
	if (dest_signaling_addr.is_ipv6())
	  size += 2*16; 
	else 
	  size += 2*4;

	if ( !ingress_node_addr.is_ip_unspec() )
	{
	  if (ingress_node_addr.is_ipv6())
	    size += 16; 
	  else 
	    size += 4;
	}

	return size;
}


// We need a compare function for storing instances in hash maps.
bool mri_explicitsigtarget::operator==(const IE& ie) const {
	const mri_explicitsigtarget* sh = dynamic_cast<const mri_explicitsigtarget*>(&ie);
    
	return sh != NULL && (mrm == sh->mrm) && (nat_flag == sh->nat_flag)
   	        && (mri_pc == sh->mri_pc)
		&& (origin_signaling_addr == sh->origin_signaling_addr)
		&& (dest_signaling_addr == sh->dest_signaling_addr)
		&& (ingress_node_addr == sh->ingress_node_addr);
}


ostream &mri_explicitsigtarget::print(ostream& os, uint32 level, const uint32 indent,
		const char* name) const {

	using std::endl;

	static const char *actions[] = { "Mandatory", "Ignore", "Forward" };

	if ( name != NULL && (name[0] != '\0') )
		os << name << " ";

	os << '<' << get_ie_name() << ">: (" << actions[action] << ")" << endl;

	os << "MRM-ID                : " << (int) mrm << endl; 
	os << "Flags:                : " << (has_nat_flag() ? "N" : "-") << endl;

	os << "PC-MRI                :"  << mri_pc << endl;

	os << "Origin Sign. Address  : " << origin_signaling_addr << endl;
	os << "Dest Sign. Address    : " << dest_signaling_addr << endl;
	os << "Ingress Nodess Address: " << ingress_node_addr << endl;

	return os;
}


istream& mri_explicitsigtarget::input(istream& is, bool istty, uint32 level,
		const uint32 indent, const char* name) {

	string s;

	nat_flag = false; // spec doesn't allow 'true' for the PC MRI
    

	if ( istty )
		cout << "Origin Signaling Address: ";
	is >> s;
	origin_signaling_addr = hostaddress();
	origin_signaling_addr.set_ip(s);


	if ( istty )
		cout << "Destination Signaling Address: ";
    	is >> s;
	dest_signaling_addr = hostaddress();
	dest_signaling_addr.set_ip(s);

	if ( istty )
		cout << "Ingress Node Address: ";
    	is >> s;
	ingress_node_addr = hostaddress();
	ingress_node_addr.set_ip(s);

	return is;
}



//@}

// EOF
