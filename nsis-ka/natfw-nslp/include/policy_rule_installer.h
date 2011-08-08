/// ----------------------------------------*- mode: C++; -*--
/// @file policy_rule_installer.h
/// The policy_rule_installer classes.
/// ----------------------------------------------------------
/// $Id: policy_rule_installer.h 2558 2007-04-04 15:17:16Z bless $
/// $HeadURL: https://svn.tm.kit.edu/nsis/natfw-nslp/trunk/include/policy_rule_installer.h $
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
#ifndef NATFW__POLICY_RULE_INSTALLER_H
#define NATFW__POLICY_RULE_INSTALLER_H


#include "session.h"
#include "policy_rule.h"

#include "natfw_config.h"


namespace natfw {

/**
 * An exception to be thrown if the policy_rule_installer failed.
 */
class policy_rule_installer_error : public request_error {
  public:
	policy_rule_installer_error(const std::string &msg,
		uint8 severity=0, uint8 response_code=0) throw ()
		: request_error(msg, severity, response_code) { }
	virtual ~policy_rule_installer_error() throw () { }
};

inline std::ostream &operator<<(
		std::ostream &out, const policy_rule_installer_error &e) {

	return out << e.get_msg();
}


/**
 * An abstract interface to the operating system's NAT/firewall package.
 *
 * Callers pass NAT and firewall policy rules to install() and this class
 * maps it to operating system specific rules. The installed rules can later
 * be removed using remove().
 *
 * Subclasses have to implement locking if necessary.
 */
class policy_rule_installer {

  public:
	policy_rule_installer(natfw_config *conf) throw () : config(conf) { }
	virtual ~policy_rule_installer() throw () { }

	/**
	 * Prepare the external NAT/firewall package for usage.
	 */
	virtual void setup() throw (policy_rule_installer_error) = 0;

	/**
	 * Check if the given policy rules are correct and supported.
	 *
	 * If they are not, an exception is thrown.
	 */
	virtual void check(const nat_policy_rule *nat_rule,
				const fw_policy_rule *fw_rule)
		throw (policy_rule_installer_error) = 0;

	/**
	 * Install the given policy rules.
	 * 
	 * Subclasses have to use operating system dependent code to access the
	 * local NAT/firewall package. After calling this method, the rules are
	 * in effect and data traffic will be treated as requested. The passed
	 * policy rule objects will be deleted.
	 */
	virtual void install(const nat_policy_rule *nat_rule,
				const fw_policy_rule *fw_rule)
		throw (policy_rule_installer_error) = 0;

	/**
	 * Remove the given policy rules.
	 *
	 * After calling this method, the previously installed rules are no
	 * longer in effect. Depending on the policy rule, this could interrupt
	 * data traffic.
	 *
	 * Note that this is the reverse operation to install(). Only previously
	 * installed policy rules may be deleted! The passed policy rule objects
	 * will be deleted.
	 */
	virtual void remove(const nat_policy_rule *nat_rule,
				const fw_policy_rule *fw_rule)
		throw (policy_rule_installer_error) = 0;

	/**
	 * Remove all previously installed policy rules.
	 *
	 * This has the same effect as calling remove() for all installed
	 * policy rules.
	 */
	virtual void remove_all() throw (policy_rule_installer_error) = 0;

  private:
	natfw_config *config;
};


/**
 * A policy rule installer which does nothing.
 *
 * Basically, all its methods are no-ops, but the requested actions are logged.
 */
class nop_policy_rule_installer : public policy_rule_installer {
  public:
	nop_policy_rule_installer(natfw_config *conf) throw ();
	virtual ~nop_policy_rule_installer() throw ();

	void setup() throw (policy_rule_installer_error);

	virtual void check(const nat_policy_rule *nat_rule,
				const fw_policy_rule *fw_rule)
		throw (policy_rule_installer_error);

	virtual void install(const nat_policy_rule *nat_rule,
				const fw_policy_rule *fw_rule)
		throw (policy_rule_installer_error);

	virtual void remove(const nat_policy_rule *nat_rule,
				const fw_policy_rule *fw_rule)
		throw (policy_rule_installer_error);

	virtual void remove_all() throw (policy_rule_installer_error);
};


/**
 * A policy rule installer for iptables.
 *
 * This implementation is Linux-specific and uses the iptables binary to
 * install and remove policy rules.
 *
 * This class is currently stateless, so no locking is required. Serialization 
 * of iptables calls is done on kernel level, so multithreaded calls to the
 * iptables binary are safe.
 */
class iptables_policy_rule_installer : public policy_rule_installer {
  public:
	iptables_policy_rule_installer(natfw_config *conf) throw ();
	virtual ~iptables_policy_rule_installer() throw ();

	void setup() throw (policy_rule_installer_error);

	virtual void check(const nat_policy_rule *nat_rule,
				const fw_policy_rule *fw_rule)
		throw (policy_rule_installer_error);

	virtual void install(const nat_policy_rule *nat_rule,
				const fw_policy_rule *fw_rule)
		throw (policy_rule_installer_error);

	virtual void remove(const nat_policy_rule *nat_rule,
				const fw_policy_rule *fw_rule)
		throw (policy_rule_installer_error);

	virtual void remove_all() throw (policy_rule_installer_error);

  private:
	std::string create_expression(const std::string &prefix,
		const fw_policy_rule *fw_rule, bool downstream) const throw ();

	std::string create_expression(const std::string &prefix,
		const nat_policy_rule *nat_rule) const throw ();
};


} // namespace natfw

#endif // NATFW__POLICY_RULE_INSTALLER_H
