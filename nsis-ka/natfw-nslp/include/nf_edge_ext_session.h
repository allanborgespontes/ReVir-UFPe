/// ----------------------------------------*- mode: C++; -*--
/// @file nf_edge_ext_session.h
/// The nf_edge_ext_session class.
/// ----------------------------------------------------------
/// $Id: nf_edge_ext_session.h 2558 2007-04-04 15:17:16Z bless $
/// $HeadURL: https://svn.tm.kit.edu/nsis/natfw-nslp/trunk/include/nf_edge_ext_session.h $
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
#ifndef NATFW__NF_EDGE_EXT_SESSION_H
#define NATFW__NF_EDGE_EXT_SESSION_H

#include "protlib_types.h"

#include "session.h"
#include "events.h"
#include "msg/ntlp_msg.h"


namespace natfw {
    using protlib::uint8;
    using protlib::uint32;
    using protlib::appladdress;

class event;
class msg_event;
class api_ext_event;

/**
 * A session for the EXT part of a forwarder (which is an edge device).
 *
 * This session is initiated by the NI+ on the local network. Only messages
 * from the local network are accepted. Everything from the public side is
 * discarded.
 */
class nf_edge_ext_session : public session {
  public:
	nf_edge_ext_session(const session_id &id, const natfw_config *conf);
	~nf_edge_ext_session();

	bool is_final() const; // inherited from session

  protected:
	/**
	 * States of a session.
	 */
	enum state_t {
		STATE_IDLE		= 0,
		STATE_SESSION		= 1,
		STATE_FINAL		= 2
	};

	nf_edge_ext_session(state_t s, const natfw_config *conf);

	inline state_t get_state() const { return state; }
	void process_event(dispatcher *d, event *evt);

	inline bool is_proxy_mode() const { return proxy_mode; }
	inline void set_proxy_mode(bool value) { proxy_mode = value; }

	inline timer &get_state_timer() { return state_timer; }

	msg::ntlp_msg *get_last_ext_message() const;
	void set_last_ext_message(msg::ntlp_msg *msg);

  private:
	state_t state;

	const natfw_config *config;

	bool proxy_mode;
	session_id proxy_session_id;

	uint32 lifetime;
	uint32 max_lifetime;
	timer state_timer;

	ntlp::mri *routing_info;

	appladdress ext_addr;

	/*
	 * The last EXT message we sent. We keep it because we have to
	 * retransmit it and to check if a received RESPONSE matches the
	 * EXT sent earlier.
	 */
	msg::ntlp_msg *last_ext_msg;


	/*
	 * Previously installed NAT and firewall rules. Required for proxy
	 * mode only.
	 */
	fw_policy_rule *fw_rule;
	nat_policy_rule *nat_rule;


	/*
	 * State machine methods:
	 */
	state_t handle_state_idle(dispatcher *d, event *evt);
	state_t handle_state_session(dispatcher *d,  event *evt);

	void setup_session(api_ext_event *evt);

	msg::ntlp_msg *build_edge_device_response(msg_event *e,
			appladdress *addr=NULL) const;
	msg::ntlp_msg *build_trace_response(ntlp_msg *msg) const;
	fw_policy_rule *build_fw_policy_rule(ntlp_msg *msg) const;
	void tg_create_proxy(dispatcher *d, ntlp_msg *msg);
	void tg_teardown_proxy(dispatcher *d);

	inline uint32 get_lifetime() const { return lifetime; }
	inline void set_lifetime(uint32 seconds) { lifetime = seconds; }

	inline uint32 get_max_lifetime() const { return max_lifetime; }
	inline void set_max_lifetime(uint32 t) { max_lifetime = t; }

	inline session_id get_proxy_session_id() const {
		return proxy_session_id; }
	inline void set_proxy_session_id(session_id s) { proxy_session_id = s; }

	ntlp::mri *get_mri() const;
	void set_mri(ntlp::mri *m);

	friend std::ostream &operator<<(std::ostream &out,
		const nf_edge_ext_session &s);
};

std::ostream &operator<<(std::ostream &out, const nf_edge_ext_session &s);

inline bool nf_edge_ext_session::is_final() const {
	return get_state() == STATE_FINAL;
}

inline msg::ntlp_msg *nf_edge_ext_session::get_last_ext_message() const {
	assert( last_ext_msg != NULL );
	return last_ext_msg;
}

inline void nf_edge_ext_session::set_last_ext_message(msg::ntlp_msg *msg) {
	if ( last_ext_msg != NULL )
		delete last_ext_msg;
	last_ext_msg = msg;
}

inline ntlp::mri *nf_edge_ext_session::get_mri() const {
	return routing_info;
}

inline void nf_edge_ext_session::set_mri(ntlp::mri *m) {
	delete routing_info;
	routing_info = m;
}


} // namespace natfw

#endif // NATFW__NF_EDGE_EXT_SESSION_H
