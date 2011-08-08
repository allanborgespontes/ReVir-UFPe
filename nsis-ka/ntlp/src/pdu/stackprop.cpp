/// ----------------------------------------*- mode: C++; -*--
/// @file stackprop.cpp
/// GIST Stack Proposal
/// ----------------------------------------------------------
/// $Id: stackprop.cpp 5249 2010-04-26 08:40:59Z roehricht $
/// $HeadURL: https://svn.tm.kit.edu/nsis/ntlp/trunk/src/pdu/stackprop.cpp $
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
#include "stackprop.h"
#include <iostream>
#include <errno.h>
#include <string>
#include <sstream>
#include "logfile.h"
#include <iomanip>
#include <stdexcept>

namespace ntlp {

using namespace protlib;

/** @defgroup iestackprop Stackprop Objects
 *  @ingroup ientlpobject
 * @{
 */

/***** class singlehandle *****/

/***** IE name *****/

const char* const stackprop::iename = "stackprop";

const char* stackprop::get_ie_name() const { return iename; }

const size_t stackprop::contlen = 16;

/***** inherited from IE *****/

stackprop* stackprop::new_instance() const {
	stackprop* sh = NULL;
	catch_bad_alloc(sh = new stackprop());
	return sh;
} // end new_instance

stackprop* stackprop::copy() const {
	stackprop* sh = NULL;
	catch_bad_alloc(sh =  new stackprop(*this));
	return sh;
} // end copy 

stackprop* 
stackprop::deserialize(NetMsg& msg, coding_t cod, IEErrorList& errorlist, uint32& bread, bool skip) 
{	
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

    //get profile count
    unsigned int profile_count=msg.decode8();

    // if profile count == 0 throw GIST_ValueNotSupported
    if (!profile_count) {
	errorlist.put(new GIST_ValueNotSupported(protocol_v1, this));
    }

    //read reserved bits to "nowhere"
    int tmp=msg.decode8();
    tmp=msg.decode16();

    unsigned int i=0;
    unsigned int count=0;
    unsigned int j=0;
    unsigned int f=0;

    stackprofile temp;

    uint32 checkempty = 0;

    // iterate over profile_count to read whole profiles
    DLog("stackprop::deserialize", "Profile count: " << (int) profile_count);
    for (i=0; i<profile_count; i++) 
    {
      temp.clear();
      
      //read count of fields:
      count=msg.decode8();
      
      //iterate through MA fields
      for (j=0; j<count; j++) 
      {
	uint8 tmp = msg.decode8();
	temp.addprotocol(tmp);
	checkempty+=tmp;
      } // end for

      //read padding
      for (f=0; f<(round_up4(count)-count-1);f++) 
	tmp=msg.decode8();

      add_profile(temp);
    } // end for

    // There is no padding.
    bread = ielen;
    
    // Throw GIST_EmptyList!
    if (!checkempty) 
    {
      errorlist.put(new GIST_EmptyList(protocol_v1, this));
    }


    //check for correct length
    if (ielen != get_serialized_size(cod)) 
    {
      ERRCLog("Stackprop", "Incorrect Object Length Error. Size encoded in PDU: " << ielen 
	  << " Size calculated from object: " << get_serialized_size(cod));
      //errorlist.put(new GIST_IncorrectObjectLength(protocol_v1, this));
      WLog("Stackprop", "Ignoring the incorrect object length error for now");
    }

    return this;
} // end deserialize


// interface function wrapper
void stackprop::serialize(NetMsg& msg, coding_t cod, uint32& wbytes) const 
{
    serializeEXT(msg, cod, wbytes, true);
}

// special serialization function with the ability to omit the TLV header
void stackprop::serializeEXT(NetMsg& msg, coding_t cod, uint32& wbytes, bool header) const 
{
  uint32 ielen = get_serialized_size(cod); //+header_length;
  //Log(ERROR_LOG,LOG_NORMAL, "stackprop::serialize()","should be of length" << ielen);
  
  // check arguments and IE state
  check_ser_args(cod,wbytes);
	
  //DLog("stackprop::serialize()", "Bytes left in NetMsg: " << msg.get_bytes_left());
  
  // calculate length and encode header
  encode_header_ntlpv1(msg,get_serialized_size(cod)-header_length);
  //Log(ERROR_LOG, LOG_NORMAL, "stackprop::serialize()", "Header encoded, 4byte, bytes left in NetMsg: " << msg.get_bytes_left());
  //Log(ERROR_LOG, LOG_NORMAL, "stackprop::serialize()", "Begin of actual serilization");

  //encode Prof_Count
  msg.encode8(profiles.size());
  //DLog("stackprop::serialize", "Encoded profile count of " << profiles.size());
  msg.encode8(0);
  msg.encode16(0);
  
  unsigned int idx=0;
  unsigned int j=0;
  unsigned int i=0;
  
  //iterate profiles,  
  for (i=0; i<profiles.size(); i++) 
  {
    idx=0;
    //encode MA field count
    msg.encode8(profiles[i].prof_vec.size());
    idx=1;    
    
    for (j=0; j<profiles[i].prof_vec.size(); j++) 
    {  
      msg.encode8(profiles[i].prof_vec[j]);
      idx++;
    } // end for

    if (idx % 4) {
      if (idx%4==3) msg.encode8(0);
      if (idx%4==2) msg.encode16(0);
      if (idx%4==1) { msg.encode16(0); msg.encode8(0); }
    }
  } // end for

  wbytes = ielen;
	
  //ostringstream tmpostr;
  //msg.hexdump(tmpostr,0,wbytes);
  //Log(DEBUG_LOG,LOG_NORMAL, "stackprop_serialize", "netmsg pos:" << msg.get_pos() << " netmsg:" << tmpostr.str().c_str());
  return;
} // end serialize

bool stackprop::check() const {
	return (true);
} // end check

size_t stackprop::get_serialized_size(coding_t cod) const {
    uint size;

    uint i=0;
    if (!supports_coding(cod)) throw IEError(IEError::ERROR_CODING);
    
    //Prof-Count field and reserved
    size=4;

    //iterate profiles, add their sizes, round up to word boundary 
    for (i=0; i<profiles.size(); i++) {
    size+=round_up4(profiles[i].prof_vec.size() + 1);
    }

    //assume only 4 byte profile field length
    //size+= profiles.size()*4;

  
    return (size)+header_length;

} // end get_serialized_size

bool stackprop::operator==(const IE& ie) const {
    const stackprop* sh = dynamic_cast<const stackprop*>(&ie);
    bool result= false;
    if (sh) 
    {
      if (profiles.size()==sh->profiles.size())
      { // size must be matching
	result= true;
	for (unsigned int i=0; i<profiles.size(); i++) {
	  if (profiles[i].prof_vec != sh->profiles[i].prof_vec)
	    {
	      result= false;
	      break;
	    }
	} // end for
      }
    }
    return result;
} // end operator==


ostream& 
stackprop::print(ostream& os, uint32 level, const uint32 indent, const char* name) const 
{
    os<<setw(level*indent)<<"";
	if (name && (*name!=0)) os<<name<<endl;

	uint j=0;
	uint i=0;

	os << "Profiles: " << (int) profiles.size() << endl;

	//iterate profiles,  
	for (i=0; i<profiles.size(); i++) {

	    //encode MA field count
	    os << profiles[i].prof_vec.size() << " MA fields: ";
  

	    for (j=0; j<profiles[i].prof_vec.size(); j++) {

		os << "<" << (int) profiles[i].prof_vec[j] << "> ";
	    }
	os <<endl;
	}


	return os;
} // end print

istream& stackprop::input(istream& is, bool istty, uint32 level, const uint32 indent, const char* name) {
    //ntlp_object* tmp;
        string s;
        if (istty) cout<<""<<"hostaddress: ";
        //(tmp = new hostaddress)->input(is,istty,level,indent,"address");
        //hostaddress* ha_p= dynamic_cast<hostaddress*>(tmp);
   
	if (istty) {
		cout<<"";
		if (name && (*name!=0)) cout<<name<<" ";
		cout<<"<"<<get_ie_name()<<"> 1st 32bit: ";
	} // end if istty
	/*
	is>>a;
	if (istty) {
		cout<<"";
		if (name && (*name!=0)) cout<<name<<" ";
		cout<<"<"<<get_ie_name()<<"> 2nd 32bit: ";
	} // end if istty
	is >>b;
	if (istty) {
		cout<<"";
		if (name && (*name!=0)) cout<<name<<" ";
		cout<<"<"<<get_ie_name()<<"> 3rd 32bit: ";
	} // end if istty
	is>>c;
	if (istty) {
		cout<<"";
		if (name && (*name!=0)) cout<<name<<" ";
		cout<<"<"<<get_ie_name()<<"> 4th 32bit: ";
	} // end if istty
	is>>d;
	*/

	return is;
} // end input


const stackprofile* 
stackprop::get_profile(uint32 index) const
{ 
	try 
	{ 
		return &(profiles.at(index)); 
	} 
	catch(std::out_of_range& orexception) 
	{ 
		return NULL;
	}  
}

//@}

} // end namespace protlib
