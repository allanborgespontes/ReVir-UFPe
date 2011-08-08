/// ----------------------------------------*- mode: C++; -*--
/// @file mri_le.cpp
/// GIST LE MRI Loose End MRI (Message Routing Information)
/// ----------------------------------------------------------
/// $Id: mri_le.cpp 5249 2010-04-26 08:40:59Z roehricht $
/// $HeadURL: https://svn.tm.kit.edu/nsis/ntlp/trunk/src/pdu/mri_le.cpp $
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
#include <errno.h>
#include <string>
#include <sstream>
#include <iomanip>

#include "logfile.h"
#include "mri_le.h"

namespace ntlp {

using namespace protlib;
 
/** @addtogroup mri_pathcoupled MRI_PATHCOUPLED Objects
    @ingroup mri
 * @{
 */

/***** class mri_pathcoupled *****/

/***** IE name *****/

const char* const mri_looseend::iename = "mri_looseend";

const char* mri_looseend::get_ie_name() const { return iename; }

const size_t mri_looseend::contlen = 26;

/***** inherited from IE *****/

mri_looseend* mri_looseend::new_instance() const {
	mri_looseend* sh = NULL;
	catch_bad_alloc(sh = new mri_looseend());
	return sh;
} // end new_instance

mri_looseend* mri_looseend::copy() const {
	mri_looseend* sh = NULL;
	catch_bad_alloc(sh =  new mri_looseend(*this));
	return sh;
} // end copy 


mri_looseend*
mri_looseend::deserialize(NetMsg& msg, coding_t cod, IEErrorList& errorlist, uint32& bread, bool skip) {

    return deserializeEXT(msg,cod,errorlist,bread,skip, true);

}


mri_looseend*
mri_looseend::deserializeNoHead(NetMsg& msg, coding_t cod, IEErrorList& errorlist, uint32& bread, bool skip) {

    return deserializeEXT(msg,cod,errorlist,bread,skip, false);

}



mri_looseend* 
mri_looseend::deserializeEXT(NetMsg& msg, coding_t cod, IEErrorList& errorlist, uint32& bread, bool skip, bool header) {
	
    uint16 len = 0;
    uint32 ielen = 0;
    uint32 saved_pos = 0;
    uint32 resume = 0;
    
    struct in6_addr tempaddr6;
    struct in_addr tempaddr4;
    uint16 flagfield;

    // check arguments
    if (!check_deser_args(cod,errorlist,bread)) 
	return NULL;
    // decode header
    if (header) {

    if (!decode_header_ntlpv1(msg,len,ielen,saved_pos,resume,errorlist,bread,skip)) 
	return NULL;
    
    }

    // next field: MRM-ID (8 bits), N-Flag (1 bit), Reserved (7 bits)
    mrm = msg.decode8();
    nat_flag = msg.decode8() >> 7;

    Log(DEBUG_LOG, LOG_NORMAL, "mri_looseend::deserialize()", "Decoded MRM, position: " << msg.get_pos());    
    // decode the bit field containing ip_version, flags and reserved bits
    flagfield = msg.decode16();
    Log(DEBUG_LOG, LOG_NORMAL, "mri_looseend::deserialize()", "Decoded Flags, position: " << msg.get_pos());   
    ip_version= flagfield >> 12;
    
    flags = flagfield & Direction; // only the D-flag is specified


    // Now fork if IP_Version is 4 or 6, decode source and destination address - IP addresses
    if (ip_version==4) {
	msg.decode(tempaddr4);
	sourceaddress.set_ip(tempaddr4);
	msg.decode(tempaddr4);
	destaddress.set_ip(tempaddr4);
	Log(DEBUG_LOG, LOG_NORMAL, "mri_looseend::deserialize()", "Decoded IPv4, position: " << msg.get_pos());  	
    } else {
	msg.decode(tempaddr6);
	sourceaddress.set_ip(tempaddr6);
	msg.decode(tempaddr6);
	destaddress.set_ip(tempaddr6);
	Log(DEBUG_LOG, LOG_NORMAL, "mri_looseend::deserialize()", "Decoded IPv6, position: " << msg.get_pos());  
    }


    bread = ielen;

    //check for correct length
    if ((header) && (ielen != get_serialized_size(cod))) {

	ERRCLog("MRI", "Incorrect Object Length Error");
	errorlist.put(new GIST_IncorrectObjectLength(protocol_v1, this));
    }





    return this;
} // end deserialize



// this is the wrapper for the different serialization functions, as we support different encodings for different MRMs
void mri_looseend::serialize(NetMsg& msg, coding_t cod, uint32& wbytes) const {

    serializeLooseEnd(msg,cod,wbytes,true);
}

// this is the wrapper for the different serialization functions, as we support different encodings for different MRMs
// encode without TLV header (for embedding as one piece in other objects)
void mri_looseend::serializeNoHead(NetMsg& msg, coding_t cod, uint32& wbytes) const {

    serializeLooseEnd(msg,cod,wbytes,false);
}



// serialization function for MRM "Loose End Routing" extended by the ability to omit header 
void mri_looseend::serializeLooseEnd(NetMsg& msg, coding_t cod, uint32& wbytes, bool header) const {
    uint32 ielen = get_serialized_size(cod); //+header_length;
	// check arguments and IE state
	check_ser_args(cod,wbytes);

        // calculate length and encode header ONLY if header flag is set. MRI_LOOSEEND can also serialize without header!
	if (header) encode_header_ntlpv1(msg,get_serialized_size(cod)-4);

        // next field: MRM-ID (8 bits), N-Flag (1 bit), Reserved (7 bits)
	msg.encode8(mrm);
	msg.encode8(nat_flag << 7);
	
        //encode IP Version and flags
	msg.encode16( (ip_version << 12) | flags );
   
        // encode source address
	if ( sourceaddress.is_ipv6() ) {
	    const struct in6_addr *ip6addr = sourceaddress.get_ip();
	    assert( ip6addr != NULL );
	    msg.encode(*ip6addr);
	}
	else if ( sourceaddress.is_ipv4() ) {
	    struct in_addr ip4addr;
	    sourceaddress.get_ip(ip4addr);
	    msg.encode(ip4addr);
	}
	else
	    assert( false );
	
	// encode destination address
	if ( destaddress.is_ipv6() ) {
	    const struct in6_addr *ip6addr = destaddress.get_ip();
	    assert( ip6addr != NULL );
	    msg.encode(*ip6addr);
	}
	else if ( destaddress.is_ipv4() ) {
	    struct in_addr ip4addr;
	    destaddress.get_ip(ip4addr);
	    msg.encode(ip4addr);
	}
	else
	    assert( false );
	
	wbytes = ielen;
	
	//ostringstream tmpostr;
    //msg.hexdump(tmpostr,0,wbytes);
    //Log(DEBUG_LOG,LOG_NORMAL, "MRI_LOOSEEND_serialize", "netmsg pos:" << msg.get_pos() << " netmsg:" << tmpostr.str().c_str());
    return;
} // end serialize



bool mri_looseend::check() const {
	return (true);
} // end check

size_t mri_looseend::get_serialized_size(coding_t cod) const {
    int size;

    if (!supports_coding(cod)) throw IEError(IEError::ERROR_CODING);
    
    // first 32bits are header and flags/ip_version;
    size=32;

    // now add 2 address fields in case of IPv6
    if (ip_version==6)size+=256; 

    // in case of IPv4
    if (ip_version==4)size+=64;


    return (size/8)+header_length;

} // end get_serialized_size

bool mri_looseend::operator==(const IE& ie) const {
    const mri_looseend* sh = dynamic_cast<const mri_looseend*>(&ie);

    return (sh != NULL) && (mrm == sh->mrm) && (nat_flag == sh->nat_flag)
		&& (ip_version == sh->ip_version)
		&& (sourceaddress == sh->sourceaddress)
		&& (destaddress == sh->destaddress)
		&& (flags == sh->flags);
} // end operator==


ostream& 
mri_looseend::print(ostream& os, uint32 level, const uint32 indent, const char* name) const 
{
    	if (name && (*name!=0)) os<<name<<" ";
	os << '<' << get_ie_name() << ">: (" << action << ")\n\n";
	os << "MRM-ID:                 " << (int)mrm << endl;
	os << "IP-Version:             " << (int)ip_version << endl;
	os << "Flags:                  " << (has_nat_flag() ? "N" : "") << endl;
	os << "Direction:              " <<
		((flags & Direction) ? "Upstream" : "Downstream") << endl;
	os << "Source Address:         " << sourceaddress.get_ip_str() << endl;
	os << "Destination Address:    " << destaddress.get_ip_str() << endl;
	
	return os;
} // end print

istream& mri_looseend::input(istream& is, bool istty, uint32 level, const uint32 indent, const char* name) {

    nat_flag = false;	// setting the N-Flag is not allowed for the LE-MRM.
    //ntlp_object* tmp;
        string s;

	if (istty) {
		cout<<"";
		if (name && (*name!=0)) cout<<name<<" ";
		cout<<"<"<<get_ie_name()<<">: ";
	} // end if istty
	//is>>handle;
	return is;
} // end input


//@}


//virtual function which allows each MRI subtype to return a routable next hop address
appladdress* 
mri_looseend::determine_dest_address() const
{

    //port and protocol are updated from the caller!
    if (flags & Direction) {
	return new appladdress(sourceaddress, "udp", 0);
    } else {
	return new appladdress(destaddress, "udp", 0);
    }

}


// virtual function inverting direction flag, Responder nodes stores MRI with inverted direction
void mri_looseend::invertDirection() {

    if (flags & Direction) {
	flags = flags^Direction;
    } else {
	flags = flags | Direction;
    }

}




} // end namespace protlib
