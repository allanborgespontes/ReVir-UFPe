/// ----------------------------------------*- mode: C++; -*--
/// @file nf_session.h
/// The nf_session class.
/// ----------------------------------------------------------
/// $Id: nf_session.h 2558 2007-04-04 15:17:16Z bless $
/// $HeadURL: https://svn.tm.kit.edu/nsis/natfw-nslp/trunk/include/nf_session.h $
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
#ifndef NATFW__NF_SESSION_H
#define NATFW__NF_SESSION_H

#include "protlib_types.h"

#include "session.h"
#include "events.h"
#include "policy_rule.h"
#include "msg/natfw_msg.h"


namespace natfw {
    using protlib::uint32;
    using protlib::hostaddress;

class event;
class msg_event;

/**
 * A session for a forwarder.
 *
 * This does *not* include the EXT part.
 */
class nf_session : public session {
  public:
	nf_session(const session_id &id, const natfw_config *conf);
	~nf_session();

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

	nf_session(state_t s, const natfw_config *conf);

	void process_event(dispatcher *d, event *evt);
	state_t get_state() const { return state; }

	uint32 get_lifetime() const { return lifetime; }
	void set_lifetime(uint32 seconds) { lifetime = seconds; }

	inline timer &get_state_timer() { return state_timer; }
	inline timer &get_response_timer() { return response_timer; }

	inline uint32 get_max_lifetime() const { return max_lifetime; }
	inline void set_max_lifetime(uint32 t) { max_lifetime = t; }

	inline uint32 get_response_timeout() const { return response_timeout; }
	inline void set_response_timeout(uint32 t) { response_timeout = t; }

	msg::ntlp_msg *get_last_create_message() const;
	void set_last_create_message(msg::ntlp_msg *msg);
	bool get_last_create_message_proxy_mode() const;

	inline bool is_proxy_mode() const { return proxy_mode; }
	inline void set_proxy_mode(bool value) { proxy_mode = value; }

	ntlp::mri_pathcoupled *get_ni_mri() const;
	void set_ni_mri(ntlp::mri_pathcoupled *m);

	ntlp::mri_pathcoupled *get_nr_mri() const;
	void set_nr_mri(ntlp::mri_pathcoupled *m);

	fw_policy_rule *get_fw_policy_rule() const;
	void set_fw_policy_rule(fw_policy_rule *r);

	nat_policy_rule *get_nat_policy_rule() const;
	void set_nat_policy_rule(nat_policy_rule *r);


  private:
	state_t state;

	const natfw_config *config;	// shared among sessions, don't delete!

	bool proxy_mode;
	uint32 lifetime;
	uint32 max_lifetime;
	uint32 response_timeout;
	timer state_timer;
	timer response_timer;

	ntlp::mri_pathcoupled *ni_mri;	// the MRI to use for reaching the NI
	ntlp::mri_pathcoupled *nr_mri;	// the MRI to use for reaching the NR
	msg::ntlp_msg *create_message;
	fw_policy_rule *fw_rule;
	nat_policy_rule *nat_rule;

	/*
	 * An NF session has to reserve an external address if it is a NAT and
	 * a CREATE comes from the private side.
	 */
	bool ext_addr_reserved;		// true if ext_addr is valid
	appladdress ext_addr;

	state_t handle_state_idle(dispatcher *d, event *evt);
	state_t handle_state_waitresp(dispatcher *d, event *evt);
	state_t handle_state_session(dispatcher *d, event *evt);

	state_t handle_state_idle_create(dispatcher *d, msg_event *e);


	ntlp::mri_pathcoupled *create_mri_inverted(
		ntlp::mri_pathcoupled *orig_mri) const;
	ntlp::mri_pathcoupled *create_mri_with_dest(
		ntlp::mri_pathcoupled *orig_mri, appladdress priv_addr) const;
	ntlp::mri_pathcoupled *create_mri_with_source(
		ntlp::mri_pathcoupled *orig_mri, appladdress priv_addr) const;

	ntlp_msg *create_msg_for_nr(ntlp_msg *msg) const;
	ntlp_msg *create_msg_for_ni(ntlp_msg *msg) const;

	bool save_fw_policy_rule(msg_event *evt);
	bool save_nat_policy_rule(msg_event *evt, const appladdress &dr_addr,
		nat_policy_rule::type_t nat_type);

	fw_policy_rule *get_fw_policy_rule_copy() const;
	nat_policy_rule *get_nat_policy_rule_copy() const;

	void set_external_address(const appladdress &addr);
	appladdress get_external_address() const { return ext_addr; }
	inline bool is_external_address_reserved() { return ext_addr_reserved; }

	friend std::ostream &operator<<(std::ostream &out, const nf_session &s);
};

std::ostream &operator<<(std::ostream &out, const nf_session &s);


inline bool nf_session::is_final() const {
	return get_state() == STATE_FINAL;
}

inline void nf_session::set_last_create_message(msg::ntlp_msg *msg) {
	delete create_message;
	create_message = msg;
}

inline msg::ntlp_msg *nf_session::get_last_create_message() const {
	assert( create_message != NULL );
	return create_message;
}

inline bool nf_session::get_last_create_message_proxy_mode() const {
	natfw_create *create =  get_last_create_message()->get_natfw_create();
	assert( create != NULL );

	return create->is_proxy_mode();
}

inline ntlp::mri_pathcoupled *nf_session::get_ni_mri() const {
	assert( ni_mri != NULL );
	return ni_mri;
}

inline void nf_session::set_ni_mri(ntlp::mri_pathcoupled *mri) {
	delete ni_mri;
	ni_mri = mri;
}

inline ntlp::mri_pathcoupled *nf_session::get_nr_mri() const {
	assert( nr_mri != NULL );
	return nr_mri;
}

inline void nf_session::set_nr_mri(ntlp::mri_pathcoupled *mri) {
	delete nr_mri;
	nr_mri = mri;
}

inline void nf_session::set_fw_policy_rule(fw_policy_rule *r) {
	delete fw_rule;
	fw_rule = r;
}

inline fw_policy_rule *nf_session::get_fw_policy_rule() const {
	return fw_rule; // may return NULL!
}

inline void nf_session::set_nat_policy_rule(nat_policy_rule *r) {
	delete nat_rule;
	nat_rule = r;
}

inline nat_policy_rule *nf_session::get_nat_policy_rule() const {
	return nat_rule; // may return NULL!
}

inline void nf_session::set_external_address(const appladdress &addr) {
	ext_addr_reserved = true;
	ext_addr = addr;
}

} // namespace natfw

#endif // NATFW__NF_SESSION_H
