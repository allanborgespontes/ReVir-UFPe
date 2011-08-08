/// ----------------------------------------*- mode: C++; -*--
/// @file ntlp_pdu.cpp
/// NTLP/GIST PDU
/// ----------------------------------------------------------
/// $Id: ntlp_pdu.cpp 6195 2011-05-25 13:31:46Z bless $
/// $HeadURL: https://svn.tm.kit.edu/nsis/ntlp/trunk/src/pdu/ntlp_pdu.cpp $
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
/** @ingroup ientlppdu
 * NTLP PDUs
 */

#include "ntlp_pdu.h"
#include "logfile.h"
#include "ntlp_global_constants.h"

using namespace ntlp;
using namespace protlib;
using namespace protlib::log;


/** @addtogroup ientlppdu NTLP PDUs
 * @{
 */

/***** class ntlp_pdu *****/

// function pointer for session authorization HMAC verification are set to NULL by default
bool (* ntlp_pdu::check_hmac)(NetMsg& msg)= NULL;
bool (* ntlp_pdu::serialize_hmac)(NetMsg& msg) = NULL;


/***** inherited from IE *****/

ntlp_pdu* ntlp_pdu::deserialize(NetMsg& msg, coding_t coding, IEErrorList& errorlist, uint32& bread, bool skip) {
	IEError *iee = NULL;
	uint32 ielen = 0;
	uint8 v = 0;   
	uint32 start_pos = msg.get_pos(); // this is where this method started to parse
	uint32 saved_pos = start_pos;     // this is for error reporting and may get updated with parsing progress
	uint8 t = 0;
	uint16 len = 0;
	uint8 h = 0;
	uint16 s = 0;
	uint8 f =0;
	uint32 resume = start_pos;

	//bool ignore = false;
	//bool forward = false;

	IE* tmpie = NULL;
	ntlp_object* tmpobj = NULL;
	known_ntlp_object* tmpknown = NULL;
	//DLog("NTLP_pdu", "ntlp_pdu::deserialize for IE " << color[blue] << get_ie_name() << color[off] << " called");
        // check arguments
	if (!check_deser_args(coding,errorlist,bread)) return NULL;

	// check if at least common_header_length bytes are available to decode header
	if (msg.get_bytes_left() < ntlp_pdu::common_header_length) {
		catch_bad_alloc(iee = new IEMsgTooShort(coding,category,saved_pos));
		ERRLog("NTLP_pdu", "ntlp_pdu::deserialize: IE " << get_ie_name() << " error, " << iee->getstr() << ", coding " << coding << ", category " << category << ", position " << saved_pos); 
		errorlist.put(iee);
		msg.set_pos(saved_pos);
		return NULL;
	} // end if bytes_left<common_header_length

	// check hmac of AuthSessionObject
	if (check_hmac != NULL)
	{
		DLog("NTLP_pdu", "checking for integrity protection by a session authorization object");
		if ( !check_hmac(msg) ) {
			WLog("NTLP_pdu", "ntlp_pdu::deserialize: check_hmac failed");
			errorlist.put(new IEHmacError);
			msg.set_pos(saved_pos);
			return NULL;
		}
	}

	// decode common PDU header
	try {
	  decode_common_header_ntlpv1(msg,v, h, len, s, t, f, c_flag, errorlist);
	  flags= f;
	  nslpid = s;
	  ntlp_hops = h;
	  //DLog("NTLP_pdu", "ntlp_pdu " << color[blue] << get_ie_name() << color[off] << " has length " << len+common_header_length << " bytes");

	} catch(IEError& ignored) {
		catch_bad_alloc(iee = new IETooBigForImpl(coding,category,saved_pos));
		ERRLog("NTLP_pdu" ,"ntlp_pdu::deserialize: IE " << get_ie_name() << ", error " << iee->getstr() << ", coding " << coding << ", category " << category << ", position " << saved_pos);
		errorlist.put(iee);
		msg.set_pos(saved_pos);
		return this;
	} // end try-catch

	// check NetMsg buffer
	ielen = len+common_header_length;
	if (msg.get_bytes_left()<len) {
		catch_bad_alloc(iee = new IEMsgTooShort(protocol_v1,category,saved_pos));
		ERRLog("NTLP_pdu", "known_ntlp_object::decode_header_ntlpv1: IE " << get_ie_name() << ", error " << iee->getstr() << ", coding " << protocol_v1 << ", category " << category << ", position " << saved_pos 
		       << " left in buffer: " << msg.get_bytes_left() << "<" << len << " required for GIST msg");
		errorlist.put(iee);
		msg.set_pos(saved_pos);
		return this;
	} // end if bytes_left<len
	if (skip) {
	  //DLog( "NTLP_pdu", "skip is set for objects");
	    resume = saved_pos+ielen; 
	    bread = ielen;
	} else {
		resume = saved_pos;
		bread = 0;
	} // end if skip

	// check NTLP protocol version
	if (v!=coding) {
		catch_bad_alloc(iee = new IEWrongVersion(coding,category,saved_pos));
		ERRLog("NTLP_pdu", "ntlp_pdu::deserialize: IE " << get_ie_name() << ", error " << iee->getstr() << ", coding " << coding << ", wrong_version " << (int)v << (skip ? ", skipping IE." : "."));
		if (skip) {
		  bread = ielen;
		} // end if skip
		errorlist.put(iee);
		msg.set_pos(resume);
		return this;
	} // end if wrong version

	// check type and subtype
	switch (accept_type_and_subtype(t)) {
		case type_error:
			catch_bad_alloc(iee = new IEWrongType(coding,category,saved_pos));
			ERRLog("NTLP_pdu", "ntlp_pdu::deserialize: IE " << get_ie_name() << ", error " << iee->getstr() << ", coding " << coding << ", wrong_type " << (int)t << (skip ? ", skipping IE." : "."));
			if (skip) {
				bread = ielen;
			} // end if skip
			errorlist.put(iee);
			msg.set_pos(resume);
			return this;
	       
		default: // do nothing
			break;
	} // end switch accept_type_and_subtype
	// decode NTLP objects
	arr.clear(true);
	uint32 objbread = 0;
	uint32 tmpbread = 1;
	uint32 objpos = msg.get_pos();
	
	uint32 designated_pos = 0;

	// len is PDU size-common_header_length
	// msg.get_pos()<common_header_length+len is checked so that we do not parse behind PDU contents as specified in the PDU itself
	while ((objbread<len) && (tmpbread!=0) && (msg.get_pos() < ielen+start_pos)) {
	  //DLog( "NTLP_pdu", "ntlp_pdu: calling next deserialize @pos" << msg.get_pos() << " ielen:" << ielen);

	  // deserialize next NTLP object
	  tmpie = NTLP_IEManager::instance()->deserialize(msg,cat_known_pdu_object,coding,errorlist,tmpbread,false); //skip == false
	  objbread += tmpbread;

	  if (tmpie)  
	    tmpobj = dynamic_cast<ntlp_object*>(tmpie);
	  else { 
	    tmpobj = NULL;
	    ERRCLog("NTLP_pdu", "could not cast to ntlp_object");
	  }

	  if (tmpobj) {
	    tmpknown = dynamic_cast<known_ntlp_object*>(tmpobj);
	    if (tmpknown) {
			    
	      if (!accept_object(tmpknown, designated_pos, errorlist)) {
		catch_bad_alloc(iee = new GIST_InvalidObject(coding, this->get_type(), tmpknown->get_type()));
		ERRLog("NTLP_pdu", "ntlp_pdu::deserialize: error " << iee->getstr() << ", coding " << coding << (skip ? ", appending IE anyway." : "."));
		errorlist.put(iee);
		if (skip) {
		  //DLog( "NTLP_pdu", "ntlp_pdu: appended known object to array");
		  arr.append(tmpknown);
		}
		else {
		  delete tmpknown;
		  msg.set_pos(resume);
		  DLog( "NTLP_pdu", "ntlp_pdu:  discarded known object, was not accepted!");
		  return this;
		} // end if skip
	      } else {
		//cout << "Before insertion, designated position is: " << (int) designated_pos;
		arr.insert(designated_pos, tmpobj);
		DLog("NTLP_pdu", "ntlp_pdu: inserted known object (" 
		     << color[blue] << tmpobj->get_ie_name() << color[off] << ") of " << tmpbread << " bytes to designated position " << designated_pos);
		
	      }// end if not accept_object
	    } 
	    else {
	      // unknown objects are always appended.
	      // Action is already checked in IEManager::deserialize and
	      // errors are handled there.
	      // append object only if no integrity set
	      arr.append(tmpobj);
	      DLog("NTLP_pdu", "ntlp_pdu: appended unknown object to array");
	    } // end if tmpknown
	  } 
	  else {		
	    // no NTLP object deserialized
	    // delete deserialized IE
	    if (tmpie) delete tmpie;
	    // do logging and recover
	    ERRLog("NTLP_pdu", "ntlp_pdu::deserialize(): no NTLP object deserialized from IE");
	    if (!skip) {
	      msg.set_pos(resume);
	      return this;
	    } // end if not skip
	  } // end if tmpobj
	  // setting position of next object
	  objpos += tmpbread;		
	} // end while objbread<len
	
	// we have read too far according to the given PDU length
	if (msg.get_pos()>ielen+start_pos)
	  ERRCLog("NTLP_pdu", "read behind PDU content! current position in netmsg buffer:" << msg.get_pos() 
		  << " started @" << start_pos
		  << " PDU length:" << common_header_length+len
		  << " objbread:" << objbread);
	
	// finish PDU deserialization
	if (!deser_end(errorlist)) {
	  msg.set_pos(resume);
	  return this;
	} // end if not deser_end
	
	uint32 serlength = this->get_serialized_size(coding);

	if (ielen != serlength ) {
	    ERRCLog("NTLP pdu", "PDU Length invalid, computed length: " << serlength << " encoded length: " << ielen);

	    errorlist.put(new GIST_IncorrectMsgLength(protocol_v1, ntlp_version, ntlp_hops, this->get_length(), nslpid, type, flags));

	} else {
	    DLog( "NTLP pdu", color[green] << "Check for correct Message Length passed. Simple message integrity is OK." << color[off]);
	}


        // padding is included in objects
	bread = ielen;
	return this;
} // end deserialize


void 
ntlp_pdu::serialize(NetMsg& msg, coding_t coding, uint32& wbytes) const {
  uint32 objwbytes = 0;
  uint32 tmpwbytes = 0;
  uint32 len = 0;
  uint32 pdulen = 0;
  uint32 startpos = msg.get_pos();

  //uint32 versionbyte = 1<<4;

  objectarray::const_iterator_t it;
  // check arguments and IE state
  check_ser_args(coding,wbytes);
  // calculate length and encode header
  pdulen = get_serialized_size(coding);
  DLog("NTLP_pdu", "ntlp_pdu::serialize: " << get_ie_name()  << " expected pdulen:" << pdulen);
  len = pdulen-common_header_length;
  // check if len is dividable by 4
  if (len%4) {
    IEWrongLength err(coding,category,type,0,msg.get_pos());
    ERRLog("NTLP_pdu", "ntlp_pdu::serialize: IE " << get_ie_name() << ", error " << err.getstr() << ", length " << len);
    throw err;
  } // end if len mod 4

  // Is msg big enough?
  if (msg.get_bytes_left()<pdulen) {
    IEMsgTooShort err(coding,category,msg.get_pos());
    ERRLog("NTLP_pdu", "ntlp_pdu::serialize: IE " << get_ie_name() << ", error " << err.getstr() << ", required " << pdulen << ", length " << msg.get_bytes_left());
    throw err;
  } // end if not enough memory

  // encode common header
  msg.encode8(ntlp_version);
  msg.encode8(ntlp_hops);
  // get_serialized_size returns
  msg.encode16(len/4);
  msg.encode16(nslpid);
  // clear C-Flag by default
  uint8 cflag_and_type= type & 0x7F;
  if (type == known_ntlp_pdu::Query)
         cflag_and_type= 0x80 | type;
  msg.encode8(cflag_and_type);
  msg.encode8(flags);
  
  // erase old object types in tlp list
  msg.clean_up_tlp_list(); 

  // encode PDU objects
  for (it=arr.begin();it!=arr.end();it++) 
  {
    if (*it) 
    {
      try 
      {
	      DLog("NTLP_pdu:: serialize", "processing " << (*it)->get_ie_name());
	
	      uint32 begin_pos = msg.get_pos();
	      (*it)->serialize(msg,coding,tmpwbytes);

	      DLog("NTLP_pdu:: serialize", "wrote " << (*it)->get_ie_name() << "@pos: " << begin_pos << " length:" << tmpwbytes);

	      uint16 obj_type = (*it)->get_type();
	      msg.get_TLP_list()->insert(tlplist_ntlp_object_type,obj_type,begin_pos);

	      if( obj_type == known_ntlp_object::Nslpdata ) {
		      msg.fill_tlp_list(begin_pos + 8,
					begin_pos + tmpwbytes,
					ntlp::ntlp_object::getLengthFromTLVHeader,
					ntlp_pdu::get_tlplist_nslp_object_type,
					ntlp::ntlp_object::getTypeFromTLVHeader,
					false); // want add nslp object types
	      }
      }
      catch(IEError& e)
      {
	Log(ERROR_LOG,LOG_CRIT,"NTLP_pdu",
	    "while processing " << (*it)->get_ie_name() << ":" << e.getstr());
	throw; // re-throw
      }
      catch(...)
      {
	throw; // re-throw
      }
     
      objwbytes += tmpwbytes;
      if (objwbytes>pdulen) break;
    } // end if it
  } // end for

  if (serialize_hmac != NULL)
  {
	  uint32 currentpos= msg.get_pos();
	  // serialize hmac, no need to fill the TLP list
	  msg.set_pos(startpos); // serialize hmac assumes to start at the ntlp header
	  serialize_hmac(msg);
	  msg.set_pos(currentpos);
  }

  // check length of encoded objects
  if ((objwbytes!=len) || (it!=arr.end())) {
    IEWrongLength err(coding,category,type,0,msg.get_pos());
    ERRLog("NTLP_pdu", "sspvo::serialize(): IE " << get_ie_name() << ", error " << err.getstr() << ", length " << len << ", wrote " << objwbytes);
    throw err;
  } // end if wrong length
  // padding is included in service object
  wbytes = pdulen;
  return;
} // end serialize
  

bool ntlp_pdu::check() const
{
  //int i=1;
  for (objectarray::const_iterator_t it = arr.begin();
       it!=arr.end();
       it++)
  {
      if ( *it && !((*it)->check()) ) 
      {
	  ERRLog("NTLP_pdu","check(): failed for " << ((*it) ? (*it)->get_ie_name() : "-NULL-"));
	  return false;
      }
  } // end for
  
  // check basic header
  return true; 
} // end check

size_t ntlp_pdu::get_serialized_size(coding_t coding) const {
	size_t res = common_header_length;
	if (supports_coding(coding)) {
	    //objectarray::const_iterator_t it = arr.begin();
	    
	    /*
	      while (it!=arr.end()) {
	      ntlp_object* obj=NULL;
	      obj=dynamic_cast<ntlp_object*>(*it);
	      if (obj) 
	      {
	      ERRLog("NTLP_pdu","serializing object: " << "ie:" << (obj)->get_ie_name() << '[' << (obj)->get_serialized_size(coding) << ']' << endl);
	      res += obj->get_serialized_size(coding);
	      //#define _DEBUG_IE
	      
	      }
	      it++;
	      } // end while
	      
	    */

	    for (unsigned int i =0; i < arr.size(); i++) {
		ntlp_object* obj=NULL;
		obj=dynamic_cast<ntlp_object*>(arr.get(i));
		if (obj) 
		{
		    //string name = obj->get_ie_name();
		    uint32 size = obj->get_serialized_size(coding);
		    //DLog("NTLP_pdu","getting size of object: " << "ie:" << name << '[' << size << ']');
		    res += size;
		    
		}		   
	    }
	    
	    DLog("NTLP_pdu","calculated PDU size of: "<<res);
	    return res;
	} else throw IEError(IEError::ERROR_CODING);
} // end get_serialized_size
    
/** Only clear those pointers that are accessible from outside. The other
 * NTLP Objects must be deleted when the PDU is deleted and therefore must
 * not be cleared.
 */
void ntlp_pdu::clear_pointers() {


} // end clear_pointers


/** Only compare objects that are accessible from outside. 
 * For a NTLP PDU this is the basic header and the integrity object. 
 */
bool ntlp_pdu::operator==(const IE& n) const {
	const ntlp_pdu* p = dynamic_cast<const ntlp_pdu*>(&n);
	if (p) return (1
	    //compare_objects(get_orig_ref(),p->get_orig_ref())
	    //&& compare_objects(get_integrity(),p->get_integrity())
		);
	else return false;

} // end operator==



/***** new members *****/

bool ntlp_pdu::compare_objects(const ntlp_object* o1, const ntlp_object* o2) {
	if (!(o1 || o2)) return true;
	else if (o1 && o2) return (*o1)==(*o2);
	else return false;
} // end compare_objects

/** Maximum content size of a NTLP PDU for this implementation. */
const uint32 ntlp_pdu::max_size = 0xFFFFFFF4;

/** Length of common header in bytes. */
const uint32 ntlp_pdu::common_header_length = 8;

/** The type field is 8 bit long, so there are 256 types. */
const uint32 ntlp_pdu::num_of_types = 256;

const uint16 ntlp_pdu::tlplist_nslp_object_type = 0;
const uint16 ntlp_pdu::tlplist_ntlp_object_type = 1;

/** Decode common header without doing any sanity checks. */
void ntlp_pdu::decode_common_header_ntlpv1(NetMsg& msg, uint8& ver, uint8& hops, uint16& clen_bytes, uint16& nslpid, uint8& type, uint8& flags, bool& c_flag, IEErrorList& errorlist) {
	uint32 tmpclen = 0;
	ver = msg.decode8();
        hops = msg.decode8();
	tmpclen = msg.decode16();
	nslpid = msg.decode16();
	type = msg.decode8();
	// get C-Flag from upper bit
	c_flag= type & 0x80;
	// finally clear the C-Flag from type
	type = type & 0x7F;
	flags = msg.decode8();
	
	// do Error throwing here!

	if (ver != ntlp_version_default) {
	    errorlist.put(new GIST_UnknownVersion(protocol_v1, ver, hops, tmpclen, nslpid, type, flags));
	}
	
	// check whether type is in undefined/non-valid range
	if (type >= known_ntlp_pdu::MaxTypeNonValid) {
	    errorlist.put(new GIST_UnknownType(protocol_v1, ver, hops, tmpclen, nslpid, type, flags));
	}

	if ((type == known_ntlp_pdu::Confirm) && (flags & R_Flag)) {
	    errorlist.put(new GIST_InvalidRFlag(protocol_v1, ver, hops, tmpclen, nslpid, type, flags));
	}

	if ((type == known_ntlp_pdu::Error) && (flags & R_Flag)) {
	    errorlist.put(new GIST_InvalidRFlag(protocol_v1, ver, hops, tmpclen, nslpid, type, flags));
	}
        // 5: Invalid C-flag:  The C flag was set on something other than a
        // Query message or Q-mode Data message, or was clear on a Query
        // message.
	if (  ((!c_flag) && ((type == known_ntlp_pdu::Query)))
	    ||
	      ( c_flag && (type != known_ntlp_pdu::Query) && (type != known_ntlp_pdu::Data))
	   )
	{
	    errorlist.put(new GIST_InvalidCFlag(protocol_v1, ver, hops, tmpclen, nslpid, type, flags));
	}



        //if (tmpclen>(max_size/4)) throw IEError(IEError::ERROR_TOO_BIG_FOR_IMPL);
	else clen_bytes = tmpclen*4;
} // end decode_common_header_ntlpv1

/** Set category to known_ntlp_pdu or unknown_ntlp_pdu.
 * @param known unknown or known PDU?
 * @param t PDU type
 * @param numobj number of objects excluding the basic header and message
 *        integrity.
 */
ntlp_pdu::ntlp_pdu(bool known, uint8 t, uint32 numobj) 
    : NTLP_IE(known?(NTLP_IE::cat_known_pdu):(NTLP_IE::cat_unknown_pdu)),
      arr(numobj+3),
      ntlp_version(ntlp_version_default),
      ntlp_hops(1),
      length(common_header_length),
      nslpid(0),
      type(t),
      flags(0),
      c_flag(false),
      nattrav_ptr(0),
      mri_ptr(0),
      sessionid_ptr(0),
      nli_ptr(0),
      querycookie_ptr(0),
      stackprop_ptr(0),
      stackconf_ptr(0),
      nslpdata_ptr(0),
      respcookie_ptr(0),
      errorobject_ptr(0),
      helloid_ptr(0)
{
} // end constructor

ntlp_pdu::ntlp_pdu(const ntlp_pdu& n) : NTLP_IE(n), 
					arr(n.arr),
					ntlp_version(n.ntlp_version),
					ntlp_hops(n.ntlp_hops),
					nslpid(n.nslpid),
					type(n.type),
					flags(n.flags),
					c_flag(n.c_flag),
					//take over the NTLP object pointers from the other object
					nattrav_ptr(n.nattrav_ptr),
					mri_ptr(n.mri_ptr),
					sessionid_ptr(n.sessionid_ptr),
					nli_ptr(n.nli_ptr),
					querycookie_ptr(n.querycookie_ptr),
					stackprop_ptr(n.stackprop_ptr),
					stackconf_ptr(n.stackconf_ptr),
					nslpdata_ptr(n.nslpdata_ptr),
					respcookie_ptr(n.respcookie_ptr),
					errorobject_ptr(n.errorobject_ptr),
					helloid_ptr(n.helloid_ptr)
{
} // end copy constructor


/** Assigns objectarray, type, subtype, basic header and integrity. */
ntlp_pdu& ntlp_pdu::operator=(const ntlp_pdu& n) { 
	arr = n.arr;
	type = n.type;
	flags = n.flags;
	//take over the NTLP object pointers from the other object
	nattrav_ptr = n.nattrav_ptr;
	mri_ptr = n.mri_ptr;
	sessionid_ptr = n.sessionid_ptr;
	nli_ptr = n.nli_ptr;
	querycookie_ptr = n.querycookie_ptr;
	respcookie_ptr = n.respcookie_ptr;
	stackprop_ptr = n.stackprop_ptr;
	stackconf_ptr = n.stackconf_ptr;
	nslpdata_ptr = n.nslpdata_ptr;
	errorobject_ptr = n.errorobject_ptr;
	helloid_ptr = n.helloid_ptr;

	return *this;
} // end operator=


/***** class ntlp_pdu::objectarray *****/

const uint32 ntlp_pdu::objectarray::unset = (uint32)-1;

/** Try to allocate memory for an array of the given size. 
 * If not possible, an empty array of default size is created.
 */
ntlp_pdu::objectarray::objectarray(uint32 size) : arr() {
    if (!reserve(size)) throw IEError(IEError::ERROR_NO_MEM);
} // end constructor

ntlp_pdu::objectarray::objectarray(const objectarray& oa) 
try : arr() {
	const_iterator_t oait;
	arr.reserve(oa.size());
	for (oait=oa.begin();oait!=oa.end();oait++) {
		if (*oait) arr.push_back((*oait)->copy());
		else arr.push_back(NULL);
	} // end for oait
} catch(ProtLibException& e) {
	throw; 
} catch(...) {
	throw IEError(IEError::ERROR_NO_MEM);
} // end copy constructor	

ntlp_pdu::objectarray::~objectarray() {
	clear(true);
	arr.clear();
} // end destructor

ntlp_pdu::objectarray& ntlp_pdu::objectarray::operator=(const objectarray& oa) {
	const_iterator_t oait;
	try {
		// clear array and delete all objects
		clear(true);
		// copy objects
		for (oait=oa.begin();oait!=oa.end();oait++) {
			if (*oait)	arr.push_back((*oait)->copy());
			else arr.push_back(NULL);
		} // end for oait
	} catch(ProtLibException& e) {
		throw; 
	} catch(...) {
		throw IEError(IEError::ERROR_NO_MEM);
	} // end try-catch
	return *this;
} // end operator=	

bool ntlp_pdu::objectarray::set(uint32 pos, ntlp_object* o) {
	if ((pos<size()) && (pos<unset)) {
		arr[pos] = o;
		return true;
	} else return false;
} // end set

ntlp_object* ntlp_pdu::objectarray::get(uint32 pos) const {
	if ((pos<size()) && (pos<unset)) return arr[pos];
	else return NULL;
} // end get

uint32 ntlp_pdu::objectarray::append(ntlp_object* o) {
	uint32 pos = arr.size();
	if (pos<unset) {
		try { arr.push_back(o); 
		} catch(...) { return unset; }
		return pos;
	} else return unset;
} // end append

bool ntlp_pdu::objectarray::insert(uint32 pos, ntlp_object* obj) {
	try {
		uint32 i, newsize;
		uint32 oldsize = arr.size();
		// insert
		if (pos < oldsize) {
		    // increase array by one element
		    arr.push_back(NULL);
		    newsize= arr.size();
		    if (newsize==oldsize) // increase failed
			    return false;
		    // now we have precond: pos < oldsize < newsize
		    // shift elements one position up
		    for (i= (newsize-2); i>=pos; i--)
		    {
			    arr[i+1] = arr[i];
			    // avoid wrap-around if i==0
			    if (i == 0)
				    break;
		    }
		    arr[pos] = obj;
		    return true;
		} else if (pos == oldsize) {
		    // append at the end
		    arr.push_back(obj);
		    return true;
		} else if (pos>oldsize) {
			// increase array as necessary
		    while (pos>arr.size()) {
			arr.push_back(NULL);
		    }
   		    // append at the end
		    arr.push_back(obj);
		    return true;
		} else return false;
	} catch(...) {}
	return false;
} // end insert

/** If destroy is set true, the array is cleared and all IEs are deleted.
 */
void ntlp_pdu::objectarray::clear(bool destroy) {
	iterator_t it;
	for (it=arr.begin();it!=arr.end();it++) {
		if (*it) {
			if (destroy) delete *it; else *it = NULL;
		} // end if *it
	} // end for
	if (destroy) arr.clear();
} // end clear(bool)

void ntlp_pdu::objectarray::clear(uint32 pos, bool del) {
	ntlp_object* o;
	if ((pos<size()) && (pos<unset)) {
		o = arr[pos];
		arr[pos] = NULL;
		if (del && o) delete o;
	} // end if pos
} // end clear(uint32,bool)

bool ntlp_pdu::objectarray::reserve(uint32 n, bool init) {
	try {
		arr.reserve(n);
		if (init) {
			size_t s;
			for (s=arr.size();s<n;s++) arr.push_back(NULL);
		} // end if init
	} catch(...) { return false; }
	return true;
} // end reserve

void ntlp_pdu::print_object(uint32 pos, ostream& os, uint32 level, const uint32 indent, bool& o, const char* name) const {
	const ntlp_object* obj = arr.get(pos);
	if (obj) {
		o = true;
		os<<endl;
		os << dec;
		obj->print(os,level,indent,name);
	} // end if obj
} // end print_object



/** @return true if this type is known to the implementation and
 *   the system can handle it at least 
 */
bool known_ntlp_pdu::is_known(uint8 t) {
	switch (t) {	

		case Query:
		case Response:
		case Confirm:
		case Data:
		case Error:
			return true;
		default: return false;
	} // end switch t
} // end is_known


/** @return true if the Data contents could be replaced successfully by the given one (no copy!)
 *
 */
bool known_ntlp_pdu::set_data(nslpdata* data) {
    
    if (nslpdata_ptr) {
	nslpdata* old = dynamic_cast<nslpdata*>(arr.get(nslpdata_ptr));
	arr.set(nslpdata_ptr, data);
	if (old) delete old;
	return true;
    } else {
	nslpdata_ptr = arr.size();
	arr.insert(nslpdata_ptr, data);
	return true;
    }
    return false;
}

//@}

