/// ----------------------------------------*- mode: C++; -*--
/// @file qos_nslp_aho_contextmap.h
/// QoS NSLP Context Map for Anticipated Handovers
/// ----------------------------------------------------------
/// $Id: qos_nslp_aho_contextmap.h 5253 2010-04-26 13:41:09Z roehricht $
/// $HeadURL: https://svn.tm.kit.edu/nsis/qos-nslp/trunk/include/qos_nslp_aho_contextmap.h $
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
#ifndef QOS_NSLP_AHO_CONTEXTMAP_H
#define QOS_NSLP_AHO_CONTEXTMAP_H


#include "nslp_aho_context.h"
#include "sessionid.h"
#include "logfile.h"
#include <pthread.h>


using namespace protlib;
using namespace protlib::log;
using namespace ntlp;


namespace qos_nslp {


class AhoContextMap {

private:
  /// aho_context_hashmap stores all NSLP_AHO contexts (state needed for handover)
  aho_contextmap_t aho_context_hashmap;

  /// This mutex is used to lock the context map 
  pthread_mutex_t mutex;

  /// mutex attribute for setting mutex kind
  pthread_mutexattr_t mutex_attr;


  /// lock map
  void lock();

  /// unlock map
  void unlock();


public:
  AhoContextMap();
  ~AhoContextMap();


  NSLP_AHO_Context *find(const sessionid& sid);
  bool insert(const sessionid& sid, NSLP_AHO_Context *context);
  void erase(const sessionid& sid);
}; // end class AhoContextMap


inline void AhoContextMap::lock()
{
  if(pthread_mutex_lock(&mutex)) {
    Log(ERROR_LOG, LOG_CRIT, "AhoContextMap", "Error while locking mutex");
  }
}

inline void AhoContextMap::unlock()
{
  pthread_mutex_unlock(&mutex); 
}

} // end namespace qos_nslp

#endif // QOS_NSLP_AHO_CONTEXTMAP_H

// vi: expandtab softtabstop=2 tabstop=2 shiftwidth=2
