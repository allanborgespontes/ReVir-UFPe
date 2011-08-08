/// ----------------------------------------*- mode: C++; -*--
/// @file nr_session.cpp
/// The session for an NSIS Responder.
/// ----------------------------------------------------------
/// $Id: nr_session.cpp 3165 2008-07-10 22:36:29Z bless $
/// $HeadURL: https://svn.tm.kit.edu/nsis/natfw-nslp/trunk/src/nr_session.cpp $
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
#include <openssl/rand.h>

#include "logfile.h"

#include "mri.h"	// from NTLP

#include "natfw_config.h"
#include "events.h"
#include "msg/natfw_msg.h"
#include "dispatcher.h"


using namespace natfw;
using namespace natfw::msg;
using namespace protlib::log;
using protlib::uint32;


#define LogError(msg) ERRLog("nr_session", msg)
#define LogWarn(msg) WLog("nr_session", msg)
#define LogInfo(msg) ILog("nr_session", msg)
#define LogDebug(msg) DLog("nr_session", msg)


/**
 * Constructor.
 *
 * Use this if the session ID is known in advance.
 */
nr_session::nr_session(const session_id &id, natfw_config *conf)
		: session(id), state(STATE_IDLE), config(conf),
		  lifetime(0), max_lifetime(0), state_timer(this) {

	assert( conf != NULL );

	set_max_lifetime(conf->get_nr_max_session_lifetime());
}


/**
 * Constructor for test cases.
 *
 * @param s the state to start in
 * @param msn the initial message sequence number
 */
nr_session::nr_session(nr_session::state_t s, uint32 msn)
		: session(), state(s), config(NULL),
		  lifetime(0), max_lifetime(60), state_timer(this) {

	set_msg_sequence_number(msn);
}


/**
 * Destructor.
 */
nr_session::~nr_session() {
	// nothing to do
}


std::ostream &natfw::operator<<(std::ostream &out, const nr_session &s) {
	static const char *const names[] = { "IDLE", "SESSION", "FINAL" };

	return out << "[nr_session: id=" << s.get_id()
		<< ", state=" << names[s.get_state()] << "]";
}


/****************************************************************************
 *
 * The state machine.
 *
 ****************************************************************************/


/*
 * state: IDLE
 */
nr_session::state_t nr_session::handle_state_idle(dispatcher *d, event *evt) {
	using namespace msg;

	/*
	 * A msg_event arrived which contains a NATFW CREATE message.
	 */
	if ( is_natfw_create(evt) ) {
		msg_event *e = dynamic_cast<msg_event *>(evt);
		ntlp_msg *msg = e->get_ntlp_msg();
		natfw_create *c = e->get_create();

		uint32 lifetime = c->get_session_lifetime();
		uint32 msn = c->get_msg_sequence_number();

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

		if ( lifetime > 0 ) {
			LogDebug("responder session initiated.");

			set_lifetime(lifetime);
			set_msg_sequence_number(msn);

			ntlp_msg *resp = msg->create_success_response(lifetime);

			d->send_message(resp);

			state_timer.start(d, lifetime);

			return STATE_SESSION;
		}
		else {
			LogWarn("invalid lifetime.");
			return STATE_FINAL;
		}
	}
	else {
		LogInfo("discarding unexpected event " << *evt);
		return STATE_FINAL;
	}
}


/*
 * state: SESSION
 */
nr_session::state_t nr_session::handle_state_session(
		dispatcher *d, event *evt) {

	/*
	 * A msg_event arrived which contains a NATFW CREATE message.
	 */
	if ( is_natfw_create(evt) ) {
		msg_event *e = dynamic_cast<msg_event *>(evt);
		ntlp_msg *msg = e->get_ntlp_msg();
		natfw_create *c = e->get_create();

		uint32 lifetime = c->get_session_lifetime();
		uint32 msn = c->get_msg_sequence_number();
	
		// Before proceeding check several preconditions.
		try {
			check_lifetime(lifetime, get_max_lifetime());
			check_authorization(d, e);
		}
		catch ( request_error &e ) {
			LogError(e);
			d->send_message( msg->create_error_response(e) );
			return STATE_SESSION;
		}

		if ( ! is_greater_than(msn, get_msg_sequence_number()) ) {
			LogWarn("duplicate response received.");
			return STATE_SESSION; // no change
		}
		else if ( lifetime > 0 ) {
			LogDebug("authentication succesful.");

			set_lifetime(lifetime); // could be a new lifetime!
			set_msg_sequence_number(msn);

			ntlp_msg *resp = msg->create_success_response(lifetime);

			d->send_message(resp);

			state_timer.restart(d, lifetime);

			return STATE_SESSION; // no change
		}
		else if ( lifetime == 0 ) {
			LogInfo("terminating session on NI request.");

			d->report_async_event("NI terminated session");
			return STATE_FINAL;
		}
		else {
			LogWarn("invalid lifetime.");

			return STATE_SESSION; // no change
		}
	}
	/*
	 * The session timeout was triggered.
	 */
	else if ( is_timer(evt, state_timer) ) {
		LogWarn("session timed out.");

		d->report_async_event("session timed out");
		return STATE_FINAL;
	}
	/*
	 * Outdated timer event, discard and don't log.
	 */
	else if ( is_timer(evt) ) {
		return STATE_SESSION; // no change
	}
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
void nr_session::process_event(dispatcher *d, event *evt) {
	LogDebug("begin process_event(): " << *this);

	switch ( get_state() ) {

		case nr_session::STATE_IDLE:
			state = handle_state_idle(d, evt);
			break;

		case nr_session::STATE_SESSION:
			state = handle_state_session(d, evt);
			break;

		case nr_session::STATE_FINAL:
			// ignore event and continue
			break;

		default:
			assert( false ); // invalid state
	}

	LogDebug("end process_event(): " << *this);
}

// EOF
