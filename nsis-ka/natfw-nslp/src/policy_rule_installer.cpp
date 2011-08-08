/// ----------------------------------------*- mode: C++; -*--
/// @file policy_rule_installer.cpp
/// The policy_rule_installer class.
/// ----------------------------------------------------------
/// $Id: policy_rule_installer.cpp 2558 2007-04-04 15:17:16Z bless $
/// $HeadURL: https://svn.tm.kit.edu/nsis/natfw-nslp/trunk/src/policy_rule_installer.cpp $
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

#include "address.h"
#include "logfile.h"

#include "policy_rule_installer.h"


using namespace natfw;
using namespace protlib::log;


#define LogError(msg) Log(ERROR_LOG, LOG_NORMAL, \
	"nop_policy_rule_installer", msg)
#define LogWarn(msg) Log(WARNING_LOG, LOG_NORMAL, \
	"nop_policy_rule_installer", msg)
#define LogInfo(msg) Log(INFO_LOG, LOG_NORMAL, \
	"nop_policy_rule_installer", msg)
#define LogDebug(msg) Log(DEBUG_LOG, LOG_NORMAL, \
	"nop_policy_rule_installer", msg)

#define LogUnimp(msg) Log(ERROR_LOG, LOG_UNIMP, "nop_policy_rule_installer", \
	msg << " at " << __FILE__ << ":" << __LINE__)


nop_policy_rule_installer::nop_policy_rule_installer(
		natfw_config *conf) throw () 
		: policy_rule_installer(conf) {

	// nothing to do
}


nop_policy_rule_installer::~nop_policy_rule_installer() throw () {
	// nothing to do
}


void nop_policy_rule_installer::setup() throw (policy_rule_installer_error) {

	LogDebug("NOP: setup()");
}


void nop_policy_rule_installer::check(const nat_policy_rule *nat_rule,
			const fw_policy_rule *fw_rule)
		throw (policy_rule_installer_error) {

	LogDebug("NOP: check()");
}


void nop_policy_rule_installer::install(const nat_policy_rule *nat_rule,
			const fw_policy_rule *fw_rule)
		throw (policy_rule_installer_error) {

	if ( nat_rule != NULL )
		LogDebug("NOP: installing NAT policy rule " << *nat_rule);

	if ( fw_rule != NULL )
		LogDebug("NOP: installing firewall policy rule " << *fw_rule);
}


void nop_policy_rule_installer::remove(const nat_policy_rule *nat_rule,
			const fw_policy_rule *fw_rule)
		throw (policy_rule_installer_error) {

	if ( nat_rule != NULL )
		LogDebug("NOP: removing NAT policy rule " << *nat_rule);

	if ( fw_rule != NULL )
		LogDebug("NOP: removing firewall policy rule " << *fw_rule);
}


void nop_policy_rule_installer::remove_all()
		throw (policy_rule_installer_error) {

	LogDebug("NOP: removing all installed policy rules");
}

// EOF
