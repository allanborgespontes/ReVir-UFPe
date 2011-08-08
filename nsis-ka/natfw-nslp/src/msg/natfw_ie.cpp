/// ----------------------------------------*- mode: C++; -*--
/// @file natfw_ie.cpp
/// The NATFW IE Manager
/// ----------------------------------------------------------
/// $Id: natfw_ie.cpp 2896 2008-02-21 00:54:43Z bless $
/// $HeadURL: https://svn.tm.kit.edu/nsis/natfw-nslp/trunk/src/msg/natfw_ie.cpp $
// ===========================================================
//                      
// Copyright (C) 2005-2007, all rights reserved by
// - Institute of Telematics, Universitaet Karlsruhe (TH)
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
#include "logfile.h"

#include "msg/natfw_ie.h"
#include "msg/natfw_msg.h"


using namespace natfw::msg;
using namespace protlib::log;

#define LogError(msg) Log(ERROR_LOG, LOG_NORMAL, "NATFW_IEManager", msg)
#define LogWarn(msg) Log(WARNING_LOG, LOG_NORMAL, "NATFW_IEManager", msg)
#define LogInfo(msg) Log(INFO_LOG, LOG_NORMAL, "NATFW_IEManager", msg)
#define LogDebug(msg) Log(DEBUG_LOG, LOG_NORMAL, "NATFW_IEManager", msg)


/**
 * This is where our single NATFW_IEManager is stored.
 */
NATFW_IEManager *NATFW_IEManager::natfw_inst = NULL;


/**
 * Constructor for child classes.
 *
 * This constructor has been made protected to only allow instantiation
 * via the static instance() method.
 */
NATFW_IEManager::NATFW_IEManager() : IEManager() {
	// nothing to do
}


/**
 * Singleton static factory method.
 *
 * Returns the same NATFW_IEManager object, no matter how often it is called.
 *
 * Note: Calling clear() causes the object to be deleted and the next call
 * to instance() create a new NATFW_IEManager object.
 *
 * @return always the same NATFW_IEManager object
 */
NATFW_IEManager *NATFW_IEManager::instance() {

	// Instance already exists, so return it.
	if ( natfw_inst )
		return natfw_inst;

	// We don't have an instance yet. Create it.
	try {
		natfw_inst = new NATFW_IEManager();
		LogDebug("created NATFW_IEManager singleton: " << natfw_inst);
	}
	catch ( bad_alloc ) {
		LogError("cannot create NATFW_IEManager singleton");
		throw IEError(IEError::ERROR_NO_IEMANAGER);
	}

	return natfw_inst;
}


/**
 * Delete the singleton instance.
 *
 * After calling this, all pointers to or into the object are invalid.
 * The instance() method has to be called before using the NATFW_IEManager
 * next time.
 */
void NATFW_IEManager::clear() {
	if ( natfw_inst != NULL ) {
		delete natfw_inst;
		natfw_inst = 0;
		LogDebug("deleted NATFW_IEManager singleton");
	}
}


/**
 * Register all known IEs.
 *
 * This method clears the registry and then registers all IEs known to this
 * implementation. It solely exists for convenience.
 */
void NATFW_IEManager::register_known_ies() {
	clear();

	NATFW_IEManager *inst = instance();

	inst->register_ie(new natfw_create());
	inst->register_ie(new natfw_ext());
	inst->register_ie(new natfw_notify());
	inst->register_ie(new natfw_response());

	// TODO: use a yet to implement natfw_msg_raw!
	//inst->register_ie(new natfw_msg()); // the catch-all

	inst->register_ie(new session_lifetime());
	inst->register_ie(new external_address());
	inst->register_ie(new extended_flow_info());
	inst->register_ie(new information_code());
	inst->register_ie(new nonce());
	inst->register_ie(new msg_sequence_number());
	inst->register_ie(new data_terminal_info());
	inst->register_ie(new icmp_types());

	// TODO: implement catch-all

	LogDebug("registered known IEs");
}


IE *NATFW_IEManager::lookup_ie(uint16 category, uint16 type, uint16 subtype) {
	IE *ret = IEManager::lookup_ie(category, type, subtype);

	if ( ret != NULL )
		return ret;

	/*
	 * No IE registered. Return a default IE if possible.
	 */
	switch ( category ) {
	    case cat_natfw_msg:
		return IEManager::lookup_ie(cat_default_natfw_msg, 0, 0);

	    case cat_natfw_object:
		return IEManager::lookup_ie(cat_default_natfw_object, 0, 0);

	    default:
		return NULL;
	}

	// not reached
}


/**
 * Parse a PDU in NetMsg and return it.
 *
 * This method parses a PDU from the given NetMsg and returns a new
 * IE object representing the PDU. Based on the category parameter,
 * the IE can be a NATFW Message or NATFW Object (that is part of a
 * message).
 *
 * All errors encountered during parsing are added to the errorlist object.
 * If no errors occurred, the errorlist will be in the same state as before
 * the call.
 *
 * @param msg a buffer containing the serialized PDU
 * @param category the category the IE belongs to
 * @param coding the protocol version used in msg
 * @param errorlist returns the exceptions caught while parsing the message
 * @param bytes_read returns the number of bytes read from msg
 * @param skip if true, try to ignore errors and continue reading
 * @return the newly created IE, or NULL on error
 */
IE *NATFW_IEManager::deserialize(NetMsg &msg, uint16 category,
		IE::coding_t coding, IEErrorList &errorlist,
		uint32 &bytes_read, bool skip) {

	// Note: The registered objects decide if they support a given coding.

	switch ( category ) {
		case cat_natfw_msg:
			return deserialize_msg(msg, coding,
					errorlist, bytes_read, skip);

		case cat_natfw_object:
			return deserialize_object(msg, coding,
					errorlist, bytes_read, skip);

		default:
			LogError("category " << category << " not supported");

			catch_bad_alloc( errorlist.put(
				new IEError(IEError::ERROR_CATEGORY)) );
			return NULL;
	}

	return NULL;	// not reached
}


/**
 * Use a registered natfw_msg instance to deserialize a NetMsg.
 *
 * Helper method for deserialize(). Parameters work like in deserialize().
 *
 * @param msg a buffer containing the serialized PDU
 * @param coding the protocol version used in msg
 * @param errorlist returns the exceptions caught while parsing the message
 * @param bytes_read returns the number of bytes read from msg
 * @param skip if true, try to ignore errors and continue reading
 * @return the newly created IE, or NULL on error
 */
IE *NATFW_IEManager::deserialize_msg(NetMsg &msg,
		IE::coding_t coding, IEErrorList &errorlist,
		uint32 &bytes_read, bool skip) {

	/*
	 * Peek ahead to find out the message type.
	 */
	uint32 msg_type;
	try {
		uint32 header_raw = msg.decode32(false); // don't move position
		msg_type = natfw_msg::extract_msg_type(header_raw);
	}
	catch ( NetMsgError ) {
		errorlist.put(new IEMsgTooShort(
				coding, cat_natfw_msg, msg.get_pos()) );
		return NULL; // fatal error
	}


	IE *ie = new_instance(cat_natfw_msg, msg_type, 0);

	if ( ie == NULL ) {
		LogError("no natfw_msg registered for ID " << msg_type);
		errorlist.put(new IEError(IEError::ERROR_WRONG_TYPE));
		return NULL;
	}

	// ie returns iteself on success
	IE *ret = ie->deserialize(msg, coding, errorlist, bytes_read, skip);

	if ( ret == NULL )
		delete ie;

	return ret;	// the deserialized object on success, NULL on error
}


/**
 * Use a registered natfw_object instance to deserialize a NetMsg.
 *
 * Helper method for deserialize(). Parameters work like in deserialize().
 *
 * @param msg a buffer containing the serialized PDU
 * @param coding the protocol version used in msg
 * @param errorlist returns the exceptions caught while parsing the message
 * @param bytes_read returns the number of bytes read from msg
 * @param skip if true, try to ignore errors and continue reading
 * @return the newly created IE, or NULL on error
 */
IE *NATFW_IEManager::deserialize_object(NetMsg &msg,
		IE::coding_t coding, IEErrorList &errorlist,
		uint32 &bytes_read, bool skip) {

	/*
	 * Peek ahead to find out the NATFW Object Type.
	 */
	uint32 object_type;
	try {
		uint32 header_raw = msg.decode32(false); // don't move position
		object_type = natfw_object::extract_object_type(header_raw);
	}
	catch ( NetMsgError ) {
		errorlist.put(new IEMsgTooShort(
				coding, cat_natfw_object, msg.get_pos()) );
		return NULL; // fatal error
	}

	IE *ie = new_instance(cat_natfw_object, object_type, 0);

	if ( ie == NULL ) {
		LogError("no natfw_object registered for ID " << object_type);
		errorlist.put(new IEError(IEError::ERROR_WRONG_SUBTYPE));
		return NULL;
	}

	// ie returns iteself on success
	IE *ret = ie->deserialize(msg, coding, errorlist, bytes_read, skip);

	if ( ret == NULL )
		delete ie;

	return ret;	// the deserialized object on success, NULL on error
}


// EOF
