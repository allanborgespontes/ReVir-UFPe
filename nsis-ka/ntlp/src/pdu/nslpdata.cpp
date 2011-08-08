/// ----------------------------------------*- mode: C++; -*--
/// @file nslpdata.cpp
/// NSLP Data
/// ----------------------------------------------------------
/// $Id: nslpdata.cpp 5921 2011-02-23 15:58:24Z bless $
/// $HeadURL: https://svn.tm.kit.edu/nsis/ntlp/trunk/src/pdu/nslpdata.cpp $
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
#include "nslpdata.h"
#include <iostream>
#include <errno.h>
#include <string>
#include <sstream>
#include "logfile.h"
#include <iomanip>

namespace ntlp {

using namespace protlib;

/** @defgroup ienslpdata NSLP Data Objects
 *  @ingroup ientlpobject
 * @{
 */

/***** class singlehandle *****/

/***** IE name *****/

const char* const nslpdata::iename = "nslpdata";

const char* nslpdata::get_ie_name() const { return iename; }

const size_t nslpdata::contlen = 16;

/***** inherited from IE *****/

nslpdata* nslpdata::new_instance() const {
	nslpdata* sh = NULL;
	catch_bad_alloc(sh = new nslpdata());
	return sh;
} // end new_instance

nslpdata* nslpdata::copy() const {
	nslpdata* sh = NULL;
	catch_bad_alloc(sh =  new nslpdata(*this));
	return sh;
} // end copy 



nslpdata*
nslpdata::deserialize(NetMsg& msg, coding_t cod, IEErrorList& errorlist, uint32& bread, bool skip) {

    return deserializeEXT(msg,cod,errorlist,bread,skip, true);

}

nslpdata*
nslpdata::deserializeNoHead(NetMsg& msg, coding_t cod, IEErrorList& errorlist, uint32& bread, bool skip) {

    return deserializeEXT(msg,cod,errorlist,bread,skip, false);

}

nslpdata* 
nslpdata::deserializeEXT(NetMsg& msg, coding_t cod, IEErrorList& errorlist, uint32& bread, bool skip, bool header) {
	
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

    // initialize our buffer (len is only object length)
    buf_length=len;
    buf = new(nothrow) uchar[buf_length];

    // copy data into our buffer, padding is of no concern
    msg.decode(buf, len);

    // There is no padding.
    bread = ielen;

    //check for correct length
    if (ielen != get_serialized_size(cod)) {

      ERRCLog("NSLPdata", "Incorrect Object Length Error: PDU specified " << len << " serialize says it should be " << get_serialized_size(cod));
	errorlist.put(new GIST_IncorrectObjectLength(protocol_v1, this));
    }
    return this;
} // end deserialize


// interface function wrapper
void nslpdata::serialize(NetMsg& msg, coding_t cod, uint32& wbytes) const {

    serializeEXT(msg, cod, wbytes, true);

}

// interface function wrapper
void nslpdata::serializeNoHead(NetMsg& msg, coding_t cod, uint32& wbytes) const {

    serializeEXT(msg, cod, wbytes, false);

}

// special serialization function with the ability to omit the TLV header
void nslpdata::serializeEXT(NetMsg& msg, coding_t cod, uint32& wbytes, bool header) const {
    uint32 ielen = get_serialized_size(cod); //+header_length;
    //Log(ERROR_LOG,LOG_NORMAL, "nslpdata::serialize()","should be of length" << ielen);
        // check arguments and IE state
	check_ser_args(cod,wbytes);
	//DLog("nslpdata::serialize()", "Bytes left in NetMsg: " << msg.get_bytes_left());
        // calculate length and encode header
	encode_header_ntlpv1(msg,get_serialized_size(cod)-4);
	//Log(ERROR_LOG, LOG_NORMAL, "nslpdata::serialize()", "Header encoded, 4byte, bytes left in NetMsg: " << msg.get_bytes_left());
	//Log(ERROR_LOG, LOG_NORMAL, "nslpdata::serialize()", "Begin of actual serilization");

        

	//Log(ERROR_LOG, LOG_NORMAL, "nslpdata::serialize()", "Copying paylod buffer into network message buffer");
	
	//Log(ERROR_LOG, LOG_NORMAL, "nslpdata::serialize()", "Position: " << msg.get_pos() << " Data length: " << buf_length << "Available in msg: " << msg.get_bytes_left());


	uint32 position = msg.get_pos();
	// copy data into our buffer, padding is of no concern
	msg.copy_from(buf, position, buf_length);
	
	msg.set_pos_r(round_up4(buf_length));

	wbytes = ielen;
	
	//ostringstream tmpostr;
	//msg.hexdump(tmpostr,0,wbytes);
	//Log(DEBUG_LOG,LOG_NORMAL, "nslpdata_serialize", "netmsg pos:" << msg.get_pos() << " netmsg:" << tmpostr.str().c_str());
    return;
} // end serialize

bool nslpdata::check() const {
	return (true);
} // end check

size_t nslpdata::get_serialized_size(coding_t cod) const {

    if (!supports_coding(cod)) throw IEError(IEError::ERROR_CODING);
  
    return round_up4(buf_length)+header_length;

} // end get_serialized_size

bool nslpdata::operator==(const IE& ie) const {
    // const nslpdata* sh = dynamic_cast<const nslpdata*>(&ie);
	//if (sh) return ((subtype==sh->subtype) && (handle==sh->handle));
	return false;
} // end operator==


ostream& 
nslpdata::print(ostream& os, uint32 level, const uint32 indent, const char* name) const 
{
    os<<setw(level*indent)<<"";
	if (name && (*name!=0)) os<<name<<" ";
	os << "Carrying " << buf_length << " bytes of opaque data payload"<<endl;

	return os;
} // end print

istream& nslpdata::input(istream& is, bool istty, uint32 level, const uint32 indent, const char* name) {
    //ntlp_object* tmp;
        string s;
        //if (istty) cout<<""<<"hostaddress: ";
        //(tmp = new hostaddress)->input(is,istty,level,indent,"address");
        //hostaddress* ha_p= dynamic_cast<hostaddress*>(tmp);
   
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


//@}

} // end namespace protlib
