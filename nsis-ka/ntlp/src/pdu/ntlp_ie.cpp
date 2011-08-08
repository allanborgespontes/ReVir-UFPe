/// ----------------------------------------*- mode: C++; -*--
/// @file ntlp_ie.cpp
/// information elements for the protocol, IE manager singleton
/// ----------------------------------------------------------
/// $Id: ntlp_ie.cpp 5249 2010-04-26 08:40:59Z roehricht $
/// $HeadURL: https://svn.tm.kit.edu/nsis/ntlp/trunk/src/pdu/ntlp_ie.cpp $
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
/** @ingroup ie
 * This file defines the base class of all information elements, the
 * information elements for the NTLP protocol and an IE manager singleton
 * object.
 * Although the IEs are closely related to the structure of GIST messages,
 * they may be used in other contexts as well because of the coding shemes.
 */

#include "ntlp_ie.h"
#include "ntlp_pdu.h"
#include "ntlp_object.h"
#include "logfile.h"

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sstream>
#include <iomanip>

namespace ntlp {
using namespace protlib;
    using namespace protlib::log;

/** @addtogroup ie Information Elements
 * @{
 */

/***** class NTLP_IEManager *****/

/**
 * Contains the only NTLP_IEManager instance ever in existence.
 */
NTLP_IEManager *NTLP_IEManager::ntlp_inst = NULL;

/**
 * Constructor.
 */
NTLP_IEManager::NTLP_IEManager() : IEManager() {
	// nothing to do
}

/** Returns the singleton IEManager object. */
NTLP_IEManager* NTLP_IEManager::instance() {
	if (!ntlp_inst) 
	{
	  try {
	      ntlp_inst = new NTLP_IEManager();
	      DLog( "NTLP_IE", "Just created NTLP IEManager singleton.");
	  } catch(bad_alloc) {
	      ntlp_inst = 0;
	  } // end try-catch
	  if (!ntlp_inst) 
	  {
	      Log(ERROR_LOG,LOG_CRIT, "IEManager", "no NTLP IEManager object allocated yet");
	      throw IEError(IEError::ERROR_NO_IEMANAGER);
	  }  // end if not inst
	} // end if not inst
	return ntlp_inst;
} // end instance

/**
 * Delete the singleton instance.
 *
 * All pointers to or into the object are invalid after calling this.
 */
void NTLP_IEManager::clear() {
	delete ntlp_inst;
	ntlp_inst = 0;
	DLog("NTLP_IEManager",
		"Deleted NTLP_IEManager singleton");
}


/** Deserialize the next IE. You must specify an IE category so the
 * IEManager is able to decode the type and subtype of the IE correctly.
 * The IEManager cannot check in all cases wether the IE at the current
 * position really is one of the expected category.
 * The returned IE may be of a different category than cat, especially since 
 * this member function handles unknown types and subtypes of NTLP objects and
 * does some of the message length checking work.
 * @param msg NetMsg object
 * @param cat IE category
 * @param coding coding sheme, this is passed to the IE deserialization
 *    function.
 * @param errorlist pointer to an IE error list in which all kinds of errors
 *   are stored.
 * @param bread reference to integer. Set to the number of bytes read by this
 *    call.
 * @param skip skip the IE in case of an error or restore NetMsg position.
 * @return a pointer to the deserialized IE or NULL if an IE was discarded
 * or rejected.
 */ 
IE* NTLP_IEManager::deserialize(NetMsg& msg, uint16 cat, IE::coding_t coding, IEErrorList& errorlist, uint32& bread, bool skip) {
	uint32 saved_pos;
	uint32 bleft;
	bread = 0;
	// check arguments
	// save current position in msg
	saved_pos = msg.get_pos();
	bleft = msg.get_bytes_left();
	if (!bleft) return NULL;
	switch (coding) {
		case IE::protocol_v1:
			switch (cat) {
				case NTLP_IE::cat_known_pdu:
				  DLog("NTLP_IE", "started to deserialize known_ntlp_pdu @pos:"<<saved_pos<< " bytes left in buffer:" << bleft);
				    return deserialize_known_ntlp_pdu(msg,coding,saved_pos,bleft,errorlist,bread,skip);
					break;
				case NTLP_IE::cat_known_pdu_object:
				    DLog("NTLP_IE", "started to deserialize known_ntlp_object @pos:"<<saved_pos<< " bytes left in buffer:" << bleft);
				    return deserialize_known_ntlp_object(msg,coding,saved_pos,bleft,errorlist,bread,skip);
					break;
				case NTLP_IE::cat_raw_protocol_object:
					return deserialize_raw_ntlp_object(msg,coding,saved_pos,bleft,errorlist,bread,skip);
					break;
			    default:
				  ERRLog("NTLP_IE", "Category " << cat << " with coding " << coding << " not supported by IEManager::deserialize");
				  catch_bad_alloc(errorlist.put(new IEError(IEError::ERROR_CATEGORY)));
				  return NULL;
			} // end switch cat
			break;
		default:
		  ERRLog("NTLP_IE", "Coding " << coding << " not supported by IEManager::deserialize");
		  catch_bad_alloc(errorlist.put(new IEError(IEError::ERROR_CODING)));
		  return NULL;
	} // end switch coding	
	return NULL;
} // end deserialize

IE* NTLP_IEManager::deserialize_known_ntlp_pdu(NetMsg& msg, IE::coding_t coding, uint32 saved_pos, uint32 bleft, IEErrorList& errorlist, uint32& bread, bool skip) {
	const NTLP_IE::category_t cat = NTLP_IE::cat_known_pdu;
	uint8 vers = 0; // not used
	uint8 type = 0;
	uint16 subtype = 0;
	uint16 len;
	uint16 nslpid;
	uint8 flags;
	bool  c_flag= false;
	uint8 hops;
	IE* ie = NULL;
	IE* result = NULL;
	IEError *iee = NULL;
	// Check if at least the common header is available.
	if (bleft<ntlp_pdu::common_header_length) {
		catch_bad_alloc(iee = new IEMsgTooShort(coding,cat,saved_pos));
		ERRLog("NTLP_IE", "deserialize_ntlp_pdu: error " << iee->getstr() << ", coding " << coding << ", category " << cat << ", position " << saved_pos);
		errorlist.put(iee);
		return NULL;
	} // end if bleft<common_header_length 
	try {
		ntlp_pdu::decode_common_header_ntlpv1(msg,vers,hops, len, nslpid, type, flags, c_flag, errorlist);
	} catch(IEError& ignored) {
		catch_bad_alloc(iee = new IETooBigForImpl(coding,cat,saved_pos));
		ERRLog("NTLP_IE", "deserialize_ntlp_pdu: error " << iee->getstr() << ", coding " << coding << ", category " << cat << ", position " << saved_pos);
		errorlist.put(iee);
		msg.set_pos(saved_pos);
		bread = 0;
		return NULL;
	} // end try-catch
	msg.set_pos(saved_pos);
	// lookup deserialization routine and create new instance if possible
	ie = new_instance(cat,type,subtype);
	// call desfunc
	if (ie) {
	  try
	  {
	    result = ie->deserialize(msg,coding,errorlist,bread,skip);
	  } // end try
	  catch(NetMsgError& ne)
	  {
	    Log(ERROR_LOG,LOG_CRIT,"NTLP_IE","deserialize_ntlp_pdu: exception during parsing NetMsg @position " << saved_pos << " readpos=" << bread  << " : [" << ne.getstr()<<"]");
	  } // end catch
	  catch(IEError& ie_err)
	  {
	    Log(ERROR_LOG,LOG_CRIT,"NTLP_IE","deserialize_ntlp_pdu: exception during parsing NetMsg @position " << saved_pos << " readpos=" << bread  << " : [" << ie_err.getstr()<<"]");
	  } // end catch

	  // check result
	  if (result) 
	    return result;
	  else 
	  { 
	    delete ie; return NULL; 
	  }
	  
	} else {
		catch_bad_alloc(
		if (known_ntlp_pdu::is_known(type))
			iee = new IEWrongSubtype(coding,cat,type,saved_pos);
		else
			iee = new IEWrongType(coding,cat,saved_pos);
		); // end catch_bad_alloc
		ERRLog("NTLP_IE", "IEManager::deserialize_ntlp_pdu: error " << iee->getstr() << ", coding " << coding << ", type " << (int)type << ", subtype " << (int)subtype << ", position " << saved_pos << (skip ? ", skipping IE." : ""));
		errorlist.put(iee);
		if (skip && (bleft>=(len+ntlp_pdu::common_header_length))) {
			// skip IE
			bread = len+ntlp_pdu::common_header_length;
			msg.set_pos_r(bread);
		} // end if skip
		return NULL;
	} // end if ie
} // end deserialize_known_ntlp_pdu

IE* NTLP_IEManager::deserialize_known_ntlp_object(NetMsg& msg, IE::coding_t coding, uint32 saved_pos, uint32 bleft, IEErrorList& errorlist, uint32& bread, bool skip) {
  const NTLP_IE::category_t cat = NTLP_IE::cat_known_pdu_object;
  uint16 type;
  uint16 subtype = 0;
  bool nat_flag = false;
  uint8 action = 0;
  uint8 objst = 0;
  uint16 len = 0;
  uint32 ielen = 0;
  IE* ie = NULL;
  IE* result = NULL;
  IEError *iee = NULL;

  // There must be at least 4 bytes available.
  if (bleft<ntlp_object::header_length) {
    catch_bad_alloc(iee = new IEMsgTooShort(coding,cat,saved_pos));
    ERRLog("NTLP_IE", "IEManager::deserialize_known_ntlp_object: error " << iee->getstr() << ", coding " << coding << ", category " << cat << ", position " << saved_pos);
    errorlist.put(iee);
    return NULL;
  } // end if bleft<4

  // decode type
  try {
    ntlp_object::decode_header_ntlpv1(msg,type,subtype,nat_flag,action,len,ielen);
  } 
  catch(IEError& ignored) {
    catch_bad_alloc(iee = new IETooBigForImpl(coding,cat,saved_pos));
    ERRLog("NTLP_IE", "IEManager::deserialize_known_ntlp_object: error " << iee->getstr() << ", coding " << coding << ", category " << cat << ", position " << saved_pos);
    errorlist.put(iee);
    msg.set_pos(saved_pos);
    bread = 0;
    return NULL;
  } // end try-catch

  DLog("NTLP_IE","IEManager::deserialize_known_ntlp_object: found object type " << type 
       << "@pos:" << saved_pos << ", total object length (incl. header):" << ielen);

  msg.set_pos(saved_pos);
  // lookup deserialization routine and create new instance if possible
  ie = new_instance(cat,type,subtype);
  // call desfunc
  if (ie) 
  {
    try {
      // deserialize object, returns corresponding C++ object for parsed PDU object
      result = ie->deserialize(msg,coding,errorlist,bread,skip);
    }
    catch(NetMsgError& ne)
    {
      Log(ERROR_LOG,LOG_CRIT,"NTLP_IE","deserialize_known_ntlp_object: exception during parsing NetMsg @position " << saved_pos << " readpos=" << bread  << " : [" << ne.getstr()<<"]");
    } // end catch
    catch(IEError& ie_err)
    {
      Log(ERROR_LOG,LOG_CRIT,"NTLP_IE","deserialize_known_ntlp_object: exception during parsing NetMsg @position " << saved_pos << " readpos=" << bread  << " : [" << ie_err.getstr()<<"]");
    } // end catch
    
    // check result
    if (result) 
      return result;
    else 
    {
      ERRLog("NTLP_IE","IEManager::deserialize_known_ntlp_object: did not deserialize object type " << type << ", total object length (incl. header):" << ielen);
      delete ie;
      return NULL; 
    }
    // if deserialization fails... here are the hooks for "action"
  } 
  else 
  {
    ERRLog("NTLP_IE", "IEManager: could not lookup the object in its registry");
    // check action
    switch (action) {
      case known_ntlp_object::Mandatory:
	// report error
	iee = new GIST_MissingObject(coding,type,saved_pos);
	
	ERRLog("NTLP_IE", "IEManager::deserialize_known_ntlp_object: error " << iee->getstr() << ", coding " << coding << ", type " << (int)type << ", subtype " << (int)objst << ", action " << (int)action << ", position " << saved_pos);
	errorlist.put(iee);
	// exit if skip is set false.
	if (!skip) return NULL;
	// do not break here!
      case known_ntlp_object::Ignore:
	// discard the IE, jump to next one.
	// Skip IE content length and IE header.
	// There must be at least len+4 bytes available.
	if (bleft<ielen) {
	  catch_bad_alloc(iee = new IEMsgTooShort(coding,cat,saved_pos));
	  ERRLog("NTLP_IE", "IEManager::deserialize_known_ntlp_object: error " << iee->getstr() << ", coding " << coding << ", category " << cat << ", position " << saved_pos);
	  errorlist.put(iee);
	  return NULL;
	} else {
	  bread = ielen;
	  msg.set_pos_r(ielen);
	  return NULL;
	} // end if bleft<len+header
	break;
      case known_ntlp_object::Forward:
	// carry on raw ntlp object
	DLog("NTLP_IE", "Carrying on NTLP Object type " << type << ", subtype " << objst << ", action " << action << " as raw_ntlp_object");
	return deserialize_raw_ntlp_object(msg,coding,saved_pos,bleft,errorlist,bread,skip);
	break;
      default: 
	ERRLog("deserialize_known_ntlp_object", "Encountered object which has neither Mandatory, Forward nor Ignore set");
	return NULL;
    } // end switch action
  } // end if ie
} // end deserialize_known_ntlp_object

IE* NTLP_IEManager::deserialize_raw_ntlp_object(NetMsg& msg, IE::coding_t coding, uint32 saved_pos, uint32 bleft, IEErrorList& errorlist, uint32& bread, bool skip) {
	const NTLP_IE::category_t cat = NTLP_IE::cat_raw_protocol_object;
	uint16 type = 0;
	uint16 subtype =0;
	bool nat_flag = false;
	uint8 action = 0;
	uint8 objst = 0;
	uint16 len;
	uint32 ielen;
	IE* ie = NULL;
	IE* result = NULL;
	IEError *iee = NULL;
	// There must be at least 4 bytes available.
	if (bleft<ntlp_object::header_length) {
		catch_bad_alloc(iee = new IEMsgTooShort(coding,cat,saved_pos));
		ERRLog("NTLP_IE", "IEManager::deserialize_raw_ntlp_object: error " << iee->getstr() << ", coding " << coding << ", category " << cat << ", position " << saved_pos);
		errorlist.put(iee);
		return NULL;
	} // end if bleft<4
	// decode type
	try {
		ntlp_object::decode_header_ntlpv1(msg,type,subtype,nat_flag,action,len,ielen);
	} catch(IEError& ignored) {
		catch_bad_alloc(iee = new IETooBigForImpl(coding,cat,saved_pos));
		ERRLog("NTLP_IE", "IEManager::deserialize_raw_ntlp_object: error " << iee->getstr() << ", coding " << coding << ", category " << cat << ", position " << saved_pos);
		errorlist.put(iee);
		msg.set_pos(saved_pos);
		bread = 0;
		return NULL;
	} // end try-catch
	msg.set_pos(saved_pos);

	// lookup deserialization routine and create new instance if possible
	//ie = new_instance(cat,type,objst);


	//create a new instance of raw_ntlp_object
	ie = new raw_ntlp_object();

	// call desfunc
	if (ie) {
		result = ie->deserialize(msg,coding,errorlist,bread,skip);
		// check result
		if (result) return result;
		else { delete ie; return NULL; }
	} else {
		catch_bad_alloc(
		if (known_ntlp_object::is_known(type))
			iee = new IEWrongSubtype(coding,cat,type,saved_pos);
		else
			iee = new IEWrongType(coding,cat,saved_pos);
		); // end catch_bad_alloc
		ERRLog("NTLP_IE", "IEManager::deserialize_ntlp_pdu: error " << iee->getstr() << ", coding " << coding << ", type " << (int)type << ", subtype " << (int)objst << ", action " << (int)action << ", position " << saved_pos << (skip ? ", skipping IE." : ""));
		errorlist.put(iee);
		if (skip && (bleft>=ielen)) {
			// skip IE
			bread = ielen;
			msg.set_pos_r(ielen);
		} // end if skip
		return NULL;
	} // end if ie
} // end deserialize_raw_ntlp_object

IE* NTLP_IEManager::deserialize_service(NetMsg& msg, IE::coding_t coding, uint32 saved_pos, uint32 bleft, IEErrorList& errorlist, uint32& bread, bool skip) {

    return NULL;
} // end deserialize_service


/**
 * Find a registered IE by category, type and subtype.
 *
 * Does @em not create a new instance. If you need a new instance, use the
 * new_instance() method instead which calls lookup_ie().
 *
 * This method implements the old fallback mechanism found in earlier
 * versions of NTLP_IEManager::lookup().
 */
IE *NTLP_IEManager::lookup_ie(uint16 category, uint16 type, uint16 subtype) {
	IE *ret = IEManager::lookup_ie(category, type, subtype);

	// found a registered IE
	if ( ret != NULL )
		return ret;

	// no IE found

	if ( category == NTLP_IE::cat_raw_protocol_object )
		return IEManager::lookup_ie(category, 0, 0);
	else if ( category == NTLP_IE::cat_known_pdu_object )
		return IEManager::lookup_ie(category, type,
				known_ntlp_object::all_subtypes);

	return NULL; // no fallback IE found
}


/***** class GIST_IncorrectMsgLength *****/
GIST_IncorrectMsgLength::GIST_IncorrectMsgLength(IE::coding_t cod, uint8 version, uint8 hops, uint16 length, uint16 nslpid, uint8 type, uint8 flags) 
	: coding(cod),
	  version(version),
	  hops(hops),
	  length(length),
	  nslpid(nslpid),
	  type(type),
	  flags(flags)
{
    ERRCLog("GIST Error" , IEError::getstr() << " version: " << (int) version);

} // end constructor


/***** class GIST_MessageTooLarge *****/
GIST_MessageTooLarge::GIST_MessageTooLarge(IE::coding_t cod, const ntlp_object& obj, const ntlp_pdu& pdu, uint32 pos) 
	: coding(cod), 
	  objecttype(obj.get_type()), objectsubtype(255),
	  pdutype(pdu.get_type()), pdusubtype(0),
	  errorpos(pos) {
    cout << IEError::getstr() << obj.get_ie_name() << (unsigned int)objecttype << (unsigned int)objectsubtype <<
	pdu.get_ie_name() << (unsigned int)pdutype << (unsigned int)pdusubtype << pos;

} // end constructor


/***** class GIST_DuplicateObject *****/
GIST_DuplicateObject::GIST_DuplicateObject(IE::coding_t cod, uint8 pdutype, uint16 objecttype) 
	: coding(cod), 
	  objecttype(objecttype),
	  pdutype(pdutype) {
    ERRCLog("GIST PDU" , IEError::getstr() << ", object type: " << (int) objecttype << ", PDU type: " << (int) pdutype);

} // end constructor


/***** class GIST_UnrecognisedObject *****/
GIST_UnrecognisedObject::GIST_UnrecognisedObject(IE::coding_t cod, uint16 objecttype) 
	: coding(cod), 
	  objecttype(objecttype) {
    cout << IEError::getstr() << "objecttype: " << (unsigned int)objecttype;
    
} // end constructor


/***** class GIST_MissingObject *****/
GIST_MissingObject::GIST_MissingObject(IE::coding_t cod, uint8 pdutype, uint16 objecttype) 
	: coding(cod), 
	  objecttype(objecttype),
	  pdutype(pdutype) {
    ERRCLog("GIST PDU" , IEError::getstr() << ", object type: " << (int) objecttype << ", PDU type: " << (int) pdutype);

} // end constructor


/***** class GIST_InvalidObject *****/
GIST_InvalidObject::GIST_InvalidObject(IE::coding_t cod, uint8 pdutype, uint16 objecttype) 
	: coding(cod), 
	  objecttype(objecttype),
	  pdutype(pdutype) {
    ERRCLog("GIST PDU" , IEError::getstr() << ", object type: " << (int) objecttype << ", PDU type: " << (int) pdutype);


} // end constructor


/***** class GIST_IncorrectObjectLength *****/
GIST_IncorrectObjectLength::GIST_IncorrectObjectLength(IE::coding_t cod, const ntlp_object* object) 
    : coding(cod) 
{
    obj = object->copy();
    ERRCLog("GIST PDU", IEError::getstr() << " objecttype: " << (int) obj->get_type());
    
} // end constructor
    

/***** class GIST_ValueNotSupported *****/
GIST_ValueNotSupported::GIST_ValueNotSupported(IE::coding_t cod, const ntlp_object* object) 
	: coding(cod)
{
    obj = object->copy();
    ERRCLog("GIST PDU", IEError::getstr() << " objecttype: " << (int) obj->get_type());
    
} // end constructor

/***** class GIST_InvalidFlagFieldCombination *****/
GIST_InvalidFlagFieldCombination::GIST_InvalidFlagFieldCombination(IE::coding_t cod, const ntlp_object* object) 
	: coding(cod)
{
    obj = object->copy();
    ERRCLog("GIST PDU", IEError::getstr() << " objecttype: " << (int) obj->get_type());
    
} // end constructor



/***** class GIST_MessageTooLarge *****/
GIST_EmptyList::GIST_EmptyList(IE::coding_t cod, const ntlp_object* object) 
	: coding(cod)
{
    obj = object->copy();
    ERRCLog("GIST PDU", IEError::getstr() << " objecttype: " << (int) obj->get_type());


} // end constructor


/***** class GIST_InvalidIPTTL *****/
GIST_InvalidIPTTL::GIST_InvalidIPTTL(IE::coding_t cod) 
	: coding(cod)
{
  ERRCLog("GIST PDU", IEError::getstr() << " - Invalid IP TTL");

} // end constructor

/***** class GIST_InvalidIPTTL *****/
GIST_IncorrectEncapsulation::GIST_IncorrectEncapsulation(IE::coding_t cod) 
	: coding(cod)
{
  ERRCLog("GIST PDU", IEError::getstr() << " - Incorrect Encapsulation");

} // end constructor




/***** class GIST_SpScdMismatch *****/
GIST_SpScdMismatch::GIST_SpScdMismatch(IE::coding_t cod, const ntlp_object& obj, const ntlp_pdu& pdu, uint32 pos) 
	: coding(cod), 
	  objecttype(obj.get_type()), objectsubtype(255),
	  pdutype(pdu.get_type()), pdusubtype(0),
	  errorpos(pos) {
    cout << IEError::getstr() << obj.get_ie_name() << (unsigned int)objecttype << (unsigned int)objectsubtype <<
	pdu.get_ie_name() << (unsigned int)pdutype << (unsigned int)pdusubtype << pos;

} // end constructor


/***** class GIST_UnknownVersion *****/
GIST_UnknownVersion::GIST_UnknownVersion(IE::coding_t cod, uint8 version, uint8 hops, uint16 length, uint16 nslpid, uint8 type, uint8 flags) 
	: coding(cod),
	  version(version),
	  hops(hops),
	  length(length),
	  nslpid(nslpid),
	  type(type),
	  flags(flags)
{
    ERRCLog("GIST Error" , IEError::getstr() << " version: " << (int) version);

} // end constructor



/***** class GIST_UnknownType *****/
GIST_UnknownType::GIST_UnknownType(IE::coding_t cod, uint8 version, uint8 hops, uint16 length, uint16 nslpid, uint8 type, uint8 flags) 
	: coding(cod),
	  version(version),
	  hops(hops),
	  length(length),
	  nslpid(nslpid),
	  type(type),
	  flags(flags)
{
    ERRCLog("GIST Error" , IEError::getstr() << " version: " << (int) version);

} // end constructor


/***** class GIST_InvalidRFlag *****/
GIST_InvalidRFlag::GIST_InvalidRFlag(IE::coding_t cod, uint8 version, uint8 hops, uint16 length, uint16 nslpid, uint8 type, uint8 flags) 
	: coding(cod),
	  version(version),
	  hops(hops),
	  length(length),
	  nslpid(nslpid),
	  type(type),
	  flags(flags)
{
    ERRCLog("GIST Error" , IEError::getstr() << " version: " << (int) version);

} // end constructor


/***** class GIST_InvalidEFlag *****/
GIST_InvalidEFlag::GIST_InvalidEFlag(IE::coding_t cod, uint8 version, uint8 hops, uint16 length, uint16 nslpid, uint8 type, uint8 flags) 
	: coding(cod),
	  version(version),
	  hops(hops),
	  length(length),
	  nslpid(nslpid),
	  type(type),
	  flags(flags)
{
    ERRCLog("GIST Error" , IEError::getstr() << " version: " << (int) version);
} // end constructor


/***** class GIST_InvalidCFlag *****/
GIST_InvalidCFlag::GIST_InvalidCFlag(IE::coding_t cod, uint8 version, uint8 hops, uint16 length, uint16 nslpid, uint8 type, uint8 flags) 
	: coding(cod),
	  version(version),
	  hops(hops),
	  length(length),
	  nslpid(nslpid),
	  type(type),
	  flags(flags)
{
    ERRCLog("GIST Error" , IEError::getstr() << " version: " << (int) version);
} // end constructor


} // end namespace protlib
