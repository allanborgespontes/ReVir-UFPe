/// ----------------------------------------*- mode: C++; -*--
/// @file session_auth_api.h
/// Interface for inclusion into NTLP/GIST
/// ----------------------------------------------------------
/// $Id$
/// $HeadURL$
// ===========================================================
//                      
// Copyright (C) 2011, all rights reserved by
// - Institute of Telematics, Karlsruhe Institute of Technology
//
// More information and contact:
// https://svn.tm.kit.edu/trac/NSIS
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
/** @ingroup session_auth
 * These interface functions can be used without declaration or definition of an session_auth_object
 * in order to avoid NTLP dependencies on NSLP
 */
#ifndef SESSION_AUTH_API_H
#define SESSION_AUTH_API_H

#include "network_message.h"
#include "ntlp_ie.h"

namespace nslp_auth {
using namespace protlib;

// Interface functions that can be called without including the session_auth_object definition


bool check_hmac(NetMsg &msg, IE::coding_t coding, bool fill_data);
bool serialize_hmac(NetMsg &msg, IE::coding_t coding, bool fill_data);

uint16 get_tlplist_nslp_object_type(const uint8* buf);
uint16 get_tlplist_ntlp_object_type(const uint8* buf);  	



} // end namespace nslp_auth

#endif
