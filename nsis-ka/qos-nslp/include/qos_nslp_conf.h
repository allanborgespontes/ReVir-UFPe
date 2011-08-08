/// ----------------------------------------*- mode: C++; -*--
/// @file qos_nslp_conf.h
/// QoS NSLP configuration stuff
/// ----------------------------------------------------------
/// $Id: qos_nslp_conf.h 6198 2011-05-25 14:59:54Z bless $
/// $HeadURL: https://svn.tm.kit.edu/nsis/qos-nslp/trunk/include/qos_nslp_conf.h $
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

#ifndef _QOS_NSLP_CONF__H
#define _QOS_NSLP_CONF__H

#include "configpar.h"
#include "configpar_repository.h"

namespace qos_nslp {

using namespace std;
using namespace protlib;

// 0 = global realm, 1 = protlib_realm, 2= gist_realm

const realm_id_t qos_nslp_realm = 3;

enum qos_nslp_configpar_id_t {
	qosnslpconf_invalid,
	qosnslpconf_rmf_max_bw,
	qosnslpconf_refresh_period,
	qosnslpconf_request_retry,
	qosnslpconf_max_retry,
	qosnslpconf_rmf_vlsp_link_up_script_drain,
	qosnslpconf_rmf_vlsp_link_up_script_source,
	qosnslpconf_rmf_vlsp_link_down_script_drain,
	qosnslpconf_rmf_vlsp_link_down_script_source,
	qosnslpconf_sauth_hmac_verify,
	qosnslpconf_maxparno
};

class qos_nslp_conf {
public:
	// constructor should not do much, because we want to allow for later 
	qos_nslp_conf() : cfp_rep(0) {};

	/// creates the repository singleton (must be explicitly called)
	static void repository_init();

	/// sets the repository pointer and registers all parameters
	void setRepository(configpar_repository* cfp_rep= configpar_repository::instance());
	configpar_repository* getRepository() { return cfp_rep; }

	string to_string();
	string parname(qos_nslp_configpar_id_t configparid) {  return cfp_rep->getConfigPar(qos_nslp_realm, configparid)->getName(); }
	string parunitinfo(qos_nslp_configpar_id_t configparid) {  return cfp_rep->getConfigPar(qos_nslp_realm, configparid)->getUnitInfo(); }

	/// register copy of configuration parameter instance
	void registerPar(const configparBase& configparid) { cfp_rep->registerPar(configparid); }
	/// register instance configuration parameter directly
	void registerPar(configparBase* configparid) { cfp_rep->registerPar(configparid);  }


	// this is just a convenience function
	template <class T> void setpar(qos_nslp_configpar_id_t configparid, const T& value)
	{
		cfp_rep->setPar(qos_nslp_realm, configparid, value);
	}

	// this is just a convenience function
	template <class T> T getpar(qos_nslp_configpar_id_t configparid)
	{
		return cfp_rep->getPar<T>(qos_nslp_realm, configparid);
	}

	// this is just a convenience function
	template <class T> T& getparref(qos_nslp_configpar_id_t configparid)
	{
		return cfp_rep->getParRef<T>(qos_nslp_realm, configparid);
	}

private:
	configpar_repository* cfp_rep;

	// register all available parameters
	// *** add new configuration parameters in this method ***
	void registerAllPars();
}; // end class

// only declared here, must be defined somewhere else
extern qos_nslp_conf qosnslpconf;

} // end namespace
#endif
