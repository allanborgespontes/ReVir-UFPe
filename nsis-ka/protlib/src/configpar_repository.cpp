/// ----------------------------------------*- mode: C++; -*--
/// @file configpar_repository.cpp
/// Configuration parameter repository class
/// ----------------------------------------------------------
/// $Id: configpar_repository.cpp 4107 2009-07-16 13:49:52Z bless $
/// $HeadURL: https://svn.tm.kit.edu/nsis/protlib/trunk/src/configpar_repository.cpp $
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

#include "configpar_repository.h"
#include <stdexcept>

#include "logfile.h"

namespace protlib {
  using namespace std;

// initializer for the static class variable pointer to singleton instance
configpar_repository* configpar_repository::configpar_repo_p= NULL;

configpar_repository::configpar_repository(unsigned int max_realms) :
  max_realms(max_realms), realmsp(0)
{
  // allocate array of pointers to realms
  realmsp= new realm_p_t[max_realms];

  // initialize each pointer with NULL
  for (unsigned int i= 0; i< max_realms; i++)
  {
    realmsp[i]= NULL;
  }
}

configpar_repository::~configpar_repository()
{
  for (unsigned int i= 0; i< max_realms; i++)
  {
    realm_entry_t* re = realmsp[i];
    if (re)
    {
      confparvector_t* cpv = re->first;
      if (cpv) {
        for (confparvector_t::iterator it = cpv->begin(); it != cpv->end(); it++)
        {
          delete *it;
        }
        delete cpv;
      }
      delete re;
    }
  } // end for
  delete realmsp;
}


void
configpar_repository::create_instance(unsigned int max_realms)
{
  DLog("configpar_repository","instance() - creating instance");
  configpar_repo_p= new(nothrow) configpar_repository(max_realms);
}




void
configpar_repository::registerRealm(realm_id_t realm, const char* realm_name, unsigned int maximum_no_of_config_parameters)
{

  // check realm id range
  if ( realm < max_realms )
  {
    if ( realmsp[realm] != NULL )
    {
      // throw Exception that this realm is already registered
      throw configParExceptionRealmAlreadyRegistered(realm, realm_name);
    }
    // register realm, allocate confpar vector
    realmsp[realm]= new realm_entry_t(make_pair(new confparvector_t(maximum_no_of_config_parameters), realm_name));
    // store also a mapping from name to realm id
    realmname_map[realm_name]= realm;
  }
  else
  { // realm id out of range 
    ERRLog("configpar::registerRealm", "out of range: " << (int) realm << " max realms:" << max_realms);
    throw configParExceptionRealmInvalid(realm, realm_name);
  }
}


configpar_repository::realm_p_t 
configpar_repository::getRealm_var(realm_id_t realm) const
{

  if ( realm < max_realms )
  {
    if ( realmsp[realm] != 0 )
    {
      return realmsp[realm];
    }
    else
      throw configParExceptionRealmNotRegistered(realm);
  }
  else
  { // realm id out of range 
    ERRLog("configpar::getRealm_var", "out of range: " << (int) realm << " max realms:" << max_realms);

    throw configParExceptionRealmInvalid(realm, NULL);
  }

}


const string& 
configpar_repository::getRealmName(realm_id_t realm) const
{
  realm_p_t realm_p= getRealm( realm );

  return realm_p->second;
}


/** get the size of a realm
 * @return size of a realm
 */
unsigned int
configpar_repository::getRealmSize(realm_id_t realm) const
{
  realm_p_t realm_p= getRealm( realm );

  if ( realm_p->first != NULL )
  {
    return  realm_p->first->size();
  }
  else
    return 0;
}


/** find corresponding realm id for a given realm name
 * @return matching id or max_realms if not found
 */
realm_id_t 
configpar_repository::getRealmId(string realmname) const 
{
	realmname_map_t::const_iterator cit= realmname_map.find(realmname);
	if ( cit != realmname_map.end() ) // found something
	{ 
	  return cit->second;
	}
	else
	{ // not found, returns invalid realm
	  return max_realms;
	}
}



/** check whether a given realm is valid and registered
 * @return true if realm with given id exists, false if either out of range
 *         or not registered.
 */
bool 
configpar_repository::existsRealm(realm_id_t realm) const
{
  if ( realm < max_realms ) {

    if ( realmsp[realm] != 0 ) {
	    return true;
    }
    else
	    return false;

  }
  else
	  return false;
}


/**
 * if there is any problem with the registration this method will throw errors
 *
 */
void
configpar_repository::registerPar(const configparBase& config_parameter)
{
  realm_p_t realm_p= getRealm( config_parameter.getRealmId() );
  configparBase* confpar_p= NULL;
  try {
    confpar_p= realm_p->first->at(config_parameter.getParId());
  }
  catch ( std::out_of_range )
  {
    throw configParExceptionParIDInvalid(config_parameter.getParId());
  }
  catch (...)
  {
    // rethrow any other exception
    throw;
  }

  if (confpar_p == 0) 
  {
    // store a copy of the given configpar subclass object
    realm_p->first->at(config_parameter.getParId())= config_parameter.copy();
  }
  else
    throw configParExceptionParAlreadyRegistered(config_parameter.getParId());
}


/**
 * if there is any problem with the registration this method will throw errors
 *
 * the parameter is not copied
 */
void
configpar_repository::registerPar(configparBase *config_parameter)
{
  if ( config_parameter == NULL)
    return;

  realm_p_t realm_p= getRealm( config_parameter->getRealmId() );
  configparBase* confpar_p= NULL;

  try {
    confpar_p= realm_p->first->at(config_parameter->getParId());
  }
  catch ( std::out_of_range )
  {
    throw configParExceptionParIDInvalid(config_parameter->getParId());
  }
  catch (...)
  {
    // rethrow any other exception
    throw;
  }
  
  if (confpar_p == 0) 
  {
    // store a copy of the given configpar object
    realm_p->first->at(config_parameter->getParId())= config_parameter;
  }
  else
    throw configParExceptionParAlreadyRegistered(config_parameter->getParId());
}


configparBase* 
configpar_repository::getConfigPar(realm_id_t realm, configpar_id_t configparid)
{
  realm_p_t realm_p= getRealm( realm );
  configparBase* cfgpar_p= NULL;
  try {
    cfgpar_p= realm_p->first->at( configparid );

    if (cfgpar_p == 0)
      throw configParExceptionParNotRegistered(configparid);
  }
  catch ( std::out_of_range )
  {
    throw configParExceptionParIDInvalid(configparid);
  }
  catch (...)
  {
    // rethrow any other exception
    throw;
  }

  return cfgpar_p;
}


const configparBase* 
configpar_repository::getConfigPar(realm_id_t realm, configpar_id_t configparid) const
{
  const realm_p_t realm_p= getRealm( realm );
  const configparBase* cfgpar_p= NULL;

  try {
    cfgpar_p= realm_p->first->at( configparid );

    if (cfgpar_p == 0)
      throw configParExceptionParNotRegistered(configparid);
  }
  catch ( std::out_of_range )
  {
    throw configParExceptionParIDInvalid(configparid);
  }
  catch (...)
  {
    // rethrow any other exception
    throw;
  }

  return cfgpar_p;
}



} // end namespace
