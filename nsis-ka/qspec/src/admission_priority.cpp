/*
 * admission_priority.cpp - The QSPEC Admission Priority Parameter
 *
 * $Id: admission_priority.cpp 4593 2009-10-20 14:30:51Z roehricht $
 * $HeadURL: https://svn.tm.kit.edu/nsis/qspec/trunk/src/admission_priority.cpp $
 */
#include <cstring>

#include "admission_priority.h"
#include "logfile.h"


using namespace qspec;


/**
 * The officially assigned parameter ID.
 */
const uint16 admission_priority::PARAM_ID = 9;

const char *const admission_priority::ie_name = "admission_priority";


/**
 * Default constructor.
 */
admission_priority::admission_priority()
  : qspec_param(PARAM_ID), y2171_ap_value(ap_best_effort_priority_flow), ap_value(ap_best_effort_priority_flow) {

	// nothing to do
}


/**
 * Constructor for manual use.
 *
 * @param value the value of the Admission Priority Parameter
 */
admission_priority::admission_priority(admission_priority_t value)
  : qspec_param(PARAM_ID), y2171_ap_value(value), ap_value(value) {

	// nothing to do
}


/**
 * Constructor for manual use.
 *
 * Allows to set the parameter Id. This is usually not required and should
 * be used with care if QSPEC-T compliant output is desired.
 *
 * @param param_id the parameter Id
 * @param value the value of the Admission Priority Parameter
 */
admission_priority::admission_priority(uint16 param_id,
			admission_priority_t value)
		: qspec_param(param_id), y2171_ap_value(value), ap_value(value) {

	// nothing to do
}


admission_priority::~admission_priority() {
	// nothing to do
}


admission_priority *
admission_priority::new_instance() const {
	admission_priority *q = NULL;
	catch_bad_alloc( q = new admission_priority() );
	return q;
}


admission_priority *
admission_priority::copy() const {
	admission_priority *q = NULL;
	catch_bad_alloc( q = new admission_priority(*this) );
	return q;
}


/* 
 * Note: According to the specs we have to ignore the reserved bits, so we
 * are not allowed to check if they are all zero.
 */
bool 
admission_priority::deserialize_body(NetMsg &msg, coding_t coding,
		uint16 body_length, IEErrorList &err, uint32 &bytes_read,
		bool skip) {

	uint8 y2171_raw_value = msg.decode8();

	uint8 raw_value = msg.decode8();
	ap_value= raw_value;

	msg.set_pos_r(2); // skip 2 bytes

	bytes_read = 4;

	if ( y2171_raw_value <= ap_high_priority_flow || y2171_raw_value == ap_other_admission_priority )
		y2171_ap_value = admission_priority_t(y2171_raw_value);
	else {
		catch_bad_alloc( err.put( 
			new PDUSyntaxError(coding, get_category(),
				get_parameter_id(), 0, msg.get_pos()-4)) );

		if ( ! skip )
			return false;
	}

	return true; // success
}


void 
admission_priority::serialize_body(NetMsg &msg, coding_t coding,
		uint32 &bytes_written) const {

	msg.encode8(y2171_ap_value);
	msg.encode8(ap_value);
	msg.encode16(0);

	bytes_written = 4;
}


bool 
admission_priority::check_body() const {
  // either both fields are the same
  // or the first field is all ones and the second holds a value

  /*
   *   If the QNI signals <Y.2171 Admission Priority>, it populates both
   *   the <Y.2171 Admission Priority> and <Admission Priority> fields with
   *   the same value. 
   *
   *  If the QNI signals admission priority according to [EMERGENCY-RSVP],
   *  it populates a locally significant value in the <Admission Priority>
   *  field and places all 1's in the <Y.2171 Admission Priority> field.
   */

  return (y2171_ap_value == ap_other_admission_priority) || (y2171_ap_value == ap_value); 
}


bool 
admission_priority::equals_body(const qspec_param &param) const {

	const admission_priority *other
		= dynamic_cast<const admission_priority *>(&param);

	return other != NULL &&  ( get_y2171value() == other->get_y2171value() ) && ( get_value() == other->get_value() );
}


const char *
admission_priority::get_ie_name() const {
	return ie_name;
}


ostream &
admission_priority::print_attributes(ostream &os) const {

  if (y2171_ap_value == ap_other_admission_priority)
  {
    return os << ", admission prio=" << (uint16) ap_value;
  }
  else
  {
	os << ", Y.2171 admission prio=";

	switch ( y2171_ap_value ) {
		case ap_high_priority_flow:
			return os << "'high priority flow'";
		case ap_normal_priority_flow:
			return os << "'normal priority flow'";
		case ap_best_effort_priority_flow:
			return os << "'best-effort priority flow'";
		default:
			return os << "'*UNKNOWN*'";
	} // end switch
  }

  return os;
  // not reached
}


/**
 * Returns the value of the Y.2171 Admission Priority field.
 *
 * @return the value of the  Y.2171 Admission Priority field
 */
admission_priority_t 
admission_priority::get_y2171value() const {
	return y2171_ap_value;
}

/**
 * Returns the value of the Admission Priority field.
 *
 * @return the value of the Admission Priority field
 */
uint8 admission_priority::get_value() const {
	return ap_value;
}


/**
 * Set the value of the Admission Priority field.
 *
 * @param val the new value of the Admission Priority field
 */
void admission_priority::set_y2171value(admission_priority_t val) {
  y2171_ap_value = val;
  ap_value= val;
}

/**
 * Set the value of the Admission Priority field.
 *
 * @param val the new value of the Admission Priority field
 */
void 
admission_priority::set_value(uint8 val) {
  y2171_ap_value = ap_other_admission_priority;
  ap_value= val;
}


// EOF
