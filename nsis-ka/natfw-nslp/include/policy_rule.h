/// ----------------------------------------*- mode: C++; -*--
/// @file policy_rule.h
/// The policy rule classes.
/// ----------------------------------------------------------
/// $Id: policy_rule.h 2558 2007-04-04 15:17:16Z bless $
/// $HeadURL: https://svn.tm.kit.edu/nsis/natfw-nslp/trunk/include/policy_rule.h $
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
#ifndef NATFW__POLICY_RULE_H
#define NATFW__POLICY_RULE_H

#include "protlib_types.h"
#include "address.h"


namespace natfw {
    using protlib::uint8;
    using protlib::uint16;
    using protlib::uint32;
    using protlib::hostaddress;

/**
 * A firewall policy rule.
 *
 * This consists of a filter and an action part. The filter part has 
 * addresses, ports, etc., and the action part decides what to do with
 * packages matching the filter part.
 *
 * The following statement is made by this rule: Allow/Deny all packets
 * coming from ds_addr:ds_port and going to dr_addr::dr_port, and vice versa.
 *
 * The ports and protocol may be 0. In this case they are wildcarded and
 * data traffic from any port or protocol is allowed/denied (depending on
 * the action).
 *
 * A policy rule is just an abstraction. It has to be mapped to a concrete
 * firewall rule, which is vendor-dependent.
 */
class fw_policy_rule {

  public:
	enum action_t {
		ACTION_ALLOW	= 1,
		ACTION_DENY	= 2
	};

	fw_policy_rule(action_t action,
		const hostaddress &ds_addr, uint8 ds_prefix, uint16 ds_port,
		const hostaddress &dr_addr, uint8 dr_prefix, uint16 dr_port,
		uint8 protocol);

	virtual ~fw_policy_rule() { }

	fw_policy_rule *copy() const;

	action_t get_action() const { return action; }

	hostaddress get_ds_address() const { return ds_addr; }
	uint8 get_ds_prefix() const { return ds_prefix; }
	uint16 get_ds_port() const { return ds_port; }

	hostaddress get_dr_address() const { return dr_addr; }
	uint8 get_dr_prefix() const { return dr_prefix; }
	uint16 get_dr_port() const { return dr_port; }

	uint8 get_protocol() const { return protocol; }

  private:
	action_t action;

	hostaddress ds_addr;
	uint8 ds_prefix;
	uint16 ds_port;

	hostaddress dr_addr;
	uint8 dr_prefix;
	uint16 dr_port;

	uint8 protocol;
};

std::ostream &operator<<(std::ostream &out, const fw_policy_rule &r);


/**
 * A NAT policy rule.
 *
 * This policy rule of type TYPE_DEST_NAT makes the following two statements:
 *
 * Rewrite the destination address and port of all packets coming from
 * ds_addr:ds_port to ext_addr:ext_port. The new destination should be
 * dr_addr:dr_port.
 *
 * Rewrite the source address and port of all packets coming from
 * dr_addr:dr_port to the externally reserved ext_addr:ext_port.
 *
 * The protocol value 0 is treated as a wildcard, otherwise only packets of
 * the specified L4 protocol are rewritten.
 *
 * A policy rule is just an abstraction. It has to be mapped to a concrete
 * NAT rule, which is vendor-dependent.
 */
class nat_policy_rule {

  public:
	enum type_t {
		TYPE_DEST_NAT,		// rewrite destination address
		TYPE_SOURCE_NAT		// rewrite source address
	};

	nat_policy_rule(const hostaddress &ds_addr, uint16 ds_port,
		const hostaddress &ext_addr, uint16 ext_port,
		const hostaddress &dr_addr, uint16 dr_port, uint8 protocol,
		type_t rule_type);

	virtual ~nat_policy_rule() { }

	nat_policy_rule *copy() const;

	hostaddress get_ds_address() const { return ds_addr; }
	uint16 get_ds_port() const { return ds_port; }

	hostaddress get_external_address() const { return ext_addr; }
	uint16 get_external_port() const { return ext_port; }

	hostaddress get_dr_address() const { return dr_addr; }
	uint16 get_dr_port() const { return dr_port; }

	uint8 get_protocol() const { return protocol; }

	type_t get_rule_type() const { return rule_type; }

  private:
	hostaddress ds_addr;
	uint16 ds_port;

	hostaddress ext_addr;
	uint16 ext_port;

	hostaddress dr_addr;
	uint16 dr_port;

	uint8 protocol;

	type_t rule_type;
};

std::ostream &operator<<(std::ostream &out, const nat_policy_rule &r);


} // namespace natfw

#endif // NATFW__POLICY_RULE_H
