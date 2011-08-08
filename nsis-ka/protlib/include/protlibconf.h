/// ----------------------------------------*- mode: C++; -*--
/// @file protlib_conf.h
/// Configuration parameter class
/// ----------------------------------------------------------
/// $Id:$
/// $HeadURL:$
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
#ifndef _PROTLIB_CONF__H
#define _PROTLIB_CONF__H

#include "configpar.h"
#include "configpar_repository.h"

namespace protlib {
  using namespace std;

  // 0 = global realm, 1 = protlib_realm

  const realm_id_t protlib_realm= 1;

  enum protlib_configpar_id_t {
    protlibconf_invalid,
    protlibconf_ipv4_only,
    protlibconf_maxparno
  };


  class protlibconf {
  public:
	  // constructor should not do much, because we want to allow for later 
	  protlibconf() : cfp_rep(0) {};

	  /// creates the repository singleton (must be explicitly called)
	  static void repository_init();

	  /// sets the repository pointer and registers all parameters
	  void setRepository(configpar_repository* cfp_rep= configpar_repository::instance());

	  string to_string();
	  string parname(protlib_configpar_id_t configparid) {  return cfp_rep->getConfigPar(protlib_realm, configparid)->getName(); }
	  const string& getDescription(protlib_configpar_id_t configparid) const { return cfp_rep->getConfigPar(protlib_realm, configparid)->getDescription(); }
	  const char *getUnitInfo(protlib_configpar_id_t configparid) const { return cfp_rep->getConfigPar(protlib_realm, configparid)->getUnitInfo(); }

	  /// register copy of configuration parameter instance
	  void registerPar(const configparBase& configparid) { cfp_rep->registerPar(configparid); }
	  /// register instance configuration parameter directly
	  void registerPar(configparBase* configparid) { cfp_rep->registerPar(configparid);  }

	  // this is just a convenience function
	  template <class T> void setpar(protlib_configpar_id_t configparid, const T& value)
	  {
		  cfp_rep->setPar(protlib_realm, configparid, value);
	  }

	  // this is just a convenience function
	  template <class T> T getpar(protlib_configpar_id_t configparid)
	  {
		  return cfp_rep->getPar<T>(protlib_realm, configparid);
	  }

	  // this is just a convenience function
	  template <class T> T& getparref(protlib_configpar_id_t configparid)
	  {
		  return cfp_rep->getParRef<T>(protlib_realm, configparid);
	  }


  private:
	  configpar_repository* cfp_rep;

	  // register all available parameters
	  // *** add new configuration parameters in this method ***
	  void registerAllPars();

  };

  // only declared here, must be defined somewhere else
  extern class protlibconf plibconf;

} // end namespace

#endif
