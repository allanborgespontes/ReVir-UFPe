/// ----------------------------------------*- mode: C++; -*--
/// @file nslp_aho_context.h
/// QoS NSLP Anticipated Handover Context
/// ----------------------------------------------------------
/// $Id: nslp_aho_context.h 6282 2011-06-17 13:31:57Z bless $
/// $HeadURL: https://svn.tm.kit.edu/nsis/qos-nslp/trunk/include/nslp_aho_context.h $
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
#ifndef _NSLP_AHO_CONTEXT_H_
#define _NSLP_AHO_CONTEXT_H_


#include "protlib_types.h"
#include "logfile.h"
#include <pthread.h>
#include <set>
#include "hashmap"
#include "llhashers.h"
#include "address.h"
#include "mri_pc.h"
#include "rii.h"
#include "rsn.h"
#include "qspec.h"
#include "refresh_period.h"
#include "bound_sessionid.h"

#ifdef NSIS_OMNETPP_SIM
#define pthread_mutexattr_init(a)
#define pthread_mutexattr_settype(a,b)
#define pthread_mutex_init(a,b)
#define pthread_mutexattr_destroy(a)
#define pthread_mutex_destroy(a)
#define pthread_mutex_lock(a) false
#define pthread_mutex_unlock(a) false
#endif

using namespace protlib;
using namespace protlib::log;


namespace qos_nslp {


typedef int64 simple_sid; // shortened sessionid in order to fit into a single 64-bit integer


class NSLP_AHO_Context {

public:
  enum node_role_t {
    MN,     // mobile node
    ARN,    // new access router
    CN,     // correspondent node
    OTHER   // all other nodes
  };

  // a comparison function for the path-coupled MRI used in conjunction with sets
  struct compare_pc_mri {
    inline bool operator()(const ntlp::mri_pathcoupled &a, const ntlp::mri_pathcoupled &b) const {
      hostaddress::ltaddr ltaddr;
      
      if (ltaddr(&a.get_sourceaddress(), &b.get_sourceaddress())) {
        return true;
      }

      if (ltaddr(&a.get_destaddress(), &b.get_destaddress())) {
        return true;
      }

      return ((!a.get_downstream()) && b.get_downstream());
    }
  };


  // constructor
  NSLP_AHO_Context();

  // destructor
  virtual ~NSLP_AHO_Context();


  void set_aho_node_role(const NSLP_AHO_Context::node_role_t role);
  NSLP_AHO_Context::node_role_t get_aho_node_role();

  void set_addr_mobile_node(const hostaddress &mn);
  const hostaddress &get_addr_mobile_node() const;

  void set_addr_new_access_router(const hostaddress &ar_n);
  const hostaddress &get_addr_new_access_router() const;

  void add_flow_to_context(const ntlp::mri_pathcoupled mri);
  void remove_flow_from_context(const ntlp::mri_pathcoupled &mri);
  bool is_flow_in_context(const ntlp::mri_pathcoupled &mri) const;
  bool is_set_of_flows_empty() const;

  void set_is_flow_sender(bool is_flow_sender);
  bool get_is_flow_sender() const;

  void set_is_flow_receiver(bool is_flow_receiver);
  bool get_is_flow_receiver() const;


  void set_rii(const rii *rii_obj);
  const rii *get_rii() const;

  void set_qspec(qspec_object *qspec_obj);
  qspec_object *get_qspec() const;

  void set_refresh_period(const rp *refresh_period);
  const rp *get_refresh_period() const;

  void set_bound_sessionid(const bound_sessionid *bs);
  const bound_sessionid *get_bound_sessionid() const;


  // lock mutex
  void lock();

  // unlock mutex
  void unlock();

private:
  NSLP_AHO_Context::node_role_t node_role;

  hostaddress mn;
  hostaddress ar_n;

  typedef set<ntlp::mri_pathcoupled, compare_pc_mri> mri_new_flow_set_t;
  typedef mri_new_flow_set_t::iterator mri_new_flow_set_it_t;
  typedef mri_new_flow_set_t::const_iterator mri_new_flow_set_const_it_t;
  mri_new_flow_set_t mri_new_flow_set;

  bool is_flow_sender;
  bool is_flow_receiver;

  rii *rii_obj;
  qspec_object *qspec_obj;
  rp *refresh_period;
  bound_sessionid *bs;

  /// This mutex is used to lock the NSLP_AHO_Context
  pthread_mutex_t mutex;

  /// mutex attribute for setting mutex kind
  pthread_mutexattr_t mutex_attr;
}; // end NSLP_AHO_Context


// NSLP_AHO_Context hashmap
typedef hashmap_t<simple_sid, NSLP_AHO_Context*> aho_contextmap_t;


inline void NSLP_AHO_Context::lock()
{
  if(pthread_mutex_lock(&mutex)) {
    ERRLog("NSLP_AHO_Context", "Error while locking mutex");
  }
}


inline void NSLP_AHO_Context::unlock()
{
  pthread_mutex_unlock(&mutex); 
}


} // end namespace qos_nslp

#endif // _NSLP_AHO_CONTEXT_H_

// vi: expandtab softtabstop=2 tabstop=2 shiftwidth=2
