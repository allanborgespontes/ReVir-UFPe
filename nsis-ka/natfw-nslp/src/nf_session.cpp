/// ----------------------------------------*- mode: C++; -*--
/// @file nf_session.cpp
/// The session for an NSIS Forwarder.
/// ----------------------------------------------------------
/// $Id: nf_session.cpp 3165 2008-07-10 22:36:29Z bless $
/// $HeadURL: https://svn.tm.kit.edu/nsis/natfw-nslp/trunk/src/nf_session.cpp $
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
#include "logfile.h"

#include "mri.h"	// from NTLP

#include "natfw_config.h"
#include "events.h"
#include "msg/natfw_msg.h"
#include "dispatcher.h"


using namespace natfw;
using namespace protlib::log;
using protlib::uint32;


#define LogError(msg) ERRLog("nf_session", msg)
#define LogWarn(msg) WLog("nf_session", msg)
#define LogInfo(msg) ILog("nf_session", msg)
#define LogDebug(msg) DLog("nf_session", msg)

#define LogUnimp(msg) Log(ERROR_LOG, LOG_UNIMP, "nf_session", \
	msg << " at " << __FILE__ << ":" << __LINE__)


/**
 * Constructor.
 *
 * Use this if the session ID is known in advance.
 */
nf_session::nf_session(const session_id &id, const natfw_config *conf)
		: session(id), state(nf_session::STATE_IDLE), config(conf),
		  proxy_mode(false), lifetime(0), max_lifetime(0),
		  response_timeout(0), state_timer(this), response_timer(this),
		  ni_mri(NULL), nr_mri(NULL), create_message(NULL),
		  fw_rule(NULL), nat_rule(NULL), ext_addr_reserved(false) {

	assert( config != NULL );
}


/**
 * Constructor for test cases.
 *
 * @param s the state to start in
 */
nf_session::nf_session(nf_session::state_t s, const natfw_config *conf)
		: session(), state(s), config(conf),
		  proxy_mode(false), lifetime(0), max_lifetime(60),
		  response_timeout(0), state_timer(this), response_timer(this),
		  ni_mri(NULL), nr_mri(NULL), create_message(NULL),
		  fw_rule(NULL), nat_rule(NULL), ext_addr_reserved(false) {

	// nothing to do
}


/**
 * Destructor.
 */
nf_session::~nf_session() {
	delete ni_mri;
	delete nr_mri;
	delete create_message;
	delete fw_rule;
	delete nat_rule;
}


std::ostream &natfw::operator<<(std::ostream &out, const nf_session &s) {
	static const char *const names[] = { "IDLE", "WAITRESP", "SESSION", "FINAL" };

	return out << "[nf_session: id=" << s.get_id()
		<< ", state=" << names[s.get_state()] << "]";
}


/****************************************************************************
 *
 * Utilities
 *
 ****************************************************************************/


/**
 * Create a new MRI but with a rewritten destination.
 *
 * This is needed for NAT operation.
 */
ntlp::mri_pathcoupled *nf_session::create_mri_with_dest(
		ntlp::mri_pathcoupled *orig_mri, appladdress priv_addr) const {

	assert( orig_mri != NULL );
	assert( orig_mri->get_downstream() == true );

	// create a new MRI with rewritten destination
	ntlp::mri_pathcoupled *new_mri = new ntlp::mri_pathcoupled(
		orig_mri->get_sourceaddress(),
		orig_mri->get_sourceprefix(),
		orig_mri->get_sourceport(),
		priv_addr,				// rewritten
		orig_mri->get_destprefix(),
		priv_addr.get_port(),			// rewritten
		orig_mri->get_protocol(),
		orig_mri->get_flowlabel(),
		orig_mri->get_ds_field(),
		orig_mri->get_spi(),
		orig_mri->get_downstream()
	);

	return new_mri;
}


/**
 * Create a new MRI but with a rewritten source.
 *
 * This is needed for NAT operation.
 */
ntlp::mri_pathcoupled *nf_session::create_mri_with_source(
		ntlp::mri_pathcoupled *orig_mri, appladdress priv_addr) const {

	assert( orig_mri != NULL );
	assert( orig_mri->get_downstream() == true );

	// create a new MRI with rewritten destination
	ntlp::mri_pathcoupled *new_mri = new ntlp::mri_pathcoupled(
		priv_addr,				// rewritten
		orig_mri->get_sourceprefix(),
		priv_addr.get_port(),			// rewritten
		orig_mri->get_destaddress(),
		orig_mri->get_destprefix(),
		orig_mri->get_destport(),
		orig_mri->get_protocol(),
		orig_mri->get_flowlabel(),
		orig_mri->get_ds_field(),
		orig_mri->get_spi(),
		orig_mri->get_downstream()
	);

	return new_mri;
}


/**
 * Create a new MRI but with the direction flag inverted.
 */
ntlp::mri_pathcoupled *nf_session::create_mri_inverted(
		ntlp::mri_pathcoupled *orig_mri) const {

	assert( orig_mri != NULL );
	assert( orig_mri->get_downstream() == true );

	ntlp::mri_pathcoupled *new_mri = orig_mri->copy();
	new_mri->invertDirection();

	return new_mri;
}


/**
 * Copy the given message and adjust it for forwarding to the NR.
 *
 * If this node is a NAT, the destination address is rewritten.
 */
ntlp_msg *nf_session::create_msg_for_nr(ntlp_msg *msg) const {

	if ( config != NULL && config->is_nf_nat() )
		return msg->copy_for_forwarding(get_nr_mri()->copy());
	else
		return msg->copy_for_forwarding();
}


/**
 * Copy the given message and adjust it for forwarding to the NI.
 *
 * If this node is a NAT, the destination address is rewritten.
 */
ntlp_msg *nf_session::create_msg_for_ni(ntlp_msg *msg) const {

	if ( config != NULL && config->is_nf_nat() )
		return msg->copy_for_forwarding(get_ni_mri()->copy());
	else
		return msg->copy_for_forwarding();
}


/**
 * Return a copy of the saved firewall policy rule if we have one.
 */
fw_policy_rule *nf_session::get_fw_policy_rule_copy() const {
	if ( get_fw_policy_rule() == NULL )
		return NULL;
	else
		return get_fw_policy_rule()->copy();
}


/**
 * Return a copy of the saved NAT policy rule if we have one.
 */
nat_policy_rule *nf_session::get_nat_policy_rule_copy() const {
	if ( get_nat_policy_rule() == NULL )
		return NULL;
	else
		return get_nat_policy_rule()->copy();
}


/**
 * Create a firewall policy rule from the given event and save it.
 *
 * If we receive a successful response later, we will activate the rule.
 * In case we don't support the requested policy rule, an exception is thrown.
 */
bool nf_session::save_fw_policy_rule(msg_event *evt) {
	using ntlp::mri_pathcoupled;

	assert( evt != NULL );

	natfw_create *create = evt->get_create();
	assert( create != NULL );

	mri_pathcoupled *pc_mri
		= dynamic_cast<mri_pathcoupled *>(evt->get_mri());

	if ( pc_mri == NULL ) {
		LogError("no path-coupled MRI found");
		return false; // failure
	}

	assert( pc_mri->get_downstream() == true );

	// TODO: We ignore sub_ports for now.
	if ( create->get_subsequent_ports() != 0 )
		throw policy_rule_installer_error("unsupported sub_ports value",
			information_code::sc_signaling_session_failures,
			information_code::sigfail_unknown_policy_rule_action);


	fw_policy_rule::action_t action;

	switch ( create->get_rule_action() ) {
	  case extended_flow_info::ra_allow:
		action = fw_policy_rule::ACTION_ALLOW;
		break;
	  case extended_flow_info::ra_deny:
		action = fw_policy_rule::ACTION_DENY;
		break;
	  default:
		throw policy_rule_installer_error("unknown policy rule action",
			information_code::sc_signaling_session_failures,
			information_code::sigfail_unknown_policy_rule_action);
	}

	fw_policy_rule *r = new fw_policy_rule(action,
		pc_mri->get_sourceaddress(), pc_mri->get_sourceprefix(),
		pc_mri->get_sourceport(),
		pc_mri->get_destaddress(), pc_mri->get_destprefix(),
		pc_mri->get_destport(),
		pc_mri->get_protocol()
	);

	set_fw_policy_rule(r);

	LogDebug("saved policy rule for later use: " << *r);

	return true; // success
}


/**
 * Create a NAT policy rule from the given event and save it.
 *
 * If we receive a successful response later, we will activate the rule.
 */
bool nf_session::save_nat_policy_rule(msg_event *evt,
		const appladdress &dr_addr, nat_policy_rule::type_t nat_type) {

	using ntlp::mri_pathcoupled;

	assert( evt != NULL );

	natfw_create *create = evt->get_create();
	assert( create != NULL );

	mri_pathcoupled *pc_mri
		= dynamic_cast<mri_pathcoupled *>(evt->get_mri());

	if ( pc_mri == NULL ) {
		LogError("no path-coupled MRI found");
		return false; // failure
	}

	assert( pc_mri->get_downstream() == true );

	// order of arguments: data sender, external address, data receiver
	nat_policy_rule *r = new nat_policy_rule(
		pc_mri->get_sourceaddress(), pc_mri->get_sourceport(),
		pc_mri->get_destaddress(), pc_mri->get_destport(),
		dr_addr, dr_addr.get_port(), pc_mri->get_protocol(),
		nat_type
	);

	set_nat_policy_rule(r);

	LogDebug("saved policy rule for later use: " << *r);

	return true; // success
}



/****************************************************************************
 *
 * The state machine
 *
 ****************************************************************************/


/*
 * state: IDLE
 */
nf_session::state_t nf_session::handle_state_idle(dispatcher *d, event *evt) {
	using namespace msg;

	/*
	 * A msg_event arrived which contains a NATFW CREATE message.
	 */
	if ( is_natfw_create(evt) ) {
		msg_event *e = dynamic_cast<msg_event *>(evt);
		ntlp_msg *msg = e->get_ntlp_msg();
		natfw_create *create = e->get_create();

		uint32 lifetime = create->get_session_lifetime();
		uint32 msn = create->get_msg_sequence_number();

		set_max_lifetime(config->get_nf_max_session_lifetime());
		set_response_timeout(config->get_nf_max_session_lifetime());

		/*
		 * Before proceeding check several preconditions.
		 */
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
		 * All basic preconditions match, the CREATE seems to be valid.
		 * Now setup the session, depending on whether this is a
		 * NAT and/or FW node.
		 */
		LogDebug("forwarder session initiated, waiting for a response");

		set_lifetime(lifetime);
		set_msg_sequence_number(msn);

		return handle_state_idle_create(d, e);
	}
	else {
		LogInfo("discarding unexpected event " << *evt);
		return STATE_FINAL; // no change
	}
}


nf_session::state_t nf_session::handle_state_idle_create(
		dispatcher *d, msg_event *e) {

	using namespace msg;
	using ntlp::mri_pathcoupled;

	ntlp_msg *msg = e->get_ntlp_msg();

	// store one copy for further reference and pass one on
	set_last_create_message( msg->copy() );

	if ( config->is_nf_firewall() ) {
		LogDebug("this node is a firewall");

		try {
			save_fw_policy_rule(e);
		}
		catch ( policy_rule_installer_error &e ) {
			LogError("policy rule not supported: " << e);
			d->send_message( msg->create_error_response(e) );

			return STATE_FINAL;
		}
	}


	mri_pathcoupled *mri_pc
		= dynamic_cast<mri_pathcoupled *>(msg->get_mri());
	assert( mri_pc != NULL );

	hostaddress ni_address = e->get_mri()->get_sourceaddress();

	/*
	 * If we are a NAT, we have to rewrite the destination address and
	 * keep all the information we need to rewrite incoming responses, too.
	 */
	if ( config->is_nf_nat() && ! d->is_from_private_side(ni_address) ) {
		LogDebug("this node is a NAT; CREATE is from public side");

		appladdress ext_addr(mri_pc->get_destaddress(),
			mri_pc->get_protocol(), mri_pc->get_destport());

		try {
			appladdress addr(d->lookup_private_address(ext_addr));

			LogDebug("reservation found for " << addr);

			// store the rewritten MRIs for later use
			set_nr_mri( create_mri_with_dest(mri_pc, addr) );
			set_ni_mri( create_mri_inverted(mri_pc) );

			save_nat_policy_rule(
				e, addr, nat_policy_rule::TYPE_DEST_NAT);
		}
		catch ( request_error &e ) {
			/*
			 * The nat_manager or the policy_rule_installer could
			 * throw an exception. Either way: We have to send an
			 * error response and terminate the session.
			 */
			LogError(e);

			d->send_message( msg->create_error_response(e) );

			return STATE_FINAL;
		}
	}
	/*
	 * If a CREATE arrived from the private side, we have to reserve an
	 * external address and rewrite the source address and port.
	 * Additionally, we have to keep all information we need to rewrite
	 * subsequent NATFW messages.
	 */
	else if ( config->is_nf_nat() && d->is_from_private_side(ni_address) ) {
		LogDebug("this node is a NAT; CREATE is from private side");

		appladdress addr;
		try {
			addr = d->reserve_external_address(get_nr_address(e));
			set_external_address(addr);
		}
		catch ( nat_manager_error &e ) {
			LogError(e);
			d->send_message(msg->create_error_response(e));
			return STATE_FINAL;
		}

		// store the rewritten MRIs for later use
		set_nr_mri( create_mri_with_source(mri_pc, addr) );
		set_ni_mri( create_mri_inverted(mri_pc) );

		save_nat_policy_rule(e, addr, nat_policy_rule::TYPE_SOURCE_NAT);
	}


	d->send_message( create_msg_for_nr(msg) );

	state_timer.start(d, get_lifetime());

	return STATE_WAITRESP;
}


/*
 * state: WAITRESP
 */
nf_session::state_t nf_session::handle_state_waitresp(
		dispatcher *d, event *evt) {

	using namespace natfw::msg;

	/*
	 * A msg_event arrived which contains a NATFW RESPONSE message.
	 */
	if ( is_natfw_response(evt, get_last_create_message()) ) {
		msg_event *e = dynamic_cast<msg_event *>(evt);
		ntlp_msg *msg = e->get_ntlp_msg();
		natfw_response *resp = e->get_response();

		// Discard if this is no RESPONSE to our original CREATE.
		natfw_create *c = get_last_create_message()->get_natfw_create();
		if ( ! resp->is_response_to(c) ) {
			LogWarn("RESPONSE doesn't match CREATE, discarding");
			return STATE_WAITRESP; // no change
		}

		if ( resp->is_success() ) {
			LogDebug("initiated session " << get_id());

			d->install_policy_rules(get_nat_policy_rule_copy(),
				get_fw_policy_rule_copy());

			d->send_message( create_msg_for_ni(msg) );

			state_timer.start(d, get_lifetime());

			return STATE_SESSION;
		}
		else {
			LogDebug("forwarding error msg from upstream peer");

			// TODO: ReportAsyncEvent() ?
			state_timer.stop();

			d->send_message( create_msg_for_ni(msg) );

			if ( is_external_address_reserved() )
				d->release_external_address(
					get_external_address());

			return STATE_FINAL;
		}
	}
	/*
	 * Another CREATE from the upstream peer arrived.
	 *
	 * Either the NI wants to shut down the session even before it started,
	 * or we didn't respond fast enough and the NI resends its CREATE.
	 *
	 * TODO: What if this is a new CREATE with lifetime > 0? Discard?
	 * Accept and even save policy rules?
	 */
	else if ( is_natfw_create(evt) ) {
		msg_event *e = dynamic_cast<msg_event *>(evt);
		ntlp_msg *msg = e->get_ntlp_msg();
		natfw_create *create = e->get_create();

		natfw_create *previous = dynamic_cast<natfw_create *>(
			get_last_create_message()->get_natfw_msg());
		assert( previous != NULL );

		try {
			check_authorization(d, e);
		}
		catch ( request_error &e ) {
			LogError(e);
			d->send_message( msg->create_error_response(e) );
			return STATE_WAITRESP; // no change
		}

		if ( create->get_msg_sequence_number() >
				previous->get_msg_sequence_number()
				&& create->get_session_lifetime() == 0 ) {

			LogDebug("terminating session.");

			state_timer.stop();

			d->send_message( msg->copy_for_forwarding() );

			if ( is_external_address_reserved() )
				d->release_external_address(
					get_external_address());

			return STATE_FINAL;
		}
		else if ( create->get_msg_sequence_number()
				== previous->get_msg_sequence_number() ) {

			LogWarn("NI resent the initial CREATE. Passing it on.");

			d->send_message( msg->copy_for_forwarding() );

			state_timer.start(d, get_response_timeout());

			return STATE_WAITRESP; // no change
		}
		else {
			LogWarn("invalid/unexpected CREATE."); // TODO
			return STATE_WAITRESP; // no change
		}
	}
	/*
	 * We can't reach the destination because there seems to be no GIST
	 * hop. If the proxy flag was set in the original CREATE, however,
	 * then this node will act as a proxy and create an nr_session.
	 * 
	 * Of course, proxy mode will only work if this is a firewall. For
	 * NATs, a reservation would be required, but there is no NR who could
	 * do this.
	 *
	 * Note: Authentication/Authorization has been checked in state IDLE,
	 *       when the CREATE message arrived.
	 */
	else if ( ! config->is_nf_nat() && is_no_next_node_found_event(evt)
			&& get_last_create_message_proxy_mode() ) {

		LogInfo("no next node found, this node will act as a proxy");

		set_proxy_mode(true);

		d->install_policy_rules(NULL, get_fw_policy_rule_copy());

		/*
		 * Create the response that usually the NR would send.
		 */
		ntlp_msg *msg = get_last_create_message();

		d->send_message( msg->create_success_response(lifetime) );

		state_timer.stop();

		return STATE_SESSION;
	}
	/*
	 * Either GIST can't reach the destination or we waited long enough
	 * for the downstream peer to respond. Anyway, send a RESPONSE.
	 */
	else if ( is_no_next_node_found_event(evt)
			|| is_timer(evt, state_timer) ) {

		LogInfo("cannot reach destination");

		ntlp_msg *latest = get_last_create_message();
		ntlp_msg *resp = latest->create_response(
			information_code::sc_permanent_failure,
			information_code::fail_nr_not_reached);

		d->send_message( resp );

		// TODO: ReportAsyncEvent() ?

		if ( is_external_address_reserved() )
			d->release_external_address(get_external_address());

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
nf_session::state_t nf_session::handle_state_session(
		dispatcher *d, event *evt) {

	using namespace natfw::msg;

	/*
	 * A msg_event arrived which contains a NATFW CREATE message.
	 */
	if ( is_natfw_create(evt) ) {
		msg_event *e = dynamic_cast<msg_event *>(evt);
		ntlp_msg *msg = e->get_ntlp_msg();
		natfw_create *create = e->get_create();

		uint32 lifetime = create->get_session_lifetime();
		uint32 msn = create->get_msg_sequence_number();

		/*
		 * Before proceeding check several preconditions.
		 */
		try {
			check_lifetime(lifetime, get_max_lifetime());
			check_authorization(d, e);
		}
		catch ( request_error &e ) {
			LogError(e);
			d->send_message( msg->create_error_response(e) );
			return STATE_SESSION; // no change!
		}

		if ( ! is_greater_than(msn, get_msg_sequence_number()) ) {
			LogWarn("discarding duplicate response.");
			return STATE_SESSION; // no change
		}

		/*
		 * All preconditions have been checked.
		 */

		if ( lifetime > 0 ) {
			LogDebug("forwarder session refreshed.");
			// TODO: shouldn't the RESPONSE refresh?

			set_lifetime(lifetime); // could be a new lifetime!
			set_msg_sequence_number(msn);

			// TODO: If we get a different policy rule -> update
			// See section 3.11.
			//save_fw_policy_rule(e);

			// store one copy for further reference and pass one on
			set_last_create_message( msg->copy() );

			if ( ! is_proxy_mode() ) {
				d->send_message( create_msg_for_nr(msg) );

				response_timer.start(d, get_response_timeout());
			}
			else
				d->send_message(
					msg->create_success_response(lifetime));

			return STATE_SESSION; // no change
		}
		else {	// lifetime == 0
			LogDebug("terminating session.");

			state_timer.stop();
			response_timer.stop();

			d->remove_policy_rules(get_nat_policy_rule_copy(),
				get_fw_policy_rule_copy());

			d->send_message( create_msg_for_nr(msg) );

			if ( is_external_address_reserved() )
				d->release_external_address(
					get_external_address());

			return STATE_FINAL;
		}
	}
	/*
	 * A RESPONSE to a CREATE arrived.
	 */
	else if ( is_natfw_response(evt, get_last_create_message()) ) {
		msg_event *e = dynamic_cast<msg_event *>(evt);
		ntlp_msg *msg = e->get_ntlp_msg();
		natfw_response *resp = e->get_response();

		// Discard if this is no RESPONSE to our original CREATE.
		natfw_create *c = get_last_create_message()->get_natfw_create();
		if ( ! resp->is_response_to(c) ) {
			LogWarn("RESPONSE doesn't match CREATE, discarding");
			return STATE_SESSION;	// no change
		}

		
		if ( resp->is_success() ) {
			LogDebug("upstream peer sent successful response.");

			d->send_message( create_msg_for_ni(msg) );

			// TODO: calc remaining!
			state_timer.start(d, get_lifetime());

			return STATE_SESSION; // no change
		}
		else {
			LogWarn("error message received.");

			d->remove_policy_rules(get_nat_policy_rule_copy(),
				get_fw_policy_rule_copy());

			d->send_message( create_msg_for_ni(msg) );

			if ( is_external_address_reserved() )
				d->release_external_address(
					get_external_address());

			return STATE_FINAL;
		}
	}
	/*
	 * Downstream peer didn't respond.
	 * 
	 * This is the synchronous case, because the upstream peer is still
	 * waiting for a response to its CREATE message.
	 *
	 * Note: We can safely ignore this when we're in proxy mode.
	 */
	else if ( ! is_proxy_mode() && ( is_no_next_node_found_event(evt)
			|| is_timer(evt, response_timer) ) ) {
		LogWarn("downstream peer did not respond");

		state_timer.stop();
		response_timer.stop();

		d->remove_policy_rules(get_nat_policy_rule_copy(),
			get_fw_policy_rule_copy());

		// TODO: check the spec!
		ntlp_msg *resp = get_last_create_message()->create_response(
			information_code::sc_permanent_failure,
			information_code::fail_nr_not_reached);

		d->send_message( resp );

		if ( is_external_address_reserved() )
			d->release_external_address(get_external_address());

		return STATE_FINAL; // TODO: check!
	}
	/*
	 * Upstream peer didn't send a refresh CREATE in time.
	 */
	else if ( is_timer(evt, state_timer) ) {
		LogWarn("session timed out");

		response_timer.stop();

		d->remove_policy_rules(get_nat_policy_rule_copy(),
			get_fw_policy_rule_copy());

		// TODO: ReportAsyncEvent()

		if ( is_external_address_reserved() )
			d->release_external_address(get_external_address());

		return STATE_FINAL;
	}
	/*
	 * GIST detected that one of our routes is no longer usable. This
	 * could be the route to the NI or to the NR.
	 */
	else if ( is_route_changed_bad_event(evt) ) {
		LogUnimp("route to the NI or to the NR is no longer usable");
		return STATE_FINAL;
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
void nf_session::process_event(dispatcher *d, event *evt) {
	LogDebug("begin process_event(): " << *this);

	switch ( get_state() ) {

		case nf_session::STATE_IDLE:
			state = handle_state_idle(d, evt);
			break;

		case nf_session::STATE_WAITRESP:
			state = handle_state_waitresp(d, evt);
			break;

		case nf_session::STATE_SESSION:
			state = handle_state_session(d, evt);
			break;

		case nf_session::STATE_FINAL:
			// ignore event and continue
			break;

		default:
			assert( false ); // invalid state
	}

	LogDebug("end process_event(): " << *this);
}

// EOF
