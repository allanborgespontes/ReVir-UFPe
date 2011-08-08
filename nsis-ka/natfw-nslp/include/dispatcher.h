/// ----------------------------------------*- mode: C++; -*--
/// @file dispatcher.h
/// The dispatcher receives events and calls handlers.
/// ----------------------------------------------------------
/// $Id: dispatcher.h 2558 2007-04-04 15:17:16Z bless $
/// $HeadURL: https://svn.tm.kit.edu/nsis/natfw-nslp/trunk/include/dispatcher.h $
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
#ifndef NATFW__DISPATCHER_H
#define NATFW__DISPATCHER_H

#include "timer_module.h"

#include "apimessage.h"		// from NTLP

#include "natfw_config.h"
#include "session_manager.h"
#include "nat_manager.h"
#include "policy_rule_installer.h"
#include "events.h"
#include "msg/ntlp_msg.h"
#include "gistka_mapper.h"


namespace natfw {

/**
 * Receive events and call handlers.
 *
 * The dispatcher is an instance that receives events, examines them, and
 * then executes a handler. In other words, it is the top-level state machine.
 *
 * Even though not all methods are declared as 'const', the dispatcher itself
 * is constant and stateless. All state is kept in the session manager, NAT
 * manager classes etc., which are shared among dispatcher instances.
 * Because of this, the dispatcher doesn't have to be thread-safe, it is enough
 * that the used components (session_manager etc.) are thread-safe.
 *
 * This class contains a rich interface which offers all functionality the
 * lower-level state machines (implemented in the session classes) require.
 * The services include sending messages, starting times, installing policy
 * rules etc.
 */
class dispatcher {
	
  public:
	dispatcher(session_manager *m, nat_manager *n,
			policy_rule_installer *p,natfw_config *conf);
	virtual ~dispatcher();

	virtual void process(event *evt) throw ();

	/*
	 * Services which are used by the event handlers.
	 */
	virtual void send_message(msg::ntlp_msg *msg) throw ();
	virtual id_t start_timer(const session *s, int secs) throw ();
	virtual void report_async_event(std::string msg) throw ();

	virtual void install_policy_rules(const nat_policy_rule *nat_rule,
			const fw_policy_rule *fw_rule)
		throw (policy_rule_installer_error);
	virtual void remove_policy_rules(const nat_policy_rule *nat_rule,
			const fw_policy_rule *fw_rule)
		throw (policy_rule_installer_error);

	virtual appladdress reserve_external_address(
			const appladdress &priv_addr) throw (nat_manager_error);
	virtual void release_external_address(
			const appladdress &ext_addr) throw ();
	appladdress lookup_private_address(const appladdress &ext_addr) const
		throw (nat_manager_error);

	virtual session_id create_ni_proxy_session(
		const hostaddress &ds_addr, const hostaddress &dr_addr,
                uint16 ds_port, uint16 dr_port, uint8 protocol,
                uint32 session_lifetime, uint32 nonce);

	virtual bool is_from_private_side(
			const hostaddress &addr) const throw();
	virtual bool is_authorized(const msg_event *evt) const throw ();


  private:
	/*
	 * The targets of these four pointers are shared among dispatchers.
	 * They may not be deleted by the destructor!
	 */
	session_manager *session_mgr;
	nat_manager *nat_mgr;
	policy_rule_installer *rule_installer;
	natfw_config *config;

	gistka_mapper mapper;

	session *create_session(event *evt) const throw ();
	void send_receive_answer(const routing_state_check_event *evt) const;
};


} // namespace natfw

#endif // NATFW__DISPATCHER_H
