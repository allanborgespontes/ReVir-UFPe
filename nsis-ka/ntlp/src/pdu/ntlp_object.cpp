/// ----------------------------------------*- mode: C++; -*--
/// @file ntlp_object.cpp
/// GIST Object base class
/// ----------------------------------------------------------
/// $Id: ntlp_object.cpp 6149 2011-05-18 12:54:22Z bless $
/// $HeadURL: https://svn.tm.kit.edu/nsis/ntlp/trunk/src/pdu/ntlp_object.cpp $
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

#include "ntlp_object.h"
#include "logfile.h"

#include <iomanip>


namespace ntlp {
    using namespace protlib;
    using namespace protlib::log;

/** @defgroup ientlpobject NTLP Objects
 * @{
 */


/***** class ntlp_object *****/

/** Maximum content size of a NTLP object for this implementation. */
const uint16 ntlp_object::max_size = 0xFFFC;

/** NTLP object header length */
const uint32 ntlp_object::header_length = 4;

const uint32 ntlp_object::num_of_types = 256;

const uint32 ntlp_object::num_of_subtypes = 64;

const uint32 ntlp_object::num_of_actions = 4;

/** Set category to known_gist_object or raw_gist_object. */
ntlp_object::ntlp_object(bool known)
	: NTLP_IE(known?(cat_known_pdu_object):(cat_raw_protocol_object)) {}

/** decompose NTLP object action hint and subtype 
 * and return them as uint8.
 */
void ntlp_object::decompose_ac_st(uint8 ac_st, uint8& ac, uint8& st) {
	ac = ac_st / num_of_subtypes;
	st = ac_st % num_of_subtypes;
} // end decompose_ac_st

/** compose NTLP object action hint and subtype */
uint8 ntlp_object::compose_ac_st(uint8 ac, uint8 st) {
	return ac * num_of_subtypes + st;
} // end compose_ac_st

/** This class method is used in known_ntlp_decode_header_ntlpv1,
 * raw_ntlp_object::deserialize and IEManager::deserialize helper functions.
 * No checks are done, nor arguments neither NetMsg size.
 * NetMsg position is not restored.
 * There is no corresponding encode method, because encoding is done
 * in raw_ntlp_object independent of encoding.
 * Position is restored in no case.
 * @param m NetMsg to be parsed
 * @param t  returns object type
 * @param ac return action type
 * @param st returns sub-type
 * @param nat_flag returns the NAT flag if this object is an MRI
 * @param dlen returns data len in bytes
 * @param ielen returns ielen (padded data len + header) in bytes
 */
void ntlp_object::decode_header_ntlpv1(NetMsg& m, uint16& t, uint16& st, bool& nat_flag, uint8& ac, uint16& dlen, uint32& ielen) {
    uint16 tmpdlen = 0;
    uint16 field1 = m.decode16();
    t = field1 % 4096;
    ac = field1 >> 14;


    tmpdlen = m.decode16() % 4096; // decode 
    //DLog("decode_header", "type/length: "<<t <<"/"<<tmpdlen*4);

    // We must check the SUBTYPE of MRI objects, distinguish them with MRM field
    if ( t == known_ntlp_object::MRI ) 
    {
	st = m.decode8();
	nat_flag = m.decode8() >> 7;
	m.set_pos_r(-2);
	Log(DEBUG_LOG,LOG_NORMAL, "NTLP_object", "ntlp_object::decodeheader, special class MRI encountered, set subtype: " << st);


    } else st=0;


    // Is dlen too much for this implementation?
    if (tmpdlen>(max_size/4)) throw IEError(IEError::ERROR_TOO_BIG_FOR_IMPL);
    else {
	dlen = tmpdlen*4;
	ielen = round_up4(dlen)+header_length;
    } // end if tmpdlen
} // end decode_header_ntlpv1


uint16 ntlp_object::getTypeFromTLVHeader(const uint8* msgbuf) { 
  return ((((uint16)(msgbuf[0]&0xF))<<8)|((uint16)(msgbuf[1]))); 
} 

	 
// object length in byte (header+content) 
uint32 ntlp_object::getLengthFromTLVHeader(const uint8* msgbuf) { 
	return 4*((((uint32)(msgbuf[2]&0xF))<<8)|((uint32)(msgbuf[3]))) + header_length; 
}


/***** class known_ntlp_object *****/

/***** inherited from IE *****/

/** Register this IE with type and subtype. */
void known_ntlp_object::register_ie(IEManager *iem) const {
	iem->register_ie(category,type,subtype,this);
} // end register_ie

/***** new members *****/
/** Decode type, subtype, action and content length. Check if NetMsg is long
 * enough.
 * @param msg network message
 * @param len gist_object content length (bytes)
 * @param ielen gist_object length including padding and header.
 * @param saved_pos position in NetMsg where this IE starts
 * @param resume position of the next IE
 * @param errorlist IEErrorList to report errors
 * @param bread returns number of processed bytes
 * @param skip try to skip this IE on error
 * @return true on success, else false.
 */
bool known_ntlp_object::decode_header_ntlpv1(NetMsg& msg, uint16& len, uint32& ielen, uint32& saved_pos, uint32& resume, IEErrorList& errorlist, uint32& bread, bool skip) {
	uint32 bytes_left = 0;
	uint16 t = 0;
	uint16 st = 255;
	bool nat_flag = false;
	uint8 ath = 0;
	IEError *iee = NULL;
	bread = 0;
	saved_pos = msg.get_pos();
	bytes_left = msg.get_bytes_left();
	// check if at least 4 bytes to decode header
	if (bytes_left<header_length) {
		catch_bad_alloc(iee = new IEMsgTooShort(protocol_v1,category,saved_pos));
		ERRLog("NTLP_object", "known_ntlp_object::decode_header_ntlpv1: IE " << get_ie_name() << ", error " << iee->getstr() << ", coding " << protocol_v1 << " , category " << category << ", position " << saved_pos << ", bytes left=" << bytes_left << " required:" << header_length);
		errorlist.put(iee);
		msg.set_pos(saved_pos);
		return false;
	} // end if bytes_left<header_length
	// decode header
	try {
	  ntlp_object::decode_header_ntlpv1(msg,t,st,nat_flag,ath,len,ielen);
	} catch(IEError& ignored) {
		catch_bad_alloc(iee = new IETooBigForImpl(protocol_v1,category,saved_pos));
		ERRLog("NTLP_object", "known_ntlp_object::decode_header_ntlpv1: IE " << get_ie_name() << ", error " << iee->getstr() << ", coding " << protocol_v1 << " , category " << category << ", position " << saved_pos);
		errorlist.put(iee);
		msg.set_pos(saved_pos);
		return false;
	} // end try-catch
	// check NetMsg buffer
	if (bytes_left<ielen) {
		catch_bad_alloc(iee = new IEMsgTooShort(protocol_v1,category,saved_pos));
		ERRLog("NTLP_object", "known_ntlp_object::decode_header_ntlpv1: IE " << get_ie_name() << ", error " << iee->getstr() << ", coding " << protocol_v1 << " , category " << category << ", position " << saved_pos << ", required: " << ielen << " bytes, available: " << bytes_left << " bytes");
		errorlist.put(iee);
		msg.set_pos(saved_pos);
		return false;
	} // end if bytes_left<4
	if (skip) {
		resume = saved_pos+ielen; 
		bread = ielen;
	} else resume = saved_pos;
	// check type
	if (t!=type) {
		catch_bad_alloc(iee = new IEWrongType(protocol_v1,category,saved_pos));
		ERRLog("NTLP_object", "known_ntlp_object::decode_header_ntlpv1: IE " << get_ie_name() << ", error " << iee->getstr() << ", coding " << protocol_v1 << ", " << t << (skip ?  ", skipping IE." : "."));
		if (skip) { 
		  bread = ielen;
		} // end if skip
		errorlist.put(iee);
		msg.set_pos(resume);
		return false;
	} // end if wrong type
	// set action
	switch (ath) {
	    case 0: 
		action = Mandatory;
		//DLog("ntlp_object", "Object is Mandatory");
		break;
	    case 1:
		action = Ignore;
		DLog("ntlp_object", "Object can be ignored");
		break;
	    case 2: 
		action = Forward;
		DLog("ntlp_object", "Object is to be forwarded, save it into raw_ntlp_object");
		break;
	    default: action = Ignore; break;
	} // end switch ath
	return true;
} // end known_decode_header_ntlpv1

/** Encode ntlp_v1 ntlp_object header. This includes type, subtype, action
 * and content length. Check if NetMsg buffer is long enough to store
 * the whole object.
 * @note Arguments are NOT checked.
 * @param msg network message
 * @param len ntlp_object content length (in bytes, excluding this header, 0 means no value present)
 */
void known_ntlp_object::encode_header_ntlpv1(NetMsg& msg, uint16 len) const {
  register uint32 objlen = round_up4(len); // including padding

  // check if len is dividable by 4
  if (len%4) 
  {
    IEWrongLength err(protocol_v1,category,type,255,msg.get_pos());
    ERRLog("NTLP_object", "known_ntlp_object::encode_header_ntlpv1(): IE " << get_ie_name() 
	<< ", error " << err.getstr() << ", length " << len << ", object is not aligned to 32-bit word boundary");
    throw err;
  } // end if len mod 4

  // Is msg big enough?
  if ( msg.get_bytes_left() < objlen )
  { 
    IEMsgTooShort err(protocol_v1,category,msg.get_pos());
    ERRLog("NTLP_object", "known_ntlp_object::encode_header_ntlpv1(): IE " << get_ie_name() 
	<< ", error " << err.getstr() << ", required: " << objlen << " bytes, available: " << msg.get_bytes_left() << " bytes");
    throw err;
  } // end if not enough memory

  // encode common object header
  msg.encode16(type | (action << 14));
  // length: length of value given in no of 32-bit words
  msg.encode16((len)/4);
  return; 
} // end encode_header_ntlpv1


/** set catgory as well as object type and action.
 * @param t object type
 * @param a IE action type hint
 */
known_ntlp_object::known_ntlp_object(type_t t, action_t a) 
	: ntlp_object(true), type(t) {
	action = a;
	subtype= all_subtypes;
} // end constructor known_ntlp_object

known_ntlp_object::known_ntlp_object(type_t t, subtype_t st, action_t a)
	: ntlp_object(true), type(t) {
	action = a;
	subtype = st;
}

/** Set GIST object action.
 * @return previous action.
 */
known_ntlp_object::action_t known_ntlp_object::set_action(action_t a) {
	register action_t oa = action;
	action = a;
	return oa;
} // end set_action



/** Report a wrong_length error. IE type and subtype MUST already be set
 * when calling this member function.
 */
void known_ntlp_object::error_wrong_length(coding_t cod, uint16 len, uint32 msgpos, bool skip, IEErrorList& elist, uint32 resume, NetMsg& msg) {
	IEError* iee = NULL;
	catch_bad_alloc(iee = new IEWrongLength(cod,category,type,255,msgpos));
	ERRLog("NTLP_object", get_ie_name() << "::deserialize(): error, " << iee->getstr() << ", coding " << cod << ", type " << (int)type << ", wrong_length " << len << ", position " << msgpos << (skip ? ", skipping IE." : "." ));
	elist.put(iee);
	msg.set_pos(resume);
} // end error_wrong_length

known_ntlp_object::known_ntlp_object(const known_ntlp_object& n) 
	: ntlp_object(true), type(n.type), action(n.action) {}

/** This asigns subtype and action, type is a constant and cannot change.
 * So take care when using this operator.
 */
known_ntlp_object& known_ntlp_object::operator=(const known_ntlp_object& n) {
	action = n.action;
	return *this;
} // end operator=



/***** class raw_ntlp_object *****/

/***** IE name *****/

const char* const raw_ntlp_object::iename = "unknown_ntlp_object";


/***** inherited from IE *****/

/** Get new raw_gist_object instance. */
raw_ntlp_object* raw_ntlp_object::new_instance() const {
	raw_ntlp_object* rdo = NULL;
	catch_bad_alloc(rdo = new raw_ntlp_object);
	return rdo;
} // end new_instance

/** Get a copy of this raw_gist_object instance. */
raw_ntlp_object* raw_ntlp_object::copy() const {
	raw_ntlp_object* rdo = NULL;
	catch_bad_alloc(rdo = new raw_ntlp_object(*this));
	return rdo;
} // end new_instance

/** Deserialize a raw_gist_object from a NetMsg. */
raw_ntlp_object* raw_ntlp_object::deserialize(NetMsg& msg, coding_t cod, IEErrorList& errorlist, uint32& bread, bool skip) {
	uint16 subtype =255;
	bool nat_flag = false;
        uint32 ielen = 0;
	uint32 saved_pos = 0;
	uint32 bytes_left = 0;
	IEError *iee = NULL;
	uchar *buf = NULL;
	// check arguments
	if (!check_deser_args(cod,errorlist,bread)) return NULL;
	// check if at least 4 bytes
	saved_pos = msg.get_pos();
	bytes_left = msg.get_bytes_left();
	if (bytes_left<header_length) {
		catch_bad_alloc(iee = new IEMsgTooShort(cod,category,saved_pos));
		ERRLog("NTLP_object", "raw_ntlp_object::decode_header_ntlpv1: IE " << get_ie_name() << ", error " << iee->getstr() << ", coding " << cod << ", category " << category << ", position " << saved_pos);
		errorlist.put(iee);
		return NULL;
	} // end if bytes_left<4
	// decode type, subtype and action
	try {
		ntlp_object::decode_header_ntlpv1(msg,type,subtype,nat_flag,action,datalen,ielen);
	} catch(IEError& ignored) {
		catch_bad_alloc(iee = new IETooBigForImpl(cod,category,saved_pos));
		ERRLog("NTLP_object", "raw_ntlp_object::deserialize: error " << iee->getstr() << ", coding " << cod << ", category " << category << ", position " << saved_pos);
		errorlist.put(iee);
		msg.set_pos(saved_pos);
		bread = 0;
		return NULL;
	} // end try-catch
	// restore position to copy the whole IE
	msg.set_pos(saved_pos);
	if (bytes_left<ielen) {
		catch_bad_alloc(iee = new IEMsgTooShort(cod,category,saved_pos));
		ERRLog("NTLP_object", "raw_ntlp_object::decode_header_ntlpv1: IE " << get_ie_name() << ", error " << iee->getstr() << ", coding " << cod << ", category " << category << ", position " << saved_pos);
		errorlist.put(iee);
		return NULL;
	} // end if bytes_left<4
	// create buffer and copy data
	buf = new(nothrow) uchar[ielen];
	if (buf) {
		msg.decode(buf,ielen);
		buffer = buf;
		buflen = ielen;
		bread = ielen;
		return this;
	} else {
	  ERRCLog("NTLP_object", "raw_ntlp_object::deserialize: Unable to allocate content buffer");
	  throw IEError(IEError::ERROR_NO_MEM);
	  return NULL;
	} // end if !buf
} // end deserialize


  /** simply copy a byte sequence
   *  This may be required for broken objects that have wrong length
   *  and resulted in parsing errors
   */
raw_ntlp_object::raw_ntlp_object(const uchar *buf, uint32 buflen)
  : ntlp_object(false) // NTLP object unkown, that's why the constructor is called
{
  buffer= new uchar[buflen];
  memcpy(buffer,buf,buflen);
}


/** Serialize a raw_ntlp_object from a NetMsg. */
void raw_ntlp_object::serialize(NetMsg& msg, coding_t cod, uint32& wbytes) const {
	check_ser_args(cod,wbytes);
	// check coding
	// Coding must be the same for encoding AND decoding.
	if (coding!=cod) {
		IEError err(IEError::ERROR_CODING);
		ERRLog("NTLP_object", "raw_ntlp_object::serialize(), error " << err.getstr() << ", coding " << cod);
		throw err;
	} // end if coding
	// check for buffer space
	if (msg.get_bytes_left()<buflen) {
		IEMsgTooShort err(cod,category,msg.get_pos());
		ERRLog("NTLP_object", "raw_ntlp_object::serialize(), error " << err.getstr() << " Required additional " << buflen);
		throw err;
	} // end if not enough memory
	// serialize object
	msg.encode(buffer,buflen);
	wbytes = buflen;
	return;
} // end serialize


/** Get size of this object in serialized form in bytes, including the common object header
 * @param c coding scheme for which the size is calculated.
 * @note the returned value includes the header_length and returns the number of bytes
 */
size_t raw_ntlp_object::get_serialized_size(IE::coding_t c) const {
	// test coding
	if (c!=coding) throw IEError(IEError::ERROR_CODING);
	// calculate size
	if (buffer) {
		return buflen;
	}
	return 0;
} // end get_serialized_size

/** Test equality of two raw_gist_objects. */
bool raw_ntlp_object::operator==(const IE& o) const {
	const raw_ntlp_object* ro = dynamic_cast<const raw_ntlp_object*>(&o);
	if (ro) {
		if ((coding==ro->coding)) {
			if (buffer) {
				// compare content length
				if (buflen==ro->buflen) {
					// compare buffers
					return (memcmp(buffer,ro->buffer,buflen)==0);
				} else {
					// lengths differ
					return false;
				} // end if lengths equal
			} else {
				// buffer of *this is NULL.
				// objects are equal if ro.buffer is NULL.
				return (ro->buffer==NULL);
			} // end if buffer
		} else return false;
	} else return false;
} // end operator==


ostream& raw_ntlp_object::print(ostream& os, uint32 level, const uint32 indent, const char* name) const {
	os<<setw(level*indent)<<"";
	if (name && (*name!=0)) os<<name<<" ";
	os<<"<"<<get_ie_name()<<">:"<<endl;
	level++;
	os<<setw(level*indent)<<"";
	os<<"type: "<<(int)get_type()<<"; ";
	os<<"action: "<<(int)get_action()<<endl;
	os<<setw(level*indent)<<"";
	os<<"content length: "<<get_content_length()<<" bytes";
	return os;
} // end print

/***** new in raw_ntlp_object *****/

/** Init and sets buffer to NULL. */
raw_ntlp_object::raw_ntlp_object() : ntlp_object(false), coding(protocol_v1) {
	buffer = NULL;
	buflen = 0;
	type = 0;
	action = 0;
	datalen = 0;
} // end constructor raw_gist_object

/** copy constructor for raw_gist_object. */
raw_ntlp_object::raw_ntlp_object(const raw_ntlp_object& o)
	: ntlp_object(false), coding(o.coding), type(o.type),
  action(o.action), datalen(o.datalen) {
	if (o.buffer) {
		// copy buffer
		buffer = new(nothrow) uchar[o.buflen];
		if (buffer) {
			buflen = o.buflen;
			memcpy(buffer,o.buffer,buflen);
		} else {
			buffer = NULL;
			buflen = 0;
			type = 0;
			action = 0;
			datalen = 0;
			throw_nomem_error();
		} // end if buffer
	} else {
		buffer = NULL;
		buflen = 0;
	} // end if o.buffer
} // end copy constructor raw_ntlp_object

/** Assignment for raw_ntlp_object. */
raw_ntlp_object& raw_ntlp_object::operator=(const raw_ntlp_object& rdo) {
	// delete own buffer
	if (buffer) delete[] buffer;
	// copy
	coding = rdo.coding;
	type = rdo.type;
	action = rdo.action;
	datalen = rdo.datalen;
	buflen = rdo.buflen;
	if (rdo.buffer) {
		// copy buffer
		buffer = new(nothrow) uchar[buflen];
		if (buffer) {
			memcpy(buffer,rdo.buffer,buflen);
		} else {
			buffer = NULL;
			buflen = 0;
			type = 0;
			action = 0;
			datalen = 0;
			throw_nomem_error();
		} // end if buffer
	} else buffer = NULL;
	return *this;
} // end copy constructor raw_ntlp_object



bool known_ntlp_object::is_known(uint8 t) {
	switch (t) {
	    case MRI:
	    case SessionID:
	    case NLI:
	    case StackProposal:
	    case StackConfiguration:
	    case NatTraversal:
	    case Nslpdata:
	    case ErrorObject:
	    case HelloID:
			return true;
	  default: return false;
	} // end switch t
} // end is_known(type)

//@}

} // end namespace protlib
