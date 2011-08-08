/// ----------------------------------------*- mode: C++; -*--
/// @file QoS_NSLP_Client_API.h
/// QoS NSLP Client API
/// ----------------------------------------------------------
/// $Id: QoS_NSLP_Client_API.h 6258 2011-06-14 11:03:15Z bless $
/// $HeadURL: https://svn.tm.kit.edu/nsis/qos-nslp/trunk/include/QoS_NSLP_Client_API.h $
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

/**
 * This is a client interface for QoS NSLP using Unix domain sockets for 
 * communicating to the qosnslp daemon
 */


#ifndef _QOS_NSLP_CLIENT_API_H_
#define _QOS_NSLP_CLIENT_API_H_

#include "QoS_NSLP_API.h"
#include "sessionid.h"
#include "qspec.h"

#include "address.h"

using namespace protlib;
using namespace qspec;

namespace qos_nslp {

/// NSLP Client API class
/// @class QoS_NSLP_Client_API
class QoS_NSLP_Client_API : QoS_NSLP_API {

public:
	/// constructor
	QoS_NSLP_Client_API(std::string sock_name);

	/// destructor
	virtual ~QoS_NSLP_Client_API();

	/// send reservation from source to destination
	ntlp::sessionid* send_reserve(appladdress &src, appladdress &dst, bool downstream, float bandwidth, const vlsp_object* vlspobj= NULL, const session_auth_object* session_auth_obj_hmac_signed= NULL, const session_auth_object* session_auth_obj= NULL);

	/// send query to trigger receiver initiated reserve from src to dest
	ntlp::sessionid* send_query(appladdress &src, appladdress &dst, bool downstream, float bandwidth, const vlsp_object* vlspobj= NULL, const session_auth_object* session_auth_obj_hmac_signed= NULL, const session_auth_object* session_auth_obj= NULL);

	/// send tear of reservation from source to destination for given sid
	void send_tear(ntlp::sessionid *sid, appladdress &src, appladdress &dst, bool downstream, float bandwith = -1, const vlsp_object* vlspobj= NULL, const session_auth_object* session_auth_obj_hmac_signed= NULL, const session_auth_object* session_auth_obj= NULL);

        /// register for getting messages (e.g., via recv_pdu)
        void register_client() { QoS_NSLP_API::register_client(); }

	/// receive response
	void recv_response(int &status, int timeout_ms= 5000);

	/// receive response
	void recv_reserve(int &status, int timeout_ms= 5000);

	void recv_pdu(int &status, int timeout_ms, 
		      uint128& sid, 
		      appladdress &source_address,
		      appladdress &dest_address,
		      qspec::qspec_pdu* &qspec_template_p, 
		      info_spec* &info_spec_p, 
		      vlsp_object* &vlspobj_p, 
		      session_auth_object* &session_auth_obj_hmac_signed_p, 
		      session_auth_object* &session_auth_obj_other_p);

	// set HMAC signing key
	void set_hmac_key(uint32 key_id, const char* key, uint32 keylength) const;

	// delete HMAC signing key
	void del_hmac_key(uint32 key_id) const;


}; // end class QoS_NSLP_Client_API

} // end namespace qos_nslp

#endif // _QOS_NSLP_CLIENT_API_H_
