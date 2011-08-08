/*
 * rph_priority.cpp - The QSPEC RPH Priority Parameter
 *
 * $Id: rph_priority.cpp 4593 2009-10-20 14:30:51Z roehricht $
 * $HeadURL: https://svn.tm.kit.edu/nsis/qspec/trunk/src/rph_priority.cpp $
 */
#include "rph_priority.h"
#include "logfile.h"


using namespace qspec;


/**
 * The officially assigned parameter ID.
 */
const uint16 rph_priority::PARAM_ID = 10;

const char *const rph_priority::ie_name = "rph_priority";


/**
 * Default constructor.
 */
rph_priority::rph_priority()
		: qspec_param(PARAM_ID),
		  rpn_namespace(0), rpn_priority(0) {

	// nothing to do
}


/**
 * Constructor for manual use.
 *
 * @param rpn_namespace the RPN namespace
 * @param rpn_priority the RPN namespace
 */
rph_priority::rph_priority(uint16 rpn_namespace,
			uint8 rpn_priority)
		: qspec_param(PARAM_ID),
		  rpn_namespace(rpn_namespace), rpn_priority(rpn_priority) {

	// nothing to do
}


/**
 * Constructor for manual use.
 *
 * Allows to set the parameter ID. This is usually not required and should
 * be used with care if QSPEC-T compliant output is desired.
 *
 * @param param_id the parameter ID
 * @param rpn_namespace the RPN namespace
 * @param rpn_priority the RPN namespace
 */
rph_priority::rph_priority(uint16 param_id,
		uint16 rpn_namespace, uint8 rpn_priority)
		: qspec_param(param_id),
		  rpn_namespace(rpn_namespace), rpn_priority(rpn_priority) {

	// nothing to do
}


rph_priority::~rph_priority() {
	// nothing to do
}


rph_priority *rph_priority::new_instance() const {
	rph_priority *q = NULL;
	catch_bad_alloc( q = new rph_priority() );
	return q;
}


rph_priority *rph_priority::copy() const {
	rph_priority *q = NULL;
	catch_bad_alloc( q = new rph_priority(*this) );
	return q;
}


bool rph_priority::deserialize_body(NetMsg &msg, coding_t coding,
		uint16 body_length, IEErrorList &err, uint32 &bytes_read,
		bool skip) {

	bytes_read = 0;

	set_rpn_namespace( msg.decode16() );
	bytes_read += 2;

	set_rpn_priority( msg.decode8() );
	bytes_read += 1;

	msg.set_pos_r(1); // skip 1 reserved byte
	bytes_read += 1;

	return true; // success, there is nothing we could check
}


void rph_priority::serialize_body(NetMsg &msg, coding_t coding,
		uint32 &bytes_written) const {

	bytes_written = 0;

	msg.encode16( get_rpn_namespace() );
	bytes_written += 2;

	msg.encode8( get_rpn_priority() );
	bytes_written += 1;

	msg.encode8(0);
	bytes_written += 1;
}


bool rph_priority::check_body() const {
	return true;	// nothing to check
}


bool rph_priority::equals_body(const qspec_param &param) const {

	const rph_priority *other
		= dynamic_cast<const rph_priority *>(&param);

	return other != NULL
		&& get_rpn_namespace() == other->get_rpn_namespace()
		&& get_rpn_priority() == other->get_rpn_priority();
}


const char *rph_priority::get_ie_name() const {
	return ie_name;
}


ostream &rph_priority::print_attributes(ostream &os) const {
	return os << ", rpn_namespace=" << get_rpn_namespace()
		<< ", rpn_priority=" << get_rpn_priority();
}


/**
 * Returns the value of the RPN Namespace field.
 *
 * @return the value of the RPN Namespace field
 */
uint16 rph_priority::get_rpn_namespace() const {
	return rpn_namespace;
}


/**
 * Set the value of the RPN Namespace field.
 *
 * @param val the new value of the RPN Namespace field
 */
void rph_priority::set_rpn_namespace(uint16 val) {
	rpn_namespace = val;
}


/**
 * Returns the value of the RPN Priority field.
 *
 * @return the value of the RPN Priority field
 */
uint8 rph_priority::get_rpn_priority() const {
	return rpn_priority;
}


/**
 * Set the value of the RPN Priority field.
 *
 * @param val the new value of the RPN Priority field
 */
void rph_priority::set_rpn_priority(uint8 val) {
	rpn_priority = val;
}


// EOF
