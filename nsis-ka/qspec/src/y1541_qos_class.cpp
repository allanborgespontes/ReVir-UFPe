/*
 * y1541_qos_class.h - The QSPEC Y.1541 QoS Class Parameter
 *
 * $Id: y1541_qos_class.cpp 4593 2009-10-20 14:30:51Z roehricht $
 * $HeadURL: https://svn.tm.kit.edu/nsis/qspec/trunk/src/y1541_qos_class.cpp $
 */
#include "y1541_qos_class.h"
#include "logfile.h"


using namespace qspec;



/**
 * The officially assigned parameter ID.
 */
const uint16 y1541_qos_class::PARAM_ID = 14;

const char *const y1541_qos_class::ie_name = "y1541_qos_class";


/**
 * Default constructor.
 */
y1541_qos_class::y1541_qos_class()
		: qspec_param(PARAM_ID), value(0) {

	// nothing to do
}


/**
 * Constructor for manual use.
 *
 * @param value a Y.1541 QoS Class value (0 to 5 are currently valid)
 */
y1541_qos_class::y1541_qos_class(uint8 value)
		: qspec_param(PARAM_ID), value(value) {

	// nothing to do
}


/**
 * Constructor for manual use.
 *
 * Allows to set the parameter ID. This is usually not required and should
 * be used with care if QSPEC-T compliant output is desired.
 *
 * @param param_id the parameter Id
 * @param value a Y.1541 QoS Class value (0 to 5 are currently valid)
 */
y1541_qos_class::y1541_qos_class(uint16 param_id, uint8 value)
		: qspec_param(param_id), value(value) {

	// nothing to do
}


y1541_qos_class::~y1541_qos_class() {
	// nothing to do
}


y1541_qos_class *y1541_qos_class::new_instance() const {
	y1541_qos_class *q = NULL;
	catch_bad_alloc( q = new y1541_qos_class() );
	return q;
}


y1541_qos_class *y1541_qos_class::copy() const {
	y1541_qos_class *q = NULL;
	catch_bad_alloc( q = new y1541_qos_class(*this) );
	return q;
}


bool y1541_qos_class::deserialize_body(NetMsg &msg, coding_t coding,
		uint16 body_length, IEErrorList &err, uint32 &bytes_read,
		bool skip) {

	bytes_read = 0;

	uint8 qos_class = msg.decode8();
	msg.set_pos_r(3); // skip 3 bytes
	bytes_read = 4;

	set_value(qos_class);

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


void y1541_qos_class::serialize_body(NetMsg &msg, coding_t coding,
		uint32 &bytes_written) const {

	bytes_written = 0;

	msg.encode8( get_value() ); // write QoS Class value
	msg.encode8(0);
	msg.encode16(0); // reserved part

	bytes_written = 4;
}


bool y1541_qos_class::check_body() const {
	// Currently only values 0 to 5 are valid (according to QSPEC-T)
	return get_value() <= 5;
}


bool y1541_qos_class::equals_body(const qspec_param &param) const {

	const y1541_qos_class *other
		= dynamic_cast<const y1541_qos_class *>(&param);

	return other != NULL && get_value() == other->get_value();
}


const char *y1541_qos_class::get_ie_name() const {
	return ie_name;
}


ostream &y1541_qos_class::print_attributes(ostream &os) const {
	return os << ", class=" << get_value();
}


/**
 * Returns the value of the Y.1541 QoS Class field
 *
 * @return the value of the Y.1541 QoS Class field
 */
uint8 y1541_qos_class::get_value() const {
	return value;
}


/**
 * Set the value of the Y.1541 QoS Class field.
 *
 * @param val the new value of the Y.1541 QoS Class field
 */
void y1541_qos_class::set_value(uint8 val) {
	value = val;
}


// EOF
