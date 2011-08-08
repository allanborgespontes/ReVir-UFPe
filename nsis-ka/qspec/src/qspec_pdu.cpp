/*
 * qspec_pdu.cpp - Implementation of the QSPEC Template PDU
 *
 * $Id: qspec_pdu.cpp 5206 2010-03-29 09:14:04Z bless $
 * $HeadURL: https://svn.tm.kit.edu/nsis/qspec/trunk/src/qspec_pdu.cpp $
 *
 */
#include <iomanip>	// for setw()

#include "logfile.h"

#include "qspec_ie.h"
#include "qspec_pdu.h"


using namespace qspec;
using namespace protlib::log;


/**
 * Length of a QSPEC header in bytes.
 */
const uint16 qspec_pdu::HEADER_LENGTH = 4;


/**
 * The QSPEC Template Version this class implements.
 */
const uint8 qspec_pdu::QSPEC_VERSION = 0;


const char *const qspec_pdu::ie_name = "qspec_pdu";


/**
 * Standard constructor.
 *
 * Creates an empty QSPEC PDU.
 */
qspec_pdu::qspec_pdu()
		: IE(cat_qspec_pdu), version(qspec_pdu::QSPEC_VERSION), qspec_type(0),
		  msg_sequence(0), obj_combination(0), local_QSPEC(false) {

	// nothing to do
}


/**
 * Constructor for manual use.
 *
 * Only basic initialization is done. No QSPEC objects (QoS Desired etc.)
 * exist yet. All other attributes are set to default values.
 * The qspec_type parameter must not be larger than 5 bit and is assigned by
 * IANA.
 *
 * Valid values for msg_seq can be obtained using msg_sequence_t. The values
 * for obj_comb depend on the msg_seq parameter.
 *
 * @param msg_seq message sequence (4 bit)
 * @param obj_comb object combination (4 bit)
 * @param qspec_type Identifies the particular type of QSPEC (5 bit), 0: Default
 * @param localQSPEC flag: local QSPEC (true) or initiator QSPEC (false), default=initiator QSPEC
 */
qspec_pdu::qspec_pdu(qspec::msg_sequence_t msg_seq, uint8 obj_comb, uint8 qspec_type, bool localQSPEC)
		: IE(cat_qspec_pdu), version(qspec_pdu::QSPEC_VERSION), qspec_type(qspec_type),
		  msg_sequence(msg_seq), obj_combination(obj_comb), local_QSPEC(localQSPEC) {

	// nothing to do
}


/**
 * Copy constructor.
 *
 * Makes a deep copy of the object passed as an argument.
 *
 * @param other the object to copy
 */
qspec_pdu::qspec_pdu(const qspec_pdu &other)
		: IE(other.category), version(other.get_version()),
		  qspec_type(other.get_qspec_type()), 
		  msg_sequence(other.get_msg_sequence()),
		  obj_combination(other.get_obj_combination()),
		  local_QSPEC(other.local_QSPEC),
		  objects(other.objects) {
	// nothing else to do
}


/**
 * Destructor.
 *
 * Deletes all objects this QSPEC contains.
 */
qspec_pdu::~qspec_pdu() {
	// Nothing to do, objects are deleted by ie_store's destructor.
}


qspec_pdu *qspec_pdu::new_instance() const {
	qspec_pdu *qp = NULL;
	catch_bad_alloc(qp = new qspec_pdu());
	return qp;
}


qspec_pdu *qspec_pdu::copy() const {
	qspec_pdu *qp = NULL;
	catch_bad_alloc(qp = new qspec_pdu(*this));
	return qp;
}


/**
 * Extract the QSPEC type.
 *
 * Extract the QSPEC type from a raw header. The header
 * is expected to be in host byte order already according to
 *
 *  0                   1                   2                   3
 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |  Vers.|I|QSPECType|r|r|  QSPEC Proc.  |        Length         |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 * @param header_raw 32 bits from a NetMsg
 * @return the QSPEC type.
 */
uint16 qspec_pdu::extract_qspec_type(uint32 header_raw) throw () {
	return ((header_raw >> 22) & 0x1F);
}


/**
 * Initialize the object from the given NetMsg.
 *
 * The object returns itself if the deserialization was successful. No
 * new object is created.
 *
 * Note: QSPEC Template doesn't contain a length field for the whole
 * PDU. The length is only known to the layer below. Because of this it
 * is very important that NetMsg is in a state where get_bytes_left()
 * returns the length of the QSPEC object before calling deserialize().
 *
 * @return this object on success, NULL on error
 */
qspec_pdu *qspec_pdu::deserialize(NetMsg &msg, coding_t coding,
		IEErrorList &errorlist, uint32 &bytes_read, bool skip) {

	bytes_read = 0;
	uint32 start_pos = msg.get_pos();

	/*
	 * Parse the QSPEC header
	 *   0                   1                   2                   3
	 *   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
	 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	 *  |  Vers.|I|QSPECType|r|r|  QSPEC Proc.  |        Length         |
	 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	 */
	uint32 header_raw;

	try {
		header_raw = msg.decode32();
	} catch (NetMsgError) {
		catch_bad_alloc( errorlist.put(
			new IEMsgTooShort(coding, get_category(), start_pos)));
		return (skip ? this : NULL); // We can't go on from here
	}

	set_version(header_raw >> 28);

	set_qspec_type(extract_qspec_type(header_raw));

	/*
	 * QSPEC procedure is split into
	 *  0 1 2 3 4 5 6 7
	 * +-+-+-+-+-+-+-+-+
	 * |Mes.Sq |Obj.Cmb|
	 * +-+-+-+-+-+-+-+-+
	 */
	set_msg_sequence((header_raw >> 16) & 0xF);
	set_obj_combination((header_raw >> 12) & 0xF);

	// check for I flag
	local_QSPEC = (header_raw & (1 << 27)) ? true : false;

	// length in units of 32-bit words excluding the common header
	uint16 qspec_total_len = header_raw & 0xFFF;
	uint32 qspec_total_len_bytes = 4 * qspec_total_len;

	// TODO: check msg_sequence and obj_combination

	bytes_read += 4;


	/* 
	 * Read as many objects from the body as possible
	 */
	IEManager *mgr = QSPEC_IEManager::instance();
	uint32 obj_bytes_read = 0;
	uint32 saved_pos = msg.get_pos();
	while ((msg.get_bytes_left() > 0) && (qspec_total_len_bytes > 0)) {
		saved_pos = msg.get_pos();

		IE *ie = mgr->deserialize(msg, cat_qspec_object, coding,
				errorlist, obj_bytes_read, skip);

		// Deserializing failed
		if (ie == NULL)
			return NULL;

		qspec_object *obj = dynamic_cast<qspec_object *>(ie);

		/*
		 * Error: no qspec_object returned
		 */
		if (obj == NULL) {
			ERRCLog("qspec_pdu", "deserialize() returned object of type "
				<< ie->get_ie_name()
				<< " but type qspec_object expected. "
				<< "Registration invalid?");
			delete ie;
			return NULL;
		}

		// test for duplicate object
		if (get_object( obj->get_object_id() ) != NULL) {
			catch_bad_alloc(errorlist.put(
				new PDUSyntaxError(coding, get_category(),
					obj->get_object_id(), 0,
					saved_pos, "Duplicate object")));

			if (!skip)
				return NULL;
		}

		bytes_read += obj_bytes_read;
		qspec_total_len_bytes -= obj_bytes_read;

		set_object(obj);
	} // end while

	if (qspec_total_len_bytes != 0)
	  ERRCLog("qspec_pdu", "deserialize(): qspec length mismatch, bytes left=" << qspec_total_len_bytes 
		  << ", total length from PDU=" << 4 * qspec_total_len);

	// empty QSPECs are not allowed
	if (get_num_objects() == 0) {
		catch_bad_alloc(errorlist.put(
			new PDUSyntaxError(coding, get_category(), 0, 0,
				start_pos, "QSPEC may not be empty")));

		if (!skip)
			return NULL;
	}

	// this would be an implementation error
	if (bytes_read != msg.get_pos() - start_pos)
	  ERRCLog("qspec_pdu", "deserialize(): byte count mismatch, bytes_read=" 
			  << bytes_read << " buffer pos read:" 
			  << msg.get_pos() - start_pos);

	return this; // no error, return the initialized object
}


/**
 * 
 * Serializing into a non-empty NetMsg object is supported. The NetMsg
 * has to be big enough and its position pointer has to be set to the
 * desired position. At the end of the method, the position pointer
 * points to the position after the last written byte.
 *
 * If serialize() throws an exception, both NetMsg and bytes_written are left
 * in an undefined state.
 *
 * @param msg a NetMsg that is large enough
 * @param coding the encoding
 * @param bytes_written returns the number of written bytes
 */
void qspec_pdu::serialize(NetMsg &msg, coding_t coding,
		uint32 &bytes_written) const throw (IEError) {


	bytes_written = 0;
	uint32 start_pos = msg.get_pos();

	// Check if the object is in a valid state. Throw an exception if not.
	uint32 tmp;
	check_ser_args(coding, tmp);

	/* 
	 * Write QSPEC header
	 *    0                   1                   2                   3
	 *    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
	 *   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	 *   |  Vers.|I|QSPECType|r|r|  QSPEC Proc.  |        Length         |
	 *   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	 *                           +-+-+-+-+-+-+-+-+
	 * QSPEC procedure           |Mes.Sq |Obj.Cmb|
	 *                           +-+-+-+-+-+-+-+-+
	 */
	uint32 header_raw = (get_version() << 28) | ((get_qspec_type() & 0x1F) << 22)
		| (get_msg_sequence() << 16) | (get_obj_combination() << 12)
	        | (is_local_qspec() << 27);

	// encode qspec pdu length as number of 32-bit words, excluding the common header
	uint32 qspec_pdu_length = get_serialized_size(protocol_v1);
	// substract header length
	if (qspec_pdu_length >= qspec_pdu::HEADER_LENGTH)
		qspec_pdu_length -= qspec_pdu::HEADER_LENGTH;
	// encode length
	if ((qspec_pdu_length / 4) > 0xFFF) 
		ERRCLog("qspec_pdu", "serialize(): length too large for 12 bit " 
				<< qspec_pdu_length / 4);

	header_raw |= ((qspec_pdu_length / 4) & 0xFFF);

	try {
		msg.encode32(header_raw);
		bytes_written += 4;
	} catch (NetMsgError) {
		throw IEMsgTooShort(coding, get_category(), msg.get_pos());
	}


	/*
	 * Write the body: Serialize each object
	 */
	for (obj_iter i = objects.begin(); i != objects.end(); i++) {
		const IE *obj = i->second;

		uint32 obj_bytes_written = 0;
		obj->serialize(msg, coding, obj_bytes_written);

		bytes_written += obj_bytes_written;
	}

	// this would be an implementation error
	if (bytes_written != msg.get_pos() - start_pos)
	  ERRCLog("qspec_pdu", "serialize(): byte count mismatch, bytes_written=" 
			  << bytes_written << " buffer pos written=" 
			  << msg.get_pos() - start_pos);
}


// TODO: write more sophisticated tests than this one
bool qspec_pdu::check() const {
	
	// Error: no objects available or invalid IDs
	if (objects.size() == 0 || (get_qspec_type() & ~0xF) != 0
			|| (get_msg_sequence() & ~0xF) != 0
			|| (get_obj_combination() & ~0xF) != 0)
		return false;

	// Check all objects for errors.
	for (obj_iter i = objects.begin(); i != objects.end(); i++) {
		const IE *obj = i->second;

		if (obj->check() == false)
			return false;
	}

	return true; // no error found
}


bool qspec_pdu::supports_coding(coding_t c) const {
	return c == protocol_v1;
}


/**
 * Calculate the serialized size of this PDU, including the header.
 * The size may depend on the selected encoding.
 *
 * @param coding the encoding
 * @return the size in bytes, including the header
 */
size_t qspec_pdu::get_serialized_size(coding_t coding) const {
	size_t size = HEADER_LENGTH;


	for ( obj_iter i = objects.begin(); i != objects.end(); i++ ) {
		const IE *obj = i->second;

		size += obj->get_serialized_size(coding);
	}

	return size;
}


bool qspec_pdu::operator==(const IE &ie) const {

	const qspec_pdu *p = dynamic_cast<const qspec_pdu *>(&ie);

	if ( p == NULL || get_version() != p->get_version()
			|| get_qspec_type() != p->get_qspec_type()
			|| get_msg_sequence() != p->get_msg_sequence()
			|| get_obj_combination() != p->get_obj_combination()
	                || is_local_qspec() != p->is_local_qspec()
	    )
		return false;

	// Return true iff all objects are equal, too.
	return ( objects == p->objects );
}


const char *qspec_pdu::get_ie_name() const {
	return ie_name;
}


ostream &qspec_pdu::print(ostream &os, uint32 level, const uint32 indent,
		const char *name) const {

	os << setw(level*indent) << "";

	if ( name )
		os << name << " = ";

	os << "[" << get_ie_name() << ": ";

	os << "version=" << (int) get_version() << ", ";
	os << "qspec_type=" << (int) get_qspec_type() << ", ";
	os << "msg_seq=" << (int)get_msg_sequence() << ", ";
os << "obj_comb=" << (int) get_obj_combination() << ", ";
	os << (is_local_qspec() ? "local QSPEC" : "initiator QSPEC") << ", ";

	os << "objects={" << endl;

	for ( obj_iter i = objects.begin(); i != objects.end(); i++ ) {
		i->second->print(os, level+1, indent) << endl;
	}

	os << setw(level*indent) << "" << "}]";

	return os;
}


void qspec_pdu::register_ie(IEManager *iem) const {
	iem->register_ie(cat_default_qspec_pdu, 0, 0, this);
}


/**
 * Returns the QSPEC Template version.
 *
 * @return the QSPEC Template version
 */
uint8 qspec_pdu::get_version() const {
	return version;
}


/**
 * Set the QSPEC Template version.
 *
 * @param ver the QSPEC Template version (4 bit)
 */
void qspec_pdu::set_version(uint8 ver) {
	version = ver;
}


/**
 * Returns the QSPEC type ID. (5 bit)
 *
 * @return the QSPEC type ID.
 */
uint8 qspec_pdu::get_qspec_type() const {
	return qspec_type;
}


/**
 * Set the QSPEC type ID.
 *
 * @param id the QSPEC type ID (5 bit)
 */
void qspec_pdu::set_qspec_type(uint8 id) {
	qspec_type = id;
}


/**
 * Returns the Message Sequence field.
 *
 * The Message Sequence field is the first part of the QSPEC Procedure
 * Identifier.
 *
 * @return the Message Sequence field.
 */
uint8 qspec_pdu::get_msg_sequence() const {
	return msg_sequence;
}


/**
 * Sets the Message Sequence field.
 *
 * @param seq the Message Sequence field (4 bit)
 */
void qspec_pdu::set_msg_sequence(uint8 seq) {
	msg_sequence = seq;
}


/**
 * Returns the Object Combination field.
 *
 * The Object Combination field depends on the Message Sequence and is the
 * second part of the QSPEC Procedure Identifier.
 *
 * @return the Object Combination field
 */
uint8 qspec_pdu::get_obj_combination() const {
	return obj_combination;
}


/**
 * Set the Object Combination field.
 *
 * @param comb the Object Combination field (4 bit)
 */
void qspec_pdu::set_obj_combination(uint8 comb) {
	obj_combination = comb;
}


/**
 * Return the number of contained objects.
 *
 * @return the number of objects this QSPEC contains
 */
size_t qspec_pdu::get_num_objects() const {
	return objects.size();
}


/**
 * Returns the QSPEC object registered for a given ID.
 *
 * The object is not removed from the PDU.
 *
 * @param the object ID (12 bit)
 * @return the QSPEC object or NULL, if none is registered for that ID
 */
qspec_object *qspec_pdu::get_object(uint16 object_id) const {
	return dynamic_cast<qspec_object *>( objects.get(object_id) );
}


/**
 * Add an object to this QSPEC.
 *
 * If there is already an object registered with the same object ID
 * the old one is deleted. Note that qspec_pdu's destructor deletes
 * all registered objects.
 *
 * @param obj the object to add
 */
void qspec_pdu::set_object(qspec_object *obj) {
	objects.set(obj->get_object_id(), obj);
}


/**
 * Remove a QSPEC object.
 *
 * Remove the object with the given ID from this qspec_pdu. If there is no
 * object with that ID, NULL is returned.
 *
 * The object is *not* deleted.
 *
 * @param id the object ID (12 bit)
 * @return the object with that ID or NULL if there is none
 */
qspec_object *qspec_pdu::remove_object(uint16 object_id) {
	return dynamic_cast<qspec_object *>( objects.remove(object_id) );
}


// EOF
