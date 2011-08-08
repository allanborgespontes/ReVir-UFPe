/*
 * qspec_ie.cpp - The QSPEC IE Manager
 *
 * $Id: qspec_ie.cpp 4593 2009-10-20 14:30:51Z roehricht $
 * $HeadURL: https://svn.tm.kit.edu/nsis/qspec/trunk/src/qspec_ie.cpp $
 */
#include "logfile.h"

#include "qspec_ie.h"
#include "qspec_pdu.h"
#include "qspec_param.h"


using namespace qspec;
using namespace protlib::log;



/**
 * This is where our single QSPEC_IEManager is stored.
 */
QSPEC_IEManager *QSPEC_IEManager::qspec_inst = NULL;


/**
 * Constructor for child classes.
 *
 * This constructor has been made protected to only allow instantiation
 * via the static instance() method.
 */
QSPEC_IEManager::QSPEC_IEManager() : IEManager() {
	// nothing to do
}


/**
 * Singleton static factory method. Returns the same QSPEC_IEManager object,
 * no matter how often it is called.
 *
 * Note: Calling clear() causes the object to be deleted and the next call
 * to instance() create a new QSPEC_IEManager object.
 *
 * @return always the same QSPEC_IEManager object
 */
QSPEC_IEManager *QSPEC_IEManager::instance() {

	// Instance already exists, so return it.
	if ( qspec_inst )
		return qspec_inst;

	// We don't have an instance yet. Create it.
	try {
		qspec_inst = new QSPEC_IEManager();
		Log(DEBUG_LOG, LOG_NORMAL, "QSPEC_IEManager",
			"Created QSPEC_IEManager singleton: " << qspec_inst);
	}
	catch ( bad_alloc ) {
		Log(ERROR_LOG, LOG_CRIT, "QSPEC_IEManager",
				"Cannot create QSPEC_IEManager singleton");
		throw IEError(IEError::ERROR_NO_IEMANAGER);
	}

	return qspec_inst;
}


/**
 * Delete the singleton instance.
 *
 * After calling this, all pointers to or into the object are invalid.
 * The instance() method has to be called before using the QSPEC_IEManager
 * next time.
 */
void QSPEC_IEManager::clear() {
	delete qspec_inst;
	qspec_inst = 0;
	Log(DEBUG_LOG, LOG_NORMAL, "QSPEC_IEManager",
		"Deleted QSPEC_IEManager singleton");
}


/**
 * Register all known IEs.
 *
 * This method clears the registry and then registers all IEs known to this
 * implementation. It solely exists for convenience.
 * Parameter ID 1: <TMOD-1>
 *              2: <TMOD-2>
 *              3: <Path Latency>
 *              4: <Path Jitter>
 *              5: <Path PLR>
 *              6: <Path PER>
 *              7: <Slack Term>
 *              8: <Preemption Priority> & <Defending Priority>
 *              9: <Admission Priority>
 *              10: <RPH Priority>
 *              11: <Excess Treatment>
 *              12: <PHB Class>
 *              13: <DSTE Class Type>
 *              14: <Y.1541 QoS Class>
 */
void QSPEC_IEManager::register_known_ies() {
	clear();

	instance()->register_ie(new qspec_pdu);
	instance()->register_ie(new qspec_object);
	instance()->register_ie(new t_mod);
	instance()->register_ie(new dste_class_type);
	instance()->register_ie(new excess_treatment);
	instance()->register_ie(new path_jitter);
	instance()->register_ie(new path_latency);
	instance()->register_ie(new path_per);
	instance()->register_ie(new path_plr);
	instance()->register_ie(new phb_class);
	instance()->register_ie(new preemption_priority);
	instance()->register_ie(new qspec_param_raw);
	instance()->register_ie(new admission_priority);
	instance()->register_ie(new rph_priority);
	instance()->register_ie(new slack_term);
	instance()->register_ie(new y1541_qos_class);
}


IE *QSPEC_IEManager::lookup_ie(uint16 category, uint16 type, uint16 subtype) {
	IE *ret = IEManager::lookup_ie(category, type, subtype);

	if ( ret != NULL )
		return ret;

	/*
	 * No IE registered. Return a default IE if possible.
	 */
	switch ( category ) {
	    case cat_qspec_pdu:
		return IEManager::lookup_ie(cat_default_qspec_pdu, 0, 0);

	    case cat_qspec_object:
		return IEManager::lookup_ie(cat_default_qspec_object, 0, 0);

	    case cat_qspec_param:
		return IEManager::lookup_ie(cat_default_qspec_param, 0, 0);

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
 * the IE can be a toplevel PDU or an object that is part of another
 * PDU.
 *
 * All errors encountered during parsing are added to the errorlist object.
 * If no errors occurred the errorlist will be in the same state as before
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
IE *QSPEC_IEManager::deserialize(NetMsg &msg, uint16 category,
		IE::coding_t coding, IEErrorList &errorlist,
		uint32 &bytes_read, bool skip) {

	// Note: The registered objects decide if they support a given coding.

	switch ( category ) {
		case cat_qspec_pdu:
			return deserialize_qspec_pdu(msg, coding,
					errorlist, bytes_read, skip);

		case cat_qspec_object:
			return deserialize_qspec_object(msg, coding,
					errorlist, bytes_read, skip);

		case cat_qspec_param:
			return deserialize_qspec_param(msg, coding,
					errorlist, bytes_read, skip);

		default:
			Log(ERROR_LOG, LOG_NORMAL, "IE", 
				"Category " << category << " not supported");

			catch_bad_alloc( errorlist.put(
				new IEError(IEError::ERROR_CATEGORY)) );
			return NULL;
	}

	return NULL;	// not reached
}


/**
 * Use a registered object to deserialize msg.
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
IE *QSPEC_IEManager::deserialize_qspec_pdu(NetMsg &msg,
		IE::coding_t coding, IEErrorList &errorlist,
		uint32 &bytes_read, bool skip) {

	/*
	 * Peek ahead to find out the QOSM ID.
	 */
	uint32 qspec_type;
	try {
		uint32 header_raw = msg.decode32(false); // don't move position
		qspec_type = qspec_pdu::extract_qspec_type(header_raw);
	}
	catch ( NetMsgError ) {
		errorlist.put(new IEMsgTooShort(
				coding, cat_qspec_pdu, msg.get_pos()) );
		return NULL; // fatal error
	}


	IE *ie = new_instance(cat_qspec_pdu, qspec_type, 0);

	if ( ie == NULL ) {
		Log(ERROR_LOG, LOG_CRIT, "QSPEC_IEManager",
			"no qspec_pdu registered");
		return NULL;
	}

	// ie returns iteself on success
	IE *ret = ie->deserialize(msg, coding, errorlist, bytes_read, skip);

	if ( ret == NULL )
		delete ie;

	return ret;	// the deserialized object on success, NULL on error
}


/**
 * Use a registered qspec_object to deserialize msg.
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
IE *QSPEC_IEManager::deserialize_qspec_object(NetMsg &msg,
		IE::coding_t coding, IEErrorList &errorlist,
		uint32 &bytes_read, bool skip) {

	/*
	 * Peek ahead to find out the QSPEC Object Type.
	 */
	uint32 object_type;
	try {
		uint32 header_raw = msg.decode32(false); // don't move position
		object_type = qspec_object::extract_object_type(header_raw);
	}
	catch ( NetMsgError ) {
		errorlist.put(new IEMsgTooShort(
				coding, cat_qspec_pdu, msg.get_pos()) );
		return NULL; // fatal error
	}

	IE *ie = new_instance(cat_qspec_object, object_type, 0);

	if ( ie == NULL ) {
		Log(ERROR_LOG, LOG_CRIT, "QSPEC_IEManager",
			"no qspec_object registered");
		return NULL;
	}

	// ie returns iteself on success
	IE *ret = ie->deserialize(msg, coding, errorlist, bytes_read, skip);

	if ( ret == NULL )
		delete ie;

	return ret;	// the deserialized object on success, NULL on error
}


/**
 * Parse a QSPEC parameter.
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
IE *QSPEC_IEManager::deserialize_qspec_param(NetMsg &msg,
		IE::coding_t coding, IEErrorList &errorlist,
		uint32 &bytes_read, bool skip) {

	/*
	 * Peek ahead to find out the parameter id.
	 */
	uint32 parameter_id;
	try {
		uint32 header_raw = msg.decode32(false); // don't move position
		parameter_id = qspec_param::extract_parameter_id(header_raw);
	}
	catch ( NetMsgError ) {
		errorlist.put(new IEMsgTooShort(
				coding, cat_qspec_param, msg.get_pos()) );
		return NULL; // fatal error
	}


	IE *ie = new_instance(cat_qspec_param, parameter_id, 0);

	if ( ie == NULL ) {
		Log(ERROR_LOG, LOG_CRIT, "QSPEC_IEManager",
			"unregistered parameter id: " << parameter_id);
		catch_bad_alloc( errorlist.put(new IEWrongType(
			coding, cat_qspec_param, msg.get_pos())) );
		return NULL;
	}

	IE *ret = ie->deserialize(msg, coding, errorlist, bytes_read, skip);

	if ( ret == NULL )
		delete ie;

	return ret; // either an initialized IE or NULL on error
}


// EOF
