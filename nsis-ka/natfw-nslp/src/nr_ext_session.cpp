/// ----------------------------------------*- mode: C++; -*--
/// @file nr_ext_session.cpp
/// A session for the EXT part of a responder (the NI+).
/// ----------------------------------------------------------
/// $Id: nr_ext_session.cpp 3165 2008-07-10 22:36:29Z bless $
/// $HeadURL: https://svn.tm.kit.edu/nsis/natfw-nslp/trunk/src/nr_ext_session.cpp $
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
#include "nr_ext_session.h"


using namespace natfw;
using namespace protlib::log;
using protlib::uint32;


#define LogWarn(msg) Log(WARNING_LOG, LOG_NORMAL, "nr_ext_session", msg)
#define LogInfo(msg) Log(INFO_LOG, LOG_NORMAL, "nr_ext_session", msg)
#define LogDebug(msg) Log(DEBUG_LOG, LOG_NORMAL, "nr_ext_session", msg)



/**
 * Constructor.
 *
 * Use this if the session ID is known in advance.
 */
nr_ext_session::nr_ext_session(const session_id &id, natfw_config *conf)
		: session(id), state(STATE_IDLE), proxy_mode(false),
		  ext_template(NULL), last_ext_msg(NULL),
		  lifetime(0), refresh_interval(20),
		  response_timeout(0), ext_counter(0), max_retries(0),
		  response_timer(this), refresh_timer(this) {

	create_random_msn();

	assert( conf != NULL );
	set_response_timeout(conf->get_nr_ext_response_timeout());
	set_max_retries(conf->get_nr_ext_max_retries());
}


/**
 * Constructor for test cases.
 *
 * @param s the state to start in
 */
nr_ext_session::nr_ext_session(state_t s)
		: session(), state(s), proxy_mode(false), ext_template(NULL),
		  last_ext_msg(NULL), lifetime(30),
		  refresh_interval(20), response_timeout(2), ext_counter(0),
		  max_retries(3), response_timer(this), refresh_timer(this) {

	create_random_msn();

	// for testing, we create a fake EXT template
	ntlp_msg *msg = new ntlp_msg(get_id(), new natfw_ext(),
				new ntlp::mri_pathcoupled(), 0);
	set_ext_template(msg);
}


/**
 * Destructor.
 */
nr_ext_session::~nr_ext_session() {
	delete ext_template;
	delete last_ext_msg;
}


/**
 * Generate a random message sequence number for this session.
 */
void nr_ext_session::create_random_msn() {
	int msn;
	int ret = RAND_bytes((unsigned char *) &msn, sizeof(msn));
	assert( ret == 1 );

	set_msg_sequence_number(msn);
}


std::ostream &natfw::operator<<(std::ostream &out, const nr_ext_session &s) {
	static const char *const names[] = { "IDLE", "WAITRESP", "SESSION", "FINAL" };

	return out << "[nr_ext_session: id=" << s.get_id()
		<< ", state=" << names[s.get_state()] << "]";
}



/****************************************************************************
 *
 * Utility methods
 *
 ****************************************************************************/

/**
 * Build a NATFW EXT message based on the session's state.
 *
 * This fetches the session lifetime and also increments the MSN.
 */
msg::ntlp_msg *nr_ext_session::build_ext_message() {
	using natfw::msg::natfw_ext;

	msg::ntlp_msg *msg = copy_ext_template();
	
	natfw_ext *ext = dynamic_cast<natfw_ext *>(msg->get_natfw_msg());
	assert( ext != NULL );

	ext->set_session_lifetime(get_lifetime());
	ext->set_msg_sequence_number(next_msg_sequence_number());

	return msg;
}


void nr_ext_session::setup_session(api_ext_event *e) {

	/*
	 * Create the message routing information (MRI) which we're going to
	 * use for all messages. We have two cases to cover, depending on
	 * whether the signaling destination address (SDA) is known:
	 *   1. SDA known: use path-coupled MRM
	 *   2. SDA not known: use loose end MRM
	 *
	 * Note that we're using upstream signaling. The source address is the
	 * SDA (the NI's address if known), because we send EXT messages from
	 * the data flow's destination address to the source address.
	 */

	ntlp::mri *mri;

	uint16 flow_label = 0;
	uint16 traffic_class = 0;		// DiffServ CodePoint
	uint32 ipsec_spi = 0;			// IPsec SPI
	bool downstream = false;


	/*
	 * Build a NATFW EXT message.
	 */
	natfw_ext *ext = new natfw_ext(e->is_proxy_mode());
	ext->set_session_lifetime(0);
	ext->set_msg_sequence_number(0);

	if ( e->get_action() == fw_policy_rule::ACTION_ALLOW )
		ext->set_extended_flow_info(extended_flow_info::ra_allow, 0);
	else if ( e->get_action() == fw_policy_rule::ACTION_DENY )
		ext->set_extended_flow_info(extended_flow_info::ra_deny, 0);
	else
		assert( false );

	if ( ! e->is_ni_address_known() )
		ext->set_data_terminal_info(e->get_protocol(),
				e->get_nr_port(), e->get_ni_port());

	if ( e->is_proxy_mode() )
		ext->set_nonce(); // creates a random nonce


	/*
	 * Create the MRI
	 */
	if ( e->is_ni_address_known() ) {
		mri = new ntlp::mri_pathcoupled(
			e->get_ni_address(), e->get_ni_prefix(),
			e->get_ni_port(),
			e->get_nr_address(), e->get_nr_prefix(),
			e->get_nr_port(),
			e->get_protocol(), flow_label, traffic_class, ipsec_spi,
			downstream
		);
	}
	else {
		mri = new ntlp::mri_looseend(
			e->get_ni_address(), e->get_nr_address(), downstream);
	}

	set_ext_template( new ntlp_msg(get_id(), ext, mri, 0) );


	if ( e->get_session_lifetime() != 0 )
		set_lifetime(e->get_session_lifetime());

	uint32 r = (uint32) ceil(
		get_lifetime() / ( (get_max_retries()+0.5) * 1.5 ) );
	assert( r > 0 );

	set_refresh_interval(r); 

	set_proxy_mode(e->is_proxy_mode());
}


/****************************************************************************
 *
 * state machine part
 *
 ****************************************************************************/

/*
 * state: IDLE
 */
nr_ext_session::state_t nr_ext_session::handle_state_idle(
		dispatcher *d, event *evt) {

	using msg::natfw_ext;

	/*
	 * API Create event received.
	 */
	if ( is_api_ext(evt) ) {
		api_ext_event *e = dynamic_cast<api_ext_event *>(evt);

		// initialize the session based on user-provided parameters
		setup_session(e);

		set_last_ext_message( build_ext_message() );

		d->send_message( get_last_ext_message()->copy() );

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
nr_ext_session::state_t nr_ext_session::handle_state_waitresp(
		dispatcher *d, event *evt) {

	using namespace natfw::msg;

	/*
	 * A msg_event arrived which contains a NATFW RESPONSE message.
	 */
	if ( is_natfw_response(evt) ) {
		msg_event *e = dynamic_cast<msg_event *>(evt);
		natfw_response *resp = e->get_response();

		// Discard if this is no RESPONSE to our original EXT.
		natfw_ext *ext = get_last_ext_message()->get_natfw_ext();
		if ( ! resp->is_response_to(ext) ) {
			LogWarn("RESPONSE doesn't match EXT, discarding");
			return STATE_WAITRESP;
		}

		if ( resp->is_success() ) {
			d->report_async_event("EXT session initiated");

			response_timer.stop();
			refresh_timer.start(d, get_refresh_interval());
			set_ext_counter(0);

			return STATE_SESSION;
		}
		else {
			d->report_async_event("cannot initiate EXT session");
			return STATE_FINAL;
		}
	}
	/*
	 * A response timeout was triggered.
	 */
	else if ( is_timer(evt, response_timer) ) {

		// Retry. Send the EXT message again and start a new timer.
		if ( get_ext_counter() < get_max_retries() ) {
			inc_ext_counter();
			d->send_message( get_last_ext_message()->copy() );

			response_timer.start(d, get_response_timeout());

			return STATE_WAITRESP; // no change
		}
		// Retry count exceeded, abort.
		else {
			d->report_async_event("got no response to our EXT");
			return STATE_FINAL;
		}
	}
	/*
	 * API teardown event received.
	 */
	else if ( is_api_teardown(evt) ) {
		LogDebug("received API teardown event");

		// Send a EXT message with a session lifetime of 0.
		set_lifetime(0);
		set_last_ext_message(NULL);

		d->send_message( build_ext_message() );

		return STATE_FINAL;
	}
	// TODO: handle no next hop found event
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
nr_ext_session::state_t nr_ext_session::handle_state_session(
		dispatcher *d, event *evt) {

	using namespace natfw::msg;

	/*
	 * A a NATFW RESPONSE message arrived in response to our EXT message.
	 */
	if ( is_natfw_response(evt) ) {
		msg_event *e = dynamic_cast<msg_event *>(evt);
		natfw_response *resp = e->get_response();

		/*
		 * Discard if this is no RESPONSE to our last EXT.
		 * Note that the last EXT may also be the initial EXT.
		 */
		natfw_ext *ext = get_last_ext_message()->get_natfw_ext();
		if ( ! resp->is_response_to(ext) ) {
			LogWarn("RESPONSE doesn't match EXT, discarding");
			return STATE_SESSION;
		}

		if ( resp->is_success() ) {
			d->report_async_event("EXT refresh successful");

			response_timer.stop();
			refresh_timer.start(d, get_refresh_interval());
			set_ext_counter(0);

			return STATE_SESSION; // no change
		}
		else {
			d->report_async_event("EXT session died");
			return STATE_FINAL;
		}
	}
	/*
	 * A refresh timer was triggered.
	 */
	else if ( is_timer(evt, refresh_timer) ) {

		LogDebug("received refresh timer");

		// Create a new EXT message and store a copy for refreshing.
		set_last_ext_message( build_ext_message() );

		// dispatcher will delete
		d->send_message( get_last_ext_message()->copy() );

		response_timer.start(d, get_response_timeout());

		return STATE_SESSION; // no change
	}
	/*
	 * A response timout was triggered.
	 */
	else if ( is_timer(evt, response_timer) ) {

		LogDebug("received response timeout timer");

		// Retry. Send the EXT message again and start a new timer.
		if ( get_ext_counter() < get_max_retries() ) {
			inc_ext_counter();

			d->send_message( get_last_ext_message()->copy() );

			response_timer.start(d, get_response_timeout());

			return STATE_SESSION; // no change
		}
		// Retry count exceeded, abort.
		else {
			d->report_async_event("got no response to our EXT");
			return STATE_FINAL;
		}
	}
	/*
	 * API teardown event received.
	 */
	else if ( is_api_teardown(evt) ) {
		LogDebug("received API teardown event");

		// Send a EXT message with a session lifetime of 0.
		set_lifetime(0);
		set_last_ext_message(NULL);

		// dispatcher will delete
		d->send_message( build_ext_message() );

		return STATE_FINAL;
	}
	// TODO: no next hop found
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
void nr_ext_session::process_event(dispatcher *d, event *evt) {
	LogDebug("begin process_event(): " << *this);

	switch ( get_state() ) {

		case nr_ext_session::STATE_IDLE:
			state = handle_state_idle(d, evt);
			break;

		case nr_ext_session::STATE_WAITRESP:
			state = handle_state_waitresp(d, evt);
			break;

		case nr_ext_session::STATE_SESSION:
			state = handle_state_session(d, evt);
			break;

		case nr_ext_session::STATE_FINAL:
			// ignore event and continue
			break;

		default:
			assert( false ); // invalid state
	}

	LogDebug("end process_event(): " << *this);
}

// EOF
