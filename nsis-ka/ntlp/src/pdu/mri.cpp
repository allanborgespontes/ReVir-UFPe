/// ----------------------------------------*- mode: C++; -*--
/// @file mri.cpp
/// MRI (Message Routing Information) base class
/// ----------------------------------------------------------
/// $Id: mri.cpp 5249 2010-04-26 08:40:59Z roehricht $
/// $HeadURL: https://svn.tm.kit.edu/nsis/ntlp/trunk/src/pdu/mri.cpp $
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
#include <mri.h>

namespace ntlp {

  using namespace protlib;
  using namespace protlib::log;


mri::mri(uint8 mrm, bool nat_flag, known_ntlp_object::subtype_t subtype)
  : known_ntlp_object(MRI, subtype, Mandatory),
    mrm(mrm), nat_flag(nat_flag)
{
  // call the fitting contructor for setting subtype!
  switch (mrm) {
    case 0 : subtype=pathcoupled; break;
    case 1 : subtype=looseend; break;
  }
}

bool mri::operator==(const IE& ie) const {
  const mri* sh = dynamic_cast<const mri*>(&ie);
  return sh != NULL && sh->mrm == mrm && sh->nat_flag == nat_flag;
}


}
