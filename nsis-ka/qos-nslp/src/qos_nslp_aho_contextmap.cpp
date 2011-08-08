/// ----------------------------------------*- mode: C++; -*--
/// @file qos_nslp_aho_contextmap.cpp
/// QoS NSLP Anticipated Handover Context Map
/// ----------------------------------------------------------
/// $Id: qos_nslp_aho_contextmap.cpp 5253 2010-04-26 13:41:09Z roehricht $
/// $HeadURL: https://svn.tm.kit.edu/nsis/qos-nslp/trunk/src/qos_nslp_aho_contextmap.cpp $
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

#include "qos_nslp_aho_contextmap.h"

namespace qos_nslp {

AhoContextMap::AhoContextMap()
{
  pthread_mutexattr_init(&mutex_attr);
#ifdef _DEBUG
  pthread_mutexattr_settype(&mutex_attr,PTHREAD_MUTEX_ERRORCHECK);
#else
  pthread_mutexattr_settype(&mutex_attr,PTHREAD_MUTEX_NORMAL);
#endif

  pthread_mutex_init(&mutex, &mutex_attr);  
}


AhoContextMap::~AhoContextMap()
{
  pthread_mutex_destroy(&mutex);
  pthread_mutexattr_destroy(&mutex_attr);
}


/**
 * finds aho context for given sessionid
 */
NSLP_AHO_Context *AhoContextMap::find(const sessionid& sid)
{
  lock();
  aho_contextmap_t::const_iterator it = aho_context_hashmap.find(sid.get_hash());
  NSLP_AHO_Context *context = (it != aho_context_hashmap.end()) ? it->second : NULL;
  unlock();
  
  return context;
}


/**
 * inserts aho context for given sessionid
 */
bool AhoContextMap::insert(const sessionid& sid, NSLP_AHO_Context *context)
{
  pair<simple_sid, NSLP_AHO_Context*> tmppair;
  pair<aho_contextmap_t::iterator, bool> tmpiterator;

  tmppair.first = sid.get_hash();
  tmppair.second = context;
  lock();
  tmpiterator = aho_context_hashmap.insert(tmppair);
  unlock();

  return tmpiterator.second;
}


void AhoContextMap::erase(const sessionid& sid)
{
  lock();
  aho_context_hashmap.erase(sid.get_hash());
  unlock();
}

} // end namespace qos_nslp

// vi: expandtab softtabstop=2 tabstop=2 shiftwidth=2
