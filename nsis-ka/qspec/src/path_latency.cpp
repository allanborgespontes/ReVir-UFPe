/*
 * path_latency.cpp - The QSPEC Path Latency Parameter
 *
 * $Id: path_latency.cpp 4593 2009-10-20 14:30:51Z roehricht $
 * $HeadURL: https://svn.tm.kit.edu/nsis/qspec/trunk/src/path_latency.cpp $
 *
 */
#include "path_latency.h"
#include "logfile.h"


using namespace qspec;



/**
 * The officially assigned parameter ID.
 */
const uint16 path_latency::PARAM_ID = 3;

const char *const path_latency::ie_name = "path_latency";


/**
 * Default constructor.
 */
path_latency::path_latency()
		: qspec_param(PARAM_ID), value(0) {

	set_mandatory(false);
}


/**
 * Constructor for manual use.
 *
 * @param value the value of the Path Latency Parameter in microseconds
 */
path_latency::path_latency(uint32 value)
		: qspec_param(PARAM_ID), value(value) {

	set_mandatory(false);
}


/**
 * Constructor for manual use.
 *
 * Allows to set the parameter ID. This is usually not required and should
 * be used with care if QSPEC-T compliant output is desired.
 *
 * @param param_id the parameter ID
 * @param value the value of the Path Latency Parameter in microseconds
 */
path_latency::path_latency(uint16 param_id, uint32 value)
		: qspec_param(param_id), value(value) {

	set_mandatory(false);
}


path_latency::~path_latency() {
	// nothing to do
}


path_latency *path_latency::new_instance() const {
	path_latency *q = NULL;
	catch_bad_alloc( q = new path_latency() );
	return q;
}


path_latency *path_latency::copy() const {
	path_latency *q = NULL;
	catch_bad_alloc( q = new path_latency(*this) );
	return q;
}


bool path_latency::deserialize_body(NetMsg &msg, coding_t coding,
		uint16 body_length, IEErrorList &err, uint32 &bytes_read,
		bool skip) {

	bytes_read = 0;

	set_value( msg.decode32() );
	bytes_read = 4;

	return true; // success successful, we can't check anything
}


void path_latency::serialize_body(NetMsg &msg, coding_t coding,
		uint32 &bytes_written) const {

	bytes_written = 0;

	msg.encode32( get_value() );
	bytes_written = 4;
}


bool path_latency::check_body() const {
	return true; // nothing to check, all values are valid
}


bool path_latency::equals_body(const qspec_param &param) const {

	const path_latency *other
		= dynamic_cast<const path_latency *>(&param);

	return other != NULL && get_value() == other->get_value();
}


const char *path_latency::get_ie_name() const {
	return ie_name;
}


ostream &path_latency::print_attributes(ostream &os) const {
	return os << ", value=" << get_value();
}


/**
 * Returns the value of the Path Latency field.
 *
 * @return the value of the Path Latency field in microseconds
 */
uint32 path_latency::get_value() const {
	return value;
}


/**
 * Set the value of the Path Latency field.
 *
 * @param val the new value of the Path Latency field in microseconds
 */
void path_latency::set_value(uint32 val) {
	value = val;
}


// EOF
