/// ----------------------------------------*- mode: C++; -*--
/// @file bound_sessionid.cpp
/// QoS NSLP Bound Session Identification
/// ----------------------------------------------------------
/// $Id: bound_sessionid.cpp 5253 2010-04-26 13:41:09Z roehricht $
/// $HeadURL: https://svn.tm.kit.edu/nsis/qos-nslp/trunk/src/bound_sessionid.cpp $
// ===========================================================
//                      
// Copyright (C) 2005-2010, all rights reserved by
// - Institute of Telematics, Karlsruhe Institute of Technology
//
// More information and contact:
// https://projekte.tm.uka.de/trac/NSIS
//                      
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; version 2 of the License
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
// 02110-1301 USA.
//
// ===========================================================

#include "bound_sessionid.h"
#include "nslp_pdu.h"
#include "logfile.h"

#include <iomanip>

using namespace protlib;
using namespace protlib::log;

namespace qos_nslp {

/** @addtogroup iebound_sessionid Bound Session Identification
 * @ingroup ienslpobject
 * @{
 */

/***** class bound_sessionid *****/

/***** IE name *****/

const char* const bound_sessionid::iename = "bound_sessionid";

const char* bound_sessionid::get_ie_name() const { return iename; }

const uint16 bound_sessionid::contlen = 20;


const uint128 bound_sessionid::bound_sessioniddefault(10000,10000,10000,10000);

/***** inherited from IE *****/

/** Generate new BOUND_SESSIONID instance.
  */
bound_sessionid* bound_sessionid::new_instance() const {
	bound_sessionid* n = NULL;
	catch_bad_alloc(n = new bound_sessionid());
	return n;
} // end new_instance

/** Copy function for BOUND_SESSIONID object. 
  */
bound_sessionid* bound_sessionid::copy() const {
	bound_sessionid* n = NULL;
	catch_bad_alloc(n =  new bound_sessionid(*this));
	return n;
} // end copy

/** Deserialize function for BOUND_SESSIONID object.
  * @param msg Network message: from this message BOUND_SESSIONID object will be deserialized.
  * @param cod it is set to nslp_v1 for this version of QoS NSLP.
  * @param errorlist List to record errors.
  * @param bread reference to integer. Set to the number of bytes read by this call.
  * @param skip skip the IE in case of an error or restore NetMsg position.
  */
bound_sessionid* bound_sessionid::deserialize(NetMsg& msg, coding_t cod, IEErrorList& errorlist, uint32& bread, bool skip) {
	uint16 t = 0;
	uint16 len = 0;
	uint32 ielen = 0;
	uint32 saved_pos = 0;
	uint32 resume = 0;
	uint32 bits;
	uint128 my_bound_sessionid;
	//Log(INFO_LOG,LOG_NORMAL, "DESERIALIZE", __PRETTY_FUNCTION__);

	// check arguments
	if (!check_deser_args(cod,errorlist,bread)) return NULL;
	// decode header
	if (!decode_header_nslpv1(msg,t,len,ielen,saved_pos,resume,errorlist,bread,skip)) return NULL;
	// check type
	if (t==BOUND_SESSION_ID) type = BOUND_SESSION_ID;
	else {
		// wrong type
		error_wrong_type(cod,t,saved_pos,skip,errorlist,resume,msg);
		return NULL;
	} // end if st
	// check length

	if (len!=contlen) {
		// wrong length
		error_wrong_length(cod,len,saved_pos,skip,errorlist,resume,msg);
		return NULL;
	} // end if wrong length
	// decode bound_sessionidref
	bits = msg.decode32();
	uint8 b_code = (uint8) bits;
	this->set_binding_code(b_code);
	my_bound_sessionid = msg.decode128();
	this->set(my_bound_sessionid);
	bread = ielen;
	return this;
} // end deserialize

/** Serialize function for BOUND_SESSIONID object.
  * @param netmsg Network message: BOUND_SESSIONID object will be serialized and added to this message.
  * @param cod it is set to nslp_v1 for this version of QoS NSLP.
  * @param wbytes Written bytes: the length of added bytes.
  */
void bound_sessionid::serialize(NetMsg& msg, coding_t cod, uint32& wbytes) const {
	// check arguments and IE state
	check_ser_args(cod,wbytes);
	encode_header_nslpv1(msg,contlen);
	uint32 first_word = bit_field | binding_code;
	msg.encode32(first_word);
	// encode bound_sessionidref
	msg.encode128(bound_sessionidref);
	wbytes = contlen+header_length;
	return;
} // end serialize

/** Check function for BOUND_SESSIONID object.
  * @returns TRUE, if the object is of type BOUND_SESSIONID.
  */
bool bound_sessionid::check() const {
	return ((type==BOUND_SESSION_ID));
} // end check

/** Function to get the size of the BOUND_SESSIONID object (incl. header length).
  */
size_t bound_sessionid::get_serialized_size(coding_t cod) const {
	if (supports_coding(cod)) return contlen+header_length;
	else throw IEError(IEError::ERROR_CODING);
} // end get_serialized_size

/** Function to compare two objects of type BOUND_SESSIONID.
  */
bool bound_sessionid::operator==(const IE& ie) const {
	const bound_sessionid* t = dynamic_cast<const bound_sessionid*>(&ie);
	if (t) {
		return ((type==t->type)
			&& (bound_sessionidref.w1==t->bound_sessionidref.w1)
			&& (bound_sessionidref.w2==t->bound_sessionidref.w2)
			&& (bound_sessionidref.w3==t->bound_sessionidref.w3)
			&& (bound_sessionidref.w4==t->bound_sessionidref.w4)
		);
	} else return false;
} // end operator==

/** Function to print nicely the object of type BOUND_SESSIONID on the monitor(or file).
  */
ostream& bound_sessionid::print(ostream& os, uint32 level, const uint32 indent, const char* name) const {
	os<<setw(level*indent)<<"";
	Log(INFO_LOG,LOG_NORMAL, "BOUND_SESSIONID", "<"<<get_ie_name()<<">: " << "["<<bound_sessionidref.w1<<", "<<bound_sessionidref.w2<<", "<<bound_sessionidref.w3<<", "<<bound_sessionidref.w4<<"]");
	return os;
} // end print

/** Function for manual input of BOUND_SESSIONID object.
  */
istream& bound_sessionid::input(istream& is, bool istty, uint32 level, const uint32 indent, const char* name) {
	uint128 i;
	char out[100];
	string c;
	int l = 0;
	int j = 0;
	int k = 0;
	char args[3][10];
	if (istty) {
		cout<<setw(level*indent)<<"";
		if (name && (*name!=0)) cout<<name<<" ";
		cout<<"<"<<get_ie_name()<<"> (bound_sessionid: ): ";
	} // end if istty
	is>>out;
	Log(INFO_LOG,LOG_NORMAL, "SESSION_ID", "input(), out = : " << out);
	for (l=0; l < 100; l++) {
	    c= out[l];
	    if (c == ".") {
		k = 0;
		j++;
		continue;
	    }
	    args[j][k] = out[l];
	    k++;
	}
	i.w1 = atoi(args[0]);
	i.w2 = atoi(args[1]);
	i.w3 = atoi(args[2]);
	i.w4 = atoi(args[3]);
	set(i);
	return is;
} // end input

/***** new in bound_sessionid *****/

/** Default constructor for BOUND_SESSIONID object without any params. The value of the object will be set to the default.
  */
bound_sessionid::bound_sessionid()
	: known_nslp_object(BOUND_SESSION_ID, mandatory) {
        set_bit_field(0);
	set_binding_code(0);
	bound_sessionidref.w1 = bound_sessioniddefault.w1;
	bound_sessionidref.w2 = bound_sessioniddefault.w2;
	bound_sessionidref.w3 = bound_sessioniddefault.w3;
	bound_sessionidref.w4 = bound_sessioniddefault.w4;
} // end constructor

/** Constructor for BOUND_SESSIONID object with given parameter.
  * @param r the session id for new BOUND_SESSIONID object.
  */
bound_sessionid::bound_sessionid(uint128 r)
	: known_nslp_object(BOUND_SESSION_ID, mandatory) {
	bound_sessionidref.w1 = r.w1;
	bound_sessionidref.w2 = r.w2;
	bound_sessionidref.w3 = r.w3;
	bound_sessionidref.w4 = r.w4;
} // end constructor

/** Constructor for BOUND_SESSIONID object with given BOUND_SESSIONID object.
  * @param n the BOUND_SESSIONID object to be copied into new BOUND_SESSIONID object.
  */
bound_sessionid::bound_sessionid(const bound_sessionid& n)
	: known_nslp_object(n) {
	bound_sessionidref.w1 = n.bound_sessionidref.w1;
	bound_sessionidref.w2 = n.bound_sessionidref.w2;
	bound_sessionidref.w3 = n.bound_sessionidref.w3;
	bound_sessionidref.w4 = n.bound_sessionidref.w4;	
} // end copy constructor

/** Assign operator for BOUND_SESSIONID object.
  * @param n the BOUND_SESSIONID object to be asigned to the current BOUND_SESSIONID object.
  */
bound_sessionid& bound_sessionid::operator=(const bound_sessionid& n) {
	type = n.type;
	known_nslp_object::operator=(n);
	bound_sessionidref.w1 = n.bound_sessionidref.w1;
	bound_sessionidref.w2 = n.bound_sessionidref.w2;
	bound_sessionidref.w3 = n.bound_sessionidref.w3;
	bound_sessionidref.w4 = n.bound_sessionidref.w4;	
	return *this;
} // end operator=

/** Function to set the bound session id to a default.
  */
void bound_sessionid::set_default() {
  set_bit_field(0);
  set_binding_code(0);
  bound_sessionidref.w1 = bound_sessioniddefault.w1;
  bound_sessionidref.w2 = bound_sessioniddefault.w2;
  bound_sessionidref.w3 = bound_sessioniddefault.w3;
  bound_sessionidref.w4 = bound_sessioniddefault.w4;
} // end set_default

/** Get function for BOUND_SESSIONID object.
  * @param r into this variable the value of BOUND_SESSIONID object will be written.
  */
void bound_sessionid::get(uint128& r) const {
	r = bound_sessionidref;
} // end get

/** Set function for BOUND_SESSIONID object.
  * @param r the value of the BOUND_SESSIONID object will be set to the value of this variable.
  */
void bound_sessionid::set(const uint128& r) {
	bound_sessionidref = r;
} // end set

/** Generate function for BOUND_SESSIONID.
  * This function is used to generate a new random value for BOUND_SESSIONID.
  */
void bound_sessionid::generate_new() {
    bound_sessionidref.w1 = rand();
    bound_sessionidref.w2 = rand();
    bound_sessionidref.w3 = rand();
    bound_sessionidref.w4 = rand();
}

/** Check function for BOUND_SESSIONID object.
  * @returns TRUE if the current object is of type BOUND_SESSIONID.
  */
bool bound_sessionid::is_bound_sid() const {
	return (type==BOUND_SESSION_ID);
} // end is_bound_sessionid

/** Set function for the bit field of the BOUND_SESSIONID object.
  * @param bits the bits of the BOUND_SESSIONID object will be set to the value of this variable.
  */
void bound_sessionid::set_bit_field(uint32 bits)
{
  bit_field = bits;
}

/** Set function for the binding code of the BOUND_SESSIONID object.
  * @param b_code the binding code of the BOUND_SESSIONID object will be set to the value of this variable.
  */
void bound_sessionid::set_binding_code(uint8 b_code)
{
  binding_code = b_code;
}
	
/** Get function for the binding code of the BOUND_SESSIONID object.
  * @return the binding code of the BOUND_SESSIONID object.
  */
uint8 bound_sessionid::get_binding_code()
{
  return binding_code;
}

/** Set TUNNEL_SESSION flag.
  */
void bound_sessionid::set_tunnel_session()
{
  binding_code |= TUNNEL_SESSION;
}

/** Set BIDIRECT_SESSION flag.
  */	
void bound_sessionid::set_bidirectional_session()
{
  binding_code |= BIDIRECT_SESSION;
}

/** Set AGGREGATE_SESSION flag.
  */	
void bound_sessionid::set_aggregate_session()
{
  binding_code |= AGGREGATE_SESSION;
}

/** Set DEPENDENT_SESSION flag.
  */	
void bound_sessionid::set_dependent_session()
{
  binding_code |= DEPENDENT_SESSION;
}

/** Check whether TUNNEL_SESSION flag is set.
  */
bool bound_sessionid::is_tunnel_session()
{
  return (binding_code & TUNNEL_SESSION);
}

/** Check whether BIDIRECT_SESSION flag is set.
  */	
bool bound_sessionid::is_bidirectional_session()
{
  return (binding_code & BIDIRECT_SESSION);
}
	
/** Check whether AGGREGATE_SESSION flag is set.
  */
bool bound_sessionid::is_aggregate_session()
{
  return (binding_code & AGGREGATE_SESSION);
}

/** Check whether DEPENDENT_SESSION flag is set.
  */	
bool bound_sessionid::is_dependent_session()
{
  return (binding_code & DEPENDENT_SESSION);
}

//@}

} // end namespace qos_nslp
