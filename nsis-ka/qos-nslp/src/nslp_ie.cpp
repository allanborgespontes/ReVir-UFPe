/// ----------------------------------------*- mode: C++; -*--
/// @file nslp_ie.cpp
/// NSLP information elements for the protocol, IE manager singleton
/// ----------------------------------------------------------
/// $Id: nslp_ie.cpp 6159 2011-05-18 15:24:52Z bless $
/// $HeadURL: https://svn.tm.kit.edu/nsis/qos-nslp/trunk/src/nslp_ie.cpp $
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
/** @ingroup ie
 * @file
 * This file defines the base class of all information elements, the
 * information elements for the NSLP protocol and an IE manager
 * singleton object.
 * Although the IEs are closely related to the structure of NSLP
 * messages, they may be used in other contexts as well because of the
 * coding shemes.
 */

#include "nslp_ie.h"
#include "nslp_pdu.h"
#include "nslp_object.h"
#include "logfile.h"

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sstream>
#include <iomanip>

using namespace protlib;

namespace qos_nslp {
    //using namespace protlib;
    using namespace protlib::log;
/** @addtogroup ie Information Elements
 * @{
 */

/***** class NSLP_IEManager *****/

/**
 * Contains the only NSLP_IEManager instance ever in existence.
 */
NSLP_IEManager *NSLP_IEManager::qos_nslp_inst = NULL;

/**
 * Constructor.
 */
NSLP_IEManager::NSLP_IEManager() : IEManager() {
	// nothing to do
}

/** Returns the singleton IEManager object. */
NSLP_IEManager* NSLP_IEManager::instance() {
	if (!qos_nslp_inst)
	{
	  try {
	    qos_nslp_inst = new NSLP_IEManager();
	    Log(DEBUG_LOG,LOG_NORMAL, "IE", "Just created IEManager singleton.");
	  } catch(bad_alloc) {
	    qos_nslp_inst = 0;
	  } // end try-catch
	  if (!qos_nslp_inst)
	  {
	    Log(ERROR_LOG,LOG_CRIT, "IEManager", "no IEManager object allocated yet");
	    throw IEError(IEError::ERROR_NO_IEMANAGER);
	  }  // end if not inst
	} // end if not inst
	return qos_nslp_inst;
} // end instance

/**
 * Delete the NSLP_IEManager singleton.
 *
 * This makes all pointers to or into it invalid. To use MARSP_IEManager
 * again, the static instance() method has to be called.
 */
void NSLP_IEManager::clear() {
	if ( ! qos_nslp_inst )
		return;

	IEManager *tmp = qos_nslp_inst;
	qos_nslp_inst = NULL;

	Log(INFO_LOG, LOG_NORMAL, "NSLP_IEManager",
		"Destroying IEManager singleton.");

	delete tmp;
}

/** Deserialize the next IE. You must specify an IE category so the
 * IEManager is able to decode the type and subtype of the IE correctly.
 * The IEManager cannot check in all cases wether the IE at the current
 * position really is one of the expected category.
 * The returned IE may be of a different category than cat, especially since
 * this member function handles unknown types and subtypes of NSLP objects and
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
IE* NSLP_IEManager::deserialize(NetMsg& msg, uint16 cat, IE::coding_t coding, IEErrorList& errorlist, uint32& bread, bool skip) {
	uint32 saved_pos;
	uint32 bleft;
	bread = 0;
	// check arguments
	// save current position in msg
	saved_pos = msg.get_pos();
	bleft = msg.get_bytes_left();
	if (!bleft) return NULL;
	switch (coding) {
		case NSLP_IE::nslp_v1:
			switch (cat) {
				case NSLP_IE::cat_known_nslp_pdu:
					return deserialize_known_nslp_pdu(msg,coding,saved_pos,bleft,errorlist,bread,skip);
					break;
				case NSLP_IE::cat_known_nslp_object:
					return deserialize_known_nslp_object(msg,coding,saved_pos,bleft,errorlist,bread,skip);
					break;
				default:
				  ERRLog("IE", "Category " << cat << " with coding " << coding << " not supported by IEManager::deserialize");
				  catch_bad_alloc(errorlist.put(new IEError(IEError::ERROR_CATEGORY)));
				  return NULL;
			} // end switch cat
			break;
		default:
		  ERRLog("IE", "Coding " << coding << " not supported by IEManager::deserialize");
		  catch_bad_alloc(errorlist.put(new IEError(IEError::ERROR_CODING)));
		  return NULL;
	} // end switch coding
	return NULL;
} // end deserialize

/**
 * Use a registered PDU to deserialize msg.
 *
 * Helper method for deserialize(). Parameters work like in deserialize().
 *
 * @param msg a buffer containing the serialized PDU.
 * @param coding the protocol version used in msg.
 * @param saved_pos saved read position in msg.
 * @param bleft the number of bytes left in the message till end.
 * @param errorlist returns the exceptions caught while parsing the message.
 * @param bread returns the number of bytes read from msg.
 * @param skip if true, try to ignore errors and continue reading.
 * @return the newly created IE, or NULL on error.
 */
IE* NSLP_IEManager::deserialize_known_nslp_pdu(NetMsg& msg, IE::coding_t coding, uint32 saved_pos, uint32 bleft, IEErrorList& errorlist, uint32& bread, bool skip) {
	const NSLP_IE::category_t cat = NSLP_IE::cat_known_nslp_pdu;
		uint16 generic_flags = 0;
		uint8 type = 0;
		uint8 specific_flags = 0;
		uint32 len;
		IE* ie = NULL;
		IE* result = NULL;
		IEError *iee = NULL;
		// Check if at least the common header is available.
		if (bleft<nslp_pdu::common_header_length) {
			catch_bad_alloc(iee = new IEMsgTooShort(coding,cat,saved_pos));
			ERRLog("IE", "1 deserialize_nslp_pdu: error " << iee->getstr() << ", coding " << coding << ", category " << cat << ", position " << saved_pos);
			errorlist.put(iee);
			return NULL;
		} // end if bleft<common_header_length
		try {
			nslp_pdu::decode_common_header_nslpv1(msg,type,specific_flags,generic_flags,len);
		} catch(IEError& ignored) {
			catch_bad_alloc(iee = new IETooBigForImpl(coding,cat,saved_pos));
			ERRLog("IE", "2 deserialize_nslp_pdu: error " << iee->getstr() << ", coding " << coding << ", category " << cat << ", position " << saved_pos);
			errorlist.put(iee);
			msg.set_pos(saved_pos);
			bread = 0;
			return NULL;
		} // end try-catch
		msg.set_pos(saved_pos);
		// lookup deserialization routine
		ie = lookup(cat,type);
		// call desfunc
		if (ie) {
		  try
		  {
		    result = ie->deserialize(msg,coding,errorlist,bread,skip);
		  } // end try
		  catch(NetMsgError& ne)
		  {
		    Log(ERROR_LOG,LOG_CRIT,"IE","deserialize_nslp_pdu: exception during parsing NetMsg @position " << saved_pos << " readpos=" << bread  << " : [" << ne.getstr()<<"]");
		  } // end catch
		  catch(IEError& ie_err)
		  {
		    Log(ERROR_LOG,LOG_CRIT,"IE","deserialize_nslp_pdu: exception during parsing NetMsg @position " << saved_pos << " readpos=" << bread  << " : [" << ie_err.getstr()<<"]");
		  } // end catch

		  // check result
		  if (result)
		    return result;
		  else
		  {
		    delete ie; return NULL;
		  }

		} 
		else { // IE not known
			catch_bad_alloc(
			if (!known_nslp_pdu::is_known(type))
				iee = new IEWrongType(coding,cat,saved_pos);
			); // end catch_bad_alloc
			ERRLog("IE", "IEManager::deserialize_nslp_pdu: error " << iee->getstr() << ", coding " << coding << ", type " << (int)type << ", position " << saved_pos << (skip ? ", skipping IE." : ""));
			errorlist.put(iee);
			if (skip && (bleft>=(len+nslp_pdu::common_header_length))) {
				// skip IE
				bread = len+nslp_pdu::common_header_length;
				msg.set_pos_r(bread);
			} // end if skip
			return NULL;
		} // end if ie
} // end deserialize_known_nslp_pdu

/**
 * Use a registered object to deserialize msg.
 *
 * Helper method for deserialize(). Parameters work like in deserialize().
 *
 * @param msg a buffer containing the serialized PDU.
 * @param coding the protocol version used in msg.
 * @param saved_pos saved read position in msg.
 * @param bleft the number of bytes left in the message till end.
 * @param errorlist returns the exceptions caught while parsing the message.
 * @param bread returns the number of bytes read from msg.
 * @param skip if true, try to ignore errors and continue reading.
 * @return the newly created IE, or NULL on error.
 */
IE* NSLP_IEManager::deserialize_known_nslp_object(NetMsg& msg, IE::coding_t coding, uint32 saved_pos, uint32 bleft, IEErrorList& errorlist, uint32& bread, bool skip) {
	const NSLP_IE::category_t cat = NSLP_IE::cat_known_nslp_object;
	NSLP_IE::action_type_t action= NSLP_IE::ignore;
		uint16 type;
		uint16 len = 0;
		uint32 ielen = 0;
		IE* ie = NULL;
		IE* result = NULL;
		IEError *iee = NULL;
		// There must be at least 4 bytes available.
		if (bleft<nslp_object::header_length) {
			catch_bad_alloc(iee = new IEMsgTooShort(coding,cat,saved_pos));
			ERRLog("NSLP_IE", "NSLP_IEManager::deserialize_known_nslp_object: error " << iee->getstr() << ", coding " << coding << ", category " << cat << ", position " << saved_pos);
			errorlist.put(iee);
			return NULL;
		} // end if bleft<4
		// decode type
		try {
		        nslp_object::decode_header_nslpv1(msg,action,type,len,ielen);
		} catch(IEError& ignored) {
			catch_bad_alloc(iee = new IETooBigForImpl(coding,cat,saved_pos));
			ERRLog("NSLP_IE", "NSLP_IEManager::deserialize_known_nslp_object: error " << iee->getstr() << " len:" << len << " ielen:" << ielen << ", coding " << coding << ", category " << cat << ", position " << saved_pos);
			errorlist.put(iee);
			msg.set_pos(saved_pos);
			bread = 0;
			return NULL;
		} // end try-catch
		msg.set_pos(saved_pos);
		// lookup deserialization routine
		ie = lookup(cat,type);
		// call desfunc
		if (ie) { // information element known (registered)
			result = ie->deserialize(msg,coding,errorlist,bread,skip);
			// check result
			if (result) return result;
			else {
				Log(INFO_LOG,LOG_NORMAL, "NSLP_IE", "NSLP_IEManager::deserialize_known_nslp_object(): in else(ie)");
				delete ie;
				return NULL;
			}
		}
		else // information element unknown
		{
		  if (action == NSLP_IE::mandatory)
		    {
			ERRLog("NSLP_IE", "IEManager::deserialize_known_nslp_object(): unknown object type, error " << iee->getstr() << ", coding " << coding << ", type " << (int)type << ", position " << saved_pos << (skip ? ", skipping IE." : ""));
			catch_bad_alloc(iee = new IEWrongType(coding,cat,saved_pos)); // end catch_bad_alloc
			errorlist.put(iee);
		    }
		  else
		    {
		      WLog("NSLP_IE", "IEManager::deserialize_known_nslp_object(): ignored unknown object type, error " << iee->getstr() << ", coding " << coding << ", type " << (int)type << ", position " << saved_pos << (skip ? ", skipping IE." : ""));
		      // TODO check for raw object parsing
		    }
		}
		return NULL;
} // end deserialize_known_nslp_object


/** Lookup an IE in the IEManager hashmaps. If one is found, a new instance
 * of it is returned.
 */
IE* NSLP_IEManager::lookup(uint16 cat, uint16 type, uint16 subtype) {
	register IE* ie = NULL;
	if (cat<NSLP_IE::num_of_categories) {
		ie = lookup_ie(cat, type, subtype);
		if (!ie) { 
			if (cat==NSLP_IE::cat_raw_protocol_object)
				ie = lookup_ie(cat, 0, 0);
			else if (cat==NSLP_IE::cat_known_pdu_object)
				ie = lookup_ie(cat, type, 0);
		} // end if no ie
		if (ie) return ie->new_instance();
		else return NULL;
	} else return NULL;
} // end lookup

//@}

} // end namespace protlib
