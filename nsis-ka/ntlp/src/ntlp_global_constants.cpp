/// ----------------------------------------*- mode: C++; -*--
/// @ntlp_global_constants.cpp
/// GIST (default) constants
/// ----------------------------------------------------------
/// $Id: ntlp_global_constants.cpp 6176 2011-05-20 07:39:55Z bless $
/// $HeadURL: https://svn.tm.kit.edu/nsis/ntlp/trunk/src/ntlp_global_constants.cpp $
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
#ifndef NTLP_GLOBAL_CONSTANTS_CPP
#define NTLP_GLOBAL_CONSTANTS_CPP

#include "ntlp_global_constants.h"

namespace ntlp {
// GIST default constants
// =======================
const char *const gist_name="GISTka";
const char *const gist_releasestring=VERSION;
const char *const gist_unix_domain_socket_defaultpath="/tmp/gist";
const char *const gist_configfilename="gist.conf";
#ifdef USE_AHO
  const char *const gist_nwn_unix_domain_socket_defaultpath="/tmp/gist_nwn";
#endif
// =======================
}
#undef NTLP_GLOBAL_CONSTANTS_CPP
#endif
