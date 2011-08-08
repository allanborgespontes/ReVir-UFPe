/*
 * preemption_priority.cpp - The QSPEC Preemption/Defending Priority Parameters
 *
 * $Id: preemption_priority.cpp 4593 2009-10-20 14:30:51Z roehricht $
 * $HeadURL: https://svn.tm.kit.edu/nsis/qspec/trunk/src/preemption_priority.cpp $
 */
#include "preemption_priority.h"
#include "logfile.h"


using namespace qspec;


/**
 * The officially assigned parameter ID.
 */
const uint16 preemption_priority::PARAM_ID = 8;

const char *const preemption_priority::ie_name = "preemption_priority";


/**
 * Default constructor.
 */
preemption_priority::preemption_priority()
		: qspec_param(PARAM_ID),
		  preemption_priority_value(0), defending_priority_value(0) {

	// nothing to do
}


/**
 * Constructor for manual use.
 *
 * @param preemption_priority the preemption priority
 * @param defending_priority the defending priority
 */
preemption_priority::preemption_priority(uint16 preemption_priority,
		uint16 defending_priority)
		: qspec_param(PARAM_ID),
		  preemption_priority_value(preemption_priority),
		  defending_priority_value(defending_priority) {

	// nothing to do
}


/**
 * Constructor for manual use.
 *
 * Allows to set the parameter ID. This is usually not required and should
 * be used with care if QSPEC-T compliant output is desired.
 *
 * @param param_id the parameter ID
 * @param preemption_priority the preemption priority
 * @param defending_priority the defending priority
 */
preemption_priority::preemption_priority(uint16 param_id,
		uint16 preemption_priority, uint16 defending_priority)
		: qspec_param(param_id),
		  preemption_priority_value(preemption_priority),
		  defending_priority_value(defending_priority) {

	// nothing to do
}


preemption_priority::~preemption_priority() {
	// nothing to do
}


preemption_priority *preemption_priority::new_instance() const {
	preemption_priority *q = NULL;
	catch_bad_alloc( q = new preemption_priority() );
	return q;
}


preemption_priority *preemption_priority::copy() const {
	preemption_priority *q = NULL;
	catch_bad_alloc( q = new preemption_priority(*this) );
	return q;
}


bool preemption_priority::deserialize_body(NetMsg &msg, coding_t coding,
		uint16 body_length, IEErrorList &err, uint32 &bytes_read,
		bool skip) {

	bytes_read = 0;

	set_preemption_priority( msg.decode16() );
	bytes_read += 2;

	set_defending_priority( msg.decode16() );
	bytes_read += 2;

	return true; // success, there is nothing we could check
}


void preemption_priority::serialize_body(NetMsg &msg, coding_t coding,
		uint32 &bytes_written) const {

	bytes_written = 0;

	msg.encode16( get_preemption_priority() );
	bytes_written += 2;

	msg.encode16( get_defending_priority() );
	bytes_written += 2;
}


bool preemption_priority::check_body() const {
	return true;	// nothing to check
}


bool preemption_priority::equals_body(const qspec_param &param) const {

	const preemption_priority *other
		= dynamic_cast<const preemption_priority *>(&param);

	return other != NULL
		&& get_preemption_priority() == other->get_preemption_priority()
		&& get_defending_priority() == other->get_defending_priority();
}


const char *preemption_priority::get_ie_name() const {
	return ie_name;
}


ostream &preemption_priority::print_attributes(ostream &os) const {
	return os << ", preemption_priority=" << get_preemption_priority() 
		<< ", defending_priority=" << get_defending_priority();
}


/**
 * Returns the value of the Preemption Priority field.
 *
 * @return the value of the Preemption Priority field
 */
uint16 preemption_priority::get_preemption_priority() const {
	return preemption_priority_value;
}


/**
 * Set the value of the Preemption Priority field.
 *
 * @param val the new value of the Preemption Priority field
 */
void preemption_priority::set_preemption_priority(uint16 val) {
	preemption_priority_value = val;
}


/**
 * Returns the value of the Defending Priority field.
 *
 * @return the value of the Defending Priority field
 */
uint16 preemption_priority::get_defending_priority() const {
	return defending_priority_value;
}


/**
 * Set the value of the Defending Priority field.
 *
 * @param val the new value of the Defending Priority field
 */
void preemption_priority::set_defending_priority(uint16 val) {
	defending_priority_value = val;
}


// EOF
