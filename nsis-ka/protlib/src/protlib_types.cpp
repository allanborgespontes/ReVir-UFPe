/// ----------------------------------------*- mode: C++; -*--
/// @file protlib_types.cpp
/// functions for protlib types. Currently uint128 only
/// ----------------------------------------------------------
/// $Id$
/// $HeadURL$
// ===========================================================
//                      
// Copyright (C) 2005-2008, all rights reserved by
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

#include "protlib_types.h"
#include <iomanip>

namespace protlib {

  using namespace std;

ostream&
operator<<(ostream& os, protlib::uint128 val)
{
  ios_base::fmtflags flags = os.flags();        // save stream flags
  os  << setw(8) << setfill('0') << hex << val.w1 << '-' 
      << setw(8) << setfill('0') << hex << val.w2 << '-' 
      << setw(8) << setfill('0') << hex << val.w3 << '-' 
      << setw(8) << setfill('0') << hex << val.w4; 

  os.setf(flags);               // reset stream flags

  return os;
}

} // end namespace
