/*
 * path_plr.cpp - The QSPEC Path PLR Parameter
 *
 * $Id: path_plr.cpp 4593 2009-10-20 14:30:51Z roehricht $
 * $HeadURL: https://svn.tm.kit.edu/nsis/qspec/trunk/src/path_plr.cpp $
 */
#include <limits>

#include "path_plr.h"
#include "logfile.h"


using namespace qspec;



/**
 * The officially assigned parameter ID.
 */
const uint16 path_plr::PARAM_ID = 5;

const char *const path_plr::ie_name = "path_plr";


/**
 * Default constructor.
 */
path_plr::path_plr()
		: qspec_param(PARAM_ID), value(0) {

	set_mandatory(false);
}


/**
 * Constructor for manual use.
 *
 * @param value the value of the Path PLR Parameter
 */
path_plr::path_plr(float value)
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
 * @param value the value of the Path PLR Parameter
 */
path_plr::path_plr(uint16 param_id, float value)
		: qspec_param(param_id), value(value) {

	set_mandatory(false);
}


path_plr::~path_plr() {
	// nothing to do
}


path_plr *path_plr::new_instance() const {
	path_plr *q = NULL;
	catch_bad_alloc( q = new path_plr() );
	return q;
}


path_plr *path_plr::copy() const {
	path_plr *q = NULL;
	catch_bad_alloc( q = new path_plr(*this) );
	return q;
}


bool path_plr::deserialize_body(NetMsg &msg, coding_t coding,
		uint16 body_length, IEErrorList &err, uint32 &bytes_read,
		bool skip) {

	set_value(decode_float(msg));
	bytes_read = 4;

	// Check for invalid values.
	if ( ! check() ) {
		catch_bad_alloc( err.put( 
			new PDUSyntaxError(coding, get_category(),
				get_parameter_id(), 0, msg.get_pos()-4)) );

		if ( ! skip )
			return false;
	}

	return true; // success
}


void path_plr::serialize_body(NetMsg &msg, coding_t coding,
		uint32 &bytes_written) const {

	encode(msg, get_value());
	bytes_written = 4;
}


// Note: It is not possible to detect a negative zero representation (-0.0).
bool path_plr::check_body() const {
	return is_non_negative_finite(get_value());
}


bool path_plr::equals_body(const qspec_param &param) const {

	const path_plr *other
		= dynamic_cast<const path_plr *>(&param);

	return other != NULL && get_value() == other->get_value();
}


const char *path_plr::get_ie_name() const {
	return ie_name;
}


ostream &path_plr::print_attributes(ostream &os) const {
	return os << ", value=" << get_value();
}


/**
 * Returns the value of the Path PLR field.
 *
 * @return the value of the Path PLR field
 */
float path_plr::get_value() const {
	return value;
}


/**
 * Set the value of the Path PLR field.
 *
 * @param val the new value of the Path PLR field
 */
void path_plr::set_value(float val) {
	value = val;
}


// EOF
