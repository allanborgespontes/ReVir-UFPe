/// ----------------------------------------*- mode: C++; -*--
/// @file natfw_daemon.h
/// The NATFW daemon thread.
/// ----------------------------------------------------------
/// $Id: natfw_daemon.h 2558 2007-04-04 15:17:16Z bless $
/// $HeadURL: https://svn.tm.kit.edu/nsis/natfw-nslp/trunk/include/natfw_daemon.h $
// ===========================================================
//                      
// Copyright (C) 2005-2007, all rights reserved by
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
#ifndef NATFW__NATFW_DAEMON_H
#define NATFW__NATFW_DAEMON_H

#include "protlib_types.h"

#include "ntlp_starter.h" // from NTLP

#include "natfw_config.h"
#include "session_manager.h"
#include "nat_manager.h"
#include "policy_rule_installer.h"


namespace natfw {
  using protlib::uint32;
  using ntlp::NTLPStarterParam;
  using ntlp::NTLPStarter;


/**
 * Encapsulated parameters for a natfw_daemon thread.
 */
class natfw_daemon_param : public ThreadParam {
  public:
	natfw_daemon_param(const char *name, const natfw_config &conf)
		: ThreadParam((uint32)-1, name), config(conf) { }

	natfw_config config;
};


/**
 * The NATFW daemon thread.
 *
 * This thread is the NATFW daemon implemenation. It starts a NTLP thread,
 * registers with it and handles all messages it gets from the NTLP.
 */
class natfw_daemon : public Thread {
  public:
	natfw_daemon(const natfw_daemon_param &param);
	virtual ~natfw_daemon();

	virtual void startup();

	virtual void main_loop(uint32 thread_num);

	virtual void shutdown();

  private:
	natfw_config config;

	session_manager session_mgr;
	nat_manager nat_mgr;
	policy_rule_installer *rule_installer;

	ThreadStarter<NTLPStarter, NTLPStarterParam> *ntlp_starter;
};


void init_framework();
void cleanup_framework();


} // namespace natfw

#endif // NATFW__NATFW_DAEMON_H
