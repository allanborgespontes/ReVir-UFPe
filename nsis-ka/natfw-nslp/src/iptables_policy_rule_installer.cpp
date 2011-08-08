/// ----------------------------------------*- mode: C++; -*--
/// @file iptables_policy_rule_installer.cpp
/// The iptables_policy_rule_installer class.
/// ----------------------------------------------------------
/// $Id: iptables_policy_rule_installer.cpp 2558 2007-04-04 15:17:16Z bless $
/// $HeadURL: https://svn.tm.kit.edu/nsis/natfw-nslp/trunk/src/iptables_policy_rule_installer.cpp $
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
#include <sstream>
#include <cstdlib>
#include <sys/types.h>
#include <unistd.h>
#include <assert.h>

#include "address.h"
#include "logfile.h"

#include "policy_rule_installer.h"


using namespace natfw;
using natfw::msg::information_code;
using namespace protlib::log;


#define LogError(msg) Log(ERROR_LOG, LOG_NORMAL, \
	"iptables_policy_rule_installer", msg)
#define LogWarn(msg) Log(WARNING_LOG, LOG_NORMAL, \
	"iptables_policy_rule_installer", msg)
#define LogInfo(msg) Log(INFO_LOG, LOG_NORMAL, \
	"iptables_policy_rule_installer", msg)
#define LogDebug(msg) Log(DEBUG_LOG, LOG_NORMAL, \
	"iptables_policy_rule_installer", msg)

#define LogUnimp(msg) Log(ERROR_LOG, LOG_UNIMP, \
	"iptables_policy_rule_installer", \
	msg << " at " << __FILE__ << ":" << __LINE__)


iptables_policy_rule_installer::iptables_policy_rule_installer(
		natfw_config *conf) throw () 
		: policy_rule_installer(conf) {

	// nothing to do
}


iptables_policy_rule_installer::~iptables_policy_rule_installer() throw () {
	// nothing to do
}


void iptables_policy_rule_installer::setup()
		throw (policy_rule_installer_error) {

	/*
	 * Create a new chain 'natfwd' and append it to the FORWARD,
	 * chain. This way, all non-NATFW rules are executed first.
	 * Similarly, add the natfw_dnat and natfw_snat chains.
	 */
	std::string cmd = "iptables -N natfwd_filter"
		" && iptables -A FORWARD -j natfwd_filter"
		" && iptables -t nat -N natfwd_dnat"
		" && iptables -t nat -A PREROUTING -j natfwd_dnat"
		" && iptables -t nat -N natfwd_snat"
		" && iptables -t nat -A POSTROUTING -j natfwd_snat";

	LogDebug("setup(): " << cmd);
	
	int ret = system(cmd.c_str());
	if ( ret != 0 ) {
		LogDebug("cannot setup iptables");
		exit(1);	// no point continueing
	}
}


/**
 * Create an iptables filter command line.
 *
 * Two different expressions can be created: One for the downstream and one for
 * the upstream direction.
 */
std::string iptables_policy_rule_installer::create_expression(
		const std::string &prefix,
		const fw_policy_rule *fw_rule, bool downstream) const throw () {

	static const char *action_names[] = { "ACCEPT", "DROP" };

	std::ostringstream expr;

	// The -p flag must appear *before* the --sport and --dport arguments!
	expr << prefix << " -p " << int(fw_rule->get_protocol());

	if ( downstream ) {
		expr << " -s " << fw_rule->get_ds_address()
				<< '/' << int(fw_rule->get_ds_prefix())
			<< " -d " << fw_rule->get_dr_address()
				<< '/' << int(fw_rule->get_dr_prefix())
			<< " --sport " << fw_rule->get_ds_port()
			<< " --dport " << fw_rule->get_dr_port();
	}
	else {
		expr << " -d " << fw_rule->get_ds_address()
				<< '/' << int(fw_rule->get_ds_prefix())
			<< " -s " << fw_rule->get_dr_address()
				<< '/' << int(fw_rule->get_dr_prefix())
			<< " --dport " << fw_rule->get_ds_port()
			<< " --sport " << fw_rule->get_dr_port();
	}

	expr << " -j " << action_names[fw_rule->get_action()-1];

	return expr.str();
}


/**
 * Create an iptables NAT command line.
 */
std::string iptables_policy_rule_installer::create_expression(
		const std::string &prefix,
		const nat_policy_rule *r) const throw () {

	std::ostringstream expr;

	// The -p flag must appear *before* the --sport and --dport arguments!

	expr << prefix
		<< " -p " << int(r->get_protocol())
		<< " -s " << r->get_ds_address()
		<< " --sport " << r->get_ds_port();

	if ( r->get_rule_type() == nat_policy_rule::TYPE_DEST_NAT ) {
		expr
			<< " -d " << r->get_external_address()
			<< " --dport " << r->get_external_port()
			<< " -j DNAT --to-destination "
			<< r->get_dr_address()
			<< ":" << r->get_dr_port();
	}
	else if ( r->get_rule_type() == nat_policy_rule::TYPE_SOURCE_NAT ) {
		expr
			<< " -d " << r->get_dr_address()
			<< " --dport " << r->get_dr_port()
			<< " -j SNAT --to-source "
			<< r->get_external_address()
			<< ":" << r->get_external_port();
	}
	else
		assert( false );

	return expr.str();
}


void iptables_policy_rule_installer::check(const nat_policy_rule *nat_rule,
			const fw_policy_rule *fw_rule)
		throw (policy_rule_installer_error) {

	if ( fw_rule->get_protocol() == 0 )
		throw policy_rule_installer_error("IP protocol has to be set",
			information_code::sc_signaling_session_failures,
			information_code::sigfail_plain_ip_not_permitted);
}


void iptables_policy_rule_installer::install(const nat_policy_rule *nat_rule,
		const fw_policy_rule *fw_rule)
		throw (policy_rule_installer_error) {

	if ( nat_rule != NULL ) {
		std::string cmd;

		if ( nat_rule->get_rule_type()
				== nat_policy_rule::TYPE_DEST_NAT )
			cmd  = create_expression(
				"iptables -t nat -A natfwd_dnat", nat_rule);
		else
			cmd  = create_expression(
				"iptables -t nat -A natfwd_snat", nat_rule);

		LogDebug("install command: " << cmd);

		int ret = system(cmd.c_str());
		if ( ret != 0 ) {
			LogError("failed to install NAT rule");
		}
	}

	if ( fw_rule != NULL ) {
		std::string cmd1 = create_expression(
				"iptables -A natfwd_filter", fw_rule, true);

		LogDebug("install command: " << cmd1);
		int ret = system(cmd1.c_str());
		if ( ret != 0 ) {
			LogError("failed to install downstream firewall rule");
		}

		std::string cmd2 = create_expression(
				"iptables -A natfwd_filter", fw_rule, false);

		LogDebug("install command: " << cmd2);
		ret = system(cmd2.c_str());
		if ( ret != 0 ) {
			LogError("failed to install upstream firewall rule");
		}
	}
}


void iptables_policy_rule_installer::remove(const nat_policy_rule *nat_rule,
		const fw_policy_rule *fw_rule)
		throw (policy_rule_installer_error) {

	if ( nat_rule != NULL ) {
		std::string cmd;

		if ( nat_rule->get_rule_type()
				== nat_policy_rule::TYPE_DEST_NAT )
			cmd  = create_expression(
				"iptables -t nat -D natfwd_dnat", nat_rule);
		else
			cmd  = create_expression(
				"iptables -t nat -D natfwd_snat", nat_rule);

		LogDebug("remove command: " << cmd);

		int ret = system(cmd.c_str());
		if ( ret != 0 ) {
			LogError("failed to remove NAT rule");
		}
	}

	if ( fw_rule != NULL ) {
		std::string cmd1 = create_expression(
				"iptables -D natfwd_filter", fw_rule, true);

		LogDebug("remove command: " << cmd1);
		int ret = system(cmd1.c_str());
		if ( ret != 0 ) {
			LogError("failed to remove downstream firewall rule");
		}

		std::string cmd2 = create_expression(
				"iptables -D natfwd_filter", fw_rule, false);

		LogDebug("remove command: " << cmd2);
		ret = system(cmd2.c_str());
		if ( ret != 0 ) {
			LogError("failed to remove upstream firewall rule");
		}
	}
}


void iptables_policy_rule_installer::remove_all()
		throw (policy_rule_installer_error) {

	/*
	 * Delete all references to the 'natfwd' chain, then delete it.
	 */
	std::string cmd = "iptables -D FORWARD -j natfwd_filter"
			" && iptables -F natfwd_filter"
			" && iptables -X natfwd_filter"
			" && iptables -t nat -D PREROUTING -j natfwd_dnat"
			" && iptables -t nat -F natfwd_dnat"
			" && iptables -t nat -X natfwd_dnat"
			" && iptables -t nat -D POSTROUTING -j natfwd_snat"
			" && iptables -t nat -F natfwd_snat"
			" && iptables -t nat -X natfwd_snat";

	LogDebug("remove_all(): " << cmd);
}

// EOF
