/// ----------------------------------------*- mode: C++; -*--
/// @file nr_session.h
/// The nr_session class.
/// ----------------------------------------------------------
/// $Id: nr_session.h 2558 2007-04-04 15:17:16Z bless $
/// $HeadURL: https://svn.tm.kit.edu/nsis/natfw-nslp/trunk/include/nr_session.h $
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
#ifndef NATFW__NR_SESSION_H
#define NATFW__NR_SESSION_H

#include "protlib_types.h"

#include "session.h"
#include "events.h"
#include "msg/ntlp_msg.h"


namespace natfw {
    using protlib::uint8;
    using protlib::uint32;
    using protlib::hostaddress;

class event;
class msg_event;

/**
 * A session for a responder.
 */
class nr_session : public session {
  public:
	nr_session(const session_id &id, natfw_config *conf);
	~nr_session();

	bool is_final() const; // inherited from session

  protected:
	/**
	 * States of a session.
	 */
	enum state_t {
		STATE_IDLE	= 0,
		STATE_SESSION	= 1,
		STATE_FINAL	= 2
	};

	void process_event(dispatcher *d, event *evt);
	state_t get_state() const;

	nr_session(state_t s=STATE_IDLE, uint32 msn=0);
	inline timer &get_state_timer() { return state_timer; }

  private:
	state_t state;

	natfw_config *config;		// shared among instances, don't delete!

	uint32 lifetime;
	uint32 max_lifetime;
	timer state_timer;

	/*
	 * State machine methods:
	 */
	state_t handle_state_idle(dispatcher *d, event *e);
	state_t handle_state_session(dispatcher *d, event *e);

	msg::ntlp_msg *build_trace_response(ntlp_msg *msg) const;

	/*
	 * Utility methods:
	 */
	uint32 get_lifetime() const { return lifetime; }
	void set_lifetime(uint32 seconds) { lifetime = seconds; }

	inline uint32 get_max_lifetime() const { return max_lifetime; }
	inline void set_max_lifetime(uint32 t) { max_lifetime = t; }

	friend std::ostream &operator<<(std::ostream &out, const nr_session &s);
};

std::ostream &operator<<(std::ostream &out, const nr_session &s);


inline nr_session::state_t nr_session::get_state() const {
	return state;
}

inline bool nr_session::is_final() const {
	return get_state() == STATE_FINAL;
}


} // namespace natfw

#endif // NATFW__NR_SESSION_H
