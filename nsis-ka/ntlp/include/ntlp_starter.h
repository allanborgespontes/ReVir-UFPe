/// ----------------------------------------*- mode: C++; -*--
/// @file ntlp_starter.h
/// GIST Starter Class which fires up all necessary threads
/// ----------------------------------------------------------
/// $Id: ntlp_starter.h 5249 2010-04-26 08:40:59Z roehricht $
/// $HeadURL: https://svn.tm.kit.edu/nsis/ntlp/trunk/include/ntlp_starter.h $
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
#ifndef NTLP_STARTER_H
#define NTLP_STARTER_H

#include <list>

// from protlib
#include "threads.h"
#include "address.h"
#include "addresslist.h"
#include "flowinfo.h"

#include "ntlp_global_constants.h"

namespace ntlp {
    using namespace protlib;
    using std::list;

extern class NTLPStarter* global_ntlpstarterthread_p;

/// starter module parameters
struct NTLPStarterParam : public ThreadParam {
	AddressList *addresses;
	Flowinfo *fi_service;

		NTLPStarterParam()   : ThreadParam(ThreadParam::default_sleep_time, "GIST Starter"), addresses(0), fi_service(0) {}
}; // end NTLPStarterParam

  class Statemodule;

class NTLPStarter : public Thread {
public:
	
    NTLPStarter(const NTLPStarterParam& p);
    /// destructor
    ~NTLPStarter();
    /// module main loop
    void main_loop(uint32 nr);

  const NTLPStarterParam& get_param() const { return param; };
  const Statemodule* getStatemodule() const { return statemodule_p; };
  Statemodule* getStatemoduleFullAccess() { return statemodule_p; };

 private:
    NTLPStarterParam param;
    Statemodule *statemodule_p;

}; // end class Signaling



} // end namespace ntlp

#endif // NTLP_STARTER_H
