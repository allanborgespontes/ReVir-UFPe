/*
 * slack_term.cpp - The QSPEC Slack Term Parameter
 *
 * $Id: slack_term.cpp 4593 2009-10-20 14:30:51Z roehricht $
 * $HeadURL: https://svn.tm.kit.edu/nsis/qspec/trunk/src/slack_term.cpp $
 */
#include "slack_term.h"
#include "logfile.h"


using namespace qspec;



/**
 * The officially assigned parameter ID.
 */
const uint16 slack_term::PARAM_ID = 7;

const char *const slack_term::ie_name = "slack_term";


/**
 * Default constructor.
 */
slack_term::slack_term()
		: qspec_param(PARAM_ID), value(0) {

	// nothing to do
}


/**
 * Constructor for manual use.
 *
 * @param value the value of the Slack Term Parameter
 */
slack_term::slack_term(uint32 value)
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
 * @param value the value of the Slack Term Parameter
 */
slack_term::slack_term(uint16 param_id, uint32 value)
		: qspec_param(param_id), value(value) {

	set_mandatory(false);
}


slack_term::~slack_term() {
	// nothing to do
}


slack_term *slack_term::new_instance() const {
	slack_term *q = NULL;
	catch_bad_alloc( q = new slack_term() );
	return q;
}


slack_term *slack_term::copy() const {
	slack_term *q = NULL;
	catch_bad_alloc( q = new slack_term(*this) );
	return q;
}


bool slack_term::deserialize_body(NetMsg &msg, coding_t coding,
		uint16 body_length, IEErrorList &err, uint32 &bytes_read,
		bool skip) {

	set_value(msg.decode32());
	bytes_read = 4;

	return true; // success (always successful, all values are valid)
}


void slack_term::serialize_body(NetMsg &msg, coding_t coding,
		uint32 &bytes_written) const {

	msg.encode32(get_value());
	bytes_written = 4;
}


bool slack_term::check_body() const {
	return true;	// nothing to check
}


bool slack_term::equals_body(const qspec_param &param) const {

	const slack_term *other
		= dynamic_cast<const slack_term *>(&param);

	return other != NULL && get_value() == other->get_value();
}


const char *slack_term::get_ie_name() const {
	return ie_name;
}


ostream &slack_term::print_attributes(ostream &os) const {
	return os << ", value=" << get_value();
}


/**
 * Returns the value of the Slack Term field.
 *
 * @return the value of the Slack Term field
 */
uint32 slack_term::get_value() const {
	return value;
}


/**
 * Set the value of the Slack Term field.
 *
 * @param val the new value of the Slack Term field
 */
void slack_term::set_value(uint32 val) {
	value = val;
}


// EOF
