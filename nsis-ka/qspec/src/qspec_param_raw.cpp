/*
 * qspec_param_raw.cpp - The fallback parameter
 *
 * $Id: qspec_param_raw.cpp 4593 2009-10-20 14:30:51Z roehricht $
 * $HeadURL: https://svn.tm.kit.edu/nsis/qspec/trunk/src/qspec_param_raw.cpp $
 */
#include <cstring>	// for memcmp()

#include "qspec_ie.h"
#include "qspec_param_raw.h"
#include "logfile.h"


using namespace qspec;


const char *const qspec_param_raw::ie_name = "qspec_param_raw";


/**
 * Default constructor.
 */
qspec_param_raw::qspec_param_raw()
		: qspec_param(0xFFF), data(NULL), data_length(0) {

	// nothing to do
}


qspec_param_raw::~qspec_param_raw() {
	if ( data )
		delete[] data;
}


qspec_param_raw *qspec_param_raw::new_instance() const {
	qspec_param_raw *q = NULL;
	catch_bad_alloc( q = new qspec_param_raw() );
	return q;
}


qspec_param_raw *qspec_param_raw::copy() const {
	qspec_param_raw *q = NULL;
	catch_bad_alloc( q = new qspec_param_raw(*this) );
	return q;
}


/*
 * We have to override this because the parent's deserialize_header() does
 * sanity checks on the length field. Ours is variable, so we can't have
 * that.
 */
bool qspec_param_raw::deserialize_header(NetMsg &msg, coding_t coding,
		uint16 &body_length, IEErrorList &errorlist, uint32 &bytes_read,
		bool skip) {

	uint32 header_raw = 0;

	try {
		header_raw = msg.decode32(false); // don't move position
	}
	catch ( NetMsgError ) {
		// Do nothing, the code below will handle that.
	}

	// The header's value is in 32bit words, we convert it to bytes.
	data_length = (header_raw & 0xFFF) * 4u;

	return qspec_param::deserialize_header(msg, coding, body_length,
		errorlist, bytes_read, skip);
}


bool qspec_param_raw::deserialize_body(NetMsg &msg, coding_t coding,
		uint16 body_length, IEErrorList &err, uint32 &bytes_read,
		bool skip) {

	bytes_read = 0;

	if ( data )
		delete[] data;

	data_length = body_length;
	data = ( data_length != 0 ) ? new uchar[data_length] : NULL;

	for ( unsigned int i = 0; i < data_length; i++ ) {
		data[i] = msg.decode8();
		bytes_read++;
	}

	return true; // success
}


void qspec_param_raw::serialize_body(NetMsg &msg, coding_t coding,
		uint32 &bytes_written) const {

	bytes_written = 0;

	if ( data == NULL )
		return;

	for ( unsigned int i = 0; i < data_length; i++ ) {
		msg.encode8(data[i]);
		bytes_written++;
	}
}


bool qspec_param_raw::check_body() const {
	return true; // nothing to check
}


bool qspec_param_raw::equals_body(const qspec_param &param) const {

	const qspec_param_raw *other
		= dynamic_cast<const qspec_param_raw *>(&param);

	if ( data_length != other->data_length )
		return false;

	// memcmp() returns 0 if the arrays are equal
	return memcmp(data, other->data, data_length) == 0;
}


size_t qspec_param_raw::get_serialized_size(coding_t coding) const {
	return qspec_param::HEADER_LENGTH + data_length;
}


const char *qspec_param_raw::get_ie_name() const {
	return ie_name;
}


ostream &qspec_param_raw::print_attributes(ostream &os) const {
	return os << ", unparsed data";
}


void qspec_param_raw::register_ie(IEManager *iem) const {
	iem->register_ie(cat_default_qspec_param, 0, 0, this);
}


/**
 * Returns a pointer to the raw data.
 *
 * @return a pointer to the raw data
 */
uchar *qspec_param_raw::get_data() const {
	return data;
}


/**
 * Returns the length of the raw data block.
 *
 * @return the length of the raw data block in bytes.
 */
size_t qspec_param_raw::get_data_length() const {
	return data_length;
}


// EOF
