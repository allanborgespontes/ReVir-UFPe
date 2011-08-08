/// ----------------------------------------*- mode: C++; -*--
/// @file capability.h
/// GIST Capability object
/// ----------------------------------------------------------
/// $Id: capability.h 6282 2011-06-17 13:31:57Z bless $
/// $HeadURL: https://svn.tm.kit.edu/nsis/ntlp/trunk/src/capability.h $
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
#include "ntlp_object.h"
#include "address.h"
#include "protlib_types.h"

#include "sessionid.h"
#include "signalingmodule_ntlp.h"
#include "mri.h"
#include "ntlp_pdu.h"
#include "nli.h"
#include "hashmap"
#include "query_cookie.h"
#include "resp_cookie.h"

#ifndef _NTLP__CAPABILITY_H_
#define _NTLP__CAPABILITY_H_




namespace ntlp {
    using namespace protlib;


/// class for transport capability negotation
class capability {

public:

  // constructor
  capability(uint16 udpport, uint16 tcpport, uint16 tlsport, uint16 sctpport);

  // destructor
  ~capability();

  appladdress* get_address(const stackprop* sp, const stack_conf_data* sc, const nli* nl, bool secure = false);
	
  stackprop* query_stackprop(bool secure = false);
  stack_conf_data* query_stackconf(bool secure = false);
  
  stackprop* resp_stackprop(const stackprop* sp, bool secure = false) const;
  stack_conf_data* resp_stackconf(stack_conf_data* sc, bool secure = false) const;
  
  bool is_secure(const appladdress* addr) const;
  bool is_reliable(const appladdress* addr) const;

  bool accept_proposal(const stackprop* sp, bool secure = false) const;
  
  static bool is_consistent(const stackprop* sp, const stack_conf_data* sc);
  
private:
  static void delete_stackprofile(stackprofile* sp) { delete sp; }
  static void delete_ma_protocol_option(ma_protocol_option* mao) { delete mao; }
  
  vector<protocol_t> secureprotocols;
  vector<protocol_t> reliableprotocols;

  vector<stackprofile*> securestackprops;
  vector<ma_protocol_option*> securestackconfs;
  vector<ma_protocol_option*> querysecurestackconfs;
  
  vector<stackprofile*> stackprops;
  vector<ma_protocol_option*> stackconfs;
  vector<ma_protocol_option*> querystackconfs;

}; // end class capability
 


} // end namespace ntlp



#endif
