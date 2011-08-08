/*
 * dste_class_type.h - The QSPEC DSTE Class Type Parameter
 *
 * $Id: dste_class_type.cpp 4593 2009-10-20 14:30:51Z roehricht $
 * $HeadURL: https://svn.tm.kit.edu/nsis/qspec/trunk/src/dste_class_type.cpp $
 */
#include "dste_class_type.h"
#include "logfile.h"


using namespace qspec;



/**
 * The officially assigned parameter ID.
 */
const uint16 dste_class_type::PARAM_ID = 13;

const char *const dste_class_type::ie_name = "dste_class_type";


/**
 * Default constructor.
 */
dste_class_type::dste_class_type()
		: qspec_param(PARAM_ID), value(0) {

	// nothing to do
}


/**
 * Constructor for manual use.
 *
 * @param value a DSTE Class Type value (0 to 7 are currently valid)
 */
dste_class_type::dste_class_type(uint8 value)
		: qspec_param(PARAM_ID), value(value) {

	// nothing to do
}


/**
 * Constructor for manual use.
 *
 * Allows to set the parameter ID. This is usually not required and should
 * be used with care if QSPEC-T compliant output is desired.
 *
 * @param param_id the parameter ID
 * @param value a DSTE Class Type value (0 to 7 are currently valid)
 */
dste_class_type::dste_class_type(uint16 param_id, uint8 value)
		: qspec_param(param_id), value(value) {

	// nothing to do
}


dste_class_type::~dste_class_type() {
	// nothing to do
}


dste_class_type *dste_class_type::new_instance() const {
	dste_class_type *q = NULL;
	catch_bad_alloc( q = new dste_class_type() );
	return q;
}


dste_class_type *dste_class_type::copy() const {
	dste_class_type *q = NULL;
	catch_bad_alloc( q = new dste_class_type(*this) );
	return q;
}


bool dste_class_type::deserialize_body(NetMsg &msg, coding_t coding,
		uint16 body_length, IEErrorList &err, uint32 &bytes_read,
		bool skip) {

	bytes_read = 0;

	uint8 class_type = msg.decode8();
	msg.set_pos_r(3); // skip 3 bytes
	bytes_read = 4;

	set_value(class_type);

	// Error: Invalid value in body.
	if ( check_body() == false ) {
		catch_bad_alloc( err.put( 
			new PDUSyntaxError(coding, get_category(),
				get_parameter_id(), 0, msg.get_pos()-4)) );

		if ( ! skip )
			return false;
	}

	return true; // success
}


void dste_class_type::serialize_body(NetMsg &msg, coding_t coding,
		uint32 &bytes_written) const {

	bytes_written = 0;

	msg.encode8( get_value() ); // write DSTE Class Type value
	msg.encode8(0); // reserved
	msg.encode16(0); // reserved

	bytes_written = 4;
}


bool dste_class_type::check_body() const {
	// Currently only values 0 to 7 are valid (according to QSPEC-T)
	return get_value() <= 7;
}


bool dste_class_type::equals_body(const qspec_param &param) const {

	const dste_class_type *other
		= dynamic_cast<const dste_class_type *>(&param);

	return other != NULL && get_value() == other->get_value();
}


const char *dste_class_type::get_ie_name() const {
	return ie_name;
}


ostream &dste_class_type::print_attributes(ostream &os) const {
	return os << ", class=" << get_value();
}


/**
 * Returns the value of the DSTE Class Type field
 *
 * @return the value of the DSTE Class Type field
 */
uint8 dste_class_type::get_value() const {
	return value;
}


/**
 * Set the value of the DSTE Class Type field.
 *
 * @param val the new value of the DSTE Class Type field
 */
void dste_class_type::set_value(uint8 val) {
	value = val;
}


// EOF
