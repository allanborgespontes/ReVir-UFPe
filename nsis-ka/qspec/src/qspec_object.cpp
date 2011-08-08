/*
 * qspec_object.cpp - QSPEC Object Class (QoS Desired, QoS Reserved etc.)
 *
 * $Id: qspec_object.cpp 4593 2009-10-20 14:30:51Z roehricht $
 * $HeadURL: https://svn.tm.kit.edu/nsis/qspec/trunk/src/qspec_object.cpp $
 */
#include <iomanip>	// for setw()

#include "logfile.h"

#include "qspec_ie.h"
#include "qspec_object.h"


using namespace qspec;
using namespace protlib::log;


/**
 * Length of a QSPEC Object header in bytes.
 */
const uint16 qspec_object::HEADER_LENGTH = 4;

const char *const qspec_object::ie_name = "qspec_object";


/**
 * Standard constructor.
 *
 * The object type defaults to ot_control_information.
 */
qspec_object::qspec_object()
		: IE(cat_qspec_object), object_id(ot_qos_desired),
		  read_only(false), error(false) {

	// nothing to do
}


/**
 * Constructor for manual use.
 *
 * Only basic initialization is done. The set of parameters is empty.
 * The obj_type parameter must not be larger than 12 bit and an even
 * smaller subset is valid according to QSPEC-T. Use object_type_t
 * for this parameter to get valid IDs.
 *
 * @param obj_type the QSPEC Object Type (12 bit)
 */
qspec_object::qspec_object(uint16 obj_type)
		: IE(cat_qspec_object), object_id(obj_type),
		  read_only(false), error(false) {
  // set read-only status
  set_read_only();
  // nothing to do
}


/**
 * Copy constructor.
 *
 * Makes a deep copy of the object passed as an argument.
 *
 * @param other the object to copy
 */
qspec_object::qspec_object(const qspec_object &other)
		: IE(cat_qspec_object), object_id(other.get_object_id()),
		  read_only(other.is_read_only()), error(other.has_error()),
		  parameters(other.parameters) {

	// nothing else to do
}


/**
 * Destructor.
 *
 * All parameters that this object contains are deleted.
 */
qspec_object::~qspec_object() {
	// Nothing to do, parameters are deleted by ie_store's destructor.
}


/**
 * Sets read-only attribute depending on type
 * read_only is set depending on obj_type: currently defined as
 * read-only parameters: 
 * - QoS Desired
 * - QoS Available in a RESPONSE message
 * - QoS Reserved
 * - Minimum QoS
 * - <Traffic Handling Directives> in s RESPONSE message
 */
void
qspec_object::set_read_only() {
  switch (object_id)
  {
    case ot_qos_desired:
    case ot_qos_reserved:
    case ot_minimum_qos:
      read_only= true;
      break;
  };
}

qspec_object *qspec_object::new_instance() const {
	qspec_object *q = NULL;
	catch_bad_alloc( q = new qspec_object() );
	return q;
}


qspec_object *qspec_object::copy() const {
	qspec_object *q = NULL;
	catch_bad_alloc( q = new qspec_object(*this) );
	return q;
}


/**
 * Extract the object type.
 *
 * Extract the object type from a raw header. The header is expected to be
 * in host byte order already.
 *
 * @param header_raw 32 bits read from a NetMsg
 * @return the object type
 */
uint16 qspec_object::extract_object_type(uint32 header_raw) throw () {
	return ( (header_raw >> 16) & 0xFFF );
}


IE *qspec_object::deserialize(NetMsg &msg, coding_t coding,
		IEErrorList &errorlist, uint32 &bytes_read, bool skip) {

	bytes_read = 0;

	uint32 start_pos = msg.get_pos();

	/*
	 * Parse the object header.
	 */
	uint32 header_raw;

	try {
		header_raw = msg.decode32();
	}
	catch ( NetMsgError ) {
		catch_bad_alloc( errorlist.put(
			new IEMsgTooShort(coding, get_category(), start_pos)) );

		return ( skip ? this : NULL ); // We can't go on from here.
	}

	uint32 flags = header_raw >> 28;
	set_error( flags & 8 );

	set_object_id( extract_object_type(header_raw) );

	set_read_only();

	uint16 body_length = header_raw & 0xFFF; // 12 bit length, given in 32bit words!


	bytes_read += 4;


	// Error: Message is shorter than the length field makes us believe.
	if ( msg.get_bytes_left() < 4u * body_length ) {
		catch_bad_alloc( errorlist.put(
			new IEMsgTooShort(coding, category, start_pos)) );

		msg.set_pos(start_pos);
		return NULL;
	}


	/*
	 * Now parse the object body.
	 */
	IEManager *mgr = QSPEC_IEManager::instance();

	while ( bytes_read < body_length*4u ) {
		uint32 saved_pos = msg.get_pos();
		uint32 param_bytes_read;

		IE *ie = mgr->deserialize(msg, cat_qspec_param, coding,
				errorlist, param_bytes_read, skip);

		// Error: no qspec_param registered.
		if ( ie == NULL ) {
			Log(ERROR_LOG, LOG_CRIT, "qspec_object",
				"no such parameter registered");
			return NULL;
		}

		qspec_param *param = dynamic_cast<qspec_param *>(ie);

		// Error: cast failed, deserialize() returned no qspec_param
		if ( param == NULL ) {
			Log(ERROR_LOG, LOG_CRIT, "qspec_object",
				"deserialize() returned no qspec_param");
			delete ie;
			return NULL;
		}

		// test for duplicate object
		if ( get_parameter( param->get_parameter_id() ) != NULL ) {
			catch_bad_alloc( errorlist.put(
				new PDUSyntaxError(coding, get_category(),
					param->get_parameter_id(), 0,
					saved_pos, "Duplicate parameter")) );

			if ( ! skip )
				return NULL;
		}

		// Success: parameter was parsed successfully.
		set_parameter(param);

		bytes_read += param_bytes_read;
	}

	// Empty objects are not allowed.
	if ( get_num_parameters() == 0 ) {
		catch_bad_alloc( errorlist.put(
			new PDUSyntaxError(coding, get_category(), 0, 0,
				start_pos, "Object may not be empty")) );

		if ( ! skip )
			return NULL;
	}


	// this would be an implementation error
	if ( bytes_read != msg.get_pos() - start_pos )
		Log(ERROR_LOG, LOG_CRIT, "qspec_object",
				"deserialize(): byte count mismatch");

	return this;
}


void qspec_object::serialize(NetMsg &msg, coding_t coding,
		uint32 &bytes_written) const throw (IEError) {

	bytes_written = 0;
	uint32 start_pos = msg.get_pos();

	// Check if the object is in a valid state. Throw an exception if not.
	uint32 tmp;
	check_ser_args(coding, tmp);


	/*
	 * Write object header.
	 *    0                   1                   2                   3
	 *    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
	 *   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	 *   |E|r|r|r|       Object Type     |r|r|r|r|         Length        |
	 *   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	 *
	 *   E Flag: Set if an error occurs on object level
	 *   Object Type = 0: QoS Desired (parameters cannot be overwritten)
	 *               = 1: QoS Available (parameters may be overwritten; see
	 *                    Section 3.3)
	 *               = 2: QoS Reserved (parameters cannot be overwritten)
	 *               = 3: Minimum QoS (parameters cannot be overwritten)
	 *
	 *   The r bits are reserved.
	 *
	 */
	uint8 flags =  (has_error() << 3);
	uint16 obj_id = get_object_id();
	uint16 length = (get_serialized_size(coding)-4) / 4;
	if ( (length & 0xfff) != length )
	{
	  ERRLog("qspec_object","object length does not fit into 12 bit field");
	  throw IEWrongLength(coding, get_category(), get_object_id(), 0, 3U);
	}

	uint32 header_raw = (flags << 28) | (obj_id << 16) | length;

	try {
		msg.encode32(header_raw);
		bytes_written += 4;
	}
	catch (NetMsgError) {
		throw IEMsgTooShort(coding, get_category(), msg.get_pos());
	}


	/*
	 * Write object body.
	 */
	for ( param_iter i = parameters.begin(); i != parameters.end(); i++ ) {
		const IE *param = i->second;

		uint32 param_bytes_written;
		param->serialize(msg, coding, param_bytes_written);

		bytes_written += param_bytes_written;
	}

	// this would be an implementation error
	if ( bytes_written != msg.get_pos() - start_pos )
		ERRCLog("qspec_object", "serialize(): byte count mismatch");
}


/**
 * Checks if this object is valid.
 *
 * It is not possible to serialize() an empty object.
 */
bool qspec_object::check() const {

	// Error: invalid object ID or no parameters available
	if ( (get_object_id() & ~0xFFF) != 0 || parameters.size() == 0 )
		return false;

	// If one parameter is invalid, the whole object is invalid.
	for ( param_iter i = parameters.begin(); i != parameters.end(); i++ ) {
		const IE *param = i->second;

		if ( param->check() == false )
		{
		  ERRCLog("qspec_object", "check(): failed for " << param->get_ie_name());
		  return false;
		}
	}

	return true; // no error found
}


bool qspec_object::supports_coding(coding_t c) const {
	return c == protocol_v1;
}


size_t qspec_object::get_serialized_size(coding_t coding) const {
	size_t size = HEADER_LENGTH;

	for ( param_iter i = parameters.begin(); i != parameters.end(); i++ )
		size += i->second->get_serialized_size(coding);

	return size;
}


bool qspec_object::operator==(const IE &ie) const {

	const qspec_object *other = dynamic_cast<const qspec_object *>(&ie);

	if ( other == NULL || get_object_id() != other->get_object_id()
			|| is_read_only() != other->is_read_only()
			|| has_error() != other->has_error() )
		return false;

	// Return true iff all parameters are equal, too.
	return ( parameters == other->parameters );
}


const char *qspec_object::get_ie_name() const {
	return ie_name;
}


ostream &qspec_object::print(ostream &os, uint32 level, const uint32 indent,
		const char *name) const {

	os << setw(level*indent) << "";

	if ( name )
	  os << name << ( is_read_only() ? " (RO)" : " (RW)" ) << " = ";

	os << "[" << get_ie_name() << ": ";

	os << "id=" << get_object_id() << ", ";
	os << "flags='"	<< ( has_error() ? "E" : "" ) << "'";

	os << ", parameters={" << endl;

	for ( param_iter i = parameters.begin(); i != parameters.end(); i++ )
		i->second->print(os, level+1, indent) << endl;

	os << setw(level*indent) << "" << "}]";

	return os;
}


void qspec_object::register_ie(IEManager *iem) const {
	iem->register_ie(cat_default_qspec_object, 0, 0, this);
}


/**
 * Returns the object ID (aka Object Type).
 *
 * @return the object ID (aka Object Type)
 */
uint16 qspec_object::get_object_id() const {
	return object_id;
}


/**
 * Set the object ID (aka Object Type).
 *
 * Note that only 12 bit values are possible with QSPEC-T limiting the number
 * of valid IDs even more. Use object_type_t for symbolic names of valid IDs.
 *
 * @param id the object ID (12 bit)
 */
void qspec_object::set_object_id(uint16 id) {
	object_id = id;
}


/**
 * Returns the read-only flag.
 *
 * @return true if this object is read-only
 */
bool qspec_object::is_read_only() const {
	return read_only;
}


/**
 * Returns the error flag.
 *
 * @return true if there was an error
 */
bool qspec_object::has_error() const {
	return error;
}


/**
 * Set the error flag.
 *
 * @param err set to true if there was an error
 */
void qspec_object::set_error(bool err) {
	error = err;
}


/**
 * Get a parameter by ID.
 *
 * @param id the parameter ID (12 bit)
 * @return the parameter if it exists or NULL if it doesn't
 */
qspec_param *qspec_object::get_parameter(uint16 id) const {
	return dynamic_cast<qspec_param *>( parameters.get(id) );
}


/**
 * Get the number of parameters this qspec_object contains.
 *
 * @return the number of parameters
 */
size_t qspec_object::get_num_parameters() const {
	return parameters.size();
}


/**
 * Add a parameter to this object.
 *
 * If there was already a parameter registered by that ID, the old parameter
 * is deleted and the new one added instead.
 *
 * The parameter ID of param MUST NOT be changed after passing a qspec_param
 * to set_parameter()!
 *
 * Note: All parameters are deleted by qspec_object's destructor.
 *
 * @param param the qspec_param to add
 */
void qspec_object::set_parameter(qspec_param *param) {
	parameters.set(param->get_parameter_id(), param);
}


/**
 * Remove a parameter.
 *
 * Remove the parameter with ID from this qspec_object. If there is no
 * parameter with that ID, NULL is returned.
 *
 * The parameter is *not* deleted.
 *
 * @param id the parameter ID (12 bit)
 * @return the parameter with that ID or NULL if there is none
 */
qspec_param *qspec_object::remove_parameter(uint16 id) {
	return dynamic_cast<qspec_param *>( parameters.remove(id) );
}


// EOF
