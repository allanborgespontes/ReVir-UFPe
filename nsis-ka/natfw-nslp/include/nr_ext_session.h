/// ----------------------------------------*- mode: C++; -*--
/// @file nr_ext_session.h
/// The nr_ext_session class.
/// ----------------------------------------------------------
/// $Id: nr_ext_session.h 2558 2007-04-04 15:17:16Z bless $
/// $HeadURL: https://svn.tm.kit.edu/nsis/natfw-nslp/trunk/include/nr_ext_session.h $
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
#ifndef NATFW__NR_EXT_SESSION_H
#define NATFW__NR_EXT_SESSION_H

#include "protlib_types.h"

#include "session.h"
#include "events.h"
#include "msg/ntlp_msg.h"


namespace natfw {
    using protlib::uint32;
    using protlib::hostaddress;

class event;
class api_ext_event;

/**
 * A session for the EXT part of a responder (the NI+).
 *
 * This is session is initiated by an application on the local node.
 */
class nr_ext_session : public session {
  public:
	nr_ext_session(const session_id &id, natfw_config *conf);
	~nr_ext_session();

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

	nr_ext_session(state_t s=STATE_IDLE);

	void process_event(dispatcher *d, event *evt);
	inline state_t get_state() const { return state; }

	inline bool is_proxy_mode() const { return proxy_mode; }
	inline void set_proxy_mode(bool value) { proxy_mode = value; }

	inline timer &get_response_timer() { return response_timer; }
	inline timer &get_refresh_timer() { return refresh_timer; }

	inline uint32 get_response_timeout() const { return response_timeout; }
	inline void set_response_timeout(uint32 t) { response_timeout = t; }

	inline uint32 get_max_retries() const { return max_retries; }
	inline void set_max_retries(uint32 m) { max_retries = m; }

	inline uint32 get_refresh_interval() const { return refresh_interval; }
	inline void set_refresh_interval(uint32 t) { refresh_interval = t; }


	msg::ntlp_msg *get_last_ext_message() const;
	void set_last_ext_message(msg::ntlp_msg *msg);

	uint32 get_ext_counter() const { return ext_counter; }
	void inc_ext_counter() { ext_counter++; }
	void set_ext_counter(uint32 num) { ext_counter = num; }


  private:
	state_t state;

	bool proxy_mode;

	/*
	 * This is a pre-built NTLP message which we can copy and adjust each
	 * time a new EXT message has to be sent.
	 */
	msg::ntlp_msg *ext_template;

	/*
	 * The last EXT message we sent. We keep it because we have to
	 * retransmit it and to check if a received RESPONSE matches the
	 * EXT sent earlier.
	 */
	msg::ntlp_msg *last_ext_msg;


	uint32 lifetime;
	uint32 refresh_interval;
	uint32 response_timeout;
	uint32 ext_counter; 
	uint32 max_retries;

	// We have two types of timers: RESPONSE and REFRESH.
	timer response_timer;
	timer refresh_timer;


	/*
	 * State machine methods:
	 */
	state_t handle_state_idle(dispatcher *d, event *evt);
	state_t handle_state_waitresp(dispatcher *d,  event *evt);
	state_t handle_state_session(dispatcher *d,  event *evt);

	void setup_session(api_ext_event *evt);
	msg::ntlp_msg *build_ext_message();
	msg::ntlp_msg *copy_ext_template() const;


	void create_random_msn();

	inline uint32 get_lifetime() const { return lifetime; }
	inline void set_lifetime(uint32 seconds) { lifetime = seconds; }

	void set_ext_template(msg::ntlp_msg *msg);

	friend std::ostream &operator<<(std::ostream &out,
		const nr_ext_session &s);
};

std::ostream &operator<<(std::ostream &out, const nr_ext_session &s);

inline bool nr_ext_session::is_final() const {
	return get_state() == STATE_FINAL;
}

inline msg::ntlp_msg *nr_ext_session::get_last_ext_message() const {
	assert( last_ext_msg != NULL );
	return last_ext_msg;
}

inline void nr_ext_session::set_last_ext_message(msg::ntlp_msg *msg) {
	delete last_ext_msg;
	last_ext_msg = msg;
}

inline msg::ntlp_msg *nr_ext_session::copy_ext_template() const {
	assert( ext_template != NULL );
	return ext_template->copy();
}

inline void nr_ext_session::set_ext_template(msg::ntlp_msg *msg) {
	delete ext_template;
	ext_template = msg;
}


} // namespace natfw

#endif // NATFW__NR_EXT_SESSION_H
