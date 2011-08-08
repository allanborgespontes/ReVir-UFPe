/// ----------------------------------------*- mode: C++; -*--
/// @file QoS_NSLP_Client_API.cpp
/// QoS NSLP Client API to be used by applications
/// ----------------------------------------------------------
/// $Id: QoS_NSLP_Client_API.cpp 6256 2011-06-13 22:08:08Z bless $
/// $HeadURL: https://svn.tm.kit.edu/nsis/qos-nslp/trunk/src/QoS_NSLP_Client_API.cpp $
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

#include "QoS_NSLP_Client_API.h"
#include "sessionid.h"
#include "hmac_keyregistry.h"

using namespace protlib;
using namespace qspec;

namespace qos_nslp {

/***** class QoS_NSLP_Client_API *****/

/** Initialize base class. */
QoS_NSLP_Client_API::QoS_NSLP_Client_API(std::string sockname) : QoS_NSLP_API(sockname)
{
	DLog("QoS NSLP Client API", "Creating QoS_NSLP_Client_API object");
} // end constructor

/** Destructor for QoS_NSLP_API.  */
QoS_NSLP_Client_API::~QoS_NSLP_Client_API() {
	DLog("QoS NSLP Client API", "Destroying QoS_NSLP_Client_API object");
} // end destructor

ntlp::sessionid*
QoS_NSLP_Client_API::send_reserve(appladdress &src, appladdress &dst, bool downstream, float bandwidth, const vlsp_object* vlspobj, const session_auth_object* session_auth_obj_hmac, const session_auth_object* session_auth_obj)
{
	DLog("QoS NSLP Client API", "Send QoS NSLP Reserve");

	const int object_combination = 1;
	sid_list sidl;
	rsn_list rsnl;

	/* generate new session id */	
	ntlp::sessionid *new_sid = new ntlp::sessionid();
	new_sid->generate_random();

	/* create qspec pdu */
	qspec_pdu* q_pdu = new qspec_pdu(ms_sender_initiated_reservations, object_combination);
	qspec::qspec_object *qos_desired = new qspec::qspec_object(ot_qos_desired);
	qos_desired->set_parameter(new qspec::t_mod(bandwidth, 100000, bandwidth, 1500));
	qos_desired->set_parameter(new qspec::admission_priority(ap_high_priority_flow));
	q_pdu->set_object(qos_desired);

	send_pdu(*new_sid, 
		known_nslp_pdu::RESERVE, 
		q_pdu, 
		false, 	/* refresh */
		sidl,	/* SID List */
		rsnl, 	/* RSN List */
		NULL, 	/* info spec */
		false, 	/* features in */
		false, 	/* break flag */
		false, 	/* request reduced refreshes */
		true, 	/* reserve-init */
		false, 	/* tear flag */
		false, 	/* replace flag */
		false, 	/* ack flag */
		false, 	/* proxy flag */
		false, 	/* scope flag */
		false, 	/* incoming interface */
		false, 	/* outgoing interface */
		src, 
		dst, 
		downstream,
		vlspobj,
		session_auth_obj_hmac,
		session_auth_obj);

	return new_sid;
}


ntlp::sessionid*
QoS_NSLP_Client_API::send_query(appladdress &src, appladdress &dst, bool downstream, float bandwidth, const vlsp_object* vlspobj, const session_auth_object* session_auth_obj_hmac, const session_auth_object* session_auth_obj)
{
	DLog("QoS NSLP Client API", "Send QoS NSLP Query (Requesting Reserve)");

	const int object_combination = 1;
	sid_list sidl;
	rsn_list rsnl;

	/* generate new session id */	
	ntlp::sessionid *new_sid = new ntlp::sessionid();
	new_sid->generate_random();

	/* create qspec pdu */
	qspec_pdu* q_pdu = new qspec_pdu(ms_sender_initiated_reservations, object_combination);
	qspec::qspec_object *qos_desired = new qspec::qspec_object(ot_qos_desired);
	qos_desired->set_parameter(new qspec::t_mod(bandwidth, 100000, bandwidth, 1500));
	qos_desired->set_parameter(new qspec::admission_priority(ap_high_priority_flow));
	q_pdu->set_object(qos_desired);

	send_pdu(*new_sid, 
		known_nslp_pdu::QUERY,
		q_pdu, 
		false, 	/* refresh */
		sidl,	/* SID List */
		rsnl, 	/* RSN List */
		NULL, 	/* info spec */
		false, 	/* features in */
		false, 	/* break flag */
		false, 	/* request reduced refreshes */
		true, 	/* reserve-init */
		false, 	/* tear flag */
		false, 	/* replace flag */
		false, 	/* ack flag */
		false, 	/* proxy flag */
		false, 	/* scope flag */
		false, 	/* incoming interface */
		false, 	/* outgoing interface */
		src, 
		dst, 
		downstream,
		vlspobj,
		session_auth_obj_hmac,
		session_auth_obj);

	return new_sid;
}


/**
 * Receive PDU from UNIX domain socket
 * @param status if the received PDU has an informational element
 *               status will be set to its GIST error class code.
 *               In case a low level error occurrs status is set to -1
 *               and errno is set accordingly.
 * @param timeout_ms poll timeout in milli seconds, specifying a
 *                   negative timeout means infinite
 * @param sid               session id
 * @param source_address    reference to source_address of the data flow
 * @param dest_address      reference to dest_address of the data flow
 * @param qspec_template_p  pointer to qspec template (if present != NULL)
 * @param info_spec_p       pointer to info spec      (if present != NULL)
 * @param vlspobj_p         pointer to vlsp object    (if present != NULL)
 * @param session_auth_obj_hmac_signed_p  pointer to session authorization object HMAC signed  (if present != NULL)
 * @param session_auth_obj_other_p  pointer to session authorization object  (if present != NULL)
 */
void 
QoS_NSLP_Client_API::recv_pdu(int &status, int timeout_ms, 
			      uint128& sid, 
			      appladdress &source_address,
			      appladdress &dest_address,
			      qspec::qspec_pdu* &qspec_template_p, 
			      info_spec* &info_spec_p, 
			      vlsp_object* &vlspobj_p, 
			      session_auth_object* &session_auth_obj_hmac_signed_p, 
			      session_auth_object* &session_auth_obj_other_p)
{
	QoS_NSLP_API::recv_pdu(status, 
			       timeout_ms, 
			       sid, 
			       source_address,
			       dest_address,
			       qspec_template_p, 
			       info_spec_p, 
			       vlspobj_p, 
			       session_auth_obj_hmac_signed_p, 
			       session_auth_obj_other_p);
}

void
QoS_NSLP_Client_API::send_tear(ntlp::sessionid *sid, appladdress &src, appladdress &dst, bool downstream, float bandwidth, const vlsp_object* vlspobj, const session_auth_object* session_auth_obj_hmac, const session_auth_object* session_auth_obj)
{
	DLog("QoS NSLP Client API", "Send QoS NSLP Tear (Tearing Reserve)");

	sid_list sidl;
	rsn_list rsnl;
	
	/* create qspec pdu if a positive bandwith has been specified */
	/* otherwise the missing qspec pdu indicates to tear all allocated ressources */
	const int object_combination = 1;
	qspec_pdu* q_pdu = NULL;
	if (bandwidth >= 0) {
		q_pdu = new qspec_pdu(ms_sender_initiated_reservations, object_combination);
		qspec::qspec_object *qos_desired = new qspec::qspec_object(ot_qos_desired);
		qos_desired->set_parameter(new qspec::t_mod(bandwidth, 100000, bandwidth, 1500));
		qos_desired->set_parameter(new qspec::admission_priority(ap_high_priority_flow));
		q_pdu->set_object(qos_desired);
	}

	send_pdu(*sid,
		known_nslp_pdu::RESERVE,
		NULL, 
		false, 	/* refresh */
		sidl,	/* SID List */
		rsnl, 	/* RSN List */
		NULL, 	/* info spec */
		false, 	/* features in */
		false, 	/* break flag */
		false, 	/* request reduced refreshes */
		false, 	/* reserve-init */
		true, 	/* tear flag */
		false, 	/* replace flag */
		false, 	/* ack flag */
		false, 	/* proxy flag */
		false, 	/* scope flag */
		false, 	/* incoming interface */
		false, 	/* outgoing interface */
		src, 
		dst, 
		downstream,
		vlspobj,
		session_auth_obj_hmac,
		session_auth_obj);
}

void 
QoS_NSLP_Client_API::recv_response(int &status, int timeout_ms)
{
	DLog("QoS NSLP Client API", "Awaiting QoS NSLP Response, timeout after " << timeout_ms << " ms");
	QoS_NSLP_API::recv_response(status, timeout_ms);
}

void 
QoS_NSLP_Client_API::recv_reserve(int &status, int timeout_ms)
{
	DLog("QoS NSLP Client API", "Awaiting QoS NSLP Reserve, timeout after " << timeout_ms << " ms");
	QoS_NSLP_API::recv_response(status, timeout_ms);
}


void
QoS_NSLP_Client_API::set_hmac_key( uint32 key_id, const char* key, uint32 keylength) const
{
	// use default algorithm for key type
	nslp_auth::sign_key_registry.storeKey(key_id, key, keylength);
}


void
QoS_NSLP_Client_API::del_hmac_key(uint32 key_id) const
{
	nslp_auth::sign_key_registry.deleteKey(key_id);
}


} // end namespace qos_nslp

