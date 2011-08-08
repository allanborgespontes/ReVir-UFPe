/// ----------------------------------------*- mode: C++; -*--
/// @file nf_non_edge_ext_session.cpp
/// The session for the EXT part of an NSIS Forwarder (for non-edge devices).
/// ----------------------------------------------------------
/// $Id: nf_non_edge_ext_session.cpp 3165 2008-07-10 22:36:29Z bless $
/// $HeadURL: https://svn.tm.kit.edu/nsis/natfw-nslp/trunk/src/nf_non_edge_ext_session.cpp $
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
#include <sstream>
#include <openssl/rand.h>

#include "logfile.h"

#include "natfw_config.h"
#include "events.h"
#include "msg/natfw_msg.h"
#include "dispatcher.h"


using namespace natfw;
using namespace protlib::log;
using protlib::uint32;


#define LogError(msg) ERRLog("nf_non_edge_ext_session", msg)
#define LogWarn(msg) WLog("nf_non_edge_ext_session", msg)
#define LogInfo(msg) ILog("nf_non_edge_ext_session", msg)
#define LogDebug(msg) DLog("nf_non_edge_ext_session", msg)

#define LogUnimp(msg) Log(ERROR_LOG, LOG_UNIMP, "nf_non_edge_ext_session", \
	msg << " at " << __FILE__ << ":" << __LINE__)



/**
 * Constructor.
 *
 * Use this if the session ID is known in advance.
 */
nf_non_edge_ext_session::nf_non_edge_ext_session(
		const session_id &id, const natfw_config *conf)
		: session(id), state(nf_non_edge_ext_session::STATE_IDLE),
		  config(conf), lifetime(0), max_lifetime(60),
		  response_timeout(0), state_timer(this), response_timer(this),
		  routing_info(NULL), last_ext_msg(NULL) {

	assert( conf != NULL );

	// This session may only be started on NFs.
	assert( conf->is_nf_nat() || conf->is_nf_firewall() );
}


/**
 * Constructor for test cases.
 *
 * @param s the state to start in
 * @param msg_sequence_number the MSN to start with
 */
nf_non_edge_ext_session::nf_non_edge_ext_session(
		nf_non_edge_ext_session::state_t s, const natfw_config *conf)
		: session(), state(s), config(conf), lifetime(0),
		  max_lifetime(60), response_timeout(0), state_timer(this),
		  response_timer(this), routing_info(NULL), last_ext_msg(NULL) {

	assert( conf != NULL );
}


/**
 * Destructor.
 */
nf_non_edge_ext_session::~nf_non_edge_ext_session() {
	// nothing to do
}


std::ostream &natfw::operator<<(
		std::ostream &out, const nf_non_edge_ext_session &s) {

	static const char *const names[] = { "IDLE", "WAITRESP", "SESSION", "FINAL" };

	return out << "[nf_non_edge_ext_session: id=" << s.get_id()
		<< ", state=" << names[s.get_state()] << "]";
}


/****************************************************************************
 *
 * Utilities
 *
 ****************************************************************************/


/****************************************************************************
 *
 * The state machine
 *
 ****************************************************************************/

/*
 * state: IDLE
 */
nf_non_edge_ext_session::state_t nf_non_edge_ext_session::handle_state_idle(
		dispatcher *d, event *evt) {

	using namespace msg;

	/*
	 * A NATFW EXT message arrived.
	 */
	if ( is_natfw_ext(evt) ) {
		msg_event *e = dynamic_cast<msg_event *>(evt);
		ntlp_msg *msg = e->get_ntlp_msg();
		natfw_ext *ext = e->get_ext();

		uint32 lifetime = ext->get_session_lifetime();
		uint32 msn = ext->get_msg_sequence_number();

		set_max_lifetime(config->get_nf_max_session_lifetime());
		set_response_timeout(config->get_nf_max_session_lifetime());

		// Before proceeding check several preconditions.
		try {
			check_lifetime(lifetime, get_max_lifetime());
			check_authorization(d, e);
		}
		catch ( request_error &e ) {
			LogError(e);
			d->send_message( msg->create_error_response(e) );
			return STATE_FINAL;
		}

		if ( lifetime == 0 ) {
			LogWarn("lifetime == 0, discarding message");
			return STATE_FINAL;
		}

		/*
		 * All basic preconditions match, the EXT seems to be valid.
		 * Now setup the session, depending on whether this is a
		 * NAT and/or FW node.
		 */
		LogDebug("forwarder session initiated, waiting for a response");

		set_lifetime(lifetime);
		set_msg_sequence_number(msn);

		return handle_state_idle_ext(d, e);
	}
	else {
		LogInfo("discarding unexpected event " << *evt);
		return STATE_FINAL;
	}
}


nf_non_edge_ext_session::state_t
nf_non_edge_ext_session::handle_state_idle_ext(dispatcher *d, msg_event *e) {

	ntlp_msg *msg = e->get_ntlp_msg();

	// store one copy for further reference and pass one on
	set_last_ext_message( msg->copy() );

	d->send_message( msg->copy_for_forwarding() );

	state_timer.start(d, get_lifetime());

	return STATE_WAITRESP;
}


/*
 * state: WAITRESP
 */
nf_non_edge_ext_session::state_t nf_non_edge_ext_session::handle_state_waitresp(
		dispatcher *d, event *evt) {

	using namespace natfw::msg;

	/*
	 * A NATFW RESPONSE message arrived.
	 */
	if ( is_natfw_response(evt) ) {
		msg_event *e = dynamic_cast<msg_event *>(evt);
		ntlp_msg *msg = e->get_ntlp_msg();
		natfw_response *resp = e->get_response();

		// Discard if this is no RESPONSE to our original EXT.
		natfw_ext *ext = get_last_ext_message()->get_natfw_ext();
		if ( ! resp->is_response_to(ext) ) {
			LogWarn("RESPONSE doesn't match EXT, discarding");
			return STATE_WAITRESP; // no change
		}

		if ( resp->is_success() ) {
			LogDebug("session initiated");

			d->send_message( msg->copy_for_forwarding() );

			state_timer.start(d, get_lifetime());

			return STATE_SESSION;
		}
		else {
			LogDebug("forwarding error msg from upstream peer");

			d->send_message( msg->copy_for_forwarding() );

			state_timer.stop();

			return STATE_FINAL;
		}
	}
	/*
	 * Another EXT from the upstream peer arrived.
	 *
	 * Either the NR wants to shut down the session even before it started,
	 * or we didn't respond fast enough and the NR resends its EXT.
	 */
	else if ( is_natfw_ext(evt) ) {
		msg_event *e = dynamic_cast<msg_event *>(evt);
		ntlp_msg *msg = e->get_ntlp_msg();
		natfw_ext *ext = e->get_ext();

		natfw_ext *previous = dynamic_cast<natfw_ext *>(
			get_last_ext_message()->get_natfw_msg());
		assert( previous != NULL );

		bool is_authorized = d->is_authorized(e);

		if ( ! is_authorized ) {
			LogWarn("authentication failed.");

			ntlp_msg *resp = msg->create_response(
				information_code::sc_permanent_failure,
				information_code::fail_authentication_failed);

			d->send_message(resp);
			return STATE_WAITRESP; // no change
		}
		else if ( ext->get_msg_sequence_number() >
				previous->get_msg_sequence_number()
				&& ext->get_session_lifetime() == 0 ) {

			LogDebug("terminating session.");

			state_timer.stop();

			d->send_message( msg->copy_for_forwarding() );

			return STATE_FINAL;
		}
		else {
			LogWarn("invalid/unexpected EXT."); // TODO
			return STATE_WAITRESP; // no change
		}
	}
	/*
	 * Either GIST can't reach the destination or we waited long enough
	 * for the upstream peer to respond. Anyway, send a RESPONSE.
	 */
	else if ( is_no_next_node_found_event(evt)
			|| is_timer(evt, state_timer) ) {

		LogInfo("cannot reach destination");

		ntlp_msg *latest = get_last_ext_message();
		ntlp_msg *resp = latest->create_response(
			information_code::sc_permanent_failure,
			information_code::fail_nr_not_reached);

		d->send_message( resp );

		return STATE_FINAL;
	}
	/*
	 * Outdated timer event, discard and don't log.
	 */
	else if ( is_timer(evt) ) {
		return STATE_WAITRESP; // no change
	}
	/*
	 * Unexpected event.
	 */
	else {
		LogInfo("discarding unexpected event " << *evt);
		return STATE_WAITRESP; // no change
	}
}


/*
 * state: SESSION
 */
nf_non_edge_ext_session::state_t nf_non_edge_ext_session::handle_state_session(
		dispatcher *d, event *evt) {

	using namespace natfw::msg;

	/*
	 * A msg_event arrived which contains a NATFW EXT message.
	 */
	if ( is_natfw_ext(evt) ) {
		msg_event *e = dynamic_cast<msg_event *>(evt);
		ntlp_msg *msg = e->get_ntlp_msg();
		natfw_ext *ext = e->get_ext();

		uint32 lifetime = ext->get_session_lifetime();
		uint32 msn = ext->get_msg_sequence_number();

		// Before proceeding check several preconditions.
		try {
			check_lifetime(lifetime, get_max_lifetime());
			check_authorization(d, e);
		}
		catch ( request_error &e ) {
			LogError(e);
			d->send_message( msg->create_error_response(e) );
			return STATE_SESSION;	// no change
		}

		if ( ! is_greater_than(msn, get_msg_sequence_number()) ) {
			LogWarn("discarding duplicate response.");
			return STATE_SESSION; // no change
		}
		else if ( lifetime > 0 ) {
			LogDebug("forwarder session refreshed.");
			// TODO: shouldn't the RESPONSE refresh?

			set_lifetime(lifetime);
			set_msg_sequence_number(msn);

			// store one copy for further reference and pass one on
			set_last_ext_message( msg->copy() );

			d->send_message( msg->copy_for_forwarding() );

			response_timer.start(d, get_response_timeout());

			return STATE_SESSION; // no change
		}
		else if ( lifetime == 0 ) {
			LogDebug("terminating session.");

			// cancel timers
			response_timer.stop();
			state_timer.stop();

			d->release_external_address(ext_addr);

			d->send_message( msg->copy_for_forwarding() );

			return STATE_FINAL;
		}
		else {
			assert( false ); // never reached
		}
	}
	/*
	 * A RESPONSE arrived.
	 */
	else if ( is_natfw_response(evt) ) {
		msg_event *e = dynamic_cast<msg_event *>(evt);
		ntlp_msg *msg = e->get_ntlp_msg();
		natfw_response *resp = e->get_response();

		// Discard if this is no RESPONSE to our original EXT.
		natfw_ext *ext = get_last_ext_message()->get_natfw_ext();
		assert( ext != NULL );
		if ( ! resp->is_response_to(ext) ) {
			LogWarn("RESPONSE doesn't match EXT, discarding");
			return STATE_SESSION;
		}
		
		if ( resp->is_success() ) {
			LogDebug("upstream peer sent successful response.");

			d->send_message( msg->copy_for_forwarding() );

			state_timer.restart(d, get_lifetime());

			return STATE_SESSION; // no change
		}
		else {
			LogWarn("error message received.");

			d->send_message( msg->copy_for_forwarding() );

			//d->release_external_address(ext_addr);

			return STATE_FINAL;
		}
	}
	/*
	 * Upstream peer didn't respond.
	 */
	else if ( is_no_next_node_found_event(evt)
			|| is_timer(evt, response_timer) ) {
		LogWarn("upstream peer did not respond");

		state_timer.stop();

		natfw_msg *last = get_last_ext_message()->get_natfw_msg();
		natfw_ext *ext = dynamic_cast<natfw_ext *>(last);
		assert( ext != NULL );

		//d->release_external_address(ext_addr);

		// TODO: check the spec!
		ntlp_msg *latest = get_last_ext_message();
		ntlp_msg *resp = latest->create_response(
			information_code::sc_permanent_failure,
			information_code::fail_nr_not_reached);

		d->send_message( resp );

		return STATE_FINAL;
	}
	/*
	 * Downstream peer didn't send a refresh EXT in time.
	 */
	else if ( is_timer(evt, state_timer) ) {
		LogWarn("session timed out");

		response_timer.stop();

		d->release_external_address(ext_addr);

		return STATE_FINAL;
	}
	/*
	 * GIST detected that one of our routes is no longer usable. This
	 * could be the route to the NI or to the NR.
	 */
	else if ( is_route_changed_bad_event(evt) ) {
		LogUnimp("route to the NI or to the NR is no longer usable");
		return STATE_SESSION;
	}
	/*
	 * Outdated timer event, discard and don't log.
	 */
	else if ( is_timer(evt) ) {
		return STATE_SESSION; // no change
	}
	/*
	 * Received unexpected event.
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
void nf_non_edge_ext_session::process_event(dispatcher *d, event *evt) {
	LogDebug("begin process_event(): " << *this);

	switch ( get_state() ) {

		case nf_non_edge_ext_session::STATE_IDLE:
			state = handle_state_idle(d, evt);
			break;

		case nf_non_edge_ext_session::STATE_WAITRESP:
			state = handle_state_waitresp(d, evt);
			break;

		case nf_non_edge_ext_session::STATE_SESSION:
			state = handle_state_session(d, evt);
			break;

		case nf_non_edge_ext_session::STATE_FINAL:
			// ignore event and continue
			break;

		default:
			assert( false ); // invalid state
	}

	LogDebug("end process_event(): " << *this);
}

// EOF
