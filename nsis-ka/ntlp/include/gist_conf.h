/// ----------------------------------------*- mode: C++; -*--
/// @file gist_conf.h
/// Configuration parameter class
/// ----------------------------------------------------------
/// $Id: gist_conf.h 6149 2011-05-18 12:54:22Z bless $
/// $HeadURL: https://svn.tm.kit.edu/nsis/ntlp/trunk/include/gist_conf.h $
// ===========================================================
//                      
// Copyright (C) 2008-2010, all rights reserved by
// - Institute of Telematics, Karlsruhe Institute of Technology
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
#ifndef _GIST_CONF__H
#define _GIST_CONF__H

#include "configpar.h"
#include "configpar_repository.h"

namespace ntlp {
  using namespace std;
  using namespace protlib;

  // 0 = global realm, 1 = protlib_realm

  const realm_id_t gist_realm= 2;

  enum gist_configpar_id_t {
    gistconf_invalid,
    gistconf_conffilename,
    gistconf_peerid,
    gistconf_udpport,
    gistconf_tcpport,
    gistconf_tlsport,
    gistconf_sctpport,              
    gistconf_localaddrv4,
    gistconf_localaddrv6,
    gistconf_home_netprefix,
    gistconf_home_address,
    gistconf_coa_interfaces,
    gistconf_homeagent_address,
    gistconf_homeagent_address_alt,
    gistconf_retrylimit,
    gistconf_retryperiod,
    gistconf_retryfactor,
    gistconf_rs_validity_time,
    gistconf_refresh_limit,
    gistconf_ma_hold_time,
    gistconf_secrets_refreshtime,
    gistconf_secrets_count,
    gistconf_secrets_length,
    gistconf_cookie_digest,
    gistconf_delayedstate,
    gistconf_confirmrequired,
    gistconf_senddatainquery,
    gistconf_reqhelloecho,
    gistconf_advertise_sctp,
    gistconf_verbose_error_responses,
    gistconf_send_rao,
    gistconf_strict_rao,
    gistconf_intercept_cmd,
    gistconf_dontstartqe,
    gistconf_debug_tp,
    gistconf_tls_client_cert,
    gistconf_tls_client_privkey,
    gistconf_tls_cacert,
    gistconf_maxparno
  };


  class gistconf {
  public:
	  // constructor should not do much, because we want to allow for later 
	  gistconf() : cfp_rep(0) {};

	  /// creates the repository singleton (must be explicitly called)
	  static void repository_init();

	  /// sets the repository pointer and registers all parameters
	  void setRepository(configpar_repository* cfp_rep= configpar_repository::instance());

	  string to_string();
	  string parname(gist_configpar_id_t configparid) {  return cfp_rep->getConfigPar(gist_realm, configparid)->getName(); }
	  const string& getDescription(gist_configpar_id_t configparid) const { return cfp_rep->getConfigPar(gist_realm, configparid)->getDescription(); }
	  const char *getUnitInfo(gist_configpar_id_t configparid) const { return cfp_rep->getConfigPar(gist_realm, configparid)->getUnitInfo(); }

	  /// register copy of configuration parameter instance
	  void registerPar(const configparBase& configparid) { cfp_rep->registerPar(configparid); }
	  /// register instance configuration parameter directly
	  void registerPar(configparBase* configparid) { cfp_rep->registerPar(configparid);  }

	  // this is just a convenience function
	  template <class T> void setpar(gist_configpar_id_t configparid, const T& value)
	  {
		  cfp_rep->setPar(gist_realm, configparid, value);
	  }

	  // this is just a convenience function
	  template <class T> T getpar(gist_configpar_id_t configparid)
	  {
		  return cfp_rep->getPar<T>(gist_realm, configparid);
	  }

	  // this is just a convenience function
	  template <class T> T& getparref(gist_configpar_id_t configparid)
	  {
		  return cfp_rep->getParRef<T>(gist_realm, configparid);
	  }


  private:
	  configpar_repository* cfp_rep;

	  // register all available parameters
	  // *** add new configuration parameters in this method ***
	  void registerAllPars();

  };

  // only declared here, must be defined somewhere else
  extern class gistconf gconf;

} // end namespace

#endif
