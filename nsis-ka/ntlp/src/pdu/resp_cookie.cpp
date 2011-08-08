/// ----------------------------------------*- mode: C++; -*--
/// @file resp_cookie.cpp
/// GIST responder cookie
/// ----------------------------------------------------------
/// $Id: resp_cookie.cpp 5249 2010-04-26 08:40:59Z roehricht $
/// $HeadURL: https://svn.tm.kit.edu/nsis/ntlp/trunk/src/pdu/resp_cookie.cpp $
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
#include "resp_cookie.h"
#include <iostream>
#include <errno.h>
#include <string>
#include <sstream>
#include "logfile.h"
#include <iomanip>

namespace ntlp {

using namespace protlib;

/** @defgroup ierespcookie Respcookie Objects
 *  @ingroup ientlpobject
 * @{
 */

/***** class singlehandle *****/

/***** IE name *****/

const char* const respcookie::iename = "respcookie";

const char* respcookie::get_ie_name() const { return iename; }

const size_t respcookie::contlen = 16;

/***** inherited from IE *****/

respcookie* respcookie::new_instance() const {
	respcookie* sh = NULL;
	catch_bad_alloc(sh = new respcookie());
	return sh;
} // end new_instance

respcookie* respcookie::copy() const {
	respcookie* sh = NULL;
	catch_bad_alloc(sh =  new respcookie(*this));
	return sh;
} // end copy 



respcookie*
respcookie::deserialize(NetMsg& msg, coding_t cod, IEErrorList& errorlist, uint32& bread, bool skip) {

    return deserializeEXT(msg,cod,errorlist,bread,skip, true);

}

respcookie*
respcookie::deserializeNoHead(NetMsg& msg, coding_t cod, IEErrorList& errorlist, uint32& bread, bool skip) {

    return deserializeEXT(msg,cod,errorlist,bread,skip, false);

}

respcookie* 
respcookie::deserializeEXT(NetMsg& msg, coding_t cod, IEErrorList& errorlist, uint32& bread, bool skip, bool header) {
	
    uint16 len = 0;
    uint32 ielen = 0;
    uint32 saved_pos = 0;
    uint32 resume = 0;
    

    // check arguments
    if (!check_deser_args(cod,errorlist,bread)) 
	return NULL;
    // decode header
    if (!decode_header_ntlpv1(msg,len,ielen,saved_pos,resume,errorlist,bread,skip)) 
	return NULL;
    
 
    // initialize our buffer (ielen in words)
    buf = new(nothrow) uchar[len];

    // get msg position pointer
    uint32 position = msg.get_pos();
    
    // copy data into our buffer, padding is of no concern
    msg.copy_to(buf, position, len);
    
    buf_length=len;

    msg.set_pos_r(round_up4(buf_length));

    // There is no padding.
    bread = ielen;

    //check for correct length
    if (ielen != get_serialized_size(cod)) {

	ERRCLog("RespCookie", "Incorrect Object Length Error");
	errorlist.put(new GIST_IncorrectObjectLength(protocol_v1, this));
    }



    return this;
} // end deserialize


// interface function wrapper
void respcookie::serialize(NetMsg& msg, coding_t cod, uint32& wbytes) const {

    serializeEXT(msg, cod, wbytes, true);

}

// interface function wrapper
void respcookie::serializeNoHead(NetMsg& msg, coding_t cod, uint32& wbytes) const {

    serializeEXT(msg, cod, wbytes, false);

}

// special serialization function with the ability to omit the TLV header
void respcookie::serializeEXT(NetMsg& msg, coding_t cod, uint32& wbytes, bool header) const {
    uint32 ielen = get_serialized_size(cod); //+header_length;
    
        // check arguments and IE state
	check_ser_args(cod,wbytes);
	//DLog("respcookie::serialize()", "Bytes left in NetMsg: " << msg.get_bytes_left());
        // calculate length and encode header
	encode_header_ntlpv1(msg,get_serialized_size(cod)-4);
	

	uint32 position = msg.get_pos();
	// copy data into our buffer, padding is of no concern (wa, come on configure sizes FITTING!)
	msg.copy_from(buf, position, buf_length);


	msg.set_pos_r(round_up4(buf_length));

	wbytes = ielen;
	
	//ostringstream tmpostr;
	//msg.hexdump(tmpostr,0,wbytes);
	//Log(DEBUG_LOG,LOG_NORMAL, "respcookie_serialize", "netmsg pos:" << msg.get_pos() << " netmsg:" << tmpostr.str().c_str());
    return;
} // end serialize

bool respcookie::check() const {
	return (true);
} // end check

size_t respcookie::get_serialized_size(coding_t cod) const {

    if (!supports_coding(cod)) throw IEError(IEError::ERROR_CODING);
  
    return round_up4(buf_length)+header_length;

} // end get_serialized_size

bool respcookie::operator==(const IE& ie) const {
    const respcookie* sh = dynamic_cast<const respcookie*>(&ie);
    bool identical = true;
    if (sh) {
	if (buf_length!=sh->buf_length) {
	    identical = false;
	} else {

	    for (unsigned int i=0; i<buf_length; i++) {
		if (buf[i]!=sh->buf[i]) identical=false;
		
	    }
	}
    }

    return identical;
} // end operator==


ostream& 
respcookie::print(ostream& os, uint32 level, const uint32 indent, const char* name) const 
{
    os<<setw(level*indent)<<"";
	if (name && (*name!=0)) os<<name<<" ";
	os << ": <0x";
	
	for(unsigned int i = 0; i < buf_length; i++) os << setfill('0') << setw(2) << hex << (int) buf[i];
	os << '>' << setfill(' ') << dec << endl;

	return os;
} // end print

istream& 
respcookie::input(istream& is, bool istty, uint32 level, const uint32 indent, const char* name) {
    //ntlp_object* tmp;
        string s;

	if (istty) {
		cout<<"";
		if (name && (*name!=0)) cout<<name<<" ";
		cout<<"<"<<get_ie_name()<<"> Enter some fake data: ";
	} // end if istty
	
	uchar *buffer = new uchar[128];
	
	is>>buffer;

	buf=buffer;
	buf_length=128;


	return is;
} // end input


string
respcookie::to_str() const
{
  ostringstream tmpostr;
  
  for(unsigned int i = 0; i < buf_length; i++) 
    tmpostr << setfill('0') << setw(2) << hex << (int) buf[i];

  return tmpostr.str();
}


nli* 
respcookie::get_nli() const
{
  uint16 transparent_data_len= get_transparent_data_len();
  uchar* transparent_data= get_transparent_data();

  // first extract the NLI from transparent data if present  
  if (transparent_data)
  {
	  // perform a sanity check first (transparent data len must be smaller than resp cookie length)
	  // if someone messed with the length value in transparent data
	  if (transparent_data_len < buf_length)
	  {
		  uint32 bytes_read= 0;
		  IEErrorList errorlist;
		  // data is not copied into NetMsg buffer
		  NetMsg tmpbuf(transparent_data, transparent_data_len, false);
		  nli* nli_obj= new nli;
		  nli_obj->deserialize(tmpbuf, IE::protocol_v1, errorlist, bytes_read, false);
		  if (errorlist.is_empty() == false)
		  { // parse error
			  ERRLog("respcookie::get_nli()","parse error while getting NLI from responder cookie - this is an implementation error or some attacker messed with it");
			  return NULL;
		  }
		  else // parsing successful
			  return nli_obj;
	  }
  }

  return NULL;
}


nattraversal* 
respcookie::get_nto(uint16 byteoffset) const
{
  uint16 transparent_data_len= get_transparent_data_len();
  uchar* transparent_data= get_transparent_data();

  if (transparent_data && byteoffset < transparent_data_len)
  {
	  uint32 bytes_read= 0;
	  IEErrorList errorlist;
	  nattraversal* nat_trav_obj= new nattraversal;
	  NetMsg tmpbuf(transparent_data+byteoffset, transparent_data_len-byteoffset, false);
	  nat_trav_obj->deserialize(tmpbuf, IE::protocol_v1, errorlist, bytes_read, false);
	  if (errorlist.is_empty() == false)
	  {
		  ERRLog("respcookie::get_nto()","parse error while getting NTO from responder cookie - this is an implementation error");
		  return NULL;
	  }
	  else
	  {
		  return nat_trav_obj;
	  }
  }
  return NULL;
}

//@}

} // end namespace protlib
