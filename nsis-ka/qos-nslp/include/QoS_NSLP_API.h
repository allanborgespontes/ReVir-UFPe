/// ----------------------------------------*- mode: C++; -*--
/// @file QoS_NSLP_API.h
/// QoS NSLP API
/// ----------------------------------------------------------
/// $Id: QoS_NSLP_API.h 6258 2011-06-14 11:03:15Z bless $
/// $HeadURL: https://svn.tm.kit.edu/nsis/qos-nslp/trunk/include/QoS_NSLP_API.h $
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
 * This is a client interface for Signaling Applications and allows to send PDUs and manage delivered information.
 * The interface uses internally a UNIX domain socket to communicate with the qosnslp daemon
 * The qosnslp daemon part of this interface is in QoS_NSLP_UDS_API
 */

#ifndef _QOS_NSLP_API_H_
#define _QOS_NSLP_API_H_

#include "QoS_Appl_Msg.h"
#include "QoS_NSLP_UDS_API.h"

#include "session_id_list.h"
#include "rsn_list.h"

namespace qos_nslp {

/// QoS NSLP API class
/// @class QoS_NSLP_API
class QoS_NSLP_API {
public:

	/// constructor
	QoS_NSLP_API(std::string);

	/// destructor
	virtual ~QoS_NSLP_API();

	/**
	 * API function to send a pdu
	 * @param sid		the NSIS session identifier
	 * @param nslp_req_type	Indicates type of request
	 * @param qos		the QSPEC object, if present
	 * @param refresh	refresh period
	 * @param sid_list	session ID list
	 * @param rsn_list	RSN list
	 * @param info_spec	INFOSPEC object, if present
	 * @param features_in	flags inbcluded in the common header of the 
	 * 			received QoS-NSLP message
	 * @param flag_break	BREAK flag
	 * @param flag_rrr	request reduced refreshes
	 * @param flag_init	reserve-init
	 * @param flag_tear	TEAR flag
	 * @param flag_replace	REPLACE flag
	 * @param flag_ack	ACK flag
	 * @param flag_proxy	PROXY flag
	 * @param flag_scope	SCOPING flag
	 * @param iface		incoming interface
	 * @param oface		outgoing interface
	 * @param src_a		IP address and possibly port of source
	 * @param dst_a		IP address and possibly port of final destination
	 * @param downstream	true if downstream, false if upstream
	 * @param session_auth_obj_hmac_signed a session authorization object for integrity protection (optional)
	 * @param session_auth_obj_other a session authorization object of another kind, e.g., authorization token (optional)
	 * @param vlsp_obj	optional VLSP object for virtual link setup extension
	 */
	void send_pdu(uint128 sid, 
	      known_nslp_pdu::type_t nslp_req_type,
	      qspec::qspec_pdu *qos, 
	      uint32 refresh, 
	      sid_list &sid_list, 
	      rsn_list &rsn_list, 
	      info_spec *info_spec, 
	      bool features_in, 
	      bool flag_break, 
	      bool flag_rrr, 
	      bool flag_init, 
	      bool flag_tear, 
	      bool flag_replace, 
	      bool flag_ack, 
	      bool flag_proxy, 
	      bool flag_scope, 
	      uint16 iface, 
	      uint16 oface, 
	      appladdress &src, 
	      appladdress &dst,
	      bool downstream,
	      const vlsp_object* vlspobj= NULL,
	      const session_auth_object* session_auth_obj_hmac_signed= NULL,
	      const session_auth_object* session_auth_obj_other= NULL);
	
	void recv_pdu(int &status, int timeout_ms, 
		      uint128& sid, 
		      appladdress &source_address,
		      appladdress &dest_address,
		      qspec::qspec_pdu* &qspec_template_p, 
		      info_spec* &info_spec_p, 
		      vlsp_object* &vlspobj_p, 
		      session_auth_object* &session_auth_obj_hmac_signed_p, 
		      session_auth_object* &session_auth_obj_other_pw);

	void recv_response(int &status, int timeout_ms= 5000);
	int connect_to_daemon();
        void register_client();
	NetMsg* generate_netmsg(const QoS_Appl_Msg *);
	QoS_Appl_Msg* decode_netmsg(NetMsg *);

	void set_hmac_key(uint32 key_id, uchar* key, uint32 length);
	void del_hmac_key(uint32 key_id);


private:
	int client_sock;
	std::string sockname;

	static const uint32 version_magic;

}; // end class QoS_NSLP_API

} // end namespace qos_nslp

#endif // _QOS_NSLP_API_H_
