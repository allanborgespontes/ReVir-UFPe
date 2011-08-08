/// ----------------------------------------*- mode: C++; -*--
/// @file configpar_repository.h
/// Configuration parameter repository class
/// ----------------------------------------------------------
/// $Id: configpar_repository.h 4107 2009-07-16 13:49:52Z bless $
/// $HeadURL: https://svn.tm.kit.edu/nsis/protlib/trunk/include/configpar_repository.h $
// ===========================================================
//                      
// Copyright (C) 2008, all rights reserved by
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
#ifndef _CONFIGPAR_REPOSITORY__H
#define _CONFIGPAR_REPOSITORY__H

#include "configpar.h"

#include <vector>
#include <map>

namespace protlib {
  using namespace std;

class configpar_repository
{
public:

	configpar_repository(unsigned int max_realms);
	~configpar_repository();

	void registerRealm(realm_id_t realm, const char* realmname, unsigned int maximum_no_of_config_parameters);
	const string& getRealmName(realm_id_t realm) const;
	realm_id_t getRealmId(string realmname) const;
	bool existsRealm(realm_id_t realm) const;
	unsigned int getMaxRealms() const { return max_realms; }
	unsigned int getRealmSize(realm_id_t realm) const;

	/// register copy of configuration parameter instance
	void registerPar(const configparBase& configparid);
	/// register instance configuration parameter directly
	void registerPar(configparBase* configparid);
	/// return pointer to base class (meta information about the parameter)
	configparBase* getConfigPar(realm_id_t realm, configpar_id_t configparid);
	/// return pointer to base class (meta information about the parameter) read only access
	const configparBase* getConfigPar(realm_id_t realm, configpar_id_t configparid) const;

	// get a parameter from type T
	template <class T> T getPar(realm_id_t realm, configpar_id_t configparid) const;
	// get a reference to parameter from type T (useful for larger types)
	template <class T> const T& getParRef(realm_id_t realm, configpar_id_t configparid) const;
	template <class T> T& getParRef(realm_id_t realm, configpar_id_t configparid);

	// set a parameter from type T
	template <class T> void setPar(realm_id_t realm, configpar_id_t configparid, const T& newval);

	// create and/or access singleton instance
	static void create_instance(unsigned int max_realms);
	static configpar_repository* instance() { return configpar_repo_p; }
	static void clear() { delete configpar_repo_p; configpar_repo_p= NULL; };

private:
	typedef vector< configparBase* > confparvector_t;
	typedef pair< confparvector_t*, string >  realm_entry_t;
	typedef realm_entry_t* realm_p_t;

	realm_p_t getRealm_var(realm_id_t realm) const;
	realm_p_t getRealm(realm_id_t realm) {   return getRealm_var(realm); } ;
	realm_p_t getRealm(realm_id_t realm) const {   return getRealm_var(realm); };

	unsigned int max_realms;

	// we have a configuration vector per realm
	// this thingy here stores vectors for all realms
	realm_p_t* realmsp;

	// name to realm_id mapping
	typedef map<const std::string, realm_id_t> realmname_map_t;
	realmname_map_t realmname_map;

	// pointer to singleton
	static configpar_repository* configpar_repo_p;
}; // end class configpar_repository



template <class T>
T 
configpar_repository::getPar(realm_id_t realm, configpar_id_t configparid) const
{
  const configparBase* cfgpar_p= getConfigPar(realm, configparid);

  const configpar<T>* cfgpar= dynamic_cast<const configpar<T>*>( cfgpar_p );
  if (cfgpar)
    return cfgpar->getPar();
  else
    throw configParExceptionTypeMismatch(configparid);
}



template <class T>
const T& 
configpar_repository::getParRef(realm_id_t realm, configpar_id_t configparid) const
{
  const configparBase* cfgpar_p= getConfigPar(realm, configparid);

  const configpar<T>* cfgpar= dynamic_cast<const configpar<T>*>( cfgpar_p );
  if (cfgpar)
    return cfgpar->getParRef();
  else
    throw configParExceptionTypeMismatch(configparid);
}


template <class T>
T& 
configpar_repository::getParRef(realm_id_t realm, configpar_id_t configparid)
{
  configparBase* cfgpar_p= getConfigPar(realm, configparid);

  configpar<T>* cfgpar= dynamic_cast<configpar<T>*>( cfgpar_p );
  if (cfgpar)
    return cfgpar->getParRef();
  else
    throw configParExceptionTypeMismatch(configparid);
}


template <class T>
void
configpar_repository::setPar(realm_id_t realm, configpar_id_t configparid, const T& newval)
{
  configparBase* cfgpar_p= getConfigPar(realm, configparid);

  configpar<T>* cfgpar= dynamic_cast<configpar<T>*>( cfgpar_p );
  if (cfgpar)
    return cfgpar->setPar(newval);
  else
    throw configParExceptionTypeMismatch(configparid);
}

class configParExceptionNoRepository : public configParException {
public:
  configParExceptionNoRepository() : configParException("No repository available (NULL)") {};
};


class configParExceptionRealmAlreadyRegistered : public configParException {
public:
	configParExceptionRealmAlreadyRegistered(realm_id_t realmid, const char* realm_name) : configParException("Realm already registered - this is a programming error") {};
};

class configParExceptionRealmInvalid : public configParException {
public:
	configParExceptionRealmInvalid(realm_id_t realmid, const char* realm_name) : configParException("Realm invalid (out of range) - this is a programming error") {};
};


class configParExceptionRealmNotRegistered : public configParException {
public:
	configParExceptionRealmNotRegistered(realm_id_t realmid) : configParException("Realm invalid (not registered) - this is a programming error") {};
};


} // end namespace
#endif
