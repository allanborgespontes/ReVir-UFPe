/// ----------------------------------------*- mode: C++; -*--
/// @file ni_session.cpp
/// The session for an NSIS Initiator.
/// ----------------------------------------------------------
/// $Id: ni_session.cpp 3165 2008-07-10 22:36:29Z bless $
/// $HeadURL: https://svn.tm.kit.edu/nsis/natfw-nslp/trunk/src/ni_session.cpp $
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
#include <cmath>
#include <sstream>
#include <openssl/rand.h>

#include "logfile.h"

#include "natfw_config.h"
#include "events.h"
#include "msg/natfw_msg.h"
#include "dispatcher.h"
#include "ni_session.h"


using namespace natfw;
using namespace protlib::log;
using protlib::uint32;


#define LogWarn(msg) Log(WARNING_LOG, LOG_NORMAL, "ni_session", msg)
#define LogInfo(msg) Log(INFO_LOG, LOG_NORMAL, "ni_session", msg)
#define LogDebug(msg) Log(DEBUG_LOG, LOG_NORMAL, "ni_session", msg)



/**
 * Constructor.
 *
 * Use this if the session ID is known in advance.
 */
ni_session::ni_session(const session_id &id, const natfw_config *conf)
		: session(id), state(STATE_IDLE), routing_info(NULL),
		  last_create_msg(NULL), lifetime(0),
		  refresh_interval(20), response_timeout(0), create_counter(0),
		  max_retries(0), proxy_session(false),
		  response_timer(this), refresh_timer(this) {

	set_msg_sequence_number(create_random_number());
	set_nonce(create_random_number());

	assert( conf != NULL );
	set_response_timeout(conf->get_ni_response_timeout());
	set_max_retries(conf->get_ni_max_retries());
}


/**
 * Constructor for test cases.
 *
 * @param s the state to start in
 */
ni_session::ni_session(state_t s)
		: session(), state(s), routing_info(NULL),
		  last_create_msg(NULL), lifetime(30),
		  refresh_interval(20), response_timeout(2), create_counter(0),
		  max_retries(3), proxy_session(false),
		  response_timer(this), refresh_timer(this) {

	set_msg_sequence_number(create_random_number());
	set_nonce(create_random_number());

	// for testing, we create an empty MRI
	routing_info = new ntlp::mri_pathcoupled();
}


/**
 * Destructor.
 */
ni_session::~ni_session() {
	delete routing_info;
	delete last_create_msg;
}


/**
 * Generate a 32 Bit random number.
 */
uint32 ni_session::create_random_number() const {
	unsigned value;
	int ret = RAND_bytes((unsigned char *) &value, sizeof(value));
	assert( ret == 1 );

	return value;
}


std::ostream &natfw::operator<<(std::ostream &out, const ni_session &s) {
	static const char *const names[] = { "IDLE", "WAITRESP", "SESSION", "FINAL" };

	return out << "[ni_session: id=" << s.get_id()
		<< ", state=" << names[s.get_state()] << "]";
}



/****************************************************************************
 *
 * state machine part
 *
 ****************************************************************************/


/**
 * Build a NATFW CREATE message based on the session's state.
 *
 * This will fetch the session lifetime and also increment the MSN.
 */
msg::ntlp_msg *ni_session::build_create_message() {
	using namespace natfw::msg;

	assert( get_mri() != NULL );

	/*
	 * Build a NATFW CREATE message.
	 */
	natfw_create *create = new natfw_create(is_proxy_mode());
	create->set_session_lifetime(get_lifetime());
	create->set_extended_flow_info(extended_flow_info::ra_allow, 0);
	create->set_msg_sequence_number(next_msg_sequence_number());

	if ( get_icmp_types().size() != 0 )
		create->set_icmp_types(get_icmp_types());

	// TODO: [credentials]

	if ( is_proxy_mode() || is_proxy_session() )
		create->set_nonce(get_nonce());


	/*
	 * Wrap the CREATE inside an ntlp_msg and add session ID and MRI.
	 */
	ntlp_msg *msg = new ntlp_msg(get_id(), create, get_mri()->copy(), 0);

	return msg;
}


void ni_session::setup_session(api_create_event *e) {
	
	/*
	 * Create the message routing information (MRI) which we're going to
	 * use for all messages.
	 */
	uint8 src_prefix = 32;
	uint8 dest_prefix = 32;
	uint16 flow_label = 0;
	uint16 traffic_class = 0;		// DiffServ CodePoint
	uint32 ipsec_spi = 0;			// IPsec SPI
	bool downstream = true;

	ntlp::mri *nslp_mri = new ntlp::mri_pathcoupled(
		e->get_source_address(), src_prefix,
		e->get_source_port(),
		e->get_destination_address(), dest_prefix,
		e->get_destination_port(),
		e->get_protocol(), flow_label, traffic_class, ipsec_spi,
		downstream
	);

	set_mri(nslp_mri);

	if ( e->get_session_lifetime() != 0 )
		set_lifetime(e->get_session_lifetime());

	uint32 r = (uint32) ceil(
		get_lifetime() / ( (get_max_retries()+0.5) * 1.5 ) );
	assert( r > 0 );

	set_refresh_interval(r); 

	uint32 t = (int) ceil( ( ( (rand() % 1000) / 1000.0) + 0.5 ) * r );

	set_response_timeout( t );


	LogDebug("using lifetime: " << get_lifetime() << " seconds");
	LogDebug("using refresh interval: " << r << " seconds");
	LogDebug("using response timeout: " << t << " seconds");

	set_proxy_mode(e->is_proxy_mode());
}


/*
 * state: IDLE
 */
ni_session::state_t ni_session::handle_state_idle(dispatcher *d, event *evt) {
	using msg::natfw_create;
	using msg::extended_flow_info;


	/*
	 * API Create event received.
	 */
	if ( is_api_create(evt) ) {
		api_create_event *e = dynamic_cast<api_create_event *>(evt);

		// initialize the session based on user-provided parameters
		setup_session(e);

		set_last_create_message( build_create_message() );

		d->send_message( get_last_create_message()->copy() );

		response_timer.start(d, get_response_timeout());

		if ( e->get_return_queue() != NULL ) {
			message *m = new NatFwEventMsg(get_id(), NULL);
			e->get_return_queue()->enqueue(m);
		}

		return STATE_WAITRESP;
	}
	/*
	 * An invalid event has been received.
	 */
	else {
		LogInfo("discarding unexpected event " << *evt);
		return STATE_FINAL;
	}
}


/*
 * state: WAITRESP
 */
ni_session::state_t ni_session::handle_state_waitresp(
		dispatcher *d, event *evt) {

	using namespace natfw::msg;

	/*
	 * A msg_event arrived which contains a NATFW RESPONSE message.
	 */
	if ( is_natfw_response(evt) ) {
		msg_event *e = dynamic_cast<msg_event *>(evt);
		natfw_response *resp = e->get_response();

		LogDebug("received response " << *resp);

		// Discard if this is no RESPONSE to our original CREATE.
		natfw_create *c = get_last_create_message()->get_natfw_create();
		if ( ! resp->is_response_to(c) ) {
			LogWarn("RESPONSE doesn't match CREATE, discarding");
			return STATE_WAITRESP; // no change
		}

		if ( resp->is_success() ) {
			LogDebug("initiated session " << get_id());
			d->report_async_event("CREATE session initiated");

			response_timer.stop();
			refresh_timer.start(d, get_refresh_interval());

			set_create_counter(0);

			return STATE_SESSION;
		}
		else {
			d->report_async_event("cannot initiate CREATE session");
			return STATE_FINAL;
		}
	}
	/*
	 * A response timeout was triggered.
	 */
	else if ( is_timer(evt, response_timer) ) {

		// Retry. Send the CREATE message again and start a new timer.
		if ( get_create_counter() < get_max_retries() ) {
			LogWarn("response timeout, restarting timer.");
			inc_create_counter();
			d->send_message( get_last_create_message()->copy() );

			response_timer.start(d, get_response_timeout());

			return STATE_WAITRESP; // no change
		}
		// Retry count exceeded, abort.
		else {
			d->report_async_event("got no response for our CREATE");
			return STATE_FINAL;
		}
	}
	/*
	 * The NTLP can't reach the destination.
	 */
	else if ( is_no_next_node_found_event(evt) ) {
		LogInfo("cannot reach destination");

		return STATE_FINAL;
	}
	/*
	 * API teardown event received.
	 */
	else if ( is_api_teardown(evt) ) {
		LogDebug("received API teardown event");

		// Send a CREATE message with a session lifetime of 0.
		set_lifetime(0);
		set_last_create_message(NULL);

		d->send_message( build_create_message() );

		return STATE_FINAL;
	}
	/*
	 * Outdated timer event, discard and don't log.
	 */
	else if ( is_timer(evt) ) {
		return STATE_WAITRESP; // no change
	}
	/*
	 * Some other, unexpected event arrived.
	 */
	else {
		LogInfo("discarding unexpected event " << *evt);
		return STATE_WAITRESP; // no change
	}
}


/*
 * state: SESSION
 */
ni_session::state_t ni_session::handle_state_session(
		dispatcher *d, event *evt) {

	using namespace natfw::msg;

	/*
	 * A NATFW RESPONSE message arrived in response to our CREATE.
	 */
	if ( is_natfw_response(evt, get_last_create_message()) ) {
		msg_event *e = dynamic_cast<msg_event *>(evt);
		natfw_response *resp = e->get_response();

		LogDebug("received RESPONSE: " << *resp);

		/*
		 * Discard if this is no RESPONSE to our last CREATE.
		 * Note that the last CREATE may also be the initial CREATE.
		 */
		natfw_create *c = get_last_create_message()->get_natfw_create();
		if ( ! resp->is_response_to(c) ) {
			LogWarn("RESPONSE doesn't match CREATE, discarding");
			return STATE_SESSION; // no change
		}

		if ( resp->is_success() ) {
			d->report_async_event("CREATE refresh successful");

			response_timer.stop();
			refresh_timer.start(d, get_refresh_interval());

			set_create_counter(0);

			return STATE_SESSION; // no change
		}
		else {
			d->report_async_event("CREATE session died");
			return STATE_FINAL;
		}
	}
	/*
	 * A refresh timer was triggered.
	 */
	else if ( is_timer(evt, refresh_timer) ) {

		LogDebug("received refresh timer");

		// Build a new CREATE message store a copy for refreshing.
		set_last_create_message( build_create_message() );

		// dispatcher will delete
		d->send_message( get_last_create_message()->copy() );

		response_timer.start(d, get_response_timeout());

		return STATE_SESSION; // no change
	}
	/*
	 * A response timout was triggered.
	 */
	else if ( is_timer(evt, response_timer) ) {

		LogDebug("received response timeout timer");

		// Retry. Send the CREATE message again and start a new timer.
		if ( get_create_counter() < get_max_retries() ) {
			inc_create_counter();

			d->send_message( get_last_create_message()->copy() );

			response_timer.start(d, get_response_timeout());

			return STATE_SESSION; // no change
		}
		// Retry count exceeded, abort.
		else {
			d->report_async_event("got no response to our CREATE");
			return STATE_FINAL;
		}
	}
	/*
	 * API teardown event received. The user wants to end the session.
	 */
	else if ( is_api_teardown(evt) ) {
		LogDebug("received API teardown event");

		// Send a CREATE message with a session lifetime of 0.
		set_lifetime(0);
		set_last_create_message(NULL);

		// dispatcher will delete
		d->send_message( build_create_message() );

		return STATE_FINAL;
	}
	/*
	 * Outdated timer event, discard and don't log.
	 */
	else if ( is_timer(evt) ) {
		return STATE_SESSION; // no change
	}
	/*
	 * Some other, unexpected event arrived.
	 */
	else {
		LogInfo("discarding unexpected event " << *evt);
		return STATE_SESSION; // no change
	}
}


/**
 * Process an event.
 *
 * This method implements the transition function of the state machine.
 */
void ni_session::process_event(dispatcher *d, event *evt) {
	LogDebug("begin process_event(): " << *this);

	switch ( get_state() ) {

		case ni_session::STATE_IDLE:
			state = handle_state_idle(d, evt);
			break;

		case ni_session::STATE_WAITRESP:
			state = handle_state_waitresp(d, evt);
			break;

		case ni_session::STATE_SESSION:
			state = handle_state_session(d, evt);
			break;

		case ni_session::STATE_FINAL:
			// ignore event and continue
			break;

		default:
			assert( false ); // invalid state
	}

	LogDebug("end process_event(): " << *this);
}

// EOF
