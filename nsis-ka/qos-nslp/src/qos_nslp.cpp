/// ----------------------------------------*- mode: C++; -*--
/// @file qos_nslp.cpp
/// 
/// ----------------------------------------------------------
/// $Id: qos_nslp.cpp 6159 2011-05-18 15:24:52Z bless $
/// $HeadURL: https://svn.tm.kit.edu/nsis/qos-nslp/trunk/src/qos_nslp.cpp $
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

#include "all_nslp_ies.h"
#include "qos_nslp.h"

namespace qos_nslp
{
/** This function registers all known PDUs and objects.  */
void 
register_ies() 
{
  NSLP_IE *tmp;
  QSPEC_IEManager::instance()->register_known_ies();
  tmp = new rsn;
  NSLP_IEManager::instance()->register_ie(tmp);
  tmp = new packet_classifier;
  NSLP_IEManager::instance()->register_ie(tmp);
  tmp = new rii;
  NSLP_IEManager::instance()->register_ie(tmp);
  tmp = new rp;
  NSLP_IEManager::instance()->register_ie(tmp);
  tmp = new bound_sessionid;
  NSLP_IEManager::instance()->register_ie(tmp);
  tmp = new info_spec;
  NSLP_IEManager::instance()->register_ie(tmp);
  tmp = new qos_nslp::qspec_object;
  NSLP_IEManager::instance()->register_ie(tmp);
  tmp = new qos_nslp::vlsp_object;
  NSLP_IEManager::instance()->register_ie(tmp);
  tmp = new reservereq;
  NSLP_IEManager::instance()->register_ie(tmp);
  tmp = new responsemsg;
  NSLP_IEManager::instance()->register_ie(tmp);
  tmp = new  querymsg;
  NSLP_IEManager::instance()->register_ie(tmp);
  tmp = new  notifymsg;
  NSLP_IEManager::instance()->register_ie(tmp);
  tmp = new session_auth_object; 
  NSLP_IEManager::instance()->register_ie(tmp);

} // end register_ies

} // end namespace nslp
