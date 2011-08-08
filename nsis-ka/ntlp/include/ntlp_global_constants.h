/// ----------------------------------------*- mode: C++; -*--
/// @file ntlp_global_constants.h
/// Global constants
/// ----------------------------------------------------------
/// $Id: ntlp_global_constants.h 5720 2010-10-14 11:55:52Z bless $
/// $HeadURL: https://svn.tm.kit.edu/nsis/ntlp/trunk/include/ntlp_global_constants.h $
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
#ifndef NTLP_GLOBAL_CONSTANTS_H
#define NTLP_GLOBAL_CONSTANTS_H

#include "protlib_types.h"

using namespace protlib;

namespace ntlp 
{
  /// GIST default values
  extern const char *const gist_name;
  extern const char *const gist_releasestring;
  extern const char *const gist_unix_domain_socket_defaultpath;
  extern const char *const gist_configfilename;
#ifdef USE_AHO
  extern const char *const gist_nwn_unix_domain_socket_defaultpath;
#endif

#ifndef NTLP_GLOBAL_CONSTANTS_CPP
  const uint8  ntlp_version_default= 1;
  const uint32 GIST_magic_number= 0x4e04bda5L; // GIST magic number in network byte order is 0x4e04bda5, this constant is stored in host-byte-order
  const uint16 GIST_default_port   = 270; // default GIST port, assigned by IANA
  const uint32 retrylimit_default= 64000UL;   // T2 = 64s: Upper limit for T1
  const uint32 retryperiod_default = 500UL;   // [ms] Initial retry period (T1)
  const float retryfactor_default = 0.75;     // This factor is used to calculate local timeout values from peer's timeout values ~0.8?
  const uint32 rs_validity_time_default = 60000; // [ms] how long to keep state open, will be told to the peer, so he can calculate refresh period
  const uint32 refreshtime_default = 120000;  // [ms] upper Boundary for Refresh Period -> small: better route change detection
  const uint32 ma_hold_time_default = 180000; // [ms] keep MA slightly longer than Upper Refresh Period boundary (makes no sense otherwise)
  const uint32 secrets_refreshtime_default = 300;  // [s] Secrets Roll-Over Time, 5 mins should be OK?
  const uint32 secrets_count_default = 2;     // count of local secrets hold at one time
  const uint32 secrets_length_default = 256;  // length of local secrets in bit  
  const bool delayedstate_default= true;      // is Delayed State Installation to be used?
  const bool senddatainquery_default= true;   // send data in query?
  const bool confirmrequired_default= true;   // always perform a full handshake?
  const bool reqhelloecho_default= false;     // don't require to answer MA-Hellos
  const bool sctpenable_default= true;        // advertise SCTP in negotation
  const uint16 dnslookup_retries_max= 10;
  const uint32 queue_poll_timeout_default= 2; // wait how many seconds until data queue is inspected
  const uint32 data_keep_timeout= 120;        // time to live in seconds for enqueued payload data 
#endif
} // end namespace
#endif // NTLP_GLOBAL_CONSTANTS_H
