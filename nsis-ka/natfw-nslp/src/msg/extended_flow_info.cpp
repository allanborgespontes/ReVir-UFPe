/// ----------------------------------------*- mode: C++; -*--
/// @file extended_flow_info.cpp
/// The Extended Flow Information Object
/// ----------------------------------------------------------
/// $Id: extended_flow_info.cpp 2558 2007-04-04 15:17:16Z bless $
/// $HeadURL: https://svn.tm.kit.edu/nsis/natfw-nslp/trunk/src/msg/extended_flow_info.cpp $
// ===========================================================
//                      
// Copyright (C) 2005-2007, all rights reserved by
// - Institute of Telematics, Universitaet Karlsruhe (TH)
//
// More information and contact:
// https://projekte.tm.uka.de/trac/NSIS
//                      
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; version 2 of the License
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
//
// ===========================================================
#include "msg/extended_flow_info.h"
#include "logfile.h"


using namespace natfw::msg;


const char *const extended_flow_info::ie_name = "extended_flow_info";


/**
 * Default constructor.
 */
extended_flow_info::extended_flow_info()
		: natfw_object(OBJECT_TYPE, tr_mandatory),
		  rule_action(0), sub_ports(0) {

	// nothing to do
}


/**
 * Constructor for manual use.
 *
 * The rule_action parameter should be a value of rule_action_t, and the
 * sub_ports value currently must be either 0 or 1. Later draft revisions
 * might change this.
 *
 * @param rule_action the action of the policy rule
 * @param sub_ports the number of subsequent ports to reserve
 */
extended_flow_info::extended_flow_info(uint16 rule_action, uint16 sub_ports,
			treatment_t treatment)
		: natfw_object(OBJECT_TYPE, treatment),
		  rule_action(rule_action), sub_ports(sub_ports) {

	// nothing to do
}


extended_flow_info::~extended_flow_info() {
	// nothing to do
}


extended_flow_info *extended_flow_info::new_instance() const {
	extended_flow_info *q = NULL;
	catch_bad_alloc( q = new extended_flow_info() );
	return q;
}


extended_flow_info *extended_flow_info::copy() const {
	extended_flow_info *q = NULL;
	catch_bad_alloc( q = new extended_flow_info(*this) );
	return q;
}


bool extended_flow_info::deserialize_body(NetMsg &msg, uint16 body_length,
		IEErrorList &err, bool skip) {

	set_rule_action(msg.decode16());
	set_sub_ports(msg.decode16());

	// Check for invalid values.
	if ( ! check() ) {
		catch_bad_alloc( err.put( 
			new PDUSyntaxError(CODING, get_category(),
				get_object_type(), 0, msg.get_pos()-4)) );

		if ( ! skip )
			return false;
	}

	return true; // success
}


void extended_flow_info::serialize_body(NetMsg &msg) const {
	msg.encode16(get_rule_action());
	msg.encode16(get_sub_ports());
}


size_t extended_flow_info::get_serialized_size(coding_t coding) const {
	return HEADER_LENGTH + 4;
}


bool extended_flow_info::check_body() const {
	// check limits of the current draft revision
	if ( get_rule_action() > 2 || get_sub_ports() > 1 )
		return false;
	else if ( get_rule_action() == 0 ) // unset value
		return false;
	else
		return true;
}


bool extended_flow_info::equals_body(const natfw_object &obj) const {

	const extended_flow_info *other
		= dynamic_cast<const extended_flow_info *>(&obj);

	return other != NULL && get_rule_action() == other->get_rule_action()
		&& get_sub_ports() == other->get_sub_ports();
}


const char *extended_flow_info::get_ie_name() const {
	return ie_name;
}


ostream &extended_flow_info::print_attributes(ostream &os) const {
	return os << ", rule_action=" << get_rule_action()
		<< ", sub_ports=" << get_sub_ports();
}


/**
 * Returns the action for a policy rule.
 *
 * The rule_action_t enum can be used to test the returned values.
 *
 * @return the policy rule action
 */
uint16 extended_flow_info::get_rule_action() const {
	return rule_action;
}


/**
 * Sets the action for a policy rule.
 *
 * You can use the rule_action_t enum to generate valid rule values.
 *
 * @param val the session lifetime in seconds.
 */
void extended_flow_info::set_rule_action(uint16 action) {
	rule_action = action;
}


/**
 * Returns the number of subsequent reserved ports.
 *
 * @return the number of subsequent reserved ports.
 */
uint16 extended_flow_info::get_sub_ports() const {
	return sub_ports;
}


/**
 * Sets the number of subsequent reserved ports.
 *
 * @param num the number (currently only 0 and 1 are valid)
 */
void extended_flow_info::set_sub_ports(uint16 num) {
	sub_ports = num;
}

// EOF
