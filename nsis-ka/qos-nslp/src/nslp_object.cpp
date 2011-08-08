/// ----------------------------------------*- mode: C++; -*--
/// @file nslp_object.cpp
/// NSLP Object base class
/// ----------------------------------------------------------
/// $Id: nslp_object.cpp 6159 2011-05-18 15:24:52Z bless $
/// $HeadURL: https://svn.tm.kit.edu/nsis/qos-nslp/trunk/src/nslp_object.cpp $
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
/** @addtogroup ienslpobject NSLP Objects
 * @ingroup ie
 * @{
 */

#include "nslp_object.h"
#include "logfile.h"
#include <iostream>
#include <sstream>

#include <iomanip>

using namespace protlib;
using namespace protlib::log;

namespace qos_nslp {

/***** class nslp_object *****/

/** Maximum content size of a NSLP object for this implementation. */
const uint16 nslp_object::max_size = 0xFFF;

/** NSLP object header length */
const uint32 nslp_object::header_length = 4;

const uint32 nslp_object::num_of_types = 4096;  // 2^12

/** Set category to known_nslp_object */
nslp_object::nslp_object(bool known, action_type_t action)
	: NSLP_IE(known?(NSLP_IE::cat_known_nslp_object):(NSLP_IE::cat_unknown_nslp_object)), action(action) {}


/** This class method is used in known_nslp_decode_header_nslpv1
 * and IEManager::deserialize helper functions.
 * No checks are done, nor arguments neither NetMsg size.
 * NetMsg position is not restored.
 * There is no corresponding encode method, because encoding is done
 * in raw_nslp_object independent of encoding.
 * Position is restored in no case.
 * @param m NetMsg to be parsed
 * @param a action for unknown objects
 * @param t returns object type
 * @param dlen returns data len
 * @param ielen returns ielen
 */
void nslp_object::decode_header_nslpv1(NetMsg& m, action_type_t &a, uint16& t, uint16& dlen, uint32& ielen) {
	uint16 tmpdlen = 0;
	uint16 upperword= m.decode16();
	// action
	uint8 action= (upperword & NSLP_OBJ_ACTION_MASK) >> NSLP_OBJ_ACTION_SHIFT;
	switch (action)
	{
	    case NSLP_IE::mandatory: a= NSLP_IE::mandatory;
	                        break;
	    case NSLP_IE::ignore: a= NSLP_IE::ignore;
	                        break;
	    case NSLP_IE::forward: a= NSLP_IE::forward;
	                        break;
	    case NSLP_IE::refresh: a= NSLP_IE::refresh;
	                        break;
	  default:
	    a= NSLP_IE::ignore;
	    break;
	} // end switch
	// type
	t = upperword & NSLP_OBJ_TYPE_MASK;
	// length
	tmpdlen = m.decode16() & NSLP_OBJ_LENGTH_MASK;
	// Is dlen too much for this implementation?
	if (tmpdlen>(max_size/4)) throw IEError(IEError::ERROR_TOO_BIG_FOR_IMPL);
	else {
		dlen = tmpdlen*4;
		ielen = round_up4(dlen)+header_length;
	} // end if tmpdlen
} // end decode_header_nslpv1

/***** class known_nslp_object *****/

/***** inherited from IE *****/

/** Register this IE with type. */
void known_nslp_object::register_ie(protlib::IEManager *iem) const {
	iem->register_ie(category,type,0,this);
} // end register_ie

/***** new members *****/

/** Decode type  and content length. Check if NetMsg is long enough.
 * @param msg network message
 * @param t nslp_object type
 * @param len nslp_object content length
 * @param saved_pos position in NetMsg where this IE starts
 * @param resume position of the next IE
 * @param errorlist IEErrorList to report errors
 * @param bread returns number of processed bytes
 * @param skip try to skip this IE on error
 * @return true on success, else false.
 */
bool known_nslp_object::decode_header_nslpv1(NetMsg& msg, uint16& t, uint16& len, uint32& ielen, uint32& saved_pos, uint32& resume, IEErrorList& errorlist, uint32& bread, bool skip) {
	uint32 bytes_left = 0;
	IEError *iee = NULL;
	bread = 0;
	saved_pos = msg.get_pos();
	bytes_left = msg.get_bytes_left();
	// check if at least 4 bytes to decode header
	if (bytes_left<header_length) {
	  catch_bad_alloc(iee = new IEMsgTooShort(nslp_v1,category,saved_pos));
	  Log(ERROR_LOG,LOG_NORMAL, "NSLP_object", "(1) known_nslp_object::decode_header_nslpv1: IE " << get_ie_name() << ", error " << iee->getstr() << ", coding " << nslp_v1 << " , category " << category << ", position " << saved_pos);
	  errorlist.put(iee);
	  msg.set_pos(saved_pos);
	  return false;
	} // end if bytes_left<header_length
	// decode header
	try {
	  nslp_object::decode_header_nslpv1(msg,action,t,len,ielen);
	} 
	catch(IEError& ignored) {
	  catch_bad_alloc(iee = new IETooBigForImpl(nslp_v1,category,saved_pos));
	  ERRLog("NSLP_object", "(2) known_nslp_object::decode_header_nslpv1: IE " << get_ie_name() << ", error " << iee->getstr() << ", coding " << nslp_v1 << " , category " << category << ", position " << saved_pos);
	  errorlist.put(iee);
	  msg.set_pos(saved_pos);
	  return false;
	} // end try-catch

	// check NetMsg buffer
	if (bytes_left < len) {
	  catch_bad_alloc(iee = new IEMsgTooShort(nslp_v1,category,saved_pos));
	  ERRLog("NSLP_object", "(3) known_nslp_object::decode_header_nslpv1: IE " << get_ie_name() << ", error " << iee->getstr() << ", coding " << nslp_v1 << " , category " << category << ", position " << saved_pos);
	  errorlist.put(iee);
	  msg.set_pos(saved_pos);
	  return false;
	} // end if bytes_left<4

	if (skip) {
	  resume = saved_pos+len;
	  bread = ielen;
	} else resume = saved_pos;

	// check type
	if (t != type) {
	  catch_bad_alloc(iee = new IEWrongType(nslp_v1,category,saved_pos));
	  ERRLog("NSLP_object", "(4) known_nslp_object::decode_header_nslpv1: IE " << get_ie_name() << ", error " << iee->getstr() << ", coding " << nslp_v1 << ", " << t << (skip ?  ", skipping IE." : "."));
	  if (skip) {
	    bread = len;
	  } // end if skip
	  errorlist.put(iee);
	  msg.set_pos(resume);
	  return false;
	} // end if wrong type
	return true;
} // end known_decode_header_nslpv1

/** Encode nslp_v1 nslp_object header. This includes type, and content length. Check if NetMsg buffer is
 * long enough to store the whole object.
 * @note Arguments are NOT checked.
 * @param msg network message.
 * @param len nslp_object content length.
 */
void known_nslp_object::encode_header_nslpv1(NetMsg& msg, uint16 len) const {
	register uint32 objlen = round_up4(len)+header_length;
	// check if len is dividable by 4
	if (len%4) {
	  IEWrongLength err(nslp_v1,category,type,0,msg.get_pos());
	  ERRLog("NSLP_object", "known_nslp_object::encode_header_nslpv1(): IE " << get_ie_name() << ", error " << err.getstr() << ", length " << len);
		throw err;
	} // end if len mod 4
	// Is msg big enough?
	if (msg.get_bytes_left()<objlen) {
	  IEMsgTooShort err(nslp_v1,category,msg.get_pos());
	  ERRLog("NSLP_object", "known_nslp_object::encode_header_nslpv1(): IE " << get_ie_name() << ", error " << err.getstr() << ", required " << objlen << " but left " << msg.get_bytes_left());
		throw err;
	} // end if not enough memory
	// encode header

	msg.encode16( ((action << NSLP_OBJ_ACTION_SHIFT ) & NSLP_OBJ_ACTION_MASK)
		      | (type & NSLP_OBJ_TYPE_MASK) ); // 12 bit only, be conservative in what you send
	msg.encode16( (len/4) & NSLP_OBJ_LENGTH_MASK ); // 12 bit only, be conservative in what you send
	return;
} // end encode_header_nslpv1

/** set category as well as object type.
 * @param t object type.
 */
known_nslp_object::known_nslp_object(type_t t, nslp_object::action_type_t a)
	: nslp_object(true, a), type(t) {
} // end constructor known_nslp_object


/** Report a wrong_type error. */
void known_nslp_object::error_wrong_type(coding_t cod, uint16 t, uint32 msgpos, bool skip, IEErrorList& elist, uint32 resume, NetMsg& msg) {
  IEError* iee = NULL;
  catch_bad_alloc(iee = new IEWrongType(cod,category,msgpos));
  ERRLog("NSLP_object", get_ie_name() << "::deserialize(): error, " << iee->getstr() << ", coding " << cod << ", type " << (int)type << ", wrong_type " << (int)t << ", position " << msgpos << (skip ? ", skipping IE." : "." ));
  elist.put(iee);
  msg.set_pos(resume);
} // end error_wrong_type


/** Report a wrong_length error. IE typeMUST already be set
 * when calling this member function.
 */
void known_nslp_object::error_wrong_length(coding_t cod, uint16 len, uint32 msgpos, bool skip, IEErrorList& elist, uint32 resume, NetMsg& msg) {
  IEError* iee = NULL;
  catch_bad_alloc(iee = new IEWrongLength(cod,category,type,0,msgpos));
  ERRLog("NSLP_object", get_ie_name() << "::deserialize(): error, " << iee->getstr() << ", coding " << cod << ", type " << (int)type << ", wrong_length " << len << ", position " << msgpos << (skip ? ", skipping IE." : "." ));
  elist.put(iee);
  msg.set_pos(resume);
} // end error_wrong_length

known_nslp_object::known_nslp_object(const known_nslp_object& n)
	: nslp_object(true,n.action), type(n.type) {}

/** This asigns the given object to the new object .Type is a constant and cannot change.
 * So take care when using this operator.
 */
known_nslp_object& known_nslp_object::operator=(const known_nslp_object& n) {
	return *this;
} // end operator=

/** This function checks whether the given type is known or not.
 * @param t object type to be checked
 * @return TRUE if the given object type is known.
 */
bool known_nslp_object::is_known(uint16 t) {
	switch (t) {
	case RII:
	case RSN:
	case REFRESH_PERIOD:
	case BOUND_SESSION_ID:
	case PACKET_CLASSIFIER:
	case INFO_SPEC:
	case SESSION_ID_LIST:
	case RSN_LIST:
	case QSPEC:
	case SESSION_AUTH_OBJECT:
	case VLSP_OBJECT:
		return true;
	default:
		return false;
	} // end switch t
} // end is_known(type)



//@}

} // end namespace qos_nslp
