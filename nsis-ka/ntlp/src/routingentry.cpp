/// ----------------------------------------*- mode: C++; -*--
/// @file routingentry.cpp
/// GIST Messaging Routing Entry
/// ----------------------------------------------------------
/// $Id: routingentry.cpp 5249 2010-04-26 08:40:59Z roehricht $
/// $HeadURL: https://svn.tm.kit.edu/nsis/ntlp/trunk/src/routingentry.cpp $
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

#include "gist_conf.h"
#include "routingentry.h"

using namespace ntlp;
using namespace protlib;

namespace ntlp {

const char* 
timername(timer_type_t timertype)
{
  return timertype<=last_timer_type ? timerstring[timertype] : (timertype==none ? "NONE/STOPPED" : "illegal timer type");
}

} // end namespace

// put as static method here to avoid circular dependencies in header file
// is, however, also the same for every routingtable object
uint32 routingentry::get_retry_limit() { return gconf.getpar<uint32>(gistconf_retrylimit); }
uint32 routingentry::get_retry_period() { return gconf.getpar<uint32>(gistconf_retryperiod); }

string
routingentry::to_string() const
{
  ostringstream os;

  os << (is_responding_node() ? "R" : "Q") << "N/" 
     << "State:" << get_state_name() << " " 
     << (dmode ? "D" : "C") << "-Mode,"
     << (secure ? "Secure" : "NotSecure") << " "
     << "NLI:";

  if (peer_nli)
    os << *peer_nli;
  else
    os << "n/a";

  os << " Valid:" << rs_validity_time << "ms "
     << " MA Hold:" << ma_hold_time << "ms ";

  return os.str();
}
