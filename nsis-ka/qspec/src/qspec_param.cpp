/*
 * qspec_param.cpp - Implementation of the abstract qspec_param class
 *
 * $Id: qspec_param.cpp 4593 2009-10-20 14:30:51Z roehricht $
 * $HeadURL: https://svn.tm.kit.edu/nsis/qspec/trunk/src/qspec_param.cpp $
 *
 * Unresolved issues:
 *   - exception handling for serialize_header()
 *   - "skip" parameter for deserialize_header()
 */
#include <iomanip>	// for setw()
#include <cstring>
#include <limits>

#include "logfile.h"

#include "qspec_ie.h"
#include "qspec_param.h"


using namespace qspec;
using namespace protlib::log;


/*
 * Utility functions
 */

/**
 * Write a float (32-bit IEEE) into the given NetMsg.
 *
 * This function relies on the caller to check that the passed value is
 * positive, not infinity or NaN. It is made sure, however, that no negative
 * zero representation (-0.0) is created.
 *
 * @param msg the NetMsg to write to
 * @param f the value to write
 */
void qspec::encode(NetMsg &msg, float f) {
	union converter_t conv;
	conv.val_float = f;

	// Turn off the MSB, this makes the float positive, too.
	conv.val_uint = ( ~0u >> 1 ) & conv.val_uint;

	msg.encode32(conv.val_uint);
}


/**
 * Read a float (32-bit IEEE) from the given NetMsg.
 *
 * @param msg the NetMsg to read from
 * @return the read floating point number
 */
float qspec::decode_float(NetMsg &msg) {
	union converter_t conv;
	conv.val_uint = msg.decode32();

	return conv.val_float;
}


/**
 * Check if a float is not negative and finite.
 *
 * Note that we can't check for -0.0.
 *
 * @param f the value to check
 * @return true if the value is not negative and finite
 */
bool qspec::is_non_negative_finite(float f) {
	return f >= 0.0 && f != std::numeric_limits<float>::infinity()
		&& f != std::numeric_limits<float>::quiet_NaN()
		&& f != std::numeric_limits<float>::signaling_NaN();
}



/*
 * qspec_param class implementation
 */


/**
 * Length of a QSPEC Parameter header in bytes.
 */
const uint16 qspec_param::HEADER_LENGTH = 4;


/**
 * Default length of a QSPEC Parameter body in bytes.
 */
const uint16 qspec_param::DEFAULT_BODY_LENGTH = 4;


/**
 * Standard constructor.
 *
 * This may leave a parameter in an uninitialized state. Use deserialize()
 * to initialize it from a NetMsg.
 *
 * The mandatory flag is set to true, all other flags are set to false.
 */
qspec_param::qspec_param()
		: IE(cat_qspec_param), parameter_id(0), mandatory(true),
		  error(false), not_supported(false) {

	// nothing to do
}


/**
 * Constructor for manual use.
 *
 * Basic initialization is done. The mandatory flag is set to true, all other
 * flags are set to false.
 *
 * The param_id value must not be larger than 12 bit. Note that only a small
 * subset is valid according to QSPEC-T.
 *
 * @param param_id the QSPEC Parameter ID (12 bit)
 */
qspec_param::qspec_param(uint16 param_id)
		: IE(cat_qspec_param), parameter_id(param_id),
		  mandatory(true), error(false), not_supported(false)
		  {

	// nothing to do
}


qspec_param::~qspec_param() {
	// nothing to do
}


bool qspec_param::is_mandatory() const {
	return mandatory;
}

bool qspec_param::has_error() const {
	return error;
}

bool qspec_param::is_not_supported() const {
	return not_supported;
}

uint16 qspec_param::get_parameter_id() const {
	return parameter_id;
}

void qspec_param::set_mandatory(bool m) {
	mandatory = m;
}

void qspec_param::set_error(bool e) {
	error = e;
}

void qspec_param::set_not_supported(bool n) {
	not_supported = n;
}

void qspec_param::set_parameter_id(uint16 id) {
	parameter_id = id;
}


/**
 * Extract the parameter ID.
 *
 * Extract the parameter ID from a raw header. The header is expected to be
 * in host byte order already.
 *
 * @param header_raw 32 bits read from a NetMsg
 * @return the parameter ID
 */
uint16 qspec_param::extract_parameter_id(uint32 header_raw) throw () {
	return ( (header_raw >> 16) & 0xFFF );
}


/**
 * Parse a QSPEC parameter header.
 *
 * If the header is invalid, usually because the NetMsg is truncated, the
 * msg's position is reset.
 *
 * This function throws no exception (that is worth catching).
 *
 * TODO: skip is not yet supported.
 *
 * @param msg the NetMsg to read from
 * @param coding encoding to use (not checked!)
 * @param body_length the length of the body in bytes (according to the header)
 * @param errorlist errors
 * @param bytes_read the number of bytes read
 * @param skip skip the header in case of parse errors
 * @return true on success, false if there's a parse error
 */
bool qspec_param::deserialize_header(NetMsg &msg, coding_t coding,
		uint16 &body_length, IEErrorList &errorlist, uint32 &bytes_read,
		bool skip) {

	uint32 start_pos = msg.get_pos();

	uint32 header_raw;

	try {
		header_raw = msg.decode32();
	}
	catch ( NetMsgError ) {
		catch_bad_alloc( errorlist.put(
			new IEMsgTooShort(coding, get_category(), start_pos)) );
		return false;
	}

	uint32 flags = header_raw >> 28;
	set_mandatory( flags & 8 );
	set_error( flags & 4 );
	set_not_supported( flags & 2 );

	set_parameter_id( extract_parameter_id(header_raw) );

	// The header's value is in 32bit words, we convert it to bytes.
	body_length = (header_raw & 0xFFF) * 4u;


	// Error: Message is shorter than the length field makes us believe.
	if ( msg.get_bytes_left() < body_length ) {
		catch_bad_alloc( errorlist.put(
			new IEMsgTooShort(coding, category, start_pos)) );

		msg.set_pos(start_pos);
		return false;
	}

	// Error: We expected the length field to be different.
	if ( HEADER_LENGTH + body_length !=
			(uint16) get_serialized_size(coding) ) {

		catch_bad_alloc( errorlist.put(
			new IEWrongLength(coding, get_category(),
				get_parameter_id(), 0, msg.get_pos())) );
		
		msg.set_pos(start_pos);
		return false;
	}


	bytes_read = msg.get_pos() - start_pos;

	return true;
}


IE *qspec_param::deserialize(NetMsg &msg, coding_t coding,
		IEErrorList &err, uint32 &bytes_read, bool skip) {

	bytes_read = 0;
	uint32 start_pos = msg.get_pos();

	// check if coding is supported
	uint32 tmp;
	if ( ! check_deser_args(coding, err, tmp) )
		return NULL;

	// Parse header
	uint16 body_length = 0;
	if ( ! deserialize_header(msg, coding, body_length, err,
			bytes_read, skip) )
		return NULL;


	// Parse body
	uint32 body_bytes_read = 0;
	if ( ! deserialize_body(msg, coding, body_length, err,
			body_bytes_read, skip) )
		return NULL;

	bytes_read += body_bytes_read;


	// Check if deserialize_body() returned the correct length.
	if ( bytes_read != msg.get_pos() - start_pos )
		Log(ERROR_LOG, LOG_CRIT, "qspec_param",
				"deserialize(): byte count mismatch");

	return this;	// success
}


/**
 * Write a QSPEC parameter header.
 *
 * Note that the body_length parameter is in bytes. It has to be a multiple
 * of 4 because QSPEC lengths are measured in 32bit words.
 *
 * @param msg the NetMsg to write to
 * @param coding encoding to use (not checked!)
 * @param bytes_written the number of bytes written
 * @param body_length the length of the body in bytes(!)
 */
void qspec_param::serialize_header(NetMsg &msg, coding_t coding,
		uint32 &bytes_written, uint16 body_length) const {

	uint32 flags = (is_mandatory() << 3) | (has_error() << 2)
			| (is_not_supported() << 1) ;

	uint16 length = body_length / 4;

	uint32 header = (flags << 28) | (get_parameter_id() << 16) | length;

	msg.encode32(header);

	bytes_written = 4;
}


void qspec_param::serialize(NetMsg &msg, coding_t coding,
		uint32 &bytes_written) const throw (IEError) {

	bytes_written = 0;

	// Check if the object is in a valid state. Throw an exception if not.
	uint32 tmp;
	check_ser_args(coding, tmp);

	/*
	 * Write header
	 */
	uint16 body_length = get_serialized_size(coding) - HEADER_LENGTH;

	try {
		uint32 header_bytes_written;
		serialize_header(msg, coding, header_bytes_written,
			body_length);

		bytes_written += header_bytes_written;
	}
	catch (NetMsgError) {
		throw IEMsgTooShort(coding, get_category(), msg.get_pos());
	}


	/*
	 * Write body
	 */
	try {
		uint32 body_bytes_written;
		serialize_body(msg, coding, body_bytes_written);

		bytes_written += body_bytes_written;
	}
	catch (NetMsgError) {
		throw IEError(IEError::ERROR_MSG_TOO_SHORT);
	}
}


/**
 * Returns the serialized size.
 *
 * Returns the number of bytes this parameter is if serialized into a
 * NetMsg using the given coding. The size consists of the header and
 * the body, with the body size defaulting to DEFAULT_BODY_LENGTH.
 * Subclasses with a different body size have to override this method.
 *
 * @param coding the coding to use
 * @return the size in bytes
 */
size_t qspec_param::get_serialized_size(coding_t coding) const {
	return HEADER_LENGTH + DEFAULT_BODY_LENGTH;
}


/**
 * Check if a given coding is supported.
 *
 * This implementation only supports protocol_v1. Subclasses supporting
 * more codings have to override this method.
 *
 * @param coding the encoding to test
 */
bool qspec_param::supports_coding(coding_t coding) const {
	return coding == protocol_v1;
}


/**
 * Register this object with an IEManager.
 *
 * Using new_instance(), each subclass of qspec_param can act as a factory
 * for itself. That way the manager can create new instances of this class
 * on request.
 *
 * @param iem the IEManager to register with
 */
void qspec_param::register_ie(IEManager *iem) const {
	iem->register_ie(cat_qspec_param, get_parameter_id(), 0, this);
}


/**
 * Check the state of this object.
 *
 * This method can only check the header, so it relies on check_body()
 * in child classes
 *
 * @return true if the object is in a valid state and false otherwise
 */
bool qspec_param::check() const {
  //cout << get_ie_name() << " check(): get_parameter_id=" << get_parameter_id() << endl;;
	if ( (get_parameter_id() & ~0xFFF) != 0 )
		return false;

	return check_body();
}


/**
 * Check for equality.
 *
 * @param ie the other operand
 * @return true if this object and the parameter are equal
 */
bool qspec_param::operator==(const IE &ie) const {

	const qspec_param *other = dynamic_cast<const qspec_param *>(&ie);

	return other != NULL && get_parameter_id() == other->get_parameter_id()
		&& is_mandatory() == other->is_mandatory()
		&& has_error() == other->has_error()
		&& is_not_supported() == other->is_not_supported()
		&& equals_body(*other);
}


/**
 * Print this object's attributes.
 *
 * This is for debugging only and is used as part of print(). The
 * implementation provided is incomplete and should be overridden by child
 * classes. Only attributes added by the child class have to be printed.
 *
 * @param os the output stream to write to
 */
ostream &qspec_param::print_attributes(ostream &os) const {
	return os << "*UNIMPLEMENTED*";
}


ostream &qspec_param::print(ostream &os, uint32 level, const uint32 indent,
		const char *name) const {

	os << setw(level*indent) << "";

	if ( name )
		os << name << " = ";

	os << "[" << get_ie_name() << ": ";

	os << "id=" << get_parameter_id() << ", ";
	os << "flags='"
		<< ( is_mandatory() ? "M" : "" )
		<< ( has_error() ? "E" : "" )
		<< ( is_not_supported() ? "N" : "" )
		<< "'";

	print_attributes(os);

	os << "]";

	return os;
}


// EOF
