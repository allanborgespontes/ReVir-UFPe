/// ----------------------------------------*- mode: C++; -*--
/// @file QoS_StateModule_AHO_Processing.cpp
/// QoS NSLP State Module for Anticipated Handover Processing
/// ----------------------------------------------------------
/// $Id: QoS_StateModule_AHO_Processing.cpp 5941 2011-02-27 17:58:14Z stud-mueller $
/// $HeadURL: https://svn.tm.kit.edu/nsis/qos-nslp/trunk/src/QoS_StateModule_AHO_Processing.cpp $
// ===========================================================
//                      
// Copyright (C) 2005-2010, all rights reserved by
// - Institute of Telematics, Karlsruhe Institute of Technology
//
// More information and contact:
// https://projekte.tm.uka.de/trac/NSIS
//                      
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; version 2 of the License
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
// 02110-1301 USA.
//
// ===========================================================

#include "QoS_StateModule.h"
#include "ProcessingModule.h"
#include "qos_nslp_aho_contextmap.h"
#include "nslp_session_context_map.h"
#include "context_manager.h"

#ifdef USE_AHO

namespace qos_nslp {


void
state_manager::process_recv_est_msg_aho(known_nslp_pdu *known_pdu, const ntlp::sessionid* sid, const ntlp::mri_explicitsigtarget* rcvd_mri)
{
    AhoContextMap aho_contextmap = Context_Manager::instance()->get_acm();
    NSLP_Session_Context_Map session_context_map = Context_Manager::instance()->get_scm();

	if (known_pdu->is_reserve()) {
		DLog(state_manager::modname, color[blue] << "Received RESERVE-AHO message by the EST-MRM." << color[off]);

		// create AHO_Context in aho_contextmap if necessary
		if(aho_contextmap->find(*sid) == NULL) {
			// no AHO_context so far, thus create one as AR_N
			NSLP_AHO_Context *aho_context = new NSLP_AHO_Context();
	
			aho_context->lock();
			// set role of node during an AHO in AHO_Context
			aho_context->set_aho_node_role(NSLP_AHO_Context::ARN);
			// save address of mobile node MN
			aho_context->set_addr_mobile_node(rcvd_mri->get_origin_sig_address());
			// save address of new access router ARN
			aho_context->set_addr_new_access_router(rcvd_mri->get_dest_sig_address());
			// add mri of the new flow to the AHO_Context
			aho_context->add_flow_to_context(rcvd_mri->get_mri_pc());
			aho_context->unlock();

			// save context in aho_contextmap
			aho_contextmap->insert(*sid, aho_context);
		}


		// determine node role within AHO
		NSLP_AHO_Context::node_role_t role = NSLP_AHO_Context::OTHER;
		hostaddress mn, ar_n;
		NSLP_AHO_Context *aho_context = aho_contextmap->find(*sid);
		if(aho_context != NULL) {
			aho_context->lock();
			role = aho_context->get_aho_node_role();
			if(role == NSLP_AHO_Context::ARN) {
				mn = aho_context->get_addr_mobile_node();
			}
			if(role == NSLP_AHO_Context::MN) {
				ar_n = aho_context->get_addr_new_access_router();
			}
			aho_context->unlock();
		}


		// proceed with anticipated handover
		if(role == NSLP_AHO_Context::ARN) {
			// we are ARN and start with AHO phase 2 / 2a

			mri_pathcoupled pc_mri = rcvd_mri->get_mri_pc();
			bool down = pc_mri.get_downstream();
	
			if(down) {
				// downstream

				// process RESERVE message just as we had received this
				// message (incoming message) by the PC-MRM
				NSLP_Session_Context *session_context=NULL;
				error_t nslpres = find_or_create_new_session_context(*known_pdu, sid, rcvd_mri, down, session_context, NULL);
				
				DLog(state_manager::modname, "find_or_create_new_session_context() - returned: " << error_string(nslpres));
				
				if (nslpres == error_ok) {
					nslpres = update_session_context(*known_pdu, sid, rcvd_mri, down, 0, session_context, NULL);
				
					DLog(state_manager::modname, "update_session_context() - returned: " << error_string(nslpres));
				}

				switch (nslpres) {
				case state_manager::error_old_pdu:
				case state_manager::error_rsn_missing:
				case state_manager::error_no_bandwidth:
					DLog(state_manager::modname, "process_tp_recv_est_msg() - processing of RESERVE msg returned an error: "
							<< error_string(nslpres));
					break;
				case state_manager::error_nothing_to_do:
				case state_manager::error_ok_forward:
				case state_manager::error_ok_send_response:
				case state_manager::error_ok:
					break;
				default:
					ERRLog(state_manager::modname, "Received an NSLP error (#"
							<< nslpres << ':' << state_manager::error_string(nslpres) << ") that cannot be handled.");
					break;
				}
				// end process RESERVE message
			}
			else {
				// upstream
				
				reservereq *reservemsg = dynamic_cast<reservereq *>(known_pdu);
				assert(reservemsg != NULL);

				rii *rcvd_rii = reservemsg->get_rii();
				rp *rcvd_rp = reservemsg->get_rp();
				bound_sessionid *rcvd_b_sid = reservemsg->get_bound_sid();
				qspec_object *qspec_obj = reservemsg->get_qspec();

				if (qspec_obj == NULL) {
					ERRLog(state_manager::modname, "Received RESERVE-AHO message without QSPEC object. Cannot proceed!");

					// erase aho_context in aho_contextmap
					NSLP_AHO_Context *aho_context = aho_contextmap->find(*sid);
					if (aho_context != NULL) {
						aho_context->lock();
						aho_context->remove_flow_from_context(pc_mri);
						aho_context->unlock();
					}

					return;
				}

				assert(qspec_obj != NULL);

				// Note that aho_context is surly != NULL, even though not checked here!
				// See assignment of variable role.
				aho_context->lock();
				aho_context->set_qspec(qspec_obj);

				if(rcvd_rii) {
					aho_context->set_rii(rcvd_rii);
				}

				if(rcvd_rp) {
					aho_context->set_refresh_period(rcvd_rp);
				}

				if(rcvd_b_sid) {
					aho_context->set_bound_sessionid(rcvd_b_sid);
				}
				aho_context->unlock();

				// create NOTIFY message
				notifymsg *notify = new notifymsg();

				info_spec *send_error = new info_spec(NULL, info_spec::information, info_spec::InitQuery, 0);
				notify->set_errorobject(send_error);
				notify->set_qspec(qspec_obj->copy());

				known_nslp_pdu *notify_pdu = dynamic_cast<known_nslp_pdu *>(notify);
				assert(notify_pdu != NULL);


				// send NOTIFY message with EST-MRM
				hostaddress orig_sig_addr = rcvd_mri->get_dest_sig_address();
				hostaddress dest_sig_addr = pc_mri.get_sourceaddress();
				mri_explicitsigtarget *est_mri = new mri_explicitsigtarget(pc_mri, orig_sig_addr, dest_sig_addr);

				ExplicitSignalingMsg* sigmsg = new ExplicitSignalingMsg();
				sigmsg->set_pdu(notify_pdu);
				sigmsg->set_sid(*sid);
				sigmsg->set_sig_mri(est_mri);
				sigmsg->send_or_delete();
			}
		}
		else if(role == NSLP_AHO_Context::MN) {
			// process RESERVE message just as we had received this
			// message (incoming message) by the PC-MRM

			reservereq *reservemsg = dynamic_cast<reservereq *>(known_pdu);
			assert(reservemsg != NULL);

			reservemsg->set_anticipated_reservation(true);

			NSLP_Session_Context *session_context = session_context_map->find_session_context(*sid);
			if (session_context == NULL) {
				ERRLog(state_manager::modname, "No NSLP_Session_Context found! Cannot proceed with the Anticipated Handover!");
				return;
			}

			mri_pathcoupled pc_mri = rcvd_mri->get_mri_pc();
			bool down = pc_mri.get_downstream();

			error_t nslpres = update_session_context(*known_pdu, sid, rcvd_mri, down, 0, session_context, NULL);

			DLog(state_manager::modname, "update_session_context() - returned: " << error_string(nslpres));


			switch (nslpres) {
			case state_manager::error_old_pdu:
			case state_manager::error_rsn_missing:
			case state_manager::error_no_bandwidth:
				DLog(state_manager::modname, "process_tp_recv_est_msg() - processing of RESERVE-AHO msg returned an error: "
						<< error_string(nslpres));
				break;
			case state_manager::error_nothing_to_do:
			case state_manager::error_ok_forward:
			case state_manager::error_ok_send_response:
			case state_manager::error_ok:
				break;
			default:
				ERRLog(state_manager::modname, "Received an NSLP error (#"
						<< nslpres << ':' << state_manager::error_string(nslpres) << ") that cannot be handled.");
				break;
			}
			// end process RESERVE message
		}
		else {
			ERRLog(state_manager::modname, "Received RESERVE-AHO, but don't know what to do because I'm neither MN nor ARN!");
			return;
		}


		return;
	}

	if (known_pdu->is_response()) {
		DLog(state_manager::modname, color[blue] << "Received RESPONSE-AHO message by the EST-MRM." << color[off]);

		responsemsg *resp = dynamic_cast<responsemsg *>(known_pdu);
		assert(resp != NULL);


		// determine node role within AHO
		NSLP_AHO_Context::node_role_t role = NSLP_AHO_Context::OTHER;
		NSLP_AHO_Context *aho_context = aho_contextmap->find(*sid);
		if(aho_context != NULL) {
			aho_context->lock();
			role = aho_context->get_aho_node_role();
			aho_context->unlock();
		}
	
		if(role == NSLP_AHO_Context::MN) {
			// process RESPONSE message just as we had received this
			// message (incoming message)
			process_response_msg(known_pdu, sid, rcvd_mri, false /* dummy */);
		}
		else if(role == NSLP_AHO_Context::ARN) {
			// process RESPONSE message just as we had received this
			// message (incoming message)
			process_response_msg(known_pdu, sid, rcvd_mri, false /* dummy */);
		}
		else {
			ILog(state_manager::modname, "Received RESPONSE-AHO, but don't know what to do because I'm neither MN nor ARN!");
			return;
		}

		return;
	}

	if (known_pdu->is_query()) {
		DLog(state_manager::modname, color[blue] << "Received QUERY-AHO message by the EST-MRM." << color[off]);

		// create AHO_Context in aho_contextmap if necessary
		if(aho_contextmap->find(*sid) == NULL) {
			NSLP_AHO_Context *aho_context = new NSLP_AHO_Context();
			
			aho_context->lock();
			// set role of node during an AHO in AHO_Context
			aho_context->set_aho_node_role(NSLP_AHO_Context::ARN);
			// save address of mobile node MN
			aho_context->set_addr_mobile_node(rcvd_mri->get_origin_sig_address());
			// save address of new access router ARN
			aho_context->set_addr_new_access_router(rcvd_mri->get_dest_sig_address());
			// add mri of the new flow to the AHO_Context
			aho_context->add_flow_to_context(rcvd_mri->get_mri_pc());
			aho_context->unlock();
			
			// save context in aho_contextmap
			aho_contextmap->insert(*sid, aho_context);
		}


		// we are ARN and start with AHO phase 2a
		mri_pathcoupled pc_mri = rcvd_mri->get_mri_pc();
		bool down = pc_mri.get_downstream();

		if(down) {
			// downstream

			// process QUERY message just as we had received this
			// message (incoming message)
			querymsg *query = dynamic_cast<querymsg *>(known_pdu);
			assert(query != NULL);

			if (!query->is_reserve_init_flag()) {
				ERRLog(state_manager::modname, "Received QUERY-AHO message without Reserve-Init flag set!");
				return;
			}

			assert(query->is_reserve_init_flag());


			NSLP_Session_Context *session_context=NULL;
			error_t err = find_or_create_new_session_context(*query, sid, rcvd_mri, down, session_context, NULL);

			DLog(state_manager::modname, "find_or_create_new_session_context() - returned: " << error_string(err));
			
			process_query_msg(query, sid, rcvd_mri, down, session_context, NULL);
		}
		else {
			// upstream
			querymsg *query = dynamic_cast<querymsg *>(known_pdu);
			assert(query != NULL);

			qspec_object *qspec_obj = query->get_qspec();
			if (qspec_obj == NULL) {
				ERRLog(state_manager::modname, "Received QUERY-AHO message without QSPEC object. Cannot proceed!");
			
				// erase aho_context in aho_contextmap
				NSLP_AHO_Context *aho_context = aho_contextmap->find(*sid);
				if (aho_context != NULL) {
					aho_context->lock();
					aho_context->remove_flow_from_context(pc_mri);
					aho_context->unlock();
				}
			
				return;
			}
			
			assert(qspec_obj != NULL);


			// create NOTIFY message
			notifymsg *notify = new notifymsg();

			info_spec *send_error = new info_spec(NULL, info_spec::information, info_spec::InitReserve, 0);
			notify->set_errorobject(send_error);
			notify->set_qspec(qspec_obj->copy());

			known_nslp_pdu *notify_pdu = dynamic_cast<known_nslp_pdu *>(notify);
			assert(notify_pdu != NULL);


			// send NOTIFY message with EST-MRM
			hostaddress orig_sig_addr = rcvd_mri->get_dest_sig_address();
			hostaddress dest_sig_addr = pc_mri.get_sourceaddress();
			mri_explicitsigtarget *est_mri = new mri_explicitsigtarget(pc_mri, orig_sig_addr, dest_sig_addr);

			ExplicitSignalingMsg* sigmsg = new ExplicitSignalingMsg();
			sigmsg->set_pdu(notify_pdu);
			sigmsg->set_sid(*sid);
			sigmsg->set_sig_mri(est_mri);
			sigmsg->send_or_delete();
		}

		return;
	}

	if (known_pdu->is_notify()) {
		DLog(state_manager::modname, color[blue] << "Received NOTIFY message by the EST-MRM." << color[off]);

		// we are in the case, where the mobile node is receiver of a flow
		notifymsg* notify = dynamic_cast<notifymsg*>(known_pdu);
		assert(notify != NULL);

		info_spec *err_obj = notify->get_errorobject();
		info_spec::errorclass_t err_class = (info_spec::errorclass_t) err_obj->get_errorclass();
		info_spec::errorcode_t err_code = (info_spec::errorcode_t) err_obj->get_errorcode();

		if(err_class != info_spec::information) {
			ILog(state_manager::modname, "Received NOTIFY message with error class " << err_obj->get_errorclass_string() << ", ending here!");
			return;
		}

		assert(err_class == info_spec::information);

		if(err_code == info_spec::InitQuery) {
			// create QUERY message
			querymsg* query = new querymsg();
	
			// QSPEC
			qspec_object* qspec_obj = notify->get_qspec()->copy();
			query->set_qspec(qspec_obj);

			// Reserve-Init-Flag
			query->set_reserve_init_flag();

			if(notify->is_x_flag()) {
				query->set_x_flag();
			}
	
			known_nslp_pdu *query_pdu = dynamic_cast<known_nslp_pdu*>(query);
			assert(query_pdu != NULL);
	

			// send QUERY message with PC-MRM
			mri_pathcoupled *pc_mri = rcvd_mri->get_mri_pc().copy();
			pc_mri->invertDirection();
			bool down = pc_mri->get_downstream();
	
			SignalingMsg* sigmsg = new SignalingMsg();
			sigmsg->set_msg(query_pdu);
			sigmsg->set_sid(*sid);
			sigmsg->set_sig_mri(pc_mri);
			sigmsg->set_downstream(down);
			sigmsg->send_or_delete();
		}
		else if(err_code == info_spec::InitReserve) {
			// create RESERVE message
			reservereq* reserve = new reservereq();
	
			// get session_context of given session
			NSLP_Session_Context *session_context = session_context_map->find_session_context(*sid);
			if(session_context == NULL) {
				ERRLog(state_manager::modname, "Unable to retrieve session_context for session with SID ["
						<< sid->to_string() << "], ending here!");

				return;
			}

			reserve->set_originator(true);

			// RII
			rii *rii_obj = new rii();
			rii_obj->generate_new();
			rii_obj->set_own(true);
			rii_obj->set_is_reserve(true);
			reserve->set_rii(rii_obj);

			// RSN
			uint32 own_rsn;
			session_context->lock();
			session_context->increment_own_rsn();
			own_rsn= session_context->get_own_rsn();
			session_context->unlock();
			rsn *rsn_obj = new rsn(own_rsn);
			reserve->set_rsn(rsn_obj);

			// BOUND SID
			uint128 bsid_num;
			session_context->lock();
			bool found = session_context->get_bound_sid(bsid_num);
			session_context->unlock();
			if(found) {
				bound_sessionid *bsid = new bound_sessionid(bsid_num);
				reserve->set_bound_sid(bsid);
			}

			// QSPEC
			qspec_object *qspec_obj = notify->get_qspec()->copy();
			reserve->set_qspec(qspec_obj);

			// Replace flag
			if(notify->is_x_flag()) {
				reserve->set_replace_flag();
			}
			else {
				reserve->set_anticipated_reservation(true);
			}

			known_nslp_pdu *reserve_pdu = dynamic_cast<known_nslp_pdu*>(reserve);
			assert(reserve_pdu != NULL);
	

			// send RESERVE message with PC-MRM
			mri_pathcoupled *pc_mri = rcvd_mri->get_mri_pc().copy();
			pc_mri->invertDirection();
			bool down = pc_mri->get_downstream();
	
			SignalingMsg* sigmsg = new SignalingMsg();
			sigmsg->set_msg(reserve_pdu);
			sigmsg->set_sid(*sid);
			sigmsg->set_sig_mri(pc_mri);
			sigmsg->set_downstream(down);
			sigmsg->send_or_delete();
		}
		else {
			ILog(state_manager::modname, "Received NOTIFY message with error class " << err_obj->get_errorclass_string()
					<< ", but unexpected error code, ending here!");
			return;
		}
		return;
	}
} // end process_recv_est_msg_aho

/**
 * Process event which indicates necessity of a network change
 * @param iface the affected network interface
 * @param l2_ap_n the layer 2 address of the new access point
 * @param l3_ar_n the layer 3 address of the new access router
 */
void state_manager::process_aho_event(const string iface, macaddress l2_ap_n, hostaddress l3_ar_n, hostaddress l3_mn)
{
    NSLP_Session_Context_Map session_context_map = Context_Manager::instance()->get_scm();

	ILog(state_manager::modname, "process_aho_event()");

	ILog(state_manager::modname, "AHO-iface: " << iface);
	ILog(state_manager::modname, "AHO-l2_ap_n: " << l2_ap_n);
	ILog(state_manager::modname, "AHO-l3_ar_n: " << l3_ar_n);
	ILog(state_manager::modname, "AHO-l3_mn: " << l3_mn);

	//
	// Anticipated Handover - Phase 1
	//
	ILog(state_manager::modname, color[yellow] << "Starting with Anticipated Handover - Phase 1 for Interface " << iface << color[off]);
	
	session_context_map->traverse_contexts(this, &qos_nslp::state_manager::aho_phase_1, (void *)&l3_ar_n, (void *)&l3_mn);

	ILog(state_manager::modname, "process_aho_event() - done");
}


/**
 * Process event which indicates an occured network change
 * @param iface the affected network interface
 * @param l2_ap_n the layer 2 address of the new access point
 * @param l3_ar_n the layer 3 address of the new access router
 * @param l3_mn the new layer 3 address of the mobile node
 */
void state_manager::process_ho_event(const string iface, macaddress l2_ap_n, hostaddress l3_ar_n, hostaddress l3_mn)
{
    NSLP_Session_Context_Map session_context_map = Context_Manager::instance()->get_scm();

	ILog(state_manager::modname, "process_ho_event()");

	ILog(state_manager::modname, "AHO-iface: " << iface);
	ILog(state_manager::modname, "AHO-l2_ap_n: " << l2_ap_n);
	ILog(state_manager::modname, "AHO-l3_ar_n: " << l3_ar_n);
	ILog(state_manager::modname, "AHO-l3_mn: " << l3_mn);

	//
	// Anticipated Handover - Phase 3 / 2a
	//
	ILog(state_manager::modname, color[yellow] << "Proceeding with Anticipated Handover - Phase 3 / 2a for Interface " << iface << color[off]);

	session_context_map->traverse_contexts(this, &qos_nslp::state_manager::ho_phase_3_2a, (void *)&l3_ar_n, (void *)&l3_mn);

	ILog(state_manager::modname, "process_ho_event() - done");
}


void state_manager::aho_phase_1(NSLP_Session_Context *session_context, void *param1, void *param2)
{
    AhoContextMap aho_contextmap = Context_Manager::instance()->get_acm();

	ILog(state_manager::modname, "aho_phase_1()");

	hostaddress *l3_ar_n = reinterpret_cast<hostaddress *>(param1);
	if(l3_ar_n == NULL) {
		ERRLog(state_manager::modname, "aho_phase_1: Missing parameter l3_ar_n!");
		return;
	}

	assert(l3_ar_n != NULL);


	hostaddress *l3_mn = reinterpret_cast<hostaddress *>(param2);
	if(l3_mn == NULL) {
		ERRLog(state_manager::modname, "aho_phase_1: Missing parameter l3_mn!");
		return;
	}

	assert(l3_mn != NULL);


	if(session_context != NULL) {
		// retrieve some informations from session_context
		uint128 tmp_sid;
		NSLP_Session_Context::qn_type_t qn_type;

		session_context->lock();
          	session_context->get_sid(tmp_sid);
		qn_type = session_context->get_qn_type();
		session_context->unlock();

		ntlp::sessionid sid(tmp_sid);

		ILog(state_manager::modname, "aho_phase_1() - Starting AHO-Phase 1 for SID [" << sid.to_string() << "]");


		// create new AHO_Context
		NSLP_AHO_Context *aho_context = new NSLP_AHO_Context();
		aho_context->lock();
		// set role of node during an AHO in AHO_Context
		aho_context->set_aho_node_role(NSLP_AHO_Context::MN);
		// save address of access router AR_N
		aho_context->set_addr_new_access_router(*l3_ar_n);
		aho_context->unlock();
		
		// insert AHO_Context into aho_contextmap
		aho_contextmap->insert(sid, aho_context);


		// create anticipated reservations for all flows of the current session that are
		// not already anticipated reservations
		session_context->lock();
		NSLP_Session_Context::flowcontext_const_it_t it;
		it = session_context->flowcontext.begin();
		while (it != session_context->flowcontext.end()) {
			ntlp::mri_pathcoupled pc_mri = it->first;
			NSLP_Flow_Context *flow_context = it->second;

			++it;

			if ((flow_context != NULL) && (!flow_context->get_anticipated_reservation())) {
				// setup anticipated reservation for the current flow

				ILog(state_manager::modname, "aho_phase_1() - Processing reservation for flow ("
						<< pc_mri.get_sourceaddress() << ", " << pc_mri.get_destaddress() << ")");

				// retrieve some informations from flow_context
				flow_context->lock();
				bool is_flow_sender = flow_context->get_is_flow_sender();
				bool is_flow_receiver = flow_context->get_is_flow_receiver();
				flow_context->unlock();


				// error checking
				if((!is_flow_sender) && (!is_flow_receiver)) {
					session_context->unlock();

					ERRLog(state_manager::modname, "AHO not implemented for case where mobile node isn't endpoint of a flow!");
					continue;
				}

				assert(is_flow_sender || is_flow_receiver);
				
				
				// build RESERVE or QUERY message depending
				// on the role of the mobile node
				known_nslp_pdu *pdu;
				if(qn_type == NSLP_Session_Context::QNI) {
					// create RESERVE message
					reservereq* res = new reservereq();

					res->set_originator(true);
					res->set_anticipated_reservation(true);
				
					// RII
					rii *rii_obj = new rii();
					rii_obj->generate_new();
					rii_obj->set_own(true);
					rii_obj->set_is_reserve(true);
					res->set_rii(rii_obj);
				
					// RSN
					uint32 own_rsn= session_context->get_own_rsn();
					rsn *rsn_obj = new rsn(own_rsn);
					res->set_rsn(rsn_obj);

					// BOUND SID
					uint128 bsid_num;
					if(session_context->get_bound_sid(bsid_num)) {
						bound_sessionid *bsid = new bound_sessionid(bsid_num);
						res->set_bound_sid(bsid);
					}

					// QSPEC
					flow_context->lock();
					qspec_object *qspec_obj = flow_context->get_qspec().copy();
					flow_context->unlock();
					res->set_qspec(qspec_obj);

					// Refresh Period
					uint32 refr_period;
					flow_context->lock();
					flow_context->get_refresh_period(refr_period);
					flow_context->unlock();
					rp *refresh_period = new rp(refr_period);
					res->set_rp(refresh_period);

					// PROXY-Flag
					res->set_proxy_flag();
				
					pdu = dynamic_cast<known_nslp_pdu *>(res);
				}
				else if(qn_type == NSLP_Session_Context::QNR) {
					// create QUERY message
					querymsg* query = new querymsg();
				
					query->set_originator(true);

					// BOUND SID
					uint128 bsid_num;
					if(session_context->get_bound_sid(bsid_num)) {
						bound_sessionid *bsid = new bound_sessionid(bsid_num);
						query->set_bound_sid(bsid);
					}

					// QSPEC
					flow_context->lock();
					qspec_object *qspec_obj = flow_context->get_qspec().copy();
					flow_context->unlock();
					query->set_qspec(qspec_obj);

					// Flags
					query->set_reserve_init_flag();
					query->set_proxy_flag();
				
					pdu = dynamic_cast<known_nslp_pdu*>(query);
				}
				else {
					ERRLog(state_manager::modname, "AHO not implemented for case where mobile node is QNE of a reservation!");
					continue;
				}
				
				assert(pdu != NULL);
				
				
				// build mri
				hostaddress orig_sig_addr;
				if(is_flow_sender) {
					orig_sig_addr = pc_mri.get_sourceaddress();
					pc_mri.set_sourceaddress(*l3_mn);
					pc_mri.set_downstream(true);
				}
				else if(is_flow_receiver) {
					orig_sig_addr = pc_mri.get_destaddress();
					pc_mri.set_destaddress(*l3_mn);
					pc_mri.set_downstream(false);
				}
				mri_explicitsigtarget *est_mri = new mri_explicitsigtarget(pc_mri, orig_sig_addr, *l3_ar_n);
				
				
				// logging
				if((qn_type == NSLP_Session_Context::QNI) && is_flow_sender) {
					ILog(state_manager::modname, "AHO with mobile node as sender and QNI, sending "
							<< color[blue] << "RESERVE-AHO" << color[off] << " to address: " << *l3_ar_n);
				}
				else if((qn_type == NSLP_Session_Context::QNI) && is_flow_receiver) {
					ILog(state_manager::modname, "AHO with mobile node as receiver and QNI, sending "
							<< color[blue] << "RESERVE-AHO" << color[off] << " to address: " << *l3_ar_n);
				}
				else if((qn_type == NSLP_Session_Context::QNR) && is_flow_sender) {
					ILog(state_manager::modname, "AHO with mobile node as sender and QNR, sending "
							<< color[blue] << "QUERY-AHO" << color[off] << " to address: " << *l3_ar_n);
				}
				else if((qn_type == NSLP_Session_Context::QNR) && is_flow_receiver) {
					ILog(state_manager::modname, "AHO with mobile node as receiver and QNR, sending "
							<< color[blue] << "QUERY-AHO" << color[off] << " to address: " << *l3_ar_n);
				}
				
				
				// send message
				ExplicitSignalingMsg* sigmsg = new ExplicitSignalingMsg();
				sigmsg->set_pdu(pdu);
				sigmsg->set_sid(tmp_sid);
				sigmsg->set_sig_mri(est_mri);
				sigmsg->send_or_delete();


				aho_context->lock();
				// add mri of the new flow to the AHO_Context
				aho_context->add_flow_to_context(pc_mri);
				aho_context->set_is_flow_sender(is_flow_sender);
				aho_context->set_is_flow_receiver(is_flow_receiver);
				aho_context->unlock();
			}
		} // end while
		session_context->unlock();
	} // end if session_context != NULL

	ILog(state_manager::modname, "aho_phase_1() - done");
}


void state_manager::ho_phase_3_2a(NSLP_Session_Context *session_context, void *param1, void *param2)
{
    AhoContextMap aho_contextmap = Context_Manager::instance()->get_acm();

	ILog(state_manager::modname, "aho_phase_3_2a");

	hostaddress *l3_ar_n = reinterpret_cast<hostaddress *>(param1);
	if(l3_ar_n == NULL) {
		ERRLog(state_manager::modname, "aho_phase_3_2a: Missing parameter l3_ar_n!");
		return;
	}

	assert(l3_ar_n != NULL);


	hostaddress *l3_mn = reinterpret_cast<hostaddress *>(param2);
	if(l3_mn == NULL) {
		ERRLog(state_manager::modname, "aho_phase_3_2a: Missing parameter l3_mn!");
		return;
	}

	assert(l3_mn != NULL);


	if (session_context != NULL) {
		// retrieve some informations from session_context
		uint128 tmp_sid;
		NSLP_Session_Context::qn_type_t qn_type;

		session_context->lock();
		session_context->get_sid(tmp_sid);
		qn_type = session_context->get_qn_type();
		session_context->unlock();

		ntlp::sessionid sid(tmp_sid);

		ILog(state_manager::modname, "aho_phase_3_2a() - Starting AHO-Phase 3 / 2a for SID [" << sid.to_string() << "]");


		bool hard_handover=false;

		// determine if the new access router is the anticipated one
		NSLP_AHO_Context *aho_context = aho_contextmap->find(sid);
		if(aho_context != NULL) {
			aho_context->lock();
			NSLP_AHO_Context::node_role_t role = aho_context->get_aho_node_role();
	
			hostaddress l3_ar_n_saved;
			if(role != NSLP_AHO_Context::MN) {
				ERRLog(state_manager::modname, "Since I received a HO-Trigger, I am MN, but AHO_Context tells something other!");
				aho_context->unlock();
				return;
			}
			else {
				l3_ar_n_saved = aho_context->get_addr_new_access_router();
			}
			aho_context->unlock();


			// check if the new access router is the expected one
			if((*l3_ar_n) != l3_ar_n_saved) {
				ILog(state_manager::modname, "The new access router is not the expected one, proceeding with Hard Handover.");
				hard_handover = true;
			}
		} // end if aho_context != NULL
		else {
			// no NSLP_AHO_Context found
			ILog(state_manager::modname, "No NSLP_AHO_Context found for SID [" << sid.to_string() << "], proceeding with Hard Handover.");
			hard_handover = true;
		}

		// (hard_handover == true)   ==>  no NSLP_AHO_Context found OR the new access router is not the expected one
		// (hard_handover == false)  ==>  we have to check for each (old) flow if there is an anticipated reservation and proceed accordingly


		// perform handover
		session_context->lock();
		NSLP_Session_Context::flowcontext_const_it_t it;
		it = session_context->flowcontext.begin();
		while (it != session_context->flowcontext.end()) {
			ntlp::mri_pathcoupled pc_mri = it->first;
			NSLP_Flow_Context *flow_context = it->second;

			++it;

			if ((flow_context != NULL) && (!flow_context->get_anticipated_reservation())) {
				ILog(state_manager::modname, "aho_phase_3_2a() - Processing reservation for flow ("
						<< pc_mri.get_sourceaddress() << ", " << pc_mri.get_destaddress() << ")");

				// retrieve some informations from flow_context
				uint32 refr_period;
				flow_context->lock();
				bool is_flow_sender = flow_context->get_is_flow_sender();
				bool is_flow_receiver = flow_context->get_is_flow_receiver();
				qspec_object *qspec_obj = flow_context->get_qspec().copy();
				flow_context->get_refresh_period(refr_period);
				flow_context->unlock();

				// error checking
				if((!is_flow_sender) && (!is_flow_receiver)) {
					session_context->unlock();

					ERRLog(state_manager::modname, "AHO not implemented for case where mobile node isn't endpoint of a flow!");
					continue;
				}

				assert(is_flow_sender || is_flow_receiver);


				// delete old reservation
				delete_flow_context(&sid, &pc_mri, session_context, flow_context);

				// invalidate route for the old flow
				SignalingMsg *sigmsg = new SignalingMsg();
				sigmsg->set_sig_mri(pc_mri.copy());
				sigmsg->set_msg(0);
				sigmsg->set_downstream(true);
				sigmsg->set_invalidate(true);
				sigmsg->send_or_delete();
				
				sigmsg = new SignalingMsg();
				sigmsg->set_sig_mri(pc_mri.copy());
				sigmsg->set_msg(0);
				sigmsg->set_downstream(false);
				sigmsg->set_invalidate(true);
				sigmsg->send_or_delete();


				// check if there is a pre-established reservation which we can now activate
				bool reservation_exists = false;

				ntlp::mri_pathcoupled new_pc_mri = pc_mri;
				if(is_flow_sender) {
					new_pc_mri.set_sourceaddress(*l3_mn);
				}
				else if(is_flow_receiver) {
					new_pc_mri.set_destaddress(*l3_mn);
				}

				NSLP_Flow_Context *new_flow_context = session_context->find_flow_context(new_pc_mri);
				if ((new_flow_context != NULL) && (new_flow_context->get_reserved())) {
					reservation_exists = true;

					new_flow_context->set_is_flow_sender(is_flow_sender);
					new_flow_context->set_is_flow_receiver(is_flow_receiver);
				}


				if ((!hard_handover) && reservation_exists) {
					// activate existing reservation

					// Proceed with an Anticipated Handover
					ILog(state_manager::modname, "Starting AHO-Phase 3 (" <<  color[green] << "Anticipated Handover" <<  color[off]
							<< ") for old flow (" << pc_mri.get_sourceaddress()
							<< ", " << pc_mri.get_destaddress() << ").");
				}
				else {
					// create new reservation, since there exists no one

					// Proceed with a Hard Handover
					ILog(state_manager::modname, "Starting AHO-Phase 3 / 2a (" <<  color[magenta] << "Hard Handover" <<  color[off]
							<< ") for old flow (" << pc_mri.get_sourceaddress()
							<< ", " << pc_mri.get_destaddress() << ").");
				}

				if (((!hard_handover) && reservation_exists) || is_flow_sender) {
					bool activate_reservation = ((!hard_handover) && reservation_exists);
					known_nslp_pdu *pdu;


					// build RESERVE or QUERY message depending
					// on the role of the mobile node
					if(qn_type == NSLP_Session_Context::QNI) {
						// create RESERVE message
						reservereq* res = new reservereq();

						res->set_originator(true);
						res->set_replace_flag();

						// RSN

						session_context->increment_own_rsn();
						uint32 own_rsn= session_context->get_own_rsn();
						rsn *rsn_obj = new rsn(own_rsn);
						res->set_rsn(rsn_obj);

						// RII
						if (!activate_reservation) {
							rii *rii_obj = new rii();
							rii_obj->generate_new();
							rii_obj->set_own(true);
							rii_obj->set_is_reserve(true);
							res->set_rii(rii_obj);
						}

						// BOUND SID
						uint128 bsid_num;
						if(session_context->get_bound_sid(bsid_num)) {
							bound_sessionid *bsid = new bound_sessionid(bsid_num);
							res->set_bound_sid(bsid);
						}

						// QSPEC
						res->set_qspec(qspec_obj);

						// Refresh Period
						rp *refresh_period = new rp(refr_period);
						res->set_rp(refresh_period);
					
					
						pdu = dynamic_cast<known_nslp_pdu *>(res);
					}
					else if(qn_type == NSLP_Session_Context::QNR) {
						// create QUERY message
						querymsg* query = new querymsg();

						query->set_originator(true);
						query->set_reserve_init_flag();
						query->set_x_flag();

						// BOUND SID
						uint128 bsid_num;
						if(session_context->get_bound_sid(bsid_num)) {
							bound_sessionid *bsid = new bound_sessionid(bsid_num);
							query->set_bound_sid(bsid);
						}

						// QSPEC
						query->set_qspec(qspec_obj);


						pdu = dynamic_cast<known_nslp_pdu *>(query);
					}
					else {
						ERRLog(state_manager::modname, "AHO not implemented for case where mobile node is QNE of a reservation!");
						return;
					}
					
					assert(pdu != NULL);
					
					
					// build mri
					bool down;
					if(is_flow_sender) {
						new_pc_mri.set_downstream(true);
						down = true;
					}
					else if(is_flow_receiver) {
						new_pc_mri.set_downstream(false);
						down = false;
					}
					
					
					// send message with PC-MRM
					SignalingMsg* sigmsg = new SignalingMsg();
					sigmsg->set_msg(pdu);
					sigmsg->set_sid(sid);
					sigmsg->set_sig_mri(new_pc_mri.copy());
					sigmsg->set_downstream(down);
					sigmsg->send_or_delete();
				} // end if (anticipated handover || is_flow_sender)
				else {
					// hard handover with (is_flow_receiver == true)

					// create NOTIFY message
					notifymsg* notify = new notifymsg();
					known_nslp_pdu *notify_pdu = NULL;
	
					if(qn_type == NSLP_Session_Context::QNI) {
						info_spec* err_obj = new info_spec(NULL, info_spec::information, info_spec::InitQuery, 0);
						notify->set_errorobject(err_obj);
						notify->set_qspec(qspec_obj);

						// set X-Flag, which causes the subsequent RESERVE to have the Replace-Flag set
						notify->set_x_flag();
			
						notify_pdu = dynamic_cast<known_nslp_pdu*>(notify);
					}
					else if(qn_type == NSLP_Session_Context::QNR) {
						info_spec* err_obj = new info_spec(NULL, info_spec::information, info_spec::InitReserve, 0);
						notify->set_errorobject(err_obj);
						notify->set_qspec(qspec_obj);

						// set X-Flag, which causes the subsequent RESERVE to have the Replace-Flag set
						notify->set_x_flag();
			
						notify_pdu = dynamic_cast<known_nslp_pdu*>(notify);
					}
					else {
						ERRLog(state_manager::modname, "AHO not implemented for case where mobile node is QNE of a reservation!");
						return;
					}
	
					assert(notify_pdu != NULL);
	

					// build mri, note that we are the flow receiver
					new_pc_mri.set_downstream(false);
		
					// send NOTIFY message with EST-MRM
					hostaddress orig_sig_addr = *l3_mn;
					hostaddress dest_sig_addr = new_pc_mri.get_sourceaddress();
					mri_explicitsigtarget *est_mri = new mri_explicitsigtarget(new_pc_mri, orig_sig_addr, dest_sig_addr);
		
					ExplicitSignalingMsg* sigmsg = new ExplicitSignalingMsg();
					sigmsg->set_pdu(notify_pdu);
					sigmsg->set_sid(sid);
					sigmsg->set_sig_mri(est_mri);
					sigmsg->send_or_delete();
				} // end else (hard handover)
			}                
		} // end while           
		session_context->unlock();

		// delete AHO_Context
		DLog(state_manager::modname, "Erasing NSLP_AHO_Context.");
		aho_contextmap->erase(sid);
	} // end if session_context != NULL
                                         
	ILog(state_manager::modname, "aho_phase_3_2a() - done");
}                                        


/* This function looks in the NSLP_AHO_Context if the current node is the new
 * access router for the given session. If then the flow in the context matches
 * also the given flow, TRUE is returned together with an EST-MRI which allows
 * to forward messages towards the mobile node.
 * @param sid the session id used to search the corresponding NSLP_AHO_Context.
 * @param rcvd_mri the MRI of the received message which possibly should be forwarded.
 * @param fwd_mri if TRUE is returned, this references a valid EST-MRI which is used to forward messages towards the mobile node.
 */
bool state_manager::is_new_access_router_and_flow_matches(const ntlp::sessionid &sid, const ntlp::mri_pathcoupled &rcvd_mri,
		ntlp::mri_explicitsigtarget *fwd_mri)
{
    AhoContextMap aho_contextmap = Context_Manager::instance()->get_acm();

	DLog(state_manager::modname, "is_new_access_router_and_flow_matches()");

	bool ret=false;

	ntlp::mri_pathcoupled pc_mri = rcvd_mri;
	// invert direction for comparison
	pc_mri.invertDirection();

	// determine node role within AHO
	NSLP_AHO_Context::node_role_t role = NSLP_AHO_Context::OTHER;
	NSLP_AHO_Context *aho_context = aho_contextmap->find(sid);
	if(aho_context != NULL) {
		aho_context->lock();
		role = aho_context->get_aho_node_role();
		if(role == NSLP_AHO_Context::ARN) {
			const hostaddress ar_n = aho_context->get_addr_new_access_router();
			const hostaddress mn = aho_context->get_addr_mobile_node();

			if (aho_context->is_flow_in_context(pc_mri)) {
				if (fwd_mri != NULL) {
					// fill EST-MRI
					fwd_mri->set_origin_sig_address(ar_n);
					fwd_mri->set_dest_sig_address(mn);
					fwd_mri->set_mri_pc(rcvd_mri);
				}

				ret = true;
			}
		}
		aho_context->unlock();
	}

	if (ret == true) {
		DLog(state_manager::modname, "is_new_access_router_and_flow_matches() - I am access router and the flow matches.");
	}
	else {
		DLog(state_manager::modname, "is_new_access_router_and_flow_matches() - I am not access router or the flow doesn't match.");
	}

	DLog(state_manager::modname, "END is_new_access_router_and_flow_matches()");

	return ret;
}

} // endif namespace qos_nslp
#endif // USE_AHO
