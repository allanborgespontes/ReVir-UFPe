/*
 * path_jitter.cpp - The QSPEC Path Jitter Parameter
 *
 * $Id: path_jitter.cpp 4593 2009-10-20 14:30:51Z roehricht $
 * $HeadURL: https://svn.tm.kit.edu/nsis/qspec/trunk/src/path_jitter.cpp $
 *
 */
#include "path_jitter.h"
#include "logfile.h"


using namespace qspec;



/**
 * The officially assigned parameter ID.
 */
const uint16 path_jitter::PARAM_ID = 4;

const char *const path_jitter::ie_name = "path_jitter";


/**
 * Default constructor.
 */
path_jitter::path_jitter()
		: qspec_param(PARAM_ID) {

	values[0] = values[1] = values[2] = values[3] = 0;
	set_mandatory(false);
}


/**
 * Constructor for manual use.
 *
 * @param value1 the value of the first Path Jitter Parameter (variance) in microseconds
 * @param value2 the value of the second Path Jitter Parameter (99.9%-ile) in microseconds
 * @param value3 the value of the third Path Jitter Parameter (minimum latency) in microseconds
 * @param value4 the value of the third Path Jitter Parameter (reserved) in microseconds
 */
path_jitter::path_jitter(uint32 value1, uint32 value2, uint32 value3, uint32 value4)
		: qspec_param(PARAM_ID) {

	values[0] = value1;
	values[1] = value2;
	values[2] = value3;
	values[3] = value4;
	set_mandatory(false);
}


/**
 * Constructor for manual use.
 *
 * Allows to set the parameter ID. This is usually not required and should
 * be used with care if QSPEC-T compliant output is desired.
 *
 * @param param_id the parameter ID
 * @param value1 the value of the first Path Jitter Parameter (variance) in microseconds
 * @param value2 the value of the second Path Jitter Parameter (99.9%-ile) in microseconds
 * @param value3 the value of the third Path Jitter Parameter (minimum latency) in microseconds
 * @param value4 the value of the third Path Jitter Parameter (reserved) in microseconds
 */
path_jitter::path_jitter(uint16 param_id,
			 uint32 value1, uint32 value2, uint32 value3, uint32 value4)
		: qspec_param(param_id) {

	values[0] = value1;
	values[1] = value2;
	values[2] = value3;
	values[3] = value4;
	set_mandatory(false);
}


path_jitter::~path_jitter() {
	// nothing to do
}


path_jitter *path_jitter::new_instance() const {
	path_jitter *q = NULL;
	catch_bad_alloc( q = new path_jitter() );
	return q;
}


path_jitter *path_jitter::copy() const {
	path_jitter *q = NULL;
	catch_bad_alloc( q = new path_jitter(*this) );
	return q;
}


bool path_jitter::deserialize_body(NetMsg &msg, coding_t coding,
		uint16 body_length, IEErrorList &err, uint32 &bytes_read,
		bool skip) {

	bytes_read = 0;

	set_value1( msg.decode32() );
	bytes_read += 4;

	set_value2( msg.decode32() );
	bytes_read += 4;

	set_value3( msg.decode32() );
	bytes_read += 4;

	set_value4( msg.decode32() );
	bytes_read += 4;

	return true; // success successful, we can't check anything
}


void path_jitter::serialize_body(NetMsg &msg, coding_t coding,
		uint32 &bytes_written) const {

	bytes_written = 0;

	msg.encode32( get_value1() );
	bytes_written += 4;

	msg.encode32( get_value2() );
	bytes_written += 4;

	msg.encode32( get_value3() );
	bytes_written += 4;

	msg.encode32( get_value4() );
	bytes_written += 4;
}


bool path_jitter::check_body() const {
	return true; // nothing to check, all values are valid
}


bool path_jitter::equals_body(const qspec_param &param) const {

	const path_jitter *other
		= dynamic_cast<const path_jitter *>(&param);

	return other != NULL && get_value1() == other->get_value1()
		&& get_value2() == other->get_value2()
		&& get_value3() == other->get_value3()
		&& get_value4() == other->get_value4();
}


const char *path_jitter::get_ie_name() const {
	return ie_name;
}


size_t path_jitter::get_serialized_size(coding_t coding) const {
	// header, three 32bit unsigned integers
	return qspec_param::HEADER_LENGTH + 4*4;
}


ostream &path_jitter::print_attributes(ostream &os) const {
	return os << ", value1=" << get_value1()
		  << ", value2=" << get_value2() 
		  << ", value3=" << get_value3()
		  << ", value4=" << get_value4();
}


/**
 * Returns the value of the first Path Jitter field.
 *
 * @return the value of the first Path Jitter field in microseconds
 */
uint32 path_jitter::get_value1() const {
	return values[0];
}


/**
 * Set the value of the first Path Jitter field.
 *
 * @param val the new value of the first Path Jitter field in microseconds
 */
void path_jitter::set_value1(uint32 val) {
	values[0] = val;
}


/**
 * Returns the value of the second Path Jitter field.
 *
 * @return the value of the second Path Jitter field in microseconds
 */
uint32 path_jitter::get_value2() const {
	return values[1];
}


/**
 * Set the value of the second Path Jitter field.
 *
 * @param val the new value of the second Path Jitter field in microseconds
 */
void path_jitter::set_value2(uint32 val) {
	values[1] = val;
}


/**
 * Returns the value of the third Path Jitter field.
 *
 * @return the value of the third Path Jitter field in microseconds
 */
uint32 path_jitter::get_value3() const {
	return values[2];
}


/**
 * Set the value of the third Path Jitter field.
 *
 * @param val the new value of the third Path Jitter field in microseconds
 */
void path_jitter::set_value3(uint32 val) {
	values[2] = val;
}


/**
 * Returns the value of the fourth Path Jitter field.
 *
 * @return the value of the fourth Path Jitter field in microseconds
 */
uint32 path_jitter::get_value4() const {
	return values[3];
}


/**
 * Set the value of the fourth Path Jitter field.
 *
 * @param val the new value of the fourth Path Jitter field in microseconds
 */
void path_jitter::set_value4(uint32 val) {
	values[3] = val;
}

// EOF
