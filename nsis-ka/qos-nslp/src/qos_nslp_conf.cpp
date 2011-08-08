/// ----------------------------------------*- mode: C++; -*--
/// @file qos_nslp_conf.cpp
/// QoS NSLP configuration methods
/// ----------------------------------------------------------
/// $Id: qos_nslp_conf.cpp 6198 2011-05-25 14:59:54Z bless $
/// $HeadURL: https://svn.tm.kit.edu/nsis/qos-nslp/trunk/src/qos_nslp_conf.cpp $
// ===========================================================
//                      
// Copyright (C) 2005-2010, all rights reserved by
// - Institute of Telematics, Karlsruhe Institute of Technology
//
// More information and contact:
// https://projekte.tm.uka.de/trac/NSIS
//                      
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; version 2 of the License
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
// 02110-1301 USA.
//
// ===========================================================

#include "qos_nslp_conf.h"
#include "nslp_session_context_map.h"

using namespace protlib;
using namespace std;
using namespace qos_nslp;

void
qos_nslp_conf::repository_init() {
	DLog("qos_nslp_conf", "start - creating configuration parameter singleton");
	configpar_repository::create_instance(qos_nslp::qos_nslp_realm+1);
}

/** sets the repository pointer and registers all parameters 
 *  (not done in constructor, because of allowing simulation hosts to initialize later)
 **/ 
void
qos_nslp_conf::setRepository(configpar_repository* givencfp_rep)
{
	if (givencfp_rep == 0)
		throw  configParExceptionNoRepository();

	// set internal pointer to given repository pointer
	cfp_rep= givencfp_rep;

	// register the realm
	//configpar_repository::instance()->registerRealm(qos_nslp_realm, "qos-nslp", qosnslpconf_maxparno);
	cfp_rep->registerRealm(qos_nslp_realm, "qos-nslp", qosnslpconf_maxparno);
	DLog("qos_nslp_conf", "registered qos_nslp realm with realm id " << (int) qos_nslp_realm);
	
	// now register all parameters
	registerAllPars();
}


void
qos_nslp_conf::registerAllPars()
{
	DLog("qosnslp::registerAllPars", "starting registering qos nslp parameters.");

	// register all qos nslp parameters now
	cfp_rep->registerPar( new configpar<uint32>(qos_nslp_realm, qosnslpconf_rmf_max_bw, "rmf-max-bw", "Maximum bandwidth that could be admitted for a link", true, 100000000, "bps") );
	cfp_rep->registerPar( new configpar<uint32>(qos_nslp_realm, qosnslpconf_refresh_period, "refresh-period", "Refresh period", true, 30, "s"));
	cfp_rep->registerPar( new configpar<uint32>(qos_nslp_realm, qosnslpconf_request_retry, "request-retry", "Wait interval before initial retransmit of the message", true, 2, "s"));
	cfp_rep->registerPar( new configpar<uint32>(qos_nslp_realm, qosnslpconf_max_retry, "max-retry-counter", "Number of maximum retries", true, 3));
	cfp_rep->registerPar( new configpar<string>(qos_nslp_realm, qosnslpconf_rmf_vlsp_link_up_script_drain, "rmf-vlsp-vlink-up-script-drain", "Script to be called when getting a link setup request at the drain side", true) );
	cfp_rep->registerPar( new configpar<string>(qos_nslp_realm, qosnslpconf_rmf_vlsp_link_up_script_source, "rmf-vlsp-vlink-up-script-source", "Script to be called when getting a link setup response at the source side", true) );
	cfp_rep->registerPar( new configpar<string>(qos_nslp_realm, qosnslpconf_rmf_vlsp_link_down_script_drain, "rmf-vlsp-vlink-down-script-drain", "Script to be called when getting a link tear down request at the drain side", true) );
	cfp_rep->registerPar( new configpar<string>(qos_nslp_realm, qosnslpconf_rmf_vlsp_link_down_script_source, "rmf-vlsp-vlink-down-script-source", "Script to be called when getting a link tear down response at the source side", true) );
	cfp_rep->registerPar( new configpar<bool>(qos_nslp_realm, qosnslpconf_sauth_hmac_verify, "session-auth-hmac-verify", "Enable verification of an incoming session authorization object", true, true) );

	DLog("qosnslp::registerAllPars", "finished registering qos nslp parameters.");
}


string
qos_nslp_conf::to_string()
{
	ostringstream os;

	os << parname(qosnslpconf_rmf_max_bw) << "= " << getpar<uint32>(qosnslpconf_rmf_max_bw) <<  parunitinfo(qosnslpconf_rmf_max_bw) << endl;
	os << parname(qosnslpconf_refresh_period) << "= " << getpar<uint32>(qosnslpconf_refresh_period) <<  parunitinfo(qosnslpconf_refresh_period) << endl;
	os << parname(qosnslpconf_request_retry) << "= " << getpar<uint32>(qosnslpconf_request_retry) <<  parunitinfo(qosnslpconf_request_retry) << endl;
	os << parname(qosnslpconf_max_retry) << "= " << getpar<uint32>(qosnslpconf_max_retry) << endl;

	return os.str();
}



