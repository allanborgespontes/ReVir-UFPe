/// ----------------------------------------*- mode: C++; -*--
/// @file authorized_peer_db.h
/// Simple class for storing peer MA policies
/// ----------------------------------------------------------
/// $Id$
/// $HeadURL$
// ===========================================================
//                      
// Copyright (C) 2005-2010, all rights reserved by
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
#ifndef AUTHORIZED_PEER_DB
#define AUTHORIZED_PEER_DB

#include "address.h"

namespace ntlp {

using namespace protlib;

struct policy_entry_t 
{
  policy_entry_t() : secure_ma(false) {};

  bool secure_ma;
};

/** Authorized Peer Database
 * this is described 
 */
class authorized_peer_db
{ 
  policy_entry_t default_policy;

public:
  // return MA policy for a node
  // @currently only returning a global policy, should be converted to a map peeraddress -> policy
  policy_entry_t get_policy(const appladdress& peer) const { return default_policy; }
  void set_policy(const appladdress& peer, policy_entry_t peerpolicy) { /* not implemented yet */ }
  void set_default_policy(policy_entry_t new_default_policy) { default_policy= new_default_policy; }
  policy_entry_t get_default_policy() const { return default_policy; }
};

} // end namespace ntlp

#endif
