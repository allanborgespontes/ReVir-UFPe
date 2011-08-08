/// ----------------------------------------*- mode: C++; -*--
/// @file ni_session.h
/// ni_session class.
/// ----------------------------------------------------------
/// $Id: ni_session.h 2558 2007-04-04 15:17:16Z bless $
/// $HeadURL: https://svn.tm.kit.edu/nsis/natfw-nslp/trunk/include/ni_session.h $
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
#ifndef NATFW__NI_SESSION_H
#define NATFW__NI_SESSION_H

#include "protlib_types.h"

#include "natfw_config.h"
#include "session.h"
#include "events.h"
#include "natfw_timers.h"
#include "msg/ntlp_msg.h"


namespace natfw {
    using protlib::uint32;
    using protlib::hostaddress;

class event;
class api_create_event;

/**
 * A session for an initiator.
 *
 * An object of this class holds all the data for a NATFW signaling session
 * and also implements the NI state machine.
 *
 * There are two different attributes for proxy operation: proxy_mode and
 * proxy_session. If proxy_mode is set, then all outgoing CREATE messages
 * will contain a nonce and the P-flag is set. If proxy_session is set, then
 * this session has been created by another session and acts as a proxy for
 * the initiator which is non NATFW-NSLP aware.
 *
 * Note that the getters and setters are declared protected to make them
 * accessible from a subclass. This is required for the test suite.
 */
class ni_session : public session {
  public:
	ni_session(const session_id &id, const natfw_config *conf);
	~ni_session();

	bool is_final() const; // inherited from session

  protected:
	/**
	 * States of a session.
	 */
	enum state_t {
		STATE_IDLE	= 0,
		STATE_WAITRESP	= 1,
		STATE_SESSION	= 2,
		STATE_FINAL	= 3
	};

	ni_session(state_t s=STATE_IDLE);

	void process_event(dispatcher *d, event *evt);
	inline state_t get_state() const { return state; }

	void set_last_create_message(msg::ntlp_msg *msg);
	msg::ntlp_msg *get_last_create_message() const;

	inline void set_create_counter(uint32 num) { create_counter = num; }
	inline uint32 get_create_counter() const { return create_counter; }

	inline uint32 get_refresh_interval() const { return refresh_interval; }
	inline void set_refresh_interval(uint32 sec) { refresh_interval = sec; }

	inline uint32 get_lifetime() const { return lifetime; }
	inline void set_lifetime(uint32 seconds) { lifetime = seconds; }

	inline ntlp::mri *get_mri() const { return routing_info; }
	void set_mri(ntlp::mri *m);

	inline bool is_proxy_mode() const { return proxy_mode; }
	inline void set_proxy_mode(bool value) { proxy_mode = value; }

	inline bool is_proxy_session() const { return proxy_session; }

	inline uint32 get_nonce() const { return nonce; }
	void set_nonce(uint32 val);

	inline uint32 get_response_timeout() const { return response_timeout; }
	inline void set_response_timeout(uint32 t) { response_timeout = t; }

	inline uint32 get_max_retries() const { return max_retries; }
	inline void set_max_retries(uint32 m) { max_retries = m; }

	inline std::list<uint8> get_icmp_types() const { return icmp_types; }
	inline void set_icmp_types(const std::list<uint8> &value) {
		icmp_types = value; }

	inline timer &get_response_timer() { return response_timer; }
	inline timer &get_refresh_timer() { return refresh_timer; }


  private:
	state_t state;

	ntlp::mri *routing_info;
	std::list<uint8> icmp_types;

	/*
	 * The latest CREATE message we sent. We keep it because we need it for
	 * retransmission and to check if a received RESPONSE matches the
	 * CREATE sent earlier.
	 */
	msg::ntlp_msg *last_create_msg;


	bool proxy_mode;
	uint32 lifetime;
	uint32 refresh_interval;
	uint32 response_timeout;
	uint32 create_counter; 
	uint32 max_retries;

	/*
	 * Set proxy_session to true if this session has been created by an
	 * edge EXT session (nf_edge_ext_session). In this case, the nonce
	 * given below will be added to each outgoing CREATE message.
	 */
	bool proxy_session;
	uint32 nonce;

	/*
	 * For an NI session, we have two types of timers: RESPONSE and REFRESH.
	 * At any time, there may be only one timer active per type.
	 */
	timer response_timer;
	timer refresh_timer;

	/*
	 * State machine methods:
	 */
	state_t handle_state_idle(dispatcher *d, event *evt);
	state_t handle_state_waitresp(dispatcher *d, event *evt);
	state_t handle_state_session(dispatcher *d, event *evt);

	/*
	 * Utility methods:
	 */
	void setup_session(api_create_event *evt);
	msg::ntlp_msg *build_create_message();

	uint32 create_random_number() const;
	void inc_create_counter();

	friend std::ostream &operator<<(std::ostream &out, const ni_session &s);
	friend class session_manager;
};

std::ostream &operator<<(std::ostream &out, const ni_session &s);

inline bool ni_session::is_final() const {
	return get_state() == STATE_FINAL;
}

inline void ni_session::inc_create_counter() {
	create_counter++;
}

inline msg::ntlp_msg *ni_session::get_last_create_message() const {
	assert( last_create_msg != NULL );
	return last_create_msg;
}

inline void ni_session::set_last_create_message(msg::ntlp_msg *msg) {
	delete last_create_msg;
	last_create_msg = msg;
}

inline void ni_session::set_mri(ntlp::mri *m) {
	delete routing_info;
	routing_info = m;
}


/**
 * Set the nonce which is required if this is a proxy session.
 *
 * This also sets the proxy_session flag. Subsequent calls to
 * is_proxy_session() will return true.
 */
inline void ni_session::set_nonce(uint32 val) {
	nonce = val;
	proxy_session = true;
}


} // namespace natfw

#endif // NATFW__NI_SESSION_H
