/// ----------------------------------------*- mode: C++; -*--
/// @file events.h
/// Event classes.
/// ----------------------------------------------------------
/// $Id: events.h 2856 2008-02-15 12:48:23Z bless $
/// $HeadURL: https://svn.tm.kit.edu/nsis/natfw-nslp/trunk/include/events.h $
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
#ifndef NATFW__EVENTS_H
#define NATFW__EVENTS_H

#include "address.h"
#include "protlib_types.h"
#include "timer_module.h"

#include "mri_pc.h"	// from NTLP
#include "mri_le.h"	// from NTLP

#include "messages.h"
#include "msg/ntlp_msg.h"
#include "session.h"
#include "policy_rule.h"
#include "natfw_timers.h"


namespace natfw {
  using namespace msg;
  using protlib::uint32;
  using protlib::hostaddress;
  using std::ostream;


/**
 * An abstract event.
 */
class event {
	
  public:
	virtual ~event();
	session_id *get_session_id() const { return sid; }
	virtual ostream &print(ostream &out) const { return out << "[event]"; }

  protected:
	event(session_id *sid=NULL) : sid(sid) { };

  private:
	session_id *sid;
};

inline event::~event() {
	if ( sid != NULL )
		delete sid;
}


inline ostream &operator<<(ostream &out, const event &e) {
	return e.print(out);
}


/**
 * This event is triggered for GIST NetworkNotifications.
 *
 * Usually, GIST informs us that a new Messaging Association (MA) has been
 * created.
 */
class network_notification_event : public event {
  public:
	network_notification_event() : event() { }
	virtual ~network_notification_event() { }
	virtual ostream &print(ostream &out) const {
		return out << "[network_notification_event]"; }
};


/**
 * Sent if GIST detected that a route changed and is no longer valid.
 *
 * This event is triggered by a GIST NetworkNotification.
 */
class route_changed_bad_event : public event {
  public:
	route_changed_bad_event(session_id *sid) : event(sid) { }
	virtual ~route_changed_bad_event() { }
	virtual ostream &print(ostream &out) const {
		return out << "[route_changed_bad_event]"; }

};


/**
 * Sent if GIST can't establish a connection to the next node.
 *
 * This event is triggered for GIST MessageStatus notifications if no GIST
 * node was found.
 */
class no_next_node_found_event : public event {
  public:
	no_next_node_found_event(session_id *sid) : event(sid) { }
	virtual ~no_next_node_found_event() { }
	virtual ostream &print(ostream &out) const {
		return out << "[no_next_node_found_event]"; }
};


/**
 * This event is triggered for incoming NTLP messages without NSLP payload.
 *
 * In this case, the event handler has to decide whether the local NTLP
 * instance should establish a session with the source NTLP instance or not.
 */
class routing_state_check_event : public event {
  public:
	routing_state_check_event(session_id *sid,
		ntlp::mri *msg_routing_info=NULL)
		: event(sid), mri(msg_routing_info) { }
	virtual ~routing_state_check_event();

	ntlp::mri *get_mri() const { return mri; }

	virtual ostream &print(ostream &out) const {
		return out << "[routing_state_check_event]"; }

  private:
	ntlp::mri *mri;
};

inline routing_state_check_event::~routing_state_check_event() {
	if ( mri != NULL )
		delete mri;
}


class msg_event : public event {
  public:
	msg_event(session_id *sid, ntlp_msg *msg, bool for_this_node=false);
	virtual ~msg_event();

	bool is_for_this_node() const { return for_this_node; }
	ntlp_msg *get_ntlp_msg()  const { return msg; }
	ntlp::mri *get_mri() const { return msg->get_mri(); }
	uint32 get_sii_handle() const { return msg->get_sii_handle(); }

	natfw_msg *get_natfw_msg() const;
	natfw_response *get_response() const;
	natfw_create *get_create() const;
	natfw_ext *get_ext() const;

	virtual ostream &print(ostream &out) const {
		return out << "[msg_event]"; }

  private:
	ntlp_msg *msg;
	bool for_this_node;
};

inline msg_event::msg_event(session_id *sid, ntlp_msg *msg, bool for_this_node)
		: event(sid), msg(msg), for_this_node(for_this_node) {

	// sid may be NULL for the test suite
	assert( msg != NULL );
}

inline msg_event::~msg_event() {
	if ( msg != NULL )
		delete msg;
}

inline natfw_msg *msg_event::get_natfw_msg() const {
	assert( msg != NULL );
	return msg->get_natfw_msg();
}

inline natfw_create *msg_event::get_create() const {
	assert( msg != NULL );
	return dynamic_cast<natfw_create *>(msg->get_natfw_msg());
}

inline natfw_response *msg_event::get_response() const {
	assert( msg != NULL );
	return dynamic_cast<natfw_response *>(msg->get_natfw_msg());
}

inline natfw_ext *msg_event::get_ext() const {
	assert( msg != NULL );
	return dynamic_cast<natfw_ext *>(msg->get_natfw_msg());
}

class timer_event : public event {
  public:
	timer_event(session_id *sid, id_t id) : event(sid), id(id) { };
	virtual ~timer_event() { };

	inline id_t get_id() const { return id; }
	inline bool is_timer(timer t) const { return this->id == t.get_id(); }

	virtual ostream &print(ostream &out) const {
		return out << "[timer_event]"; }

  private:
	id_t id;
};


class api_event : public event {
  public:
	api_event(session_id *sid=NULL) : event(sid) { };
	virtual ~api_event() { };
};


/**
 * An API request to send a CREATE message.
 *
 * Conditions: tg_CREATE and tg_CREATE_PROXY
 */
class api_create_event : public api_event {
  public:
	api_create_event(const hostaddress &source, const hostaddress &dest,
		uint16 source_port=0, uint16 dest_port=0, uint8 protocol=0,
		std::list<uint8> icmp_types=std::list<uint8>(),
		uint32 lifetime=0, bool proxy=false, FastQueue *rq=NULL)
		: api_event(), source_addr(source), dest_addr(dest),
		  source_port(source_port), dest_port(dest_port),
		  protocol(protocol), icmp_types(icmp_types),
		  session_lifetime(lifetime), proxy(proxy), return_queue(rq) { }

	virtual ~api_create_event() { }

	inline hostaddress get_source_address() const { return source_addr; }
	inline uint16 get_source_port() const { return source_port; }

	inline hostaddress get_destination_address() const { return dest_addr; }
	inline uint16 get_destination_port() const { return dest_port; }

	inline uint8 get_protocol() const { return protocol; }
	inline const std::list<uint8> &get_icmp_types() const {
		return icmp_types; }

	inline uint32 get_session_lifetime() const { return session_lifetime; }
	inline bool is_proxy_mode() const { return proxy; }

	inline FastQueue *get_return_queue() const { return return_queue; }

	virtual ostream &print(ostream &out) const {
		return out << "[api_create_event]"; }

  private:
	hostaddress source_addr;
	hostaddress dest_addr;
	uint16 source_port;
	uint16 dest_port;
	uint8 protocol;
	std::list<uint8> icmp_types;

	uint32 session_lifetime;
	bool proxy;

	FastQueue *return_queue;
};


/**
 * An API request to send a EXT message.
 *
 * You have to pass the IP address of this node (the NI+ address) and the
 * address of the NI. If the NI's address isn't known in advance, you have to
 * set it to the so-called signaling destination address (SDA) and set the
 * ni_is_known flag to false. The SDA is an address which must be outside of
 * the local domain, beyond the edge NAT or firewall and on the path between
 * NR and the NI.
 *
 * Condition: tg_EXT
 */
class api_ext_event : public api_event {
  public:
	api_ext_event(fw_policy_rule::action_t action,
		const hostaddress &nr_address, uint8 nr_prefix, uint16 nr_port,
		const hostaddress &ni_address, uint8 ni_prefix, uint16 ni_port,
		uint8 protocol, uint32 lifetime=0,
		bool ni_address_is_known=true, bool proxy=false,
		FastQueue *rq=NULL)
		: api_event(), action(action),
		  nr_address(nr_address), nr_prefix(nr_prefix),
		  nr_port(nr_port),
		  ni_address(ni_address), ni_prefix(ni_prefix),
		  ni_port(ni_port),
		  protocol(protocol),
		  session_lifetime(lifetime),
		  ni_address_known(ni_address_is_known), proxy(proxy),
		  return_queue(rq) { }

	virtual ~api_ext_event() { }

	inline fw_policy_rule::action_t get_action() const { return action; }

	inline hostaddress get_nr_address() const { return nr_address; }
	inline uint8 get_nr_prefix() const { return nr_prefix; }
	inline uint16 get_nr_port() const { return nr_port; }

	inline hostaddress get_ni_address() const { return ni_address; }
	inline uint8 get_ni_prefix() const { return ni_prefix; }
	inline uint16 get_ni_port() const { return ni_port; }

	inline uint8 get_protocol() const { return protocol; }
	inline bool is_ni_address_known() const { return ni_address_known; }

	inline bool is_proxy_mode() const { return proxy; }
	inline uint32 get_session_lifetime() const { return session_lifetime; }
	inline FastQueue *get_return_queue() const { return return_queue; }

	virtual ostream &print(ostream &out) const {
		return out << "[api_ext_event]"; }

  private:
	fw_policy_rule::action_t action;

	hostaddress nr_address;
	uint8 nr_prefix;
	uint16 nr_port;
	hostaddress ni_address;
	uint8 ni_prefix;
	uint16 ni_port;
	uint8 protocol;
	uint32 session_lifetime;
	bool ni_address_known;
	bool proxy;

	FastQueue *return_queue;
};


/**
 * An API request to shut down a session.
 *
 * Conditions: tg_TEARDOWN and tg_TEARDOWN_PROXY
 */
class api_teardown_event : public api_event {
  public:
	api_teardown_event(session_id *sid, bool proxy=false)
		: api_event(sid), proxy(proxy) { }
	virtual ~api_teardown_event() { }

	bool is_proxy_mode() const { return proxy; }

	virtual ostream &print(ostream &out) const {
		return out << "[api_teardown_event]"; }

  private:
	bool proxy;
};


/**
 * Check if the event is a timer event with the given timer ID.
 */
inline bool is_timer(const event *evt, timer t) {
	const timer_event *e = dynamic_cast<const timer_event *>(evt);

	if ( e == NULL )
		return false;
	else
		return e->is_timer(t);
}


inline bool is_timer(const event *evt) {
	return dynamic_cast<const timer_event *>(evt) != NULL;
}


inline bool is_api_create(const event *evt) {
	return dynamic_cast<const api_create_event *>(evt) != NULL;
}

inline bool is_api_ext(const event *evt) {
	return dynamic_cast<const api_ext_event *>(evt) != NULL;
}

inline bool is_api_teardown(const event *evt) {
	return dynamic_cast<const api_teardown_event *>(evt) != NULL;
}

inline bool is_routing_state_check(const event *evt) {
	return dynamic_cast<const routing_state_check_event *>(evt) != NULL;
}

inline bool is_route_changed_bad_event(const event *evt) {
	return dynamic_cast<const route_changed_bad_event *>(evt) != NULL;
}

inline bool is_no_next_node_found_event(const event *evt) {
	return dynamic_cast<const no_next_node_found_event *>(evt) != NULL;
}

inline bool is_natfw_create(const event *evt) {
	const msg_event *e = dynamic_cast<const msg_event *>(evt);

	if ( e == NULL )
		return false;
	else
		return e->get_create() != NULL;
}

inline bool is_natfw_response(const event *evt) {
	const msg_event *e = dynamic_cast<const msg_event *>(evt);

	return e != NULL && e->get_response() != NULL;
}

inline bool is_natfw_response(const event *evt, uint32 msn) {
	const msg_event *e = dynamic_cast<const msg_event *>(evt);

	if ( e == NULL )
		return false;

	natfw_response *r = e->get_response();
	if ( r == NULL )
		return false;

	return r->has_msg_sequence_number()
		&& msn == r->get_msg_sequence_number();
}

inline bool is_natfw_response(const event *evt, ntlp_msg *msg) {
	if ( msg == NULL )
		return false;

	natfw_msg *m = msg->get_natfw_msg();

	if ( m == NULL || ! m->has_msg_sequence_number() )
		return false;

	return is_natfw_response(evt, m->get_msg_sequence_number());
}

inline bool is_natfw_ext(const event *evt) {
	const msg_event *e = dynamic_cast<const msg_event *>(evt);

	if ( e == NULL )
		return false;
	else
		return e->get_ext() != NULL;
}

inline bool is_invalid_natfw_msg(const event *evt) {
	const msg_event *e = dynamic_cast<const msg_event *>(evt);

	if ( e == NULL )
		return false;
	else
		return e->get_natfw_msg() == NULL;
}


/**
 * Wrap a natfw event in a protlib message.
 *
 * This may be used to handle natfw::event classes using FastQueues.
 *
 * Note: The event is not deleted by the destructor!
 */
class NatFwEventMsg : public message {
  public:
	NatFwEventMsg(session_id id, event *e, qaddr_t source=qaddr_unknown)
		: message(message::type_transport, source), sid(id), evt(e) { }

	session_id get_session_id() const { return sid; }
	event *get_event() const { return evt; }

  private:
	session_id sid;
	event *evt;
};


} // namespace natfw


#endif // NATFW__EVENTS_H
