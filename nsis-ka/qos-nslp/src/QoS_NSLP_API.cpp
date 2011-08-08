/// ----------------------------------------*- mode: C++; -*--
/// @file QoS_NSLP_API.cpp
/// QoS NSLP API - sends NSLP PDUs and receives feedback
/// ----------------------------------------------------------
/// $Id: QoS_NSLP_API.cpp 6263 2011-06-14 15:03:47Z bless $
/// $HeadURL: https://svn.tm.kit.edu/nsis/qos-nslp/trunk/src/QoS_NSLP_API.cpp $
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
 * The Daemon part of this interface is in QoS_NSLP_UDS_API
 */

#include "QoS_NSLP_API.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <poll.h>
#include <errno.h>

using namespace protlib::log;
using namespace protlib;
using namespace qspec;

namespace qos_nslp {

/***** class QoS_NSLP_API *****/

const uint32 QoS_NSLP_API::version_magic = 0x9737c69dL;

/** Initialize base class. */
QoS_NSLP_API::QoS_NSLP_API(std::string sock_name) 
	: client_sock(-1), sockname(sock_name)
{
	DLog("QoS NSLP API", "Creating QoS_NSLP_API object");

	client_sock = connect_to_daemon();
	if (client_sock < 0) {
		ERRLog("QoS NSLP API", "Couldn't establish connection to socket");
		return;
	}
} // end constructor

/** Destructor for QoS_NSLP_API.  */
QoS_NSLP_API::~QoS_NSLP_API() {
	DLog("QoS NSLP API", "Destroying QoS_NSLP_API object");
} // end destructor


/**
 * Register a client app for listening on messages
 */
void 
QoS_NSLP_API::register_client() {
	DLog("QoS NSLP API", "trying to register");
	NetMsg* m= new NetMsg(uds_api_header_length);
	/**
	 * | version magic |
	 * | REGISTER      |
	 * | totallen=0    |
	 **/
	m->encode32(version_magic);
	m->encode8(REGISTER);
	m->encode32(0);
	// send message to client via UDS
	if (send(client_sock, m->get_buffer(), m->get_size(), 0) == -1)
		ERRLog("QoS NSLP API", "register_client() - send to socket failed");
}

/**
 * Send PDU via QoS NSLP using UNIX domain socket
 */
void QoS_NSLP_API::send_pdu(uint128 sid,
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
		const vlsp_object* vlspobj,
		const session_auth_object* session_auth_obj_hmac_signed,
		const session_auth_object* session_auth_obj_other
	)
{
	DLog("QoS NSLP API", "Send QoS NSLP PDU");

	QoS_Appl_Msg *res_msg = new QoS_Appl_Msg();

	res_msg->set_pdu_type(nslp_req_type);
	res_msg->set_direction(downstream);
	res_msg->set_sid(sid);
	res_msg->set_qspec(qos);
	res_msg->set_info_spec(info_spec);
	if (nslp_req_type == known_nslp_pdu::RESERVE)
		res_msg->set_request_response_with_rii(true);
	else
		res_msg->set_request_response_with_rii(false);

	if (flag_replace)
		res_msg->set_replace_flag(true);
	if (flag_ack)
		res_msg->set_acknowledge_flag(true);
	if (flag_scope)
		res_msg->set_scoping_flag(true);
	if (flag_tear)
		res_msg->set_tear_flag(true);
	if (flag_init)
		res_msg->set_reserve_init_flag(true);

	res_msg->set_source_addr(src);
	res_msg->set_dest_addr(dst);
	
	// add vlsp object
	if (vlspobj)
		res_msg->set_vlsp_object(vlspobj);

	// add other session_auth object 
	if (session_auth_obj_other)
		res_msg->set_session_auth_object(session_auth_obj_other);

	// add session_auth object HMAC signed
	if (session_auth_obj_hmac_signed)
		res_msg->set_session_auth_object_hmac_signed(session_auth_obj_hmac_signed);


	EVLog("QoS NSLP API", "Delivering translated message to client at address " << sockname);

	// send pdu
	NetMsg *nm = generate_netmsg(res_msg);
	if (nm == NULL) {
		ERRLog("QoS NSLP API", "generate_netmsg() failed");
		return;
	}

	if (send(client_sock, nm->get_buffer(), nm->get_size(), 0) == -1)
		ERRLog("QoS NSLP API", "send() failed");

	delete nm;
	nm = NULL;
} // end send_pdu


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
QoS_NSLP_API::recv_pdu(int &status, int timeout_ms, 
		       uint128& sid, 
		       appladdress &source_address,
		       appladdress &dest_address,
		       qspec::qspec_pdu* &qspec_template_p, 
		       info_spec* &info_spec_p, 
		       vlsp_object* &vlspobj_p, 
		       session_auth_object* &session_auth_obj_hmac_signed_p, 
		       session_auth_object* &session_auth_obj_other_p)
{
	if (client_sock < 0) {
		status = -1;
		ERRLog("QoS NSLP API", "Couldn't establish connection to socket");
		return;
	}

	// init all pointers
	qspec_template_p= NULL;
	info_spec_p= NULL;
	vlspobj_p= NULL;
	session_auth_obj_hmac_signed_p= NULL;
	session_auth_obj_other_p= NULL;

	struct pollfd pollfds;

	bool done= false;
	while (! done)
	{
		pollfds.fd = client_sock;
		pollfds.events = POLLIN;
		int res = 0;

		status= 0;
		res = poll(&pollfds, 1, timeout_ms);
		if (res == 0) {
			errno = ETIMEDOUT;
			status = -1;
			DLog("QoS NSLP API", "timed out");
			return;
		}
		if (res == -1) {
			status = -1;
			DLog("QoS NSLP API", "an error occurred");
			return;
		}

		// read data to buffer
		int buf_size = 1024 * 10;
		uchar *buffer = new uchar[buf_size];
		memset(buffer, 0, buf_size);
		size_t nm_size = recv(client_sock, buffer, buf_size, 0);
		if (nm_size <= 0) {
			status = -1;
			ERRLog("QoS NSLP API", "lost connection");
			return;
		};

		// convert received bytestream into NetMsg
		NetMsg *nm = new NetMsg(buffer, nm_size, true);

		// convert NetMsg into QoS_Appl_Msg
		DLog("QoS NSLP API", "convert received NetMsg into QoS_Appl_Msg");
		QoS_Appl_Msg *applmsg = decode_netmsg(nm);

		if (applmsg)
		{
			done= true;
		}

		applmsg->get_sid(sid);
		source_address= applmsg->get_source_addr(); // assignment operator will copy actual contents
		dest_address= applmsg->get_dest_addr();     // assignment operator will copy actual contents

		info_spec_p = applmsg->get_info_spec() ? applmsg->get_info_spec()->copy() : NULL;

		if (info_spec_p) {
			// debug output
			info_spec::errorclass_t er_class = (info_spec::errorclass_t) info_spec_p->get_errorclass();
			info_spec_p= applmsg->get_info_spec()->copy();
			// status values are >0
			status = static_cast<int>(er_class);
		} 	

	        qspec_template_p= applmsg->get_qspec() ? applmsg->get_qspec()->copy() : NULL; 

		vlspobj_p= applmsg->get_vlsp_object() ? applmsg->get_vlsp_object()->copy() : NULL;

		session_auth_obj_hmac_signed_p= applmsg->get_session_auth_object_hmac_signed() ? applmsg->get_session_auth_object_hmac_signed()->copy() : NULL;

		session_auth_obj_other_p= applmsg->get_session_auth_object() ? applmsg->get_session_auth_object()->copy() : NULL;

		delete applmsg;
		applmsg = NULL;
		delete nm;
		nm = NULL;
		delete[] buffer;
		buffer = NULL;
	} // end while

} // end recv_pdu



/**
 * Receive PDU from UNIX domain socket
 * @param status if the received PDU has an informational element
 *               status will be set to its GIST error class code.
 *               In case a low level error occurrs status is set to -1
 *               and errno is set accordingly.
 * @param timeout_ms poll timeout in milli seconds, specifying a
 *                   negative timeout means infinite
 */
void 
QoS_NSLP_API::recv_response(int &status, int timeout_ms)
{
	if (client_sock < 0) {
		status = -1;
		ERRLog("QoS NSLP API", "Couldn't establish connection to socket");
		return;
	}

	struct pollfd pollfds;

	bool done= false;
	while (! done)
	{
		pollfds.fd = client_sock;
		pollfds.events = POLLIN;
		int res = 0;

		res = poll(&pollfds, 1, timeout_ms);
		if (res == 0) {
			errno = ETIMEDOUT;
			status = -1;
			ERRLog("QoS NSLP API", "timed out");
			return;
		}
		if (res == -1) {
			status = -1;
			ERRLog("QoS NSLP API", "an error occurred");
			return;
		}

		// read data to buffer
		int buf_size = 1024 * 10;
		uchar *buffer = new uchar[buf_size];
		memset(buffer, 0, buf_size);
		size_t nm_size = recv(client_sock, buffer, buf_size, 0);
		if (nm_size <= 0) {
			status = -1;
			ERRLog("QoS NSLP API", "lost connection");
			return;
		};

		// convert received bytestream into NetMsg
		NetMsg *nm = new NetMsg(buffer, nm_size, true);

		// convert NetMsg into QoS_Appl_Msg
		DLog("QoS NSLP API", "convert received NetMsg into QoS_Appl_Msg");
		QoS_Appl_Msg *applmsg = NULL;
		applmsg = decode_netmsg(nm);

		if (applmsg->get_pdu_type() != known_nslp_pdu::RESPONSE)
		{
			EVLog("QoS NSLP API", "received other message than RESPONSE, type=" << (int) applmsg->get_pdu_type() << ", ignoring" );
			// clean allocated stuff
			delete applmsg;
			delete nm;
			nm = NULL;
			delete[] buffer;
			buffer = NULL;
			// start over listening until RESPONSE comes back
			continue;
		}
		else
			done= true;
	
		const info_spec *infospec = NULL;
		infospec = applmsg->get_info_spec();

		if (infospec) {
			// debug output
			info_spec::errorclass_t er_class = (info_spec::errorclass_t) infospec->get_errorclass();
			switch (er_class) {
			case info_spec::information:
				ILog("QoS NSLP API", "InfoSpec Error Class: Informational");
				break;
			case info_spec::success: 
				ILog("QoS NSLP API", "InfoSpec Error Class: Success");
				break;
			case info_spec::protocol:
				ILog("QoS NSLP API", "InfoSpec Error Class: Protocol Error");
				break;
			case info_spec::transient:
				ILog("QoS NSLP API", "InfoSpec Error Class: Transient Failure");
				break;
			case info_spec::permanent:
				ILog("QoS NSLP API", "InfoSpec Error Class: Permanent Failure");
				break;
			case info_spec::qos_model_error:
				ILog("QoS NSLP API", "InfoSpec Error Class: QoS Model Error");
				break;
			default:
				ILog("QoS NSLP API", "InfoSpec Error Class: " << er_class);
			}

			status = static_cast<int>(er_class);
		} 
		else
		{
			ERRLog("QoS NSLP API", "InfoSpec object is missing, cannot infer status of Response message");
			status = -1;
		}

		delete applmsg;
		applmsg = NULL;
		delete nm;
		nm = NULL;
		delete[] buffer;
		buffer = NULL;
	} // end while

} // end recv_response


int QoS_NSLP_API::connect_to_daemon()
{
	struct sockaddr_un daemon_addr;

	memset((char *) &daemon_addr, 0, sizeof(daemon_addr));
	daemon_addr.sun_family = AF_UNIX;
	strncpy(daemon_addr.sun_path, sockname.c_str(), sockname.length());
	int len = sizeof(daemon_addr);
	int sock;

	if ((sock = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
		ERRLog("QoS NSLP API", "socket() failed: " << strerror(errno));
	else if (connect(sock, (struct sockaddr *) &daemon_addr, len) < 0) {
		ERRLog("QoS NSLP API", "connect() failed: " << strerror(errno));
		return -1;
	}

	return sock;

} // end connect_to_daemon

/// encode a QoS_Appl_Msg into a NetMsg
NetMsg* QoS_NSLP_API::generate_netmsg(const QoS_Appl_Msg *applmsg)
{
	DLog("QoS NSLP API", "generate_netmsg() start");
	IE::coding_t coding = IE::nslp_v1;
       

	if (!applmsg) {
		ERRLog("QoS NSLP API", "Got empty QoS_Appl_Msg");
		return NULL;
	}

	// decode InfoSpec and QSPEC PDU pointers if present
	const info_spec *infospec = applmsg->get_info_spec();
	const qspec::qspec_pdu *qspec = applmsg->get_qspec();
	const vlsp_object* vlspobj = applmsg->get_vlsp_object();
	const session_auth_object* session_auth_obj_hmac_signed = applmsg->get_session_auth_object_hmac_signed();
	const session_auth_object* session_auth_obj = applmsg->get_session_auth_object();
	
	uint32 size_applmsg= sizeof(QoS_Appl_Msg);

	uint32 size_infospec= 0;
	if (infospec)
		size_infospec = infospec->get_serialized_size(coding);

	uint32 size_qspec= 0;
	if (qspec)
		size_qspec = qspec->get_serialized_size(coding);

	uint32 size_vlspobj= (vlspobj != NULL) ? vlspobj->get_serialized_size(coding) : 0;

	uint32 size_session_auth_obj_hmac_signed= (session_auth_obj_hmac_signed != NULL) ? session_auth_obj_hmac_signed->get_serialized_size(coding) : 0;

	uint32 size_session_auth_obj= (session_auth_obj != NULL) ? session_auth_obj->get_serialized_size(coding) : 0;
	
	// create NetMsg
	// +----------------------------+
	// | UDS API Header		|
	// | Size InfoSpec Object	|
	// | Size QSPEC PDU		|
	// | Size VLSP Object           |
	// | Size ApplMsg		|
	// | ApplMsg			|
	// | InfoSpec Object		|
	// | QSPEC PDU			|
	// | VLSP Object		|
	// | Session Auth ObjectHMACSig |
	// | Session Auth Object	|
	// +----------------------------+
	uint32 size_field_infospec = sizeof(uint32);
	uint32 size_field_qspec    = sizeof(uint32);
	uint32 size_field_vlspobj  = sizeof(uint32);
	uint32 size_field_sauthobj = sizeof(uint32);
	uint32 size_field_applmsg  = sizeof(uint32);

	uint32 totallen = uds_api_header_length + size_field_infospec +
		size_field_qspec + size_field_vlspobj + size_field_sauthobj + size_field_sauthobj + size_field_applmsg + size_applmsg +
		size_infospec + size_qspec + size_vlspobj + size_session_auth_obj_hmac_signed + size_session_auth_obj;

	NetMsg *nm = new NetMsg(totallen);
	
	// version magic number
	nm->encode32(QoS_NSLP_API::version_magic);
	// type
	nm->encode8(SEND_MESSAGE);


	// length
	nm->encode32(totallen - uds_api_header_length);
	// value
	nm->encode32(size_infospec);
	nm->encode32(size_qspec);
	nm->encode32(size_vlspobj);
	nm->encode32(size_session_auth_obj);
	nm->encode32(size_session_auth_obj_hmac_signed);
	nm->encode32(size_applmsg);
	QoS_Appl_Msg *new_applmsg= reinterpret_cast<QoS_Appl_Msg*>((nm->get_buffer() + nm->get_pos()));
	nm->encode(reinterpret_cast<const uchar *>(applmsg), size_applmsg);
	
	// invalidate pointers of infospec and qspec within new QoS_Appl_Msg
	new_applmsg->clear_pointers(false);

	if (infospec)
		infospec->serialize(*nm, coding, size_infospec);
	if (qspec)
		qspec->serialize(*nm, coding, size_qspec);
	if (vlspobj)
		vlspobj->serialize(*nm, coding, size_vlspobj);
	if (session_auth_obj_hmac_signed)
		session_auth_obj_hmac_signed->serialize(*nm, coding, size_session_auth_obj_hmac_signed);
	if (session_auth_obj)
		session_auth_obj->serialize(*nm, coding, size_session_auth_obj);

	DLog("QoS NSLP API", "generate_netmsg() end - size:" << totallen);

	nm->truncate();

	return nm;
} // end generate_netmsg

/// decode a NetMsg into a QoS_Appl_Msg
QoS_Appl_Msg* QoS_NSLP_API::decode_netmsg(NetMsg* m) 
{
	DLog("QoS NSLP API", "decode_netmsg() start");
	IE::coding_t coding = IE::nslp_v1;

	// reset to beginning (just in case...)
	m->to_start();

	uint32 this_magic = m->decode32();
	uint8 type = m->decode8();
	uint32 totallen      = m->decode32();
	uint32 size_infospec = m->decode32();
	uint32 size_qspec    = m->decode32();
	uint32 size_vlspobj  = m->decode32();
	uint32 size_sauthobj = m->decode32();
	uint32 size_sauthobj_hmac_signed = m->decode32();
	uint32 size_applmsg  = m->decode32();
	DLog("QoS NSLP API", "magic 0x" << hex << this_magic << dec << ", type: " << (uint32) type << ", total length: " << 
			totallen << ", size infospec: " << size_infospec <<
			", size qspec: " << size_qspec <<
			", size vlsp obj: " << size_vlspobj <<
			", size sauth obj hmac signed: " << size_sauthobj_hmac_signed <<
			", size sauth obj: " << size_sauthobj <<
			", size applmsg: " << size_applmsg);

	if (this_magic != QoS_NSLP_API::version_magic)
		ERRLog("QoS NSLP API", "Magic number mismatch - different UDS API version? Seen: 0x" << hex << this_magic << " Supported: 0x" << version_magic << dec );


	QoS_Appl_Msg *applmsg = new QoS_Appl_Msg(*(reinterpret_cast<const
				QoS_Appl_Msg*>(m->get_buffer() + m->get_pos())));

	// move netmsg buffer pointer relative to current position
	m->set_pos_r(size_applmsg);

	IEErrorList errorlist;
	uint32 bread = 0; // bytes read

	if (size_infospec != 0) {
		info_spec *infospec = new info_spec;
		info_spec *result_infospec = infospec->deserialize(*m, coding,
				errorlist, bread, false);
		DLog("QoS NSLP API", "infospec set");
		applmsg->set_info_spec(result_infospec);
		delete result_infospec;
	}

	if (size_qspec != 0) {
		qspec::qspec_pdu *qspec = new qspec::qspec_pdu;
		qspec->deserialize(*m, coding, errorlist, bread, false);
		DLog("QoS NSLP API", "qspec set " << qspec);
		applmsg->set_qspec(qspec);
		delete qspec;
	}

	if (size_vlspobj != 0) {
		vlsp_object *vlspobj = new vlsp_object;
		vlspobj->deserialize(*m, coding, errorlist, bread, false);
		DLog("QoS NSLP API", "VLSP object set " << vlspobj);
		applmsg->set_vlsp_object(vlspobj);
		delete vlspobj;
	}

	if (size_sauthobj_hmac_signed != 0) {
		session_auth_object *sauthobj = new session_auth_object;
		sauthobj->deserialize(*m, coding, errorlist, bread, false);
		DLog("QoS NSLP API", "Session Authorization object set HMAC Signed " << sauthobj);
		applmsg->set_session_auth_object_hmac_signed(sauthobj);
		delete sauthobj;
	}

	if (size_sauthobj != 0) {
		session_auth_object *sauthobj = new session_auth_object;
		sauthobj->deserialize(*m, coding, errorlist, bread, false);
		DLog("QoS NSLP API", "Session Authorization object set " << sauthobj);
		applmsg->set_session_auth_object(sauthobj);
		delete sauthobj;
	}


	DLog("QoS NSLP API", "decode_netmsg() end");
	return applmsg;
} // end decode_netmsg



void
QoS_NSLP_API::set_hmac_key(uint32 key_id, uchar* key, uint32 length)
{
	uint32 totallen=4*sizeof(uint32)+3+length;
	NetMsg* m= new NetMsg(totallen);
	/**
	 * | version magic |
	 * | SET_MAC_KEY   |
	 * | total length  |
	 * | key id        |
	 * | transform id  | (hash algorithm id)
	 * | key length    | (in bytes)
	 * | key data      | (variable)
	 **/
	m->encode32(version_magic);
	m->encode8(SET_HMAC_KEY);
	m->encode32(totallen);
	m->encode32(key_id);
	m->encode16(2); // transform ID (currently unused)
        m->encode32(length);
	// copy the key
	m->copy_from(key,m->get_pos(),length);
	
	// send message to client via UDS
	if (send(client_sock, m->get_buffer(), m->get_size(), 0) == -1)
		ERRLog("QoS NSLP API", "set_hmac_key(), send()  failed");
}


void
QoS_NSLP_API::del_hmac_key(uint32 key_id)
{
	uint32 totallen=2*sizeof(uint32)+1;
	NetMsg* m= new NetMsg(totallen);
	m->encode32(version_magic);
	m->encode8(DEL_HMAC_KEY);
	m->encode32(key_id);

	// send message to client via UDS
	if (send(client_sock, m->get_buffer(), m->get_size(), 0) == -1)
		ERRLog("QoS NSLP API", "del_hmac_key(), send()  failed");
}


}  //end namespace qos_nslp

