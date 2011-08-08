/*
 * excess_treatment.cpp - The QSPEC Excess Treatment Parameter
 *
 * $Id: excess_treatment.cpp 4593 2009-10-20 14:30:51Z roehricht $
 * $HeadURL: https://svn.tm.kit.edu/nsis/qspec/trunk/src/excess_treatment.cpp $
 */
#include <cstring>

#include "excess_treatment.h"
#include "logfile.h"


using namespace qspec;


/**
 * The officially assigned parameter ID.
 */
const uint16 excess_treatment::PARAM_ID = 11;

const char *const excess_treatment::ie_name = "excess_treatment";


/**
 * Default constructor.
 */
excess_treatment::excess_treatment()
  : qspec_param(PARAM_ID), value(et_drop), remark_to_value(0) {

	// nothing to do
}


/**
 * Constructor for manual use.
 *
 * @param value the value of the Excess Treatment Parameter
 * @param remark the value of the remarked packets (6-bit DSCP)
 */
excess_treatment::excess_treatment(excess_treatment_t value, uint8 remark)
  : qspec_param(PARAM_ID), value(value), remark_to_value(remark & 0x3f) {

	// nothing to do
}


/**
 * Constructor for manual use.
 *
 * Allows to set the parameter Id. This is usually not required and should
 * be used with care if QSPEC-T compliant output is desired.
 *
 * @param param_id the parameter Id
 * @param value the value of the Excess Treatment Parameter
 * @param remark the value of the remarked packets (6-bit DSCP)
 */
excess_treatment::excess_treatment(uint16 param_id, excess_treatment_t value, uint8 remark)
  : qspec_param(param_id), value(value), remark_to_value(remark) {

	// nothing to do
}


excess_treatment::~excess_treatment() {
	// nothing to do
}


excess_treatment *excess_treatment::new_instance() const {
	excess_treatment *q = NULL;
	catch_bad_alloc( q = new excess_treatment() );
	return q;
}


excess_treatment *excess_treatment::copy() const {
	excess_treatment *q = NULL;
	catch_bad_alloc( q = new excess_treatment(*this) );
	return q;
}


/* 
 * Note: According to the specs we have to ignore the reserved bits, so we
 * are not allowed to check if they are all zero.
 */
bool excess_treatment::deserialize_body(NetMsg &msg, coding_t coding,
		uint16 body_length, IEErrorList &err, uint32 &bytes_read,
		bool skip) {

	uint8 raw_value = msg.decode8();
	uint8 remark_raw_value = msg.decode8();
	remark_to_value= remark_raw_value >> 2;

	msg.set_pos_r(2); // skip 3 bytes

	bytes_read = 4;

	if ( raw_value <= et_no_metering )
		value = excess_treatment_t(raw_value);
	else {
		catch_bad_alloc( err.put( 
			new PDUSyntaxError(coding, get_category(),
				get_parameter_id(), 0, msg.get_pos()-4)) );

		if ( ! skip )
			return false;
	}

	return true; // success
}


void excess_treatment::serialize_body(NetMsg &msg, coding_t coding,
		uint32 &bytes_written) const {

	msg.encode8(get_value());
	msg.encode8(remark_to_value << 2);
	msg.encode16(0);

	bytes_written = 4;
}


bool excess_treatment::check_body() const {
	return true;	// nothing to check
}


bool excess_treatment::equals_body(const qspec_param &param) const {

	const excess_treatment *other
		= dynamic_cast<const excess_treatment *>(&param);

	return other != NULL && get_value() == other->get_value() && get_remark_value() == other->get_remark_value();
}


const char *excess_treatment::get_ie_name() const {
	return ie_name;
}


ostream &excess_treatment::print_attributes(ostream &os) const {

	os << ", value=";

	switch ( get_value() ) {
		case et_drop:		return os << "'drop'";
		case et_shape:		return os << "'shape'";
	  case et_remark:		return os << "'remark' to" << (uint16) remark_to_value;
		case et_no_metering:	return os << "'no metering/policing'";
		default:		return os << "'*UNKNOWN*'";
	}

	// not reached
}


/**
 * Returns the value of the Excess Treatment field.
 *
 * @return the value of the Excess Treatment field
 */
excess_treatment_t excess_treatment::get_value() const {
	return value;
}


/**
 * Set the value of the Excess Treatment field.
 *
 * @param val the new value of the Excess Treatment field
 */
void excess_treatment::set_value(excess_treatment_t val) {
	value = val;
}


/**
 * Returns the value of the Remark Value field.
 *
 * @return the value of the Remark Value field
 */
uint8
excess_treatment::get_remark_value() const {
	return remark_to_value;
}


/**
 * Set the value of the Remark Value field.
 *
 * @param val the new value of the Remark Value field (only 6 bit allowed)
 */
void excess_treatment::set_remark_value(uint8 val) {
	remark_to_value = val & 0x3F;
}

// EOF
