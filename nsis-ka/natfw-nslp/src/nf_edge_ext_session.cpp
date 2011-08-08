/// ----------------------------------------*- mode: C++; -*--
/// @file nf_edge_ext_session.cpp
/// The session for the EXT part of an NSIS Forwarder (for edge devices).
/// ----------------------------------------------------------
/// $Id: nf_edge_ext_session.cpp 3165 2008-07-10 22:36:29Z bless $
/// $HeadURL: https://svn.tm.kit.edu/nsis/natfw-nslp/trunk/src/nf_edge_ext_session.cpp $
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

#include "logfile.h"

#include "natfw_config.h"
#include "events.h"
#include "msg/natfw_msg.h"
#include "dispatcher.h"


using namespace natfw;
using namespace protlib::log;
using protlib::uint32;


#define LogError(msg) ERRLog("nf_edge_ext_session", msg)
#define LogWarn(msg) WLog("nf_edge_ext_session", msg)
#define LogInfo(msg) ILog("nf_edge_ext_session", msg)
#define LogDebug(msg) DLog("nf_edge_ext_session", msg)

#define LogUnimp(msg) Log(ERROR_LOG, LOG_UNIMP, "nf_edge_ext_session", \
	msg << " at " << __FILE__ << ":" << __LINE__)



/**
 * Constructor.
 *
 * Use this if the session ID is known in advance.
 */
nf_edge_ext_session::nf_edge_ext_session(
		const session_id &id, const natfw_config *conf)
		: session(id), state(nf_edge_ext_session::STATE_IDLE),
		  config(conf), proxy_mode(false), lifetime(0),
		  max_lifetime(60), state_timer(this), routing_info(NULL),
		  last_ext_msg(NULL), fw_rule(NULL), nat_rule(NULL) {

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
nf_edge_ext_session::nf_edge_ext_session(nf_edge_ext_session::state_t s,
		const natfw_config *conf)
		: session(), state(s), config(conf), proxy_mode(false),
		  lifetime(0), max_lifetime(60), state_timer(this),
		  routing_info(NULL), last_ext_msg(NULL),
		  fw_rule(NULL), nat_rule(NULL) {

	assert( conf != NULL );
}


/**
 * Destructor.
 */
nf_edge_ext_session::~nf_edge_ext_session() {
	delete nat_rule;
	delete fw_rule;
	delete routing_info;
	delete last_ext_msg;
}


std::ostream &natfw::operator<<(std::ostream &out, const nf_edge_ext_session &s) {
	static const char *const names[] = { "IDLE", "SESSION", "FINAL" };

	return out << "[nf_edge_ext_session: id=" << s.get_id()
		<< ", state=" << names[s.get_state()] << "]";
}


/****************************************************************************
 *
 * Utilities
 *
 ****************************************************************************/


/**
 * Creates a message which is to be sent on success from an edge device.
 *
 * For NATs (if addr is not NULL), it tells the receiver that the given
 * external address and port have been reserved.
 */
msg::ntlp_msg *nf_edge_ext_session::build_edge_device_response(
		msg_event *e, appladdress *addr) const {

	natfw_ext *ext = e->get_ext();
	assert( ext != NULL );

	ntlp::mri *received_mri = e->get_mri();
	assert( received_mri != NULL );

	natfw_response *resp = new natfw_response(ext,
		information_code::sc_success,
		information_code::suc_successfully_processed);

	resp->set_session_lifetime(get_lifetime());

	if ( addr != NULL )
		resp->set_external_address(*addr, addr->get_port());

	/*
	 * We use the received_mri, but we have to change the flow direction.
	 * TODO: If we're getting a le_mri, should we map it to a pc_mri?
	 */
	ntlp::mri *new_mri = received_mri->copy();
	new_mri->invertDirection();
	msg::ntlp_msg *msg = new msg::ntlp_msg(get_id(), resp, new_mri, 0);

	return msg;
}


fw_policy_rule *nf_edge_ext_session::build_fw_policy_rule(ntlp_msg *msg) const {
	using ntlp::mri_pathcoupled;

	assert( msg != NULL );

	natfw_ext *ext = msg->get_natfw_ext();
	assert( ext != NULL );

	mri_pathcoupled *pc_mri
		= dynamic_cast<mri_pathcoupled *>(msg->get_mri());

	// TODO: check spec!
	if ( pc_mri == NULL ) {
		LogError("no path-coupled MRI found");
		return NULL; // failure
	}

	// TODO: we ignore sub_ports for now
	if ( ext->get_subsequent_ports() != 0 )
		throw policy_rule_installer_error("unsupported sub_ports value",
			information_code::sc_signaling_session_failures,
			information_code::sigfail_unknown_policy_rule_action);

	assert ( ext->get_rule_action() == extended_flow_info::ra_deny );

	fw_policy_rule *r = new fw_policy_rule(fw_policy_rule::ACTION_DENY,
		pc_mri->get_sourceaddress(), pc_mri->get_sourceprefix(),
		pc_mri->get_sourceport(), 
		pc_mri->get_destaddress(), pc_mri->get_destprefix(),
		pc_mri->get_destport(),
		pc_mri->get_protocol()
	);

	return r;
}


/**
 * Start an NI session with this node being the NI.
 */
void nf_edge_ext_session::tg_create_proxy(dispatcher *d, ntlp_msg *msg) {
	using ntlp::mri_pathcoupled;

	assert( msg != NULL );

	natfw_ext *ext = msg->get_natfw_ext();
	assert( ext != NULL );

	mri_pathcoupled *pc_mri
		= dynamic_cast<mri_pathcoupled *>(msg->get_mri());
	assert( pc_mri != NULL );

	session_id sid = d->create_ni_proxy_session(
		pc_mri->get_sourceaddress(), pc_mri->get_destaddress(),
		pc_mri->get_sourceport(), pc_mri->get_destport(),
		pc_mri->get_protocol(), get_lifetime(),
		ext->get_nonce()
	);

	set_proxy_session_id(sid);


	/*
	 * Install policy rules to allow data traffic from the outside.
	 */
	nat_rule = NULL;
	fw_rule = NULL;

	if ( config->is_nf_nat() ) {
		nat_rule = new nat_policy_rule(
			pc_mri->get_sourceaddress(), pc_mri->get_sourceport(),
			ext_addr, ext_addr.get_port(),
			pc_mri->get_destaddress(), pc_mri->get_destport(),
			pc_mri->get_protocol(),
			nat_policy_rule::TYPE_DEST_NAT
		);
	}

	if ( config->is_nf_firewall() ) {
		fw_rule = new fw_policy_rule(fw_policy_rule::ACTION_ALLOW,
			pc_mri->get_sourceaddress(), pc_mri->get_sourceprefix(),
			pc_mri->get_sourceport(),
			pc_mri->get_destaddress(), pc_mri->get_destprefix(),
			pc_mri->get_destport(),
			pc_mri->get_protocol()
		);
	}

	d->install_policy_rules(
		( nat_rule != NULL ? nat_rule->copy() : NULL ),
		( fw_rule != NULL ? fw_rule->copy() : NULL )
	);
}


/**
 * Shutdown the NI session we created earlier.
 */
void nf_edge_ext_session::tg_teardown_proxy(dispatcher *d) {

	session_id sid = get_proxy_session_id();

	event *e = new api_teardown_event(new session_id(sid));

	d->process(e);

	delete e;

	/*
	 * Remove any policy rules we might have installed.
	 */
	d->remove_policy_rules(
		( nat_rule != NULL ? nat_rule->copy() : NULL ),
		( fw_rule != NULL ? fw_rule->copy() : NULL )
	);
}


/****************************************************************************
 *
 * The state machine
 *
 ****************************************************************************/

/*
 * state: IDLE
 */
nf_edge_ext_session::state_t nf_edge_ext_session::handle_state_idle(
		dispatcher *d, event *evt) {

	/*
	 * A NATFW EXT message arrived.
	 */
	if ( is_natfw_ext(evt) ) {
		msg_event *e = dynamic_cast<msg_event *>(evt);
		ntlp_msg *msg = e->get_ntlp_msg();
		natfw_ext *ext = e->get_ext();
		hostaddress nr_address = e->get_mri()->get_destaddress();

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
			return STATE_FINAL;
		}

		if ( ! d->is_from_private_side(nr_address) ) {
			LogWarn("EXT received on the public side, discarding.");
			return STATE_FINAL;
		}

		if ( lifetime == 0 ) {
			LogWarn("invalid EXT message: lifetime == 0");
			return STATE_FINAL;
		}

		/*
		 * Preconditions have been checked.
		 */

		LogDebug("edge EXT session initiated.");
		set_lifetime(lifetime);
		set_msg_sequence_number(msn);
		set_proxy_mode(ext->is_proxy_mode());


		if ( config->is_nf_firewall() ) {
			if ( ext->get_rule_action()
					== extended_flow_info::ra_deny ) {
				d->install_policy_rules(
					NULL, build_fw_policy_rule(msg));
			}
			else {
				/*
				 * TODO: According to the spec, we should
				 * remember the policy rule here. But I have no
				 * idea why. Maybe to detect changes in the
				 * policy rule?
				 */
				LogUnimp("remember policy rule");
			}
		}

		if ( config->is_nf_nat() ) {
			try {
				ext_addr = d->reserve_external_address(
					get_nr_address(e));
			}
			catch ( nat_manager_error &e ) {
				LogError(e);
				d->send_message(msg->create_error_response(e));
				return STATE_FINAL;
			}
		}


		/*
		 * If this node is a NAT, our response includes the reserved
		 * external address.
		 */
		ntlp_msg *resp;
		if ( config->is_nf_nat() )
			resp = build_edge_device_response(e, &ext_addr);
		else
			resp = build_edge_device_response(e);

		d->send_message(resp);

		state_timer.start(d, lifetime);

		/*
		 * If proxy mode is requested, start an ni_session.
		 * The ni_session will be terminated together with
		 * this session.
		 */
		if ( is_proxy_mode() )
			tg_create_proxy(d, msg);

		return STATE_SESSION;
	}
	else {
		LogInfo("discarding unexpected event " << *evt);
		return STATE_FINAL; // no change
	}
}


/*
 * state: SESSION
 */
nf_edge_ext_session::state_t nf_edge_ext_session::handle_state_session(
		dispatcher *d, event *evt) {

	/*
	 * A NATFW EXT message has arrived.
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
			return STATE_SESSION;
		}

		if ( ! is_greater_than(msn, get_msg_sequence_number()) ) {
			LogWarn("discarding duplicate response.");
			return STATE_SESSION; // no change
		}
		else if ( lifetime > 0 ) {
			LogDebug("authentication succesful.");

			set_lifetime(lifetime);
			set_msg_sequence_number(msn);

			ntlp_msg *resp = msg->create_response(
				information_code::sc_success,
				information_code::suc_successfully_processed);

			natfw_response *r = dynamic_cast<natfw_response *>(
				resp->get_natfw_msg());
			assert( r != NULL );

			r->set_session_lifetime(lifetime);

			d->send_message(resp);

			state_timer.restart(d, lifetime);

			return STATE_SESSION; // no change
		}
		else if ( lifetime == 0 ) {
			LogInfo("terminating session on NI+ request.");

			// Note: this is a no-op for firewalls
			d->release_external_address(ext_addr);

			if ( is_proxy_mode() )
				tg_teardown_proxy(d);

			// TODO: necessary?
			//d->report_async_event("NI+ terminated session");
			return STATE_FINAL;
		}
		else {
			LogWarn("invalid EXT message.");
			return STATE_SESSION; // no change
		}
	}
	/*
	 * The session timeout was triggered.
	 */
	else if ( is_timer(evt, state_timer) ) {
		LogWarn("session timed out.");

		// Note: This is a no-op for firewalls
		d->release_external_address(ext_addr);

		if ( is_proxy_mode() )
			tg_teardown_proxy(d);

		// TODO: necessary?
		//d->report_async_event("EXT session timed out");
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
void nf_edge_ext_session::process_event(dispatcher *d, event *evt) {
	LogDebug("begin process_event(): " << *this);

	switch ( get_state() ) {

		case nf_edge_ext_session::STATE_IDLE:
			state = handle_state_idle(d, evt);
			break;

		case nf_edge_ext_session::STATE_SESSION:
			state = handle_state_session(d, evt);
			break;

		case nf_edge_ext_session::STATE_FINAL:
			// ignore event and continue
			break;

		default:
			assert( false ); // invalid state
	}

	LogDebug("end process_event(): " << *this);
}

// EOF
