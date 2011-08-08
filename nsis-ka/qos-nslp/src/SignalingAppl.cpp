/// ----------------------------------------*- mode: C++; -*--
/// @file SignalingAppl.cpp
/// Signaling Application that sends NSLP PDUs and receives feedback
/// ----------------------------------------------------------
/// $Id: SignalingAppl.cpp 6211 2011-06-03 19:58:27Z bless $
/// $HeadURL: https://svn.tm.kit.edu/nsis/qos-nslp/trunk/src/SignalingAppl.cpp $
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

#include <unistd.h>
#include <iostream>
#include <string>
#include <sstream>
#include <netdb.h>

#include "SignalingAppl.h"
#include "sessionid.h"
#include "messages.h"
#include "queuemanager.h"
#include "logfile.h"
#include "info_spec.h"

#include "QoS_NSLP_Client_API.h"

#include "qspec_ie.h"
#include "qspec_pdu.h"
#include "qspec_param.h"

using namespace protlib::log;
using namespace protlib;
using namespace qspec;

namespace qos_nslp   {

/** @addtogroup SignalingAppl Signaling Application
 * @{
 */

/***** class SignalingAppl *****/

SignalingApplParam::SignalingApplParam(uint32 sleep_time, bool see, bool sre)
	: ThreadParam(sleep_time,"SignalingApplModule"),
	  source(message::qaddr_qos_appl_signaling),
	  send_error_expedited(see),
	  send_reply_expedited(sre)
{
} // end constructor

/** This array contains info types. */
const char* const SignalingAppl::info_type_string[] = {
	"undefined",
	"QoS NSLP did not receive a RESPONSE after retransmitting a message RII MAX_RETRY_COUNTER times. Deleting context.",
	"Not enough ressources in the current node. No context created."
}; // end type_string

/** Initialize Thread base class. */
SignalingAppl::SignalingAppl(const SignalingApplParam& p)
  : Thread(p),
    param(p)
{
	// register queue
	QueueManager::instance()->register_queue(get_fqueue(), p.source);
	DLog("SignalingAppl", "Created SignalingAppl object");
} // end constructor

/** Destructor for Signaling Application.  */
SignalingAppl::~SignalingAppl() 
{
	DLog("SignalingAppl", "Destroying SignalingAppl object");
	QueueManager::instance()->unregister_queue(param.source);
} // end destructor

/** This function is the main loop of the current thread. Will be called until this thread will be stopped.  
 *  @param nr the id of the current thread.
 */
void SignalingAppl::main_loop(uint32 nr) 
{
	DLog("SignalingAppl", "Starting thread #" << nr);

	// Legacy function for testing only
	//get_and_send_pdu();

	// process queue of internal messages
	process_queue();
	DLog("SignalingAppl", "Thread #" << nr << " stopped");
} // end main_loop



/**
 * process queue of internal messages for the ProcessingModule
 * (usually message::qaddr_qos_appl_signaling)
 */
void SignalingAppl::process_queue()
{
	uint32 wait = param.sleep_time * 1000; // max. waiting time at internal msg queue
	message* msg = NULL;
	QoS_Appl_Msg* qos_msg = NULL;

	FastQueue* fq = QueueManager::instance()->get_queue(param.source);
	if (!fq) {
		ERRLog("SignalingAppl", "Cannot find input msg queue"); 
		return;
	} // end if not fq


	// wait for messages if in state running
	while (get_state()==STATE_RUN) {
		// dequeue message from internal message queue
		msg= fq->dequeue_timedwait(wait);
		if (msg) {
			Log(EVENT_LOG, LOG_NORMAL, "SignalingAppl", "process_queue() received a message");
			qos_msg = dynamic_cast<QoS_Appl_Msg*>(msg);
			if (qos_msg) {
				process_qos_msg(qos_msg);
			}
			else {
				ERRLog("SignalingAppl", "Cannot cast message from source " << msg->get_qaddr_name() << " to QoS-NSLP-Msg");
				delete msg;
			} // end if qos_msg
		}
	} // end while running
} // end process_queue

/** process incoming message from ProcessingModule.
  * (incoming signaling message from ProcessingModule).
  * @param qos_msg Internal QoS-Appl-Msg from ProcessingModule.
  */
void SignalingAppl::process_qos_msg(QoS_Appl_Msg* qos_msg)
{
	Log(INFO_LOG,LOG_NORMAL, "SignalingAppl", "process_qos_msg()");
	if (qos_msg->get_info_spec()) {
		cout << (*qos_msg->get_info_spec());
	}
	else {
		Log(INFO_LOG,LOG_NORMAL, "SignalingAppl", "QSPEC is empty!");
	}
	QoS_Appl_Msg::qos_info_type_t inf_type = qos_msg->get_info_type();
	if (inf_type != QoS_Appl_Msg::not_set) {
		Log(INFO_LOG,LOG_NORMAL, "SignalingAppl", "Received info from QoS NSLP: " << get_info_type_string(inf_type));
	}
	Log(INFO_LOG,LOG_NORMAL, "SignalingAppl", "END process_qos_msg()");
} // end process_qos_msg

/** This function reads the input values from a file and sends them to ProcessingModule.  */
/* For testing purposes only -- OBSOLETE */
void SignalingAppl::get_and_send_pdu()
{
	QoS_Appl_Msg* 	qosapplmsg;
	ntlp::sessionid* new_sid= NULL;
	uint32 		down, the_same_session, id, band, number, pdu_format;
	string		src_addr, dst_addr, set_req_rsp_rii;
	hostaddress 	orig_a;
	appladdress     source_a, dest_a;
	ifstream	receiver, flags;
	bool 		ipv4 = false;

	receiver.open("receiver.input");
	if (!receiver) {
		ERRLog("get_and_set_pdu()", "Error: Can't open receiver.input.");
		return;
	}
	DLog("get_and_set_pdu()", "Opened receiver.input.");

	/*
	 * Example for receiver.input
	 * 10.10.1.1 10.10.2.3 1 yes 65 1024 1 1 1
	 * <src_addr> <dst_addr> <down> <set_req_resp_rii> <id> <band> <number> <the_same_session> <pdu_format>
	 */
	receiver >> src_addr >> dst_addr >> down >> set_req_rsp_rii >> id >> band >> number;
	receiver >> the_same_session >> pdu_format;

	receiver.close();

	source_a.set_ip(src_addr.c_str());
	dest_a.set_ip(dst_addr.c_str());

	if (source_a.is_ipv4())
		ipv4 = true;

	in6_addr* addr6 = NULL;
	in_addr*  addr4 = NULL;
	hostent* host;
	char* buffer = new char[255];
	
	gethostname(buffer, 255);

	if (!ipv4) {
		host = gethostbyname2(buffer, AF_INET6);
		uint8 retries = 0;

		while (!host && (retries < 20)) {
			//lookup IPv6
			host = gethostbyname2(buffer, AF_INET6);
			if (!host) cout << "retrying reverse DNS lookup" << endl;
			retries++;
		}
		if (!host) {
			DLog("QOS_STATEMODULE", "Communication via IPv6 not possible, host has no AAAA record");
			addr6 = new in6_addr;
		} else {
			addr6 = (struct in6_addr *) *host->h_addr_list;
		}
		orig_a.set_ip(*addr6);
	}
	else {
		host = gethostbyname2(buffer, AF_INET);
		uint8 retries = 0;

		while (!host && (retries < 20)) {
			//lookup IPv6
			host = gethostbyname2(buffer, AF_INET);
			if (!host) 
				cout << "retrying reverse DNS lookup" << endl;
			retries++;
		}
		if (!host) {
			DLog("QOS_STATEMODULE", "Communication via IPv6 not possible, host has no AAAA record");
			addr4 = new in_addr;
		} else {
			addr4 = (struct in_addr *) *host->h_addr_list;
		}
#ifdef TESTING
		if (strcmp(buffer, "tb1") == 0) {
			//orig_a.set_ip(*addr4);
			orig_a.set_ip("10.10.1.1");
		}
#endif
	}

	if (orig_a != source_a)
		return;

	bool downstream = false;
	if (down == 1)
		downstream = true;

	qspec_pdu* q_pdu = new qspec_pdu(qspec::ms_sender_initiated_reservations, 1);
	qspec::qspec_object *qos_desired = new qspec::qspec_object(ot_qos_desired);
	qos_desired->set_parameter(new t_mod(band,100000,band,1500));
	q_pdu->set_object(qos_desired);

	flags.open("flags.input");
	if (!flags) {
		ERRLog("get_and_set_pdu()", "Error: Can't open flags.input.");
		return;
	}
	DLog("get_and_set_pdu()", "Opened flags.input.");

	/*
	 * Example for flags.input
	 * 1 0 0 0 0
	 * <replace_flag> <acknowledge_flag> <scoping_flag> <tear_flag> <reserve_init_flag>
	 */

	int replace, ack, scope, tear, reserve_init;

	flags >> replace >> ack >> scope >> tear >> reserve_init;

	if (the_same_session == 1) {
	  new_sid = new ntlp::sessionid();
	  new_sid->generate_random();
	}

	for (uint32 i = 0; i < number; i++) {
		qosapplmsg = new QoS_Appl_Msg();	
		qosapplmsg->set_direction(downstream);

		qosapplmsg->set_qspec(q_pdu);

		if (replace) 
			qosapplmsg->set_replace_flag(true);
		if (ack) 
			qosapplmsg->set_acknowledge_flag(true);
		if (scope)
			qosapplmsg->set_scoping_flag(true);
		if (tear)
			qosapplmsg->set_tear_flag(true);
		if (reserve_init)
			qosapplmsg->set_reserve_init_flag(true);

		if (pdu_format == 1)
			qosapplmsg->set_pdu_type(qos_nslp::known_nslp_pdu::RESERVE);
		if (pdu_format == 2)
			qosapplmsg->set_pdu_type(qos_nslp::known_nslp_pdu::QUERY);
		if (the_same_session == 2) {
		  new_sid = new ntlp::sessionid();
		  new_sid->generate_random();
		}
		qosapplmsg->set_sid(*new_sid);
		qosapplmsg->set_source_addr(source_a);
		qosapplmsg->set_dest_addr(dest_a);
		if (set_req_rsp_rii == "yes")
			qosapplmsg->set_request_response_with_rii(true);
		qosapplmsg->send_or_delete();
	}  // end for
} // end get_and_send_pdu

}  //end namespace qos_nslp

//@}

