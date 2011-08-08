/// ----------------------------------------*- mode: C++; -*--
/// @file nf_non_edge_ext_session.h
/// The nf_non_edge_ext_session class.
/// ----------------------------------------------------------
/// $Id: nf_non_edge_ext_session.h 2558 2007-04-04 15:17:16Z bless $
/// $HeadURL: https://svn.tm.kit.edu/nsis/natfw-nslp/trunk/include/nf_non_edge_ext_session.h $
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
#ifndef NATFW__NF_NON_EDGE_EXT_SESSION_H
#define NATFW__NF_NON_EDGE_EXT_SESSION_H

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
 * A session for the EXT part of a forwarder (which is a non-edge device).
 *
 * This session is initiated by the NI+ on the local network. Only messages
 * from the local network are accepted. Everything from the public side is
 * discarded.
 */
class nf_non_edge_ext_session : public session {
  public:
	nf_non_edge_ext_session(const session_id &id, const natfw_config *conf);
	~nf_non_edge_ext_session();

	bool is_final() const; // inherited from session

  protected:
	/**
	 * States of a session.
	 */
	enum state_t {
		STATE_IDLE		= 0,
		STATE_WAITRESP		= 1,
		STATE_SESSION		= 2,
		STATE_FINAL		= 3
	};

	nf_non_edge_ext_session(state_t s, const natfw_config *conf);

	void process_event(dispatcher *d, event *evt);
	inline state_t get_state() const { return state; }

	inline timer &get_state_timer() { return state_timer; }
	inline timer &get_response_timer() { return response_timer; }

	inline uint32 get_response_timeout() const { return response_timeout; }
	inline void set_response_timeout(uint32 t) { response_timeout = t; }

	msg::ntlp_msg *get_last_ext_message() const;
	void set_last_ext_message(msg::ntlp_msg *msg);

  private:
	state_t state;

	const natfw_config *config;

	uint32 lifetime;
	uint32 max_lifetime;
	uint32 response_timeout;
	timer state_timer;
	timer response_timer;

	ntlp::mri *routing_info;

	appladdress ext_addr;

	/*
	 * The latest EXT message we sent. We keep it because we have to
	 * retransmit it and to check if a received RESPONSE matches the
	 * EXT sent earlier.
	 */
	msg::ntlp_msg *last_ext_msg;


	/*
	 * State machine methods:
	 */
	state_t handle_state_idle(dispatcher *d, event *evt);
	state_t handle_state_idle_ext(dispatcher *d, msg_event *e);
	state_t handle_state_waitresp(dispatcher *d,  event *evt);
	state_t handle_state_session(dispatcher *d,  event *evt);

	void setup_session(api_ext_event *evt);

	inline uint32 get_lifetime() const { return lifetime; }
	inline void set_lifetime(uint32 seconds) { lifetime = seconds; }

	inline uint32 get_max_lifetime() const { return max_lifetime; }
	inline void set_max_lifetime(uint32 t) { max_lifetime = t; }

	ntlp::mri *get_mri() const;
	void set_mri(ntlp::mri *m);

	friend std::ostream &operator<<(std::ostream &out,
		const nf_non_edge_ext_session &s);
};

std::ostream &operator<<(std::ostream &out, const nf_non_edge_ext_session &s);

inline bool nf_non_edge_ext_session::is_final() const {
	return get_state() == STATE_FINAL;
}

inline msg::ntlp_msg *nf_non_edge_ext_session::get_last_ext_message() const {
	assert( last_ext_msg != NULL );
	return last_ext_msg;
}

inline void nf_non_edge_ext_session::set_last_ext_message(msg::ntlp_msg *msg) {
	if ( last_ext_msg != NULL )
		delete last_ext_msg;
	last_ext_msg = msg;
}

inline ntlp::mri *nf_non_edge_ext_session::get_mri() const {
	return routing_info;
}

inline void nf_non_edge_ext_session::set_mri(ntlp::mri *m) {
	delete routing_info;
	routing_info = m;
}


} // namespace natfw

#endif // NATFW__NF_NON_EDGE_EXT_SESSION_H
