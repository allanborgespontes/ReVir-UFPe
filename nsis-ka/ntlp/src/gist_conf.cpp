/// ----------------------------------------*- mode: C++; -*--
/// @file gist_conf.cpp
/// GIST configuration methods
/// ----------------------------------------------------------
/// $Id: gist_conf.cpp 6149 2011-05-18 12:54:22Z bless $
/// $HeadURL: https://svn.tm.kit.edu/nsis/ntlp/trunk/src/gist_conf.cpp $
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
#include "ntlp_global_constants.h"

#include <gist_conf.h>

#include <sstream>

#include "nli.h"

namespace ntlp {

  using namespace protlib;
  using namespace std;




/// register realm and register all used GIST parameters
void
gistconf::registerAllPars()
{
  DLog("gistconf::registerAllPars", "starting registering gist parameters.");

  // register all gist parameters now
  // *** add new configuration parameters here ***
  registerPar( new configpar<string>(gist_realm, gistconf_conffilename, "config", "configuration file name", false, "nsis-ka.conf") );
  registerPar( new configpar<peer_identity>(gist_realm, gistconf_peerid, "peerid", "this node's peer ID", true) );
  registerPar( new configpar<uint16>(gist_realm, gistconf_udpport, "udp-port", "UDP receive port", false, GIST_default_port) );
  registerPar( new configpar<uint16>(gist_realm, gistconf_tcpport, "tcp-port", "TCP listen port",       false, GIST_default_port) );
  registerPar( new configpar<uint16>(gist_realm, gistconf_tlsport, "tls-port", "TLS/TCP listen port",   false, GIST_default_port+1) );
  registerPar( new configpar<uint16>(gist_realm, gistconf_sctpport,"sctp-port","SCTP listen port",      false, GIST_default_port) );
  registerPar( new configpar< hostaddresslist_t >(gist_realm, gistconf_localaddrv4, "localaddr-v4", "Local IPv4 listen addresses", true, list<hostaddress>() ) );
  registerPar( new configpar< hostaddresslist_t >(gist_realm, gistconf_localaddrv6, "localaddr-v6", "Local IPv6 listen addresses", true, list<hostaddress>() ) );
  registerPar( new configpar< netaddress >(gist_realm, gistconf_home_netprefix, "home-netprefix", "Home Network Prefix (Mobility)", true) );
  registerPar( new configpar< hostaddress >(gist_realm, gistconf_home_address, "home-address", "Home Address (Mobility)", true) );
  registerPar( new configpar< string >(gist_realm, gistconf_coa_interfaces, "coa-interfaces", "Care-Of Interfaces (Mobility)", true) );
  registerPar( new configpar< hostaddress >(gist_realm, gistconf_homeagent_address, "homeagent-address", "Home Agent Address (Mobility)", true) );
  registerPar( new configpar< hostaddress >(gist_realm, gistconf_homeagent_address_alt, "homeagent-address-alt", "Home Agent Alternative Address (Mobility)", true) );
  registerPar( new configpar<uint32>(gist_realm, gistconf_retrylimit,  "retry-limit", "Upper limit for retry limit threshold (ms)", true, retrylimit_default, "ms") );
  registerPar( new configpar<uint32>(gist_realm, gistconf_retryperiod, "retry-period", "Retries will be started initially after this period (ms)", true, retryperiod_default, "ms") );
  registerPar( new configpar<float>( gist_realm, gistconf_retryfactor, "retry-factor", "limit up to which random values will be drawn, target value is 0.5*x", true, retryfactor_default) );

  registerPar( new configpar<uint32>(gist_realm, gistconf_rs_validity_time, "state-lifetime", "how long (ms) is a routing state valid by default", true, rs_validity_time_default, "ms") );
  registerPar( new configpar<uint32>(gist_realm, gistconf_refresh_limit,    "refresh-limit", "upper limit for the refresh period (ms), will trigger GIST probing every x milliseconds", true, refreshtime_default, "ms") );
  registerPar( new configpar<uint32>(gist_realm, gistconf_ma_hold_time,     "ma-hold-time", "default time to keep an MA open (ms)", true, ma_hold_time_default, "ms") );
  registerPar( new configpar<uint32>(gist_realm, gistconf_secrets_refreshtime, "secrets-refreshtime", "Local secrets rollover time (s)", true, secrets_refreshtime_default, "s") );
  registerPar( new configpar<uint32>(gist_realm, gistconf_secrets_count,   "secrets-count", "Amount of local secrets", false, secrets_count_default) );
  registerPar( new configpar<uint16>(gist_realm, gistconf_secrets_length,  "secrets-length","Length of local secrets in bit", false, secrets_length_default, "bit" ) );
  registerPar( new configpar<string>(gist_realm, gistconf_cookie_digest,  "cookie-digest","Hash algorithm to be used for cookie creation", false, "SHA1") );
  registerPar( new configpar<bool>(  gist_realm, gistconf_delayedstate,       "delayed-state-installation", "Use delayed state installation (bool)", true, delayedstate_default) );
  registerPar( new configpar<bool>(  gist_realm, gistconf_confirmrequired, "confirm-required", "Require a full handshake at any time (bool)", true, confirmrequired_default) );
  registerPar( new configpar<bool>(  gist_realm, gistconf_senddatainquery, "send-data-in-query", "Send NSLP data also in query (bool)", true, senddatainquery_default) );
  registerPar( new configpar<bool>(  gist_realm, gistconf_reqhelloecho,    "req-hello-echo", "Request a reply to an MA-hello (bool)", true, reqhelloecho_default) );
  registerPar( new configpar<bool>(  gist_realm, gistconf_advertise_sctp,  "advertise-sctp", "advertise SCTP as supported transport protocol at handshake (bool)", true, sctpenable_default) );
  registerPar( new configpar<bool>(  gist_realm, gistconf_verbose_error_responses,  "verbose-errors", "send more error responses back (should be only enabled for protocol debugging)", true, true) );
  registerPar( new configpar<bool>(  gist_realm, gistconf_send_rao,  "send-rao", "send Query with router alert option", true, true) );
  registerPar( new configpar<bool>(  gist_realm, gistconf_strict_rao,  "intercept-requires-rao", "intercept requires presence of Router Alert Option", true, false) );
  registerPar( new configpar<bool>(  gist_realm, gistconf_debug_tp,        "debug-tp", "hex dump PDUs when received or sent PDUs (bool)", true, false) );
  registerPar( new configpar<bool>(  gist_realm, gistconf_dontstartqe, "dont-start-query-encapsulation", "if true do not start the query encapsulation module", false, false) );
  registerPar( new configpar<string>(gist_realm, gistconf_intercept_cmd,   "intercept-cmd",  "string that contains the name of the script to enable GIST packet interception (string)", false, "./intercept") );
  registerPar( new configpar<string>(gist_realm, gistconf_tls_client_cert, "tls-cert",  "filename pointing to the SSL/TLS client certificate (may contain absolute path)", false, "client_cert.pem") );
  registerPar( new configpar<string>(gist_realm, gistconf_tls_client_privkey, "tls-privkey",  "filename pointing to the SSL/TLS client private key file (may contain absolute path)", false, "client_privkey.pem") );
  registerPar( new configpar<string>(gist_realm, gistconf_tls_cacert, "tls-cacert",  "filename pointing to the SSL/TLS CA cert file (may contain absolute path)", false, "root_cert.pem") );

  DLog("gistconf::registerAllPars", "finished registering gist parameters.");
} 


void 
gistconf::repository_init()
{
	DLog("gistconf_rep_init", "start - creating configuration parameter singleton");
	configpar_repository::create_instance(ntlp::gist_realm+1);
}



/** sets the repository pointer and registers all parameters 
 *  (not done in constructor, because of allowing simulation hosts to initialize later)
 **/ 
void
gistconf::setRepository(configpar_repository* newcfp_rep)
{
	if (newcfp_rep == 0)
		throw  configParExceptionNoRepository();
	
	cfp_rep= newcfp_rep;
	// register the realm
	cfp_rep->registerRealm(gist_realm, "gist", gistconf_maxparno);
	DLog("gistconf", "registered gist realm with realm id " << (int) gist_realm);
	
	// now register all parameters
	registerAllPars();
}


string
gistconf::to_string()
{
  ostringstream os;

  unsigned int max_pars= configpar_repository::instance()->getRealmSize(gist_realm);
  configparBase* cfgpar= NULL;
  for (unsigned int i= 0; i < max_pars; i++) {
	  try {
		  // get config par with id i
		  cfgpar= configpar_repository::instance()->getConfigPar(gist_realm, i);
		  os << cfgpar->getName() << (( cfgpar->getName().length() < 8 ) ? "\t\t: " : "\t: ");
		  cfgpar->writeVal(os);
		  if (cfgpar->getUnitInfo())
			  os << " " << cfgpar->getUnitInfo();
		  os << endl;
	  }
	  catch (configParExceptionParNotRegistered) {
		  cfgpar= NULL;
		  continue;
	  } // end catch
	  catch (...) {
		  throw;
	  }
  } // end for
	    
  return os.str();
}


} // end namespace

