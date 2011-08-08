/// ----------------------------------------*- mode: C++; -*--
/// @file nattraversal.cpp
/// GIST NAT traversal object
/// ----------------------------------------------------------
/// $Id: nattraversal.cpp 5249 2010-04-26 08:40:59Z roehricht $
/// $HeadURL: https://svn.tm.kit.edu/nsis/ntlp/trunk/src/pdu/nattraversal.cpp $
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
#include "nattraversal.h"
#include <iostream>
#include <errno.h>
#include <string>
#include <sstream>
#include "logfile.h"
#include <iomanip>

#include "mri_le.h"
#include "mri_pc.h"

namespace ntlp {

using namespace protlib;

/** @defgroup ienattraversal Nattraversal Objects
 *  @ingroup ientlpobject
 * @{
 */



/***** IE name *****/

const char* const nattraversal::iename = "nattraversal";

const char* nattraversal::get_ie_name() const { return iename; }

const size_t nattraversal::contlen = 16;

/***** inherited from IE *****/


nattraversal* 
nattraversal::new_instance() const {
	nattraversal* sh = NULL;
	catch_bad_alloc(sh = new nattraversal());
	return sh;
} // end new_instance

nattraversal* 
nattraversal::copy() const {
	nattraversal* sh = NULL;
	catch_bad_alloc(sh =  new nattraversal(*this));
	return sh;
} // end copy 


nattraversal* 
nattraversal::deserialize(NetMsg& msg, coding_t cod, IEErrorList& errorlist, uint32& bread, bool skip) {
	const char *const methodname= "nattraversal::deserialize()";
    uint16 len = 0;
    uint32 ielen = 0;
    uint32 saved_pos = 0;
    uint32 resume = 0;
    
/**
 *
 *  0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   | MRI-Length    | Type-Count    |  NAT-Count    |  Reserved     |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   //            Original Message-Routing-Information             //
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   //                 List of translated objects                  //
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   | Length of opaque information  |                               |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+                              //
   //                Information replaced by NAT #1                |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   :                                                               :
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   | Length of opaque information  |                               |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+                              //
   //                Information replaced by NAT #N                |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

   **
   */

    // check arguments
    if (!check_deser_args(cod,errorlist,bread)) 
	return NULL;
    // decode header
    if (!decode_header_ntlpv1(msg,len,ielen,saved_pos,resume,errorlist,bread,skip)) 
	return NULL;

    // get mri length (is given in 32 words), object header is missing
    uint16 mri_length= msg.decode8()*4;
    DLog(methodname, "MRI length: " << mri_length << " bytes");

    // get type count
    uint type_count= msg.decode8();
    DLog(methodname, "type count: " << type_count);

    // get nat count
    uint nat_count= msg.decode8();
    DLog(methodname, "NAT count: " << nat_count);

    // read reserved bits to "nowhere"
    uint tmp= msg.decode8();

    // read mri
   
    // read MRM and instantiate correct MRI subclass

    uint mrm_type= msg.decode16(false);

    DLog(methodname, "Reading MRI of type " << mrm_type);

    if (mrm_type == 0) {

	embedded_mri = new mri_pathcoupled;
        dynamic_cast<mri_pathcoupled*>(embedded_mri)->deserializeNoHead(msg, cod, errorlist, bread, skip);
    }
    else
    if (mrm_type == 1) {

	embedded_mri = new mri_looseend;
	dynamic_cast<mri_looseend*>(embedded_mri)->deserializeNoHead(msg, cod, errorlist, bread, skip);
    }
    else
    {
	    WLog(methodname, "NAT traversal contains unknown MRM type " << mrm_type << ", skipping");
	    // should skip over this object
	    msg.set_pos_r(mri_length);
    }


    DLog(methodname, "Reading translated objects.");
    // read translated objects
    uint i= 0;
    for (i=0; i<type_count; i++) {
	translated_object_types.push_back(msg.decode16());
    }
    // skip padding with 2 NULL bytes if necessary
    if (translated_object_types.size() % 2) 
      	tmp= msg.decode16();

    // read opaque NAT infos
    // iterate over NAT count to read all NAT information objects
    for (i=0; i<nat_count; i++) {
	// read length of total opaque information fields of NAT i:
	len = msg.decode16();

	// allocate new NetMsg buffer and don't copy contents!
	NetMsg* netmsgbuf= new NetMsg(msg.get_buffer()+msg.get_pos(), len, false);
	// move forward within the message
	msg.set_pos_r(len);

	natinfo nat_info_temp;
	nat_info_temp.set(netmsgbuf);

	// this will actually create a copy of the natinfo netmsg buffer
	push_natinfo(nat_info_temp);

	// handle padding, initial length field of NAT info is 16-bit
	uint8 padbytes= (2+len) % 4;
	while(padbytes > 0)
	{
		msg.decode8();
		padbytes--;
	}
    } // end for iterating over NAT info objects


    // There is no further padding.
    bread = ielen;
    return this;
} // end deserialize


// interface function wrapper
void nattraversal::serialize(NetMsg& msg, coding_t cod, uint32& wbytes) const {

    serializeEXT(msg, cod, wbytes, true);

}

// special serialization function with the ability to omit the TLV header
void nattraversal::serializeEXT(NetMsg& msg, coding_t cod, uint32& wbytes, bool header) const {
    const char *const methodname= "nattraversal::serialize()";

    // get_serialized_size includes object header
    uint32 nto_length = get_serialized_size(cod) - (header ? 0 : header_length); 

    DLog(methodname,"should be of length " << nto_length);
    // check arguments and IE state
    check_ser_args(cod,wbytes);
    DLog(methodname, "Bytes left in NetMsg: " << msg.get_bytes_left());
    // calculate length and encode header
    if (header)
    {
	    encode_header_ntlpv1(msg, nto_length - header_length); // length should exclude the header
	    DLog(methodname, "Common Object header encoded (objlen=" << nto_length - header_length << " bytes), 4 bytes, bytes left in NetMsg: " << msg.get_bytes_left());
    }
    DLog(methodname, "Begin of object content serialization");
	
    // encode mri length (get_serialized_size currently returns byte size INCLUDING TLV header, we must subtract 1 word = 4 byte)
    uint32 mri_len= embedded_mri->get_serialized_size(cod);
    // MRI length is given in number of 32-bit words
    msg.encode8((mri_len/4)-1);
	
    DLog(methodname, "MRI length (" << mri_len-4 << " bytes) encoded in 1 byte, bytes left in NetMsg: " << msg.get_bytes_left());

    // encode type count
    msg.encode8(translated_object_types.size());
	
    DLog(methodname, "translated object count (" <<  translated_object_types.size() << ") encoded, 1 byte, bytes left in NetMsg: " << msg.get_bytes_left());
	
    // encode nat_count
    msg.encode8(nat_information.size()); 
	
    DLog(methodname, "NAT count (" << nat_information.size() << ") encoded, 1 byte, bytes left in NetMsg: " << msg.get_bytes_left());

    // reserved
    msg.encode8(0);
    DLog(methodname, "Reserved byte encoded, 1 byte, bytes left in NetMsg: " << msg.get_bytes_left());
    
    uint i=0;
    uint32 written=0;
    // encode MRI (ugly way, I could not get it right using virtual functions in mri superclass..linker would not link it :-/
    if (embedded_mri->get_mrm()==0) dynamic_cast<mri_pathcoupled*>(embedded_mri)->serializeNoHead(msg, cod, written);
    if (embedded_mri->get_mrm()==1) dynamic_cast<mri_looseend*>(embedded_mri)->serializeNoHead(msg, cod, written);
    
    DLog(methodname, "MRI encoded, "<< mri_len-4 << " bytes, bytes left in NetMsg: " << msg.get_bytes_left());
    
    
    // iterate translated objects
    for (i=0; i<translated_object_types.size(); i++) {
	    // encode 16bit type numbers
	    msg.encode16(translated_object_types[i]);
    }
    // pad with 2 NULL bytes if necessary
    if (translated_object_types.size() % 2)
    {
      msg.encode16(0);
    }


    DLog(methodname, "Translated objects encoded, "<< translated_object_types.size()*2 + (translated_object_types.size() % 2)*2 << " bytes, bytes left in NetMsg: " << msg.get_bytes_left());


    // iterate over NAT info,  
    for (i=0; i<nat_information.size(); i++) {
	    // The length fields for each opaque payload are byte counts, not
	    // including the 2 bytes of the length field itself.
	    uint16 info_len = nat_information[i].length();
	    msg.encode16(info_len);
	    
	    if (nat_information[i].get())
	    {
		    msg.copy_from(nat_information[i].get()->get_buffer(), msg.get_pos(), info_len);
		    msg.set_pos_r(info_len);
	    }
	    // must pad two length bytes + NAT info part to a 32-bit boundary
	    uint8 padbytes= (2 + info_len) % 4;
	    while(padbytes>0)
	    {
		    msg.encode8(0);
		    padbytes--;
	    }

    } //end for NAT info #i

    DLog(methodname, "Translated opaque information encoded, bytes left in NetMsg: " << msg.get_bytes_left());

    wbytes = nto_length;
    
    //ostringstream tmpostr;
    //msg.hexdump(tmpostr,0,wbytes);
    //DLog(methodname, "netmsg pos:" << msg.get_pos() << " netmsg:" << tmpostr.str());

    return;
} // end serialize

bool nattraversal::check() const {
	return (true);
} // end check


// returns the object size in bytes, including the common object header
size_t 
nattraversal::get_serialized_size(coding_t cod) const {
    uint size;

    uint i=0;
    if (!supports_coding(cod)) throw IEError(IEError::ERROR_CODING);
    
    // MRI-Count, Type-Count, NAT-Count field and reserved, 1 32-bit word
    size= 32;

    // MRI length in byte length - 4 byte object header
    size+= (embedded_mri->get_serialized_size(cod)*8)-32;

    // translations list, 1 16-word each item and padding of 16 bit if necessary
    size+= (translated_object_types.size() + (translated_object_types.size() % 2)) * 16;

    //uint32 to_size=size; // only for debugging
    // iterate over translated information of different NATs, add their sizes, round each up to word boundary
    for (i=0; i<nat_information.size(); i++) {
	    // length info for all translated info of NAT #i
	    size+= 16; 
	    // translated objects of NAT#i
	    // the length of opaque information and the padding to a 32bit alignment
	    uint16 natinfosize= nat_information[i].length();
	    size+= natinfosize * 8;
	    //DLog("nattraversal::get_serialized_size()","nat info objects size " << natinfosize << " bytes");

	    // add padding if necessary
	    // starting from length field, padding up to the next 32-bit boundary
	    size+= ((2 + natinfosize)%4) *8;
    } // end for NAT info #i
    //DLog("nattraversal::get_serialized_size()","nat info objects use " << (size-to_size) / 8 << " bytes");

    // include header length
    return (size/8) + header_length;
} // end get_serialized_size


bool 
nattraversal::operator==(const IE& ie) const {
    const nattraversal* nto = dynamic_cast<const nattraversal*>(&ie);

	if (nto) 
	{
		//DLog("nattraversal::operator==","dynamic cast successful.");

		if (embedded_mri != NULL && nto != NULL)
		{
			if ( *embedded_mri != *(nto->get_embedded_mri()) )
			{
				return false;
			}
		}
		//DLog("nattraversal::operator==","MRI ok.");

		if (translated_object_types.size() != nto->translated_object_types.size())
			return false;
		//DLog("nattraversal::operator==","no of translated objects ok.");

		for (uint32 i=0; i<translated_object_types.size(); i++) {
			if (translated_object_types[i] != nto->translated_object_types[i])
				return false;
		}
		//DLog("nattraversal::operator==","translated objects ok.");

		if (nat_information.size() != nto->nat_information.size())
			return false;

		//DLog("nattraversal::operator==","No of NAT info ok.");

		for (uint32 i=0; i<nat_information.size(); i++) {
			// translated objects of NAT#i
			if (nat_information[i].length() != nto->nat_information[i].length())
				return false;

			// size of both buffers is equal due to precondition
			if (memcmp(nat_information[i].get()->get_buffer(), nto->nat_information[i].get()->get_buffer(),nat_information[i].length()))
				return false;
		} // end for NAT info #i

		return true;
	}
	else
	return false;
} // end operator==


ostream& 
nattraversal::print(ostream& os, uint32 level, const uint32 indent, const char* name) const 
{
    os<<setw(level*indent)<<"";
    if (name && (*name!=0)) os<<name<< endl;

    uint i=0;
	

    os << "MRI-Length:   " << (int) (embedded_mri ?  embedded_mri->get_serialized_size(protocol_v1)-4 : 0) << " bytes" << endl;
    
    os << "Type-Count:   " << (int) translated_object_types.size() << endl;
    
    os << "NAT Count:    " << (int) nat_information.size() << endl;
    
    os << "Embedded MRI: ";
    // print MRI
    
    if (embedded_mri) 
	    embedded_mri->print(os, level, indent);
    else
	    os << "- (missing)" << endl;
    
    os << "Translated object types: ";
    
    // iterate translated_object_types
    for (i=0; i<translated_object_types.size(); i++) {
	    
      os << hex << (uint16) translated_object_types[i] << ":" << dec;
    }
    os << endl;
    os << "Opaque NAT information objects: ";

    // iterate nat_information,  
    for (i=0; i<nat_information.size(); i++) {	    
	    os << *(nat_information[i].get()) << endl;
    }
    
    return os;
} // end print
 
istream& nattraversal::input(istream& is, bool istty, uint32 level, const uint32 indent, const char* name) {
	//ntlp_object* tmp;
        string s;
        if (istty) cout<<""<<"hostaddress: ";
	if (istty) {
		cout<<"";
		if (name && (*name!=0)) cout<<name<<" ";
		cout<<"<"<<get_ie_name()<<"> 1st 32bit: ";
	} // end if istty
	return is;
} // end input


//@}

} // end namespace protlib
