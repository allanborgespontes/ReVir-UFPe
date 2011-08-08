/// ----------------------------------------*- mode: C++; -*--
/// @file policy_rule.cpp
/// The policy rule classes.
/// ----------------------------------------------------------
/// $Id: policy_rule.cpp 3166 2008-07-11 07:28:45Z bless $
/// $HeadURL: https://svn.tm.kit.edu/nsis/natfw-nslp/trunk/src/policy_rule.cpp $
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
#include <assert.h>

#include "logfile.h"

#include "policy_rule.h"

using namespace natfw;


/*****************************************************************************
 *
 * The fw_policy_rule class.
 *
 *****************************************************************************/

/**
 * Constructor.
 */
fw_policy_rule::fw_policy_rule(fw_policy_rule::action_t action,
		const hostaddress &ds_addr, uint8 ds_prefix, uint16 ds_port,
		const hostaddress &dr_addr, uint8 dr_prefix, uint16 dr_port,
		uint8 protocol)
		: action(action),
		  ds_addr(ds_addr), ds_prefix(ds_prefix), ds_port(ds_port),
		  dr_addr(dr_addr), dr_prefix(dr_prefix), dr_port(dr_port),
		  protocol(protocol) {

	// nothing to do
}


/**
 * Create a deep copy of this object.
 */
fw_policy_rule *fw_policy_rule::copy() const {
	return new fw_policy_rule(*this);
}


std::ostream &natfw::operator<<(std::ostream &out, const fw_policy_rule &r) {
	const char *const names[] = { "allow", "deny" };

	return out << "[fw_policy_rule: " << names[r.get_action() - 1]
		<< " ds=" << r.get_ds_address() << '/' << int(r.get_ds_prefix())
		<< " ds_port=" << r.get_ds_port()
		<< " dr=" << r.get_dr_address() << '/' << int(r.get_dr_prefix())
		<< " dr_port=" << r.get_dr_port()
		<< " protocol=" << int(r.get_protocol())
		<< "]";
}


/*****************************************************************************
 *
 * The fw_policy_rule class.
 *
 *****************************************************************************/

/**
 * Constructor.
 */
nat_policy_rule::nat_policy_rule(const hostaddress &ds_addr, uint16 ds_port,
		const hostaddress &ext_addr, uint16 ext_port,
		const hostaddress &dr_addr, uint16 dr_port, uint8 protocol,
		type_t rule_type)
		: ds_addr(ds_addr), ds_port(ds_port),
		  ext_addr(ext_addr), ext_port(ext_port),
		  dr_addr(dr_addr), dr_port(dr_port),
		  protocol(protocol), rule_type(rule_type) {

	// nothing to do
}


/**
 * Create a deep copy of this object.
 */
nat_policy_rule *nat_policy_rule::copy() const {
	return new nat_policy_rule(*this);
}


std::ostream &natfw::operator<<(std::ostream &out, const nat_policy_rule &r) {
	static const char *const type_names[] = { "DEST_NAT", "SOURCE_NAT" };

	return out << "[nat_policy_rule:"
		<< " ds=" << r.get_ds_address()
		<< " ds_port=" << r.get_ds_port()
		<< " ext=" << r.get_external_address()
		<< " ext_port=" << r.get_external_port()
		<< " dr=" << r.get_dr_address()
		<< " dr_port=" << r.get_dr_port()
		<< " protocol=" << int(r.get_protocol())
		<< " rule_type=" << type_names[r.get_rule_type()]
		<< "]";
}


// EOF
