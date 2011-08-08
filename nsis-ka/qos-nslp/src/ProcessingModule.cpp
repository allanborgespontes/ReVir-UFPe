/// ----------------------------------------*- mode: C++; -*--
/// @file ProcessingModule.cpp
/// QoS NSLP Processing Module
/// ----------------------------------------------------------
/// $Id: ProcessingModule.cpp 6287 2011-06-18 04:37:17Z bless $
/// $HeadURL: https://svn.tm.kit.edu/nsis/qos-nslp/trunk/src/ProcessingModule.cpp $
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
/** @file
 * This Processing Module allows to send and receive PDUs.
 * It uses the NTLP for transport and the NSLP state machine.
 */

#include <unistd.h>
#include <iostream>
#include <string>
#include <sstream>

// for setting the epoch id with gettimeofday
#include "sys/time.h"

// from protlib
#include "ie.h"
#include "queuemanager.h"

// from NTLP
#ifdef USE_FLOWINFO
#include "flowinfo.h"
#endif

#ifdef USE_AHO
#include "nwn_uds_msg.h"
#endif

// QoS NSLP
#include "all_nslp_ies.h"
#include "info_spec.h"
#include "ProcessingModule.h"
#include "nslp_session_context_map.h"
#include "context_manager.h"

#include "qos_nslp_conf.h"

#include "nslpdata.h"

#include "benchmark_journal.h"


using namespace qos_nslp;
using namespace protlib::log;
using namespace protlib;
using namespace ntlp;

namespace qos_nslp   {

extern benchmark_journal journal;

/** @addtogroup ProcessingModule Processing Module
 * @{
 */


#ifndef NSIS_OMNETPP_SIM
// TEST
pthread_mutex_t session_mutex;
pthread_mutexattr_t session_mutex_attr;
// END TEST
#endif

/***** struct ProcessingModuleParam *****/

  ProcessingModuleParam::ProcessingModuleParam(uint32 sleep_time,
			       AddressList &addresses,
			       Flowinfo &fi_service,
			       bool see, bool sre
			       )
  : ThreadParam(sleep_time,"QoSProcessing"),
    source(message::qaddr_qos_nslp_signaling),
    addresses(addresses),
    fi_service(fi_service),
    send_error_expedited(see),
    send_reply_expedited(sre)
{
	// we have to set the epoch id at system start time
	struct timeval timenow;

	gettimeofday(&timenow, NULL);
	rsn::local_epoch_id= timenow.tv_sec;
} // end constructor

/***** class ProcessingModule *****/

const char* const ProcessingModule::errstr[] = {
	"success",
	"Message sent to peer.",
	"No address or wrong address."
}; // end errstr

const char* ProcessingModule::get_error_string(error_t e) { return errstr[e]; }

/** 
 * Initialize Thread base class and NSLP protocol. 
 */
ProcessingModule::ProcessingModule(const ProcessingModuleParam& p)
  : Thread(p),
    param(p)
{
  // register queue
	QueueManager::instance()->register_queue(get_fqueue(),p.source);
	DLog(param.name, "Created ProcessingModule object");
	Context_Manager::instance()->add_context_map();
	session_context_map = Context_Manager::instance()->get_scm();
	statemodule = new state_manager(param.addresses, param.fi_service);
	// TEST
	pthread_mutexattr_init(&session_mutex_attr);
#ifdef _DEBUG
	pthread_mutexattr_settype(&session_mutex_attr, PTHREAD_MUTEX_ERRORCHECK);
#else
	pthread_mutexattr_settype(&session_mutex_attr, PTHREAD_MUTEX_NORMAL);
#endif
	pthread_mutex_init(&session_mutex, &session_mutex_attr);
	// END TEST
} // end constructor

/** 
 * Destructor for ProcessingModule.  
 */
ProcessingModule::~ProcessingModule() 
{
	DLog(param.name, "Destroying ProcessingModule object");
	QueueManager::instance()->unregister_queue(param.source);
	delete statemodule;
	// TEST
	pthread_mutex_destroy(&session_mutex);
	pthread_mutexattr_destroy(&session_mutex_attr);
	// END TEST
} // end destructor

/** 
 * This function is the main loop of the current thread. 
 * Will be called until this thread will be stopped.  
 * @param nr the id of the current thread.
 */
void ProcessingModule::main_loop(uint32 nr) 
{
	ILog(param.name, "Starting thread #" << nr);

	switch (nr) { 
	case 1:
		process_queue();
		break;
	case 2:
		rcv_appl_messages();
		break;
	case 3:
		rcv_gist_messages();
		break;
	case 4:
		process_elapsed_timers();
		break;
	default: 
		break;
	} // end switch for threads

	ILog(param.name, "Thread #" << nr << " stopped");
} // end main_loop


/**
 * process queue of internal messages for the state module
 * (usually message::qaddr_qos_nslp_signaling).
 */
void
ProcessingModule::process_queue()
{ 
	message* msg = NULL;
	SignalingMsg* sigmsg = NULL;
	ExplicitSignalingMsg* explsigmsg = NULL;

	FastQueue* fq = QueueManager::instance()->get_queue(param.source);
	if (!fq) {
		Log(ERROR_LOG,LOG_ALERT, param.name, "Cannot find input msg queue");
		return;
	}

	// max. waiting time at internal msg queue
	const uint32 wait = param.sleep_time * 1000;
	// wait for messages if in state running
	while (get_state() == STATE_RUN) {
		// dequeue message from internal message queue
		msg = fq->dequeue_timedwait(wait);
		if (!msg)
			continue;

		switch (msg->get_type()) {
		case message::type_signaling:
			sigmsg = dynamic_cast<SignalingMsg*>(msg);
			if (sigmsg)
				// outgoing signaling message, sent to GIST
				process_sig_msg(sigmsg);
			else {
				ERRLog(param.name, "Cannot cast message from source " << 
						msg->get_qaddr_name() << " of type " << 
						msg->get_type_name() << " to SignalingMsg");
				delete msg;
			}
			break;
		case message::type_explicit_signaling:
			explsigmsg = dynamic_cast<ExplicitSignalingMsg*>(msg);
			if (explsigmsg) {
				// outgoing explicit signaling message, sent to GIST
				process_est_sig_msg(explsigmsg);
			}
			else {
				ERRLog(param.name, "Cannot cast message from source " << 
						msg->get_qaddr_name() << " of type " << 
						msg->get_type_name() << " to ExplicitSignalingMsg");
				delete msg;
			}
			break;
		default:
			ERRLog(param.name, "Received a message from " << msg->get_qaddr_name() << 
					" of type " << msg->get_type_name() << 
					" that cannot be processed here, TYPE: " << msg->get_type());
			delete msg;
		} // end switch
	} // end while running

	// signal other threads for faster termination
	FastQueue* tmpfq = QueueManager::instance()->get_queue(message::qaddr_appl_qos_signaling);
	tmpfq->signal_queue();
	tmpfq = QueueManager::instance()->get_queue(message::qaddr_api_1);
	tmpfq->signal_queue();
	tmpfq = QueueManager::instance()->get_queue(message::qaddr_qos_nslp_timerprocessing);
	tmpfq->signal_queue();

} // end process_queue


/** This function processes internal message from Signaling Application.
 * @param applmsg received internal message from Signaling Application.
 */
void
ProcessingModule::process_appl_msg(QoS_Appl_Msg* applmsg)
{
	hostaddress source_addr;
	hostaddress dest_addr;
	uint8 source_prefix, dest_prefix;
	source_prefix = dest_prefix = 0;
	uint16 source_port, dest_port;
	source_port = dest_port = 0;

	source_addr = applmsg->get_source_addr();
	dest_addr = applmsg->get_dest_addr();
	bool downstream = applmsg->get_direction();
	ntlp::mri_pathcoupled *nslp_mri = new ntlp::mri_pathcoupled(source_addr, source_prefix, source_port, dest_addr, dest_prefix, dest_port, "udp", 0, 0, 0, downstream);

	SignalingMsg* sigmsg = new SignalingMsg;

	sigmsg->set_downstream(downstream);
	uint128 sid_number;
	applmsg->get_sid(sid_number);
	sigmsg->set_sid(sid_number);
	sigmsg->set_sig_mri(nslp_mri);

	known_nslp_pdu* pdu = NULL;
	qos_nslp::known_nslp_pdu::type_t type = applmsg->get_pdu_type();
	switch (type) {
	case qos_nslp::known_nslp_pdu::RESERVE: 
		pdu = new reservereq; 
		break;
	case qos_nslp::known_nslp_pdu::QUERY: 
		pdu = new querymsg; 
		break;
	case 0: 
		pdu = NULL; 
		break;
	default: 
		DEBUG_LEAVE(param.name, "unknown pdu type");
		return;
	}

	if (type == qos_nslp::known_nslp_pdu::RESERVE) {
		ILog(param.name, "Sending message of type RESERVE");
		reservereq* new_res = new reservereq();

		// only send a packet classifier if a QSPEC is present
		const qspec::qspec_pdu* q_pdu = applmsg->get_qspec();
		if (q_pdu) {
			// FIXME: why is this only initialized with a default value?
			packet_classifier* new_pc = new packet_classifier();
			new_pc->set_spi_flag();
			new_res->set_packet_classifier(new_pc);
		}

		if (applmsg->get_request_response_with_rii()) {
			rii* new_rii = new rii();
			new_rii->generate_new();
			new_rii->set_own(true);
			new_res->set_rii(new_rii);
		}

		qos_nslp::qspec_object* new_qspec = new qos_nslp::qspec_object(q_pdu);
		new_res->set_qspec(new_qspec);

		// this is optional and experimental
		if (applmsg->get_vlsp_object())
		{
		  MP(benchmark_journal::PRE_VLSP_SETUP);

			DLog(param.name,"VLSP object found - added to RESERVE message");
			new_res->set_vlsp_object(applmsg->get_vlsp_object()->copy());
		}

		// this is a session authorization object of RFC 5981
		if (applmsg->get_session_auth_object())
		{
			DLog(param.name,"Session Authorization object found - added to RESERVE message");
			new_res->set_sessionauth(applmsg->get_session_auth_object()->copy());
		}

		if (applmsg->get_session_auth_object_hmac_signed())
		{
			DLog(param.name,"Session Authorization object HMAC_SIGNED found - added to RESERVE message");
			new_res->set_sessionauth_hmac_signed(applmsg->get_session_auth_object_hmac_signed()->copy());
		}

		new_res->set_originator(true);

		if (applmsg->get_replace_flag()) 
			new_res->set_replace_flag();
		if (applmsg->get_acknowledge_flag())
			new_res->set_acknowledge_flag();
		if (applmsg->get_scoping_flag())
			new_res->set_scoping_flag();
		if (applmsg->get_tear_flag())
			new_res->set_tear_flag();

		pdu = dynamic_cast<known_nslp_pdu*>(new_res);

		sigmsg->set_msg(pdu);

		sigmsg->set_reliable(applmsg->get_reliable());
		sigmsg->set_secure(applmsg->get_secure());

		//DLog(param.name,"transfer attribute " << (sigmsg->get_reliable() ? "reliable" : "unreliable") << " requested");
	}

	if (type == qos_nslp::known_nslp_pdu::QUERY) {
		ILog(param.name, "Sending message of type QUERY");
		querymsg* new_query = new querymsg();

		packet_classifier* new_pc = new packet_classifier();
		new_pc->set_spi_flag();
		new_query->set_packet_classifier(new_pc);
		const qspec::qspec_pdu* q_pdu = applmsg->get_qspec();
		qos_nslp::qspec_object* new_qspec = new qos_nslp::qspec_object(q_pdu);
		new_query->set_qspec(new_qspec);
		new_query->set_originator(true);
		new_query->set_x_flag();

		if (applmsg->get_request_response_with_rii()) {
			rii* new_rii = new rii();
			new_rii->generate_new();
			new_rii->set_own(true);
			new_query->set_rii(new_rii);
		}

		if (applmsg->get_scoping_flag())
			new_query->set_scoping_flag();
		if (applmsg->get_reserve_init_flag())
			new_query->set_reserve_init_flag();

		pdu = dynamic_cast<known_nslp_pdu*>(new_query);
		sigmsg->set_msg(pdu);

		if (applmsg->get_reliable())
			sigmsg->set_reliable(true);
		if (applmsg->get_secure())
			sigmsg->set_secure(true);

		// this is a session authorization object of RFC 5981
		if (applmsg->get_session_auth_object())
		{
			DLog(param.name,"Session Authorization object found - added to QUERY message");
			new_query->set_sessionauth(applmsg->get_session_auth_object()->copy());
		}

	}

	sigmsg->send_or_delete();
}


/** 
 * process incoming message from NTLP module
 * (incoming signaling message from NTLP).
 * @param apimsg Internal ApiMsg from NTLP module
 */
void ProcessingModule::process_gist_api_msg(ntlp::APIMsg* apimsg)
{
	state_manager::error_t nslperror;
	known_nslp_pdu* result_pdu = NULL;
	state_manager statemodule(param.addresses, param.fi_service);
	APIMsg::subtype_t apimsg_subtype;

	assert(apimsg != NULL);
	DLog(param.name, "process_gist_api_msg() - received message from NTLP!");


	ntlp::mri* r_mri = apimsg->get_mri();
	ntlp::mri_pathcoupled* rcvd_mri = r_mri ? dynamic_cast<ntlp::mri_pathcoupled*>(r_mri) : NULL;
	ntlp::nslpdata* nslp_data = apimsg->get_data();
	uint32 sii = apimsg->get_sii_handle();
	ntlp::sessionid* rcvd_sid = apimsg->get_sessionid();
	apimsg_subtype = apimsg->get_subtype();

	if (rcvd_sid == NULL) {
		if (apimsg_subtype == APIMsg::NetworkNotification) {
#ifdef USE_AHO
			APIMsg::error_t nf_type = apimsg->get_msgstatus();
			if ((nf_type != APIMsg::anticipated_handover_event) &&
			    (nf_type != APIMsg::handover_event)) {
#endif
				if(rcvd_mri == NULL) {
					ERRLog(param.name, "process_gist_api_msg() - rcvd_mri == NULL");
					return;
				}
#ifdef USE_FLOWINFO
				DLog(param.name, "Processing mobility event");
				statemodule.process_mobility(rcvd_mri, nf_type);
#endif

#ifdef USE_AHO
			}
			else {
				// nf_type == APIMsg::anticipated_handover_event ||
				// nf_type == APIMsg::handover_event
				DLog(param.name, "Processing handover event.");

				if (nf_type == APIMsg::anticipated_handover_event) {
					const struct aho_msg_body *aho_message = (struct aho_msg_body *)nslp_data->get_buffer();
					macaddress l2_ap_n;
					hostaddress l3_ar_n;
					hostaddress l3_mn;

					l2_ap_n.set_mac(aho_message->l2_ap_n);

					if (aho_message->l3_af == AF_INET) {
						l3_ar_n.set_ip(aho_message->l3_ar_n_ipv4);
						l3_mn.set_ip(aho_message->l3_mn_ipv4);
					}
					else if (aho_message->l3_af == AF_INET6) {
						l3_ar_n.set_ip(aho_message->l3_ar_n_ipv6);
						l3_mn.set_ip(aho_message->l3_mn_ipv6);
					}

					DLog(param.name, "AHO-iface: " << aho_message->iface);
					DLog(param.name, "AHO-l2_ap_n: " << l2_ap_n);
					DLog(param.name, "AHO-l3_ar_n: " << l3_ar_n);
					DLog(param.name, "AHO-l3_mn: " << l3_mn);

					statemodule.process_aho_event(aho_message->iface, l2_ap_n, l3_ar_n, l3_mn);

				}
				else if (nf_type == APIMsg::handover_event) {
					const struct ho_msg_body *ho_message = (struct ho_msg_body *)nslp_data->get_buffer();
					macaddress l2_ap_n;
					hostaddress l3_ar_n;
					hostaddress l3_mn;

					l2_ap_n.set_mac(ho_message->l2_ap_n);

					if (ho_message->l3_af == AF_INET) {
						l3_ar_n.set_ip(ho_message->l3_ar_n_ipv4);
						l3_mn.set_ip(ho_message->l3_mn_ipv4);
					}
					else if (ho_message->l3_af == AF_INET6) {
						l3_ar_n.set_ip(ho_message->l3_ar_n_ipv6);
						l3_mn.set_ip(ho_message->l3_mn_ipv6);
					}

					DLog(param.name, "AHO-iface: " << ho_message->iface);
					DLog(param.name, "AHO-l2_ap_n: " << l2_ap_n);
					DLog(param.name, "AHO-l3_ar_n: " << l3_ar_n);
					DLog(param.name, "AHO-l3_mn: " << l3_mn);

					statemodule.process_ho_event(ho_message->iface, l2_ap_n, l3_ar_n, l3_mn);
				}

				// nslp_data will be deleted also
				delete apimsg;
				DLog(param.name,"process_gist_api_msg() - done.");
			} // end anticipated_handover_event || handover_event
#endif
		} // end NetworkNotification
		else {
			DLog(param.name,"SID is NULL");
		}

		return;
	}


	if(r_mri == NULL) {
		ERRLog(param.name, "process_gist_api_msg() - r_mri == NULL");
		return;
	}

	// assertion: rcvd_sid != NULL
	if(r_mri->get_mrm() == mri::mri_t_pathcoupled) {
		if (nslp_data == NULL) {
			pthread_mutex_lock(&session_mutex);
			DLog(param.name, "SII handle is " << sii);
			DLog("ProcessingModule", "NN status is: " << apimsg->get_msgstatus());
			if (apimsg_subtype == APIMsg::NetworkNotification) {
				bool down = rcvd_mri->get_downstream();
				statemodule.process_sii_handle(rcvd_sid, rcvd_mri, down, sii, apimsg->get_msgstatus());
			}
			pthread_mutex_unlock(&session_mutex);
		}
		else {
			uchar* nslp_buf = nslp_data->get_buffer();
			uint32 nslp_size = nslp_data->get_size();
			DLog(param.name, "Size of nslp_data is " << nslp_size);

			NetMsg* netmsg= new NetMsg(nslp_size + 4);
			netmsg->encode32(nslp_size);
			netmsg->encode(nslp_buf, nslp_size);
			netmsg->to_start();
	
			ntlp::sessionid* my_sid = new ntlp::sessionid(*rcvd_sid);
	      
	
			pthread_mutex_lock(&session_mutex);
			DLog(param.name, "process_gist_api_msg() - received PDU now parsing...");
	
			bool down = rcvd_mri->get_downstream();

			// process NetMsg by NSLP (generates C++ objects from byte stream)
			nslperror = statemodule.process_tp_recv_msg(*netmsg, result_pdu, down, my_sid, rcvd_mri, sii);
			if (nslperror != state_manager::error_old_pdu) {
				if (!result_pdu->is_reserve()) {
					// if the direction of the MRI is downstream, the message came from the upstream peer
					statemodule.process_sii_handle(my_sid, rcvd_mri, !down, sii, APIMsg::error_msg_delivered);
				}
			}
			DLog(param.name, "process_gist_api_msg() - parsed PDU");

			// now result_pdu is returned, check for result/error during decoding
			switch (nslperror) {
			case state_manager::error_old_pdu:
			case state_manager::error_rsn_missing:
			case state_manager::error_no_bandwidth:
				  DLog(param.name, "process_gist_api_msg() - process_tp_recv_msg() returned an error: " << state_manager::error_string(nslperror));
			case state_manager::error_ok:
			case state_manager::error_nothing_to_do:
			case state_manager::error_ok_forward:
				{
				  if (result_pdu) 
				    delete result_pdu;
				}
				break;

			default:
				{
					ERRLog(param.name, "Received an NSLP error (#" 
					       << nslperror << ':' << state_manager::error_string(nslperror) << ") that cannot be handled. Deleting NetMsg");
					if (result_pdu) 
						delete result_pdu;
				}
				break;
			} // end switch nslperror

			// NetMsg is no longer needed
			if (netmsg) {
				delete netmsg;
			}
	
			// because pointers are cleared, objects peer and netmsg
			// are not deleted by calling tpmsg destructor now
			delete apimsg;
			DLog(param.name,"process_gist_api_msg() - done.");
			pthread_mutex_unlock(&session_mutex);
		} // end if nslp_data in APIMsg existing
	}
	else if(r_mri->get_mrm() == mri::mri_t_explicitsigtarget) {
		if (nslp_data != NULL) {
			uchar* nslp_buf = nslp_data->get_buffer();
			uint32 nslp_size = nslp_data->get_size();
			DLog(param.name, "Size of nslp_data is " << nslp_size);

			NetMsg* netmsg= new NetMsg(nslp_size + 4);
			netmsg->encode32(nslp_size);
			netmsg->encode(nslp_buf, nslp_size);
			netmsg->to_start();
	
			ntlp::sessionid* my_sid = new ntlp::sessionid(*rcvd_sid);
	      
	
			pthread_mutex_lock(&session_mutex);
			DLog(param.name, "process_gist_api_msg() - received PDU now parsing...");
	

			ntlp::mri_explicitsigtarget* rcvd_est_mri = dynamic_cast<ntlp::mri_explicitsigtarget*>(r_mri);
			if(rcvd_est_mri != NULL) {
				statemodule.process_tp_recv_est_msg(*netmsg, my_sid, rcvd_est_mri);
			}
			else {
				ERRLog(param.name, "Cannot cast MRI of type ntlp::mri to ntlp::mri_explicitsigtarget");
			}
	      
			// NetMsg is no longer needed
			if (netmsg) {
				delete netmsg;
			}
	
			// because pointers are cleared, objects peer and netmsg
			// are not deleted by calling tpmsg destructor now
			delete apimsg;
			DLog(param.name,"process_gist_api_msg() - done.");
			pthread_mutex_unlock(&session_mutex);
		} // end if nslp_data in APIMsg existing
	}
	else {
		ERRLog(param.name, "Received message with unknown MRI - MRI-ID: " << r_mri->get_mrm());
	}
} // end process_api_msg



/**
 * process outgoing message that should be transferred by GIST
 * (outgoing signaling message, sent subsequently to TP module).
 */
void
ProcessingModule::process_sig_msg(SignalingMsg* sigmsg)
{
  state_manager::error_t nslperror = state_manager::error_ok;
  known_nslp_pdu* pdu = NULL;
  NetMsg* netmsg = NULL;
  uchar* buffer;

  assert(sigmsg!=NULL);

  sessionid* process_sid = new sessionid( sigmsg->get_sid() );
  pthread_mutex_lock(&session_mutex);

  if (sigmsg->get_pdu())
  {
    // extract data from sigmsg
    pdu = sigmsg->get_pdu();
    // add basic header if necessary
    // send it to NSLP
    bool d = sigmsg->get_downstream();
    ntlp::mri_pathcoupled* sig_mri = sigmsg->get_sig_mri()->copy();
    nslperror = statemodule->process_outgoing(*pdu,netmsg, d, process_sid, &sig_mri);
    if (sigmsg->get_allow_translate())
      sigmsg->set_sig_mri(sig_mri);
    
    if (nslperror!=state_manager::error_ok)
    {
      delete sigmsg;
    }
    else
    {
      // send the returned NetMsg
      assert(netmsg!=NULL);
      netmsg->to_start();
      buffer = netmsg->get_buffer();

      uint128 sid;
      sigmsg->get_sid(sid);
      ntlp::sessionid* ntlp_sid = new ntlp::sessionid(sid);

      uint32 sii_handle = 0;
      sii_handle = sigmsg->get_sii_handle();
      
      if (d != sig_mri->get_downstream()) {
	      DLog(param.name, "Message should be sent " << (d?"downstream":"upstream") << ", Inverting MRI");
	      sig_mri->invertDirection();
      }

	uint32 nslpmsghandle = 0;
	tx_attr_t tx_attr;
	tx_attr.reliable = sigmsg->get_reliable();
	tx_attr.secure = sigmsg->get_secure();

	//DLog(param.name,"Message transfer attributes: reliable=" << tx_attr.reliable << ", secure=" << tx_attr.secure);
	
	nslperror = SendMessage(buffer, netmsg->get_size(), ntlp_sid,
			sig_mri, sii_handle, nslpmsghandle, tx_attr);
	delete netmsg;
    }
  }
  if (sigmsg->get_invalidate()) {
    bool downstream = sigmsg->get_downstream();
    ntlp::mri_pathcoupled* nslp_mri = NULL;
    nslp_mri = sigmsg->get_sig_mri()->copy(); 
    if (downstream != nslp_mri->get_downstream()) {
      DLog("ProcessingModule", "Message should be sent " << (downstream?"downstream":"upstream") << ", Inverting MRI");
      nslp_mri->invertDirection();
    }
    ntlp::APIMsg *apimsg = new ntlp::APIMsg();
    apimsg->set_source(message::qaddr_api_1);
    apimsg->set_invalidateroutingstate(1, nslp_mri, APIMsg::bad, false);
    apimsg->send_to(message::qaddr_coordination);
  }
  pthread_mutex_unlock(&session_mutex);

} // end process_sig_msg


/**
 * process outgoing EST message that should be transferred by GIST
 * (outgoing explicit signaling message, sent subsequently to TP module).
 */
void
ProcessingModule::process_est_sig_msg(ExplicitSignalingMsg* sigmsg)
{
	known_nslp_pdu* pdu = NULL;
	NetMsg* netmsg = NULL;
	uchar* buffer;
	uint32 buffer_size;
	state_manager::error_t nslperror = state_manager::error_ok;


	assert(sigmsg!=NULL);

	pdu = sigmsg->get_pdu();
	if(pdu) {
		sessionid* sid = new sessionid(sigmsg->get_sid());
		pthread_mutex_lock(&session_mutex);

		ntlp::mri_explicitsigtarget* sig_mri = sigmsg->get_sig_mri()->copy();

		uint32 sii_handle = 0;
		sii_handle = sigmsg->get_sii_handle();


		nslperror = statemodule->process_outgoing_est_msg(*pdu, netmsg, sid, &sig_mri);
		if (nslperror!=state_manager::error_ok) {
			delete sigmsg;

			ERRLog(param.name, "process_outgoing_est_msg returned error: " << state_manager::error_string(nslperror));
		}
		else {
			assert(netmsg!=NULL);

			netmsg->to_start();
			buffer = netmsg->get_buffer();

			buffer_size = netmsg->get_size();
		
			SendMessage(buffer, buffer_size, sid, sig_mri, sii_handle);
		}
		pthread_mutex_unlock(&session_mutex);
	}
} // end process_est_sig_msg



/***** class SignalingMsg *****/

/** Default constructor for SignalingMsg. */
SignalingMsg::SignalingMsg()
	: message(type_signaling), pdu(NULL), sig_mri(NULL), sii_handle(0), down(true), 
	  translate(true), invalidate(false), reliable(false), secure(false)
{
} 

/** Destructor for SignalingMsg. */
SignalingMsg::~SignalingMsg() {
	delete pdu;
	delete sig_mri;
} // end destructor

/** This function sets the MRI of the current SignalingMsg.
  * @param set_mri the MRI of the current SignalingMsg.
  */
void SignalingMsg::set_sig_mri(const ntlp::mri_pathcoupled*  set_mri)
{
  delete sig_mri;
  sig_mri = set_mri->copy();
}

/* Set signaling message.
 */

void SignalingMsg::set_msg(known_nslp_pdu* p) {
	set_source(qaddr_qos_nslp_signaling);
	pdu = p;
} // end set_msg


/** Clear all pointers. */
void SignalingMsg::clear_pointers() {
	pdu = NULL;
	sig_mri = NULL;
} // end clear_pointers


/** This function send the current SignalingMsg to the appropriate queue.  */
bool SignalingMsg::send(bool exp) 
{
	qaddr_t s = get_source();

	// if source was not signaling send it to signaling
	// Sanity check: normally this wont be true since SignalingMsg::set_msg ~~> set_source(qaddr_qos_nslp_coordination)
	// but if set_msg() is not called due to some reason, we may enter here
	if (s != qaddr_qos_nslp_signaling) {
		if (s != qaddr_unknown) {
			DLog("SIGNALING", "Sending to qaddr_qos_nslp_signaling");
			return send_to(qaddr_qos_nslp_signaling, exp);
		} else {
			ERRLog("SIGNALING", "Cannot send internal message - undefined qaddr as source");
			return false;
		}
	}

	DLog("SIGNALING", "Sending to qaddr_qos_nslp_signaling, i.e., to myself");
	return send_to(qaddr_qos_nslp_signaling,exp);
} // end send


/** This function sets the direction of the flow.
 * @param d the direction of the flow.
 */ 
void SignalingMsg::set_downstream(bool d) {
	down = d;
} // end set_downstream


// TODO:
// * Merge classes SignalingMsg and ExplicitSignalingMsg
// * Remove type_explicit_signaling

/***** class ExplicitSignalingMsg *****/

/** Default constructor for ExplicitSignalingMsg. */
ExplicitSignalingMsg::ExplicitSignalingMsg()
  : message(type_explicit_signaling), pdu(NULL), sig_mri(NULL), sii_handle(0)
{
} 

/** Destructor for ExplicitSignalingMsg. */
ExplicitSignalingMsg::~ExplicitSignalingMsg() {
	if (pdu) delete pdu;
	if (sig_mri) delete sig_mri;
} // end destructor

/** This function sets the EST-MRI of the current ExplicitSignalingMsg.
  * @param set_mri the EST-MRI of the current ExplicitSignalingMsg.
  */
void ExplicitSignalingMsg::set_sig_mri(const ntlp::mri_explicitsigtarget* set_mri)
{
  delete sig_mri;
  sig_mri = set_mri->copy();
}

/* Set signaling message.
 */

void ExplicitSignalingMsg::set_pdu(known_nslp_pdu* p) {
	set_source(qaddr_qos_nslp_signaling);
	pdu = p;
} // end set_msg


/** Clear all pointers. */
void ExplicitSignalingMsg::clear_pointers() {
	pdu = NULL;
	sig_mri = NULL;
} // end clear_pointers

/** This function sends the current ExplicitSignalingMsg to the appropriate queue.  */
bool ExplicitSignalingMsg::send(bool exp) 
{
	qaddr_t s = get_source();

	if (s != qaddr_unknown) {
		DLog("EXPL-SIGNALING", "Sending to qaddr_qos_nslp_signaling");
		return send_to(qaddr_qos_nslp_signaling, exp);
	} else {
		ERRLog("EXPL-SIGNALING", "Unknown qaddr");
		return false;
	}
} // end send


bool done = false;
bool running = false;

/** This thread checks for internal messages from a signaling application on the queue.  */
void ProcessingModule::rcv_appl_messages() 
{
	message* msg = NULL;
	QoS_Appl_Msg* applmsg = NULL;

	DEBUG_ENTER(param.name, "started");
	FastQueue* fq = QueueManager::instance()->get_queue(message::qaddr_appl_qos_signaling);

	int retries = 3;
	while ((fq == NULL) && retries > 0) {
		DLog("NSLP-appmsgrecv", "waiting for qaddr_appl_qos_signaling message queue");

		// wait for other thread to fire up
		sleep(1);
		retries--;
	} // end retry loop

	if (retries == 0) {
		ERRLog("NSLP-appmsgrecv", "rcv_appl_messages() cannot attach to qaddr_appl_qos_signaling message queue");
		DEBUG_LEAVE(param.name, "cannot attach to message queue");
		return;
	}

	running = true;

	// max. waiting time at internal msg queue
	const uint32 wait = param.sleep_time * 1000;

	EVLog("NSLP-appmsgrecv", "rcv_appl_messages() started");
	while (!done && (get_state() == STATE_RUN) ) {
		msg = fq->dequeue_timedwait(wait);
		if (!msg) 
			continue;

		EVLog("NSLP-appmsgrecv", "rcv_appl_messages() received a message");
		applmsg = dynamic_cast<QoS_Appl_Msg*>(msg);
		if (applmsg) {
			// evaluate appl message
			process_appl_msg(applmsg);
			applmsg = NULL;
		} else {
			ERRLog("NSLP-appmsgrecv", "cannot cast message to QoS_Appl_Msg");
		} // end if applmsg
		msg = NULL;
	} // end while not done
	fq->cleanup();
	DEBUG_LEAVE(param.name, "end of function");
	running = false;
} // end applmsgchecker



/** This thread checks for internal messages from NTLP on the queue.  */
void ProcessingModule::rcv_gist_messages() 
{
	message* msg = NULL;
	ntlp::APIMsg* apimsg = NULL;
	FastQueue* fq = QueueManager::instance()->get_queue(message::qaddr_api_1);
	if (!fq) {
		ERRLog("NSLP-GISTmsg receiver", "GISTmsg receiver cannot attach to qaddr_api_1 message queue");
		return;
	} // end if not fq
	running = true;

	ntlp::APIMsg* api_msg = new ntlp::APIMsg();
	api_msg->set_source(message::qaddr_api_1);
	api_msg->set_register(1, 0);

	// send the message
	ILog(param.name, "sending register to NTLP");
	api_msg->send_to(message::qaddr_coordination);

	// max. waiting time at internal msg queue
	const uint32 wait = param.sleep_time * 1000;

	EVLog("GISTmsg receiver", "GISTmsg receiver started");
	while ( !done && (get_state() == STATE_RUN) ) {
		msg = fq->dequeue_timedwait(wait);
		if (!msg)
			continue;

		EVLog("GISTmsg receiver", "GISTmsg receiver received a message");
		apimsg = dynamic_cast<ntlp::APIMsg*>(msg);
		if (apimsg) {
			// evaluate api message
			if (apimsg->get_subtype() == ntlp::APIMsg::RecvMessage) {
				// if first API message from GIST, we must decide on peering
				if (apimsg->get_adjacency_check()) {
					ntlp::APIMsg* mesg = new ntlp::APIMsg;
					mesg->set_source(message::qaddr_api_1);
					mesg->set_recvmessageanswer(apimsg->get_nslpid(),
							apimsg->get_sessionid()->copy(),
							apimsg->get_mri()->copy(),
							NULL,
							APIMsg::directive_establish);
					mesg->send_to(message::qaddr_coordination);
				}
			}

			// process incoming GIST API messages
			process_gist_api_msg(apimsg);
			apimsg = NULL;
		} else {
			ERRLog("GISTmsg receiver", "cannot cast message of type " << msg->get_type_name() << " to APIMsg");
		} // end if sigmsg

		//delete msg;
		msg = NULL;
	} // end while not done
	fq->cleanup();
	EVLog("GISTmsg receiver", "GISTmsg receiver stopped");
	running = false;
	return;
} // end GISTmsg receiver

/** This thread checks for internal timer messages on the queue.  */
void ProcessingModule::process_elapsed_timers() 
{
	message* msg = NULL;

	FastQueue* fq = QueueManager::instance()->get_queue(message::qaddr_qos_nslp_timerprocessing);
	
	if (!fq) {
		ERRLog("Timerchecker", "Timerchecker cannot find message queue");
		return;
	} // end if not fq

	// max. waiting time at internal msg queue
	const uint32 wait = param.sleep_time * 1000;

	EVLog("Timerchecker", "Timerchecker started");
	while ( !done && (get_state() == STATE_RUN) ) {
		msg = fq->dequeue_timedwait(wait);
		if (!msg)
			continue;

		EVLog("Timerchecker", "Received a message");
		if (msg->get_type() != message::type_timer)
			continue;

		process_timer_message(msg);

		// delete msg;
		msg = NULL;
	} // end while not done

	if (msg) 
		delete msg;

	fq->cleanup();
	EVLog("Timerchecker", "Timerchecker stopped");
} // end timerchecker


/** Function to send a message via NTLP.
 * @param nslp_data NSLP data in form of unsigned char.
 * @param nslp_data_size the size of NSLP data.
 */
state_manager::error_t 
ProcessingModule::SendMessage(uchar* nslp_data, uint32 nslp_data_size, sessionid* sid, mri* mr, uint32 sii_handle,
		msghandle_t nslpmsghandle, tx_attr_t tx_attr, uint32 timeout, uint16 ip_ttl, uint32 ghc)
{
	state_manager::error_t nslpres = state_manager::error_ok;

	uint32 size = nslp_data_size + sizeof(size);
	NetMsg* netmsg = NULL;

	if (size > NetMsg::max_size) {
		ERRLog(param.name, "state_manager::generate_pdu()."
			<< " Cannot allocate "	<< size << " bytes NetMsg."
			<< " PDU too big for NetMsg maxsize " << NetMsg::max_size);
		nslpres = state_manager::error_pdu_too_big;
	}
	else {
		// allocate a netmsg with the corresponding buffer size
		netmsg = new(nothrow) NetMsg(size);
		if (netmsg) {
			netmsg->encode32(size / 4);
			netmsg->encode(nslp_data, nslp_data_size);
			
			ntlp::nslpdata* data = new ntlp::nslpdata(nslp_data, nslp_data_size);
			const uint32 nslpid = 1;

			ntlp::APIMsg* msg = new ntlp::APIMsg();
			//DLog(param.name,"Message transfer attributes: reliable=" << tx_attr.reliable << ", secure=" << tx_attr.secure);

			msg->set_sendmessage(data, nslpmsghandle, nslpid, sid, mr, sii_handle, tx_attr, timeout, ip_ttl, ghc);

			// set sender
			msg->set_source(message::qaddr_api_1);

			// send the message
			DLog(param.name, "Sending message to GIST");
			msg->send_to(message::qaddr_coordination);
		}
		else {
			ERRLog(param.name, "state_manager::SendMessage() memory allocation failed for NetMsg");
			delete netmsg;
		}
	}

	return nslpres;
}

/** Used to process timer after it was received from timer queue  */
void ProcessingModule::process_timer_message(message* msg)
{
	TimerMsg* tmsg = NULL;
	TimerMsg* new_tmsg = NULL;
	sessionid* sid = NULL;
	rii* temp_rii = NULL;
	uint32 rii_number;
	timer_t time_to_live;
	state_manager statemodule(param.addresses, param.fi_service);
	NSLP_Session_Context_Map::rii_hashmap_it_t rii_hit;

	int32 sec = 0;
	int32 msec = 0;

	tmsg = dynamic_cast<TimerMsg*>(msg);

	void *param1;
	void *param2;
	void *param3;
	tmsg->get_params(param1, param2, param3);

	tmsg->get_time(sec, msec);
	DLog("Timerchecker", "Message: messageid " << tmsg->get_id() <<
			", action " << tmsg->get_action() << ", ok? " <<
			tmsg->is_ok() << ", absolute " <<
			tmsg->is_absolute() << ", sec " << sec <<
			", msec " << msec);

	sid = reinterpret_cast<sessionid *>(param1);
	if (!sid) {
		return;
	}

	DLog("Timerchecker", "SID in TIMER_MSG is - [" << sid->to_string() << "]");

	pthread_mutex_lock(&session_mutex);
	NSLP_Session_Context *session_context = session_context_map->find_session_context(*sid);
		if (session_context == NULL) {
		pthread_mutex_unlock(&session_mutex);
		return;
	}

	DLog(param.name, "Found context for SID [" << sid->to_string() << "].");

	ntlp::mri_pathcoupled *mri = reinterpret_cast<ntlp::mri_pathcoupled *>(param2);
	if (mri) {
		// MRI saved in TimerMsg
		// Depending on the existence of a PDU saved in the TimerMsg
		// we have to either retransmit / refresh a message or tear
		// down per flow based state.

		DLog("Timerchecker", "Found MRI in TimerMsg with ID " << tmsg->get_id() << ".");

		known_nslp_pdu *pdu = reinterpret_cast<known_nslp_pdu *>(param3);
		if (pdu) {
			// PDU saved in TimerMsg
			// Thus we have to either retransmit or refresh a message.

			reservereq* reserve = NULL;
			querymsg* query = NULL;

			// RB-REMARK: I don't understand why this necessarily means create tunnel reservation?
			if (pdu->is_notify())  {
				session_context->lock();
				NSLP_Flow_Context *flow_context = session_context->find_flow_context(*mri);
				session_context->unlock();

				if (flow_context != NULL) {
#ifdef USE_FLOWINFO
					statemodule.create_tunnel_reserve(session_context, flow_context);
#endif
				}

				msg = NULL;
				tmsg = NULL;
				pthread_mutex_unlock(&session_mutex);
				return;
			}

			if (pdu->is_query())  {
				DLog(param.name, "Param3 is QUERY");
				query = dynamic_cast<querymsg*>(pdu);
				if (query->get_bool_rii()) {
					ILog(param.name, "QUERY with RII");
					temp_rii = query->get_rii();
				}
				else {
					DLog(param.name, "QUERY without RII");
					temp_rii = NULL;
				}
			}

			if (pdu->is_reserve())   {
				DLog(param.name, "Param3 is RESERVE");
				reserve = dynamic_cast<reservereq*>(pdu);
				if (reserve->get_bool_rii()) {
					ILog(param.name, "RESERVE with RII");
					temp_rii = reserve->get_rii();
				}
				else {
					DLog(param.name, "RESERVE without RII");
					temp_rii = NULL;
				}
			}

			if (temp_rii) {
				temp_rii->get(rii_number);
				DLog(param.name, "Expired Timer with RII " << rii_number);

				NSLP_Session_Context_Map::riimap_t &rii_hashmap = session_context_map->get_rii_hashmap(*sid);

				if ((rii_hit = rii_hashmap.find(rii_number)) != rii_hashmap.end()) {
					DLog(param.name, "Found RII in hashmap");

					uint32 retry_counter;
					temp_rii->get_retry_counter(retry_counter);

					DLog(param.name, "Expired Timer: retry_counter is " << retry_counter);

					if (retry_counter < (qosnslpconf.getpar<uint32>(qosnslpconf_max_retry) - 1)) {
						if (query) {
							statemodule.retransmit_message_with_rii(sid, mri, session_context, query, NULL, temp_rii);
						}
						if (reserve) {
							statemodule.retransmit_message_with_rii(sid, mri, session_context, NULL, reserve, temp_rii);
						}
					}  // end if < MAX_RETRY_COUNTER
					else {
						session_context->lock();
						NSLP_Flow_Context *flow_context = session_context->find_flow_context(*mri);
						session_context->unlock();

						if (flow_context != NULL) {
							statemodule.delete_flow_context(sid, mri, session_context, flow_context);
						}

						statemodule.notify_application(QoS_Appl_Msg::type_response_for_rii_not_received, sid);

						DLog(param.name, "Exceeded MAX_RETRY_COUNT for RII - deleting context");

						rii_hashmap.erase(rii_number);

						DLog(param.name, "Expired Timer: deleting RII " << rii_number);

					}
				}  // end if RII found in HASHMAP
				else {
					ILog(param.name, "Expired Timer: RII " << rii_number << " not found");
				}
			}  // end if rii found
			else {
				session_context->lock();
				NSLP_Flow_Context *flow_context = session_context->find_flow_context(*mri);
				session_context->unlock();

				if (flow_context != NULL) {
					if (reserve && reserve->get_bool_rp()) {
						ILog(param.name, "Expired Timer: REFRESH_PERIOD.");

						// send refreshing reserve
						statemodule.send_refreshing_reserve(*sid, *mri, session_context, flow_context);
					}
				}
				else {
					if (query) {
						uint32 r_counter;

						query->get_retry_counter(r_counter);

						DLog(param.name, "Expired Timer for RESERVE: r_counter is " << r_counter);
						if (r_counter < (qosnslpconf.getpar<uint32>(qosnslpconf_max_retry)-1)) {
							// retry counter will be incremented inside retransmit_message_with_rii()
							statemodule.retransmit_message_with_rii(sid, mri, session_context, query, NULL, NULL);
						}
						else {
							ILog(param.name, "Exceeded MAX_RETRY_COUNT for QUERY without RII object!");
						}
					}
				}
			}
		}  // end if - PDU saved in TimerMsg
		else {
			// no PDU saved in TimerMsg
			// Thus we have to check if per flow based state has to be torn down.

			session_context->lock();
			NSLP_Flow_Context *flow_context = session_context->find_flow_context(*mri);
			session_context->unlock();

				if (flow_context != NULL) {
				flow_context->lock();
				flow_context->get_time_to_live(time_to_live);
				flow_context->unlock();

				ILog(param.name, "Time To Live for NSLP_Flow_Context with Flow ("
						<< mri->get_sourceaddress() << ", " << mri->get_destaddress()
						<< ") is " << time_to_live);

				if (time_to_live > sec) {
					// restart timer
					msec = 0;
					new_tmsg = new TimerMsg(message::qaddr_qos_nslp_timerprocessing);
					new_tmsg->set_source(message::qaddr_qos_nslp_timerprocessing);
					id_t new_lifetime_id = new_tmsg->get_id();

					flow_context->lock();
					flow_context->set_timer_id_for_lifetime(new_lifetime_id);
					flow_context->unlock();

					// start timer with two parameters (SID, MRI)
					std::cout << "QoS_NSLP TTL1: " << time_to_live << std::endl;
					new_tmsg->start_absolute(time_to_live, msec, sid->copy(), mri->copy());
					if (new_tmsg->send_to(message::qaddr_timer)) {
						DLog(param.name, "Resetting Timer " << new_lifetime_id << " to live to " << time_to_live << " s");
					}
				}
				else {
					// deleting NSLP_Flow_Context for MRI
					ILog(param.name, "Timeout - deleting NSLP_Flow_Context with ("
							<< mri->get_sourceaddress() << ", " << mri->get_destaddress() << ")");

					session_context->lock();
					session_context->unset_predecessor_rsn_for_est_msg();
					statemodule.delete_flow_context(sid, mri, session_context, flow_context);
					session_context->unlock();
				}
			}
		} // end else - no PDU saved in TimerMsg

		// should be safe to delete mri
		delete mri;

	} // end if - MRI saved in TimerMsg
	else {
		// no MRI saved in TimerMsg
		// Thus we have to check if session state has to be torn down.

		DLog("Timerchecker", "No MRI found in TimerMsg with ID " << tmsg->get_id() << ".");

		session_context->lock();
		session_context->get_time_to_live(time_to_live);
		session_context->unlock();

		ILog(param.name, "Time To Live for NSLP_Session_Context with SID [" << sid->to_string() << "] is " << time_to_live);

		if (time_to_live > sec) {
			// restart timer

			new_tmsg = new TimerMsg(message::qaddr_qos_nslp_timerprocessing);
			new_tmsg->set_source(message::qaddr_qos_nslp_timerprocessing);
			id_t new_lifetime_id = new_tmsg->get_id();

			// update timer id of the lifetime timer in the NSLP_Session_Context
			session_context->lock();
			session_context->set_timer_id_for_lifetime(new_lifetime_id);
			session_context->unlock();

			// start timer with one parameter (SID)
			const int32 msec = 0;
			std::cout << "QoS_NSLP TTL2: " << time_to_live << std::endl;
			new_tmsg->start_absolute(time_to_live, msec, sid->copy()); //is this correct?
			if (new_tmsg->send_to(message::qaddr_timer)) {
				ILog(param.name, "Resetting Timer " << new_lifetime_id << " to live to " << time_to_live << " s");
			}
		}
		else {
			// deleting NSLP_Session_Context for SID
			ILog(param.name, "Timeout - deleting NSLP_Session_Context with SID [" << sid->to_string() << "]");

			session_context_map->erase(*sid);
			delete session_context;
		}
	} // end else - no MRI saved in TimerMsg

	// should be safe to delete sid
	delete sid;

	// delete tmsg;
	tmsg = NULL;

	pthread_mutex_unlock(&session_mutex);
}

}  //end namespace qos_nslp


//@}

