/// ----------------------------------------*- mode: C++; -*--
/// @file natfw_config.cpp
/// Configuration for a NATFW instance.
/// ----------------------------------------------------------
/// $Id: natfw_config.cpp 4118 2009-07-16 16:13:10Z bless $
/// $HeadURL: https://svn.tm.kit.edu/nsis/natfw-nslp/trunk/src/natfw_config.cpp $
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

#include "natfw_config.h"

using namespace natfw;

void
natfw_config::repository_init() 
{
  DLog("natfw_config", "start - creating configuration parameter singleton");
  configpar_repository::create_instance(natfw::natfw_realm+1);
} // end namespace



/** sets the repository pointer and registers all parameters 
 *  (not done in constructor, because of allowing simulation hosts to initialize later)
 **/ 
void
natfw_config::setRepository(configpar_repository* cfp_rep)
{
	cfgpar_rep= cfp_rep;

	if (cfgpar_rep == 0)
		throw  configParExceptionNoRepository();
	
	// register the realm
	cfgpar_rep->registerRealm(natfw_realm, "natfw", natfwconf_maxparno);
	DLog("natfw_config", "registered natfw realm with realm id " << (int) natfw_realm);
	
	// now register all parameters
	registerAllPars();
}


void 
natfw_config::registerAllPars()
{
  DLog("natfw_config::registerAllPars", "starting registering natfw parameters.");

  // register all natfw parameters now
  registerPar( new configpar<string>(natfw_realm, natfwconf_conffilename, "config", "configuration file name", true, "nsis-ka.conf") );
  registerPar( new configpar<uint32>(natfw_realm, natfwconf_dispatcher_threads, "dispatcher-threads", "number of dispatcher threads", true, 1) );
  registerPar( new configpar<uint32>(natfw_realm, natfwconf_ni_max_session_lifetime, "ni-max-session-lifetime", "NI session lifetime in seconds", true, 30, "s") );
  registerPar( new configpar<uint32>(natfw_realm, natfwconf_ni_max_retries, "ni-max-retries", "NI max retries", true, 3) );
  registerPar( new configpar<uint32>(natfw_realm, natfwconf_ni_response_timeout, "ni-response-timeout", "NI response timeout", true, 2, "s") );
  registerPar( new configpar<uint32>(natfw_realm, natfwconf_nf_max_session_lifetime, "nf-max-session-lifetime", "NF max session lifetime in seconds", true, 60, "s") );
  registerPar( new configpar<uint32>(natfw_realm, natfwconf_nf_response_timeout, "nf-response-timeout", "NF response timeout", true, 2, "s") );
  registerPar( new configpar<bool>(natfw_realm, natfwconf_nf_is_nat, "nf-is-nat", "NF is NAT", true, false) );
  registerPar( new configpar<bool>(natfw_realm, natfwconf_nf_is_firewall, "nf-is-firewall", "NF is firewall", true, false) );
  registerPar( new configpar<bool>(natfw_realm, natfwconf_nf_is_edge_node, "nf-is-edge-node", "NF is edge node", true, false) );
  registerPar( new configpar<bool>(natfw_realm, natfwconf_nf_is_edge_nat, "nf-is-edge-nat", "NF is edge NAT", true, true) );
  registerPar( new configpar<bool>(natfw_realm, natfwconf_nf_is_edge_firewall, "nf-is-edge-firewall", "NF is edge firewall", true, true) );
  registerPar( new configpar<hostaddresslist_t>(natfw_realm, natfwconf_nf_private_networks, "nf-private-networks", "List of private networks inside", true, list<hostaddress>()) );
  registerPar( new configpar<hostaddress>(natfw_realm, natfwconf_nf_nat_public_address, "nf-nat-public-address", "External public IPv4 address", true, hostaddress()) );
  registerPar( new configpar<uint16>(natfw_realm, natfwconf_nf_nat_public_port_begin, "nf-nat-public-port-begin", "NF NAT public port range low bound", true, 10000) );
  registerPar( new configpar<uint16>(natfw_realm, natfwconf_nf_nat_public_port_end, "nf-nat-public-port-end", "NF NAT public port range upper bound", true, 20000) );
  registerPar( new configpar<bool>(natfw_realm, natfwconf_nf_install_policy_rules, "nf-install-policy-rules", "NF install policy rules", true, false) );
  registerPar( new configpar<uint32>(natfw_realm, natfwconf_nr_max_session_lifetime, "nr-max-session-lifetime", "NR max session lifetime in seconds", true, 60, "s") );
  registerPar( new configpar<uint32>(natfw_realm, natfwconf_nr_ext_session_lifetime, "nr-ext-session-lifetime", "NR ext session lifetime in seconds", true, 30, "s") );
  registerPar( new configpar<uint16>(natfw_realm, natfwconf_nr_ext_max_retries, "nr-ext-max-retries", "NR ext max retries", true, 3) );
  registerPar( new configpar<uint32>(natfw_realm, natfwconf_nr_ext_response_timeout, "nr-ext-response-timeout", "NR ext response timeout", true, 2) );

  DLog("natfw_config::registerAllPars", "finished registering natfw parameters.");
}


// EOF
