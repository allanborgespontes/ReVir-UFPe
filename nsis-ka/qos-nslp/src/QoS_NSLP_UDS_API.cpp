/// ----------------------------------------*- mode: C++; -*--
/// @file QoS_NSLP_UDS_API.h
/// QoS NSLP UDS API - maps internal API to Unix Domain Socket API
/// ----------------------------------------------------------
/// $Id: QoS_NSLP_UDS_API.cpp 6282 2011-06-17 13:31:57Z bless $
/// $HeadURL: https://svn.tm.kit.edu/nsis/qos-nslp/trunk/src/QoS_NSLP_UDS_API.cpp $
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
 * @description
 * This is the UNIX domain socket based interface on the daemon side.
 * Please see QoS_NSLP_API.h for the client side. Both must use the same
 * message format via the Unix Domain Socket.
 *
 *  this API is used on the qosnslp daemon side to convert messages 
 *  received via Unix Domain Sockets (sent by the application)
 *  to the inernal QoSApplMsg format and vice versa (e.g., notifications
 *  from qosnslp daemon to the application)
 **/

#include "QoS_NSLP_UDS_API.h"
#include "queuemanager.h"
#include "hmac_keyregistry.h"

namespace qos_nslp {


UDS_API_Param::UDS_API_Param(TP* proto,
		const message::qaddr_t clientqueue,
		const message::qaddr_t internalqueue,
		uint32 sleep_time) 
	: ThreadParam(sleep_time, "QoS NSLP UDS API"),
	proto(proto),
	clientqueue(clientqueue),
	internalqueue(internalqueue)
{
} // end constructor


const uint32 UDS_API::version_magic = 0x9737c69dL;

/** 
 * Initialize Thread base class 
 */
UDS_API::UDS_API(const UDS_API_Param& p)
  : Thread(p),
    param(p)
{
	DLog(p.name, "Created QoS NSLP UDS API");
} // end constructor

UDS_API::~UDS_API() 
{
	DLog(param.name, "Destroying QoS NSLP UDS API");
} // end destructor

void UDS_API::main_loop(uint32 nr) 
{
	DLog(param.name, "Starting thread #" << nr);

	// process queue of internal messages
	process_queue();

	DLog(param.name, "Thread #" << nr << " stopped");
} // end main_loop

/**
 * process queue of internal messages
 */
void 
UDS_API::process_queue() 
{ 
	DLog(param.name, "process_queue() start");

	// max. waiting time at internal msg queue
	uint32 wait = param.sleep_time * 1000;
	message* msg = NULL;
	QoS_Appl_Msg* applmsg = NULL;
	TPMsg* tpmsg = NULL;

	// get queue
	FastQueue* fq = QueueManager::instance()->get_queue(param.clientqueue);
	if (!fq) {
		ERRLog(param.name, "Cannot find input msg queue");
		return;
	}

	// wait for messages if in state running
	while (get_state() == STATE_RUN) {
		// dequeue message from internal message queue
		msg = fq->dequeue_timedwait(wait);
		if (!msg) 
			continue;

		DLog(param.name, "Received incoming message on Queue");

		switch(msg->get_type()) {
		
		// message from transport socket
		case message::type_transport: 
			tpmsg = dynamic_cast<TPMsg*>(msg);
			if (!tpmsg) {
				ERRLog(param.name, "Cannot cast message from source " << 
						msg->get_qaddr_name() << " of type " << 
						msg->get_type_name() << " to TPMsg");
				break;
			}
			DLog(param.name, "Received incoming message from TP Module");
			// process incoming message from TP module (network)
			process_tp_msg(tpmsg);
			break;

		// message from ProcessingModule
		case message::type_signaling:
			applmsg = dynamic_cast<QoS_Appl_Msg*>(msg);
			if (!applmsg) {
				ERRLog(param.name, "Cannot cast message from source " << 
						msg->get_qaddr_name() << " of type " << 
						msg->get_type_name() << " to QoS_Appl_Msg");
				break;
			}
			DLog(param.name, "Received incoming message from ProcessingModule");
			// process message from coordinator to TP
			// (outgoing message to client process via unix domain socket)
			process_appl_msg(applmsg);
			break;

		default:
			ERRLog(param.name, "Received a message from " << msg->get_qaddr_name() << 
					" of type " << msg->get_type_name() << 
					" that cannot be processed here");
		} // end switch
			
		// clean up, messages directed to us are disposable
		delete msg;
	} // end while running
} // end process_queue

/** 
 * process incoming message from Unix domain socket, i.e., from the application
 * @param tpmsg Internal TPMsg from UDS TP module
 */
void 
UDS_API::process_tp_msg(const TPMsg* tpmsg) 
{

	assert(tpmsg != NULL);

	DLog(param.name, "process_tp_msg() - received message #" << tpmsg->get_id() << " from TP");

	// remember netmsg and peer from TPmsg
	NetMsg* netmsg = tpmsg->get_message();
	uint32 this_version_magic= netmsg->decode32();
	uint8 type = netmsg->decode8();

	const udsaddress* peer = dynamic_cast<const udsaddress*>(tpmsg->get_peeraddress());

	if (peer)
		EVLog(param.name, "Received an incoming call from peer: " << *peer << " - translating");
	else
		ERRLog(param.name, "Received an incoming call from unknown peer socket");

	if (this_version_magic != UDS_API::version_magic)
	{
		ERRLog(param.name, "Magic number mismatch - different UDS API version? Seen: 0x" << hex << this_version_magic << " Supported: 0x" << version_magic << dec << " - ignored message." ); 
		return;
	}

    	QoS_Appl_Msg* applmsg = NULL;
	switch (type) {
		case SEND_MESSAGE:
			DLog(param.name, "Received: " << color[red] << "SEND_MESSAGE" << color[off]);
			applmsg = decode_netmsg(netmsg);
			break;
		case SET_HMAC_KEY:
			DLog(param.name, "Received: " << color[red] << "SET_HMAC_KEY" << color[off]);
			set_hmac_key(netmsg);
			break;
		case DEL_HMAC_KEY:
			DLog(param.name, "Received: " << color[red] << "DEL_HMAC_KEY" << color[off]);
			del_hmac_key(netmsg);
			break;
	        case REGISTER:
 			DLog(param.name, "Received: " << color[red] << "REGISTER" << color[off]);
			break;
		default:
			ERRLog(param.name, "Received unknown message type");
	}

	// decoding went fine, if applmsg is filled
	
	if (peeraddr.is_invalid() && peer != NULL)
	{
		DLog(param.name, "Saving socket address" << *peer);
	
		// save peer address for later answers
		peeraddr = *peer;
	}
	
	if (applmsg) 
	{
	  DLog(param.name, "Received applmsg - will forward");

	  applmsg->set_source(param.clientqueue);
	  applmsg->send_to(param.internalqueue);
	}

	DLog(param.name, "process_tp_msg() - done.");
} // end process_tp_msg

/**
 * process message from coordinator
 * (outgoing signaling message, sent subsequently to TP module)
 */
void 
UDS_API::process_appl_msg(const QoS_Appl_Msg* applmsg) 
{

	EVLog(param.name, "Received a QoS_Appl_Msg to translate and deliver to connected clients");

	if (peeraddr.is_invalid())
	{
		EVLog(param.name, "No client connected, discarding message.");
		return;
	}

	udsaddress* peer = new udsaddress(peeraddr);
	if (!peer) {
		EVLog(param.name, "Address memory allocation failed.");
		return;
	}

	NetMsg* netmsg = generate_netmsg(applmsg);

	// send message to client via UDS (but do not try to connect if no client is connected)
	EVLog(param.name, "Trying to deliver translated message to client at address " << *peer);
	param.proto->send(netmsg, *peer, false); 
} // end process_appl_msg


/// encode a QoS_Appl_Msg into a NetMsg
NetMsg* UDS_API::generate_netmsg(const QoS_Appl_Msg* applmsg) 
{
	DLog(param.name, "generate_netmsg() start");
	IE::coding_t coding = IE::nslp_v1;
	

	if (!applmsg) {
		ERRLog(param.name, "Got empty QoS_Appl_Msg");
		return NULL;
	}

	// decode InfoSpec and QSPEC PDU pointers if present
	const info_spec *infospec = applmsg->get_info_spec();
	const qspec::qspec_pdu *qspec = applmsg->get_qspec();
	const vlsp_object* vlspobj = applmsg->get_vlsp_object();
	const session_auth_object* session_auth_obj = applmsg->get_session_auth_object();
	const session_auth_object* session_auth_obj_hmac_signed = applmsg->get_session_auth_object_hmac_signed();
	
	uint32 size_applmsg = sizeof(QoS_Appl_Msg);

	uint32 size_infospec = 0;
	if (infospec)
		size_infospec = infospec->get_serialized_size(coding);

	uint32 size_qspec = 0;
	if (qspec)
		size_qspec = qspec->get_serialized_size(coding);

	uint32 size_vlspobj= (vlspobj != NULL) ? vlspobj->get_serialized_size(coding) : 0;

	uint32 size_session_auth_obj= (session_auth_obj != NULL) ? session_auth_obj->get_serialized_size(coding) : 0;
	uint32 size_session_auth_obj_hmac_signed= (session_auth_obj_hmac_signed != NULL) ? session_auth_obj_hmac_signed->get_serialized_size(coding) : 0;

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
	// | SessionAuthObj_HMAC_Signed |
	// | SessionAuthObj             |
	// +----------------------------+
	uint32 size_field_infospec = sizeof(uint32);
	uint32 size_field_qspec    = sizeof(uint32);
	uint32 size_field_vlspobj  = sizeof(uint32);
	uint32 size_field_sauthobj = sizeof(uint32);
	uint32 size_field_applmsg  = sizeof(uint32);

	uint32 totallen = uds_api_header_length + 
	  size_field_infospec +
	  size_field_qspec + 
	  size_field_vlspobj + 
	  size_field_sauthobj + 
	  size_field_sauthobj + 
	  size_field_applmsg + 
	  size_applmsg +
	  size_infospec + 
	  size_qspec + 
	  size_vlspobj + 
	  size_session_auth_obj_hmac_signed + 
	  size_session_auth_obj;

	/*
	DLog(param.name, "magic 0x" << hex << UDS_API::version_magic << dec << ", type: " << SEND_MESSAGE << ", total length: " << 
			totallen << ", size infospec: " << size_infospec <<
			", size qspec: " << size_qspec <<
			", size vlsp obj: " << size_vlspobj <<
			", size sauth obj: " << size_session_auth_obj <<
			", size sauth obj hmac signed: " << size_session_auth_obj_hmac_signed <<
			", size applmsg: " << size_applmsg);
	*/
	NetMsg *nm = new NetMsg(totallen);
	DLog(param.name, "length netmsg allocation: " << totallen);
	
	// UDS API magic number (should match QoS_NSLP_API::version_magic)
	nm->encode32(UDS_API::version_magic);
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
	QoS_Appl_Msg *new_applmsg = reinterpret_cast<QoS_Appl_Msg*>(nm->get_buffer() + nm->get_pos());
	nm->encode(reinterpret_cast<const uchar *>(applmsg), size_applmsg);
	//DLog(param.name, "generate_netmsg() serialized applmsg, pos:" << nm->get_pos());

	// invalidate pointers of infospec and qspec within new QoS_Appl_Msg
	new_applmsg->clear_pointers(false);

	if (infospec)
	{
	  infospec->serialize(*nm, coding, size_infospec);
	  //DLog(param.name, "generate_netmsg() serialized info spec, pos:" << nm->get_pos());
	}
	if (qspec)
	{
	  qspec->serialize(*nm, coding, size_qspec);
	  //DLog(param.name, "generate_netmsg() serialized qspec, pos:" << nm->get_pos());
	}

	if (vlspobj)
	{
	  vlspobj->serialize(*nm, coding, size_vlspobj);
	  //DLog(param.name, "generate_netmsg() serialized vlspobject, pos:" << nm->get_pos());
	}

	if (session_auth_obj_hmac_signed)
	{
	  session_auth_obj_hmac_signed->serialize(*nm, coding, size_session_auth_obj_hmac_signed);
	  //DLog(param.name, "generate_netmsg() serialized session auth object (hmac), pos:" << nm->get_pos());
	}

	if (session_auth_obj)
	{
	  session_auth_obj->serialize(*nm, coding, size_session_auth_obj);
	  //DLog(param.name, "generate_netmsg() serialized session auth object (other), pos:" << nm->get_pos());
	}

	nm->truncate();
	DLog(param.name, "generate_netmsg() end - size:" << totallen);

	return nm;
} // end generate_netmsg


/// decode a NetMsg into a QoS_Appl_Msg
QoS_Appl_Msg* UDS_API::decode_netmsg(NetMsg* m) 
{
	DLog(param.name, "decode_netmsg() start");
	IE::coding_t coding = IE::nslp_v1;

	// reset to beginning (just in case...)
	m->to_start();

	uint32 this_version_magic = m->decode32();
	uint8 type = m->decode8();
	uint32 totallen      = m->decode32();
	uint32 size_infospec = m->decode32();
	uint32 size_qspec    = m->decode32();
	uint32 size_vlspobj  = m->decode32();
	uint32 size_sauthobj = m->decode32();
	uint32 size_sauthobj_hmac_signed = m->decode32();
	uint32 size_applmsg  = m->decode32();

	DLog(param.name, "magic 0x" << hex << this_version_magic << dec << " type " << (uint32) type << " total length " << 
			totallen << ", size infospec " << size_infospec <<
			", size qspec " << size_qspec <<
			", size vlsp obj " << size_vlspobj <<
			", size session auth obj " << size_sauthobj <<
			", size session auth obj hmac signed " << size_sauthobj_hmac_signed <<
			", size applmsg " << size_applmsg);

	QoS_Appl_Msg *applmsg = new QoS_Appl_Msg(*(reinterpret_cast<const QoS_Appl_Msg*>((m->get_buffer() + m->get_pos()))));

	if (this_version_magic != UDS_API::version_magic)
		ERRLog(param.name, "Magic number mismatch - different UDS API version? Seen: 0x" << hex << this_version_magic << " Supported: 0x" << UDS_API::version_magic << dec );

	// move netmsg buffer pointer relative to current position
	m->set_pos_r(size_applmsg);

	IEErrorList errorlist;
	uint32 bread = 0; // bytes read

	if (size_infospec != 0) {
		info_spec *infospec = new info_spec;
		info_spec *result_infospec = infospec->deserialize(*m, coding,
				errorlist, bread, false);
		//DLog(param.name, "set infospec " << result_infospec);
		applmsg->set_info_spec(result_infospec);
		delete result_infospec;
	}

	if (size_qspec != 0) {
		qspec::qspec_pdu *qspec = new qspec::qspec_pdu;
		qspec->deserialize(*m, coding, errorlist, bread, false);
		//DLog(param.name, "set qspec " << qspec);
		applmsg->set_qspec(qspec);
		delete qspec;
	}

	if (size_vlspobj != 0) {
		vlsp_object *vlspobj = new vlsp_object;
		vlspobj->deserialize(*m, coding, errorlist, bread, false);
		//DLog("QoS NSLP API", "VLSP object set " << vlspobj);
		applmsg->set_vlsp_object(vlspobj);
		delete vlspobj;
	}

	if (size_sauthobj_hmac_signed != 0) {
		session_auth_object *sauthobj = new session_auth_object;
		sauthobj->deserialize(*m, coding, errorlist, bread, false);
		DLog("QoS NSLP API", "Session Authorization object HMAC Signed set " << sauthobj);
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


	DLog(param.name, "decode_netmsg() end");
	return applmsg;
} // end decode_netmsg


void 
UDS_API::set_hmac_key(NetMsg* m)
{
	m->to_start();

	/**
	 * | version magic |
	 * | SET_MAC_KEY   |
	 * | total length  |
	 * | key id        |
	 * | transform id  | (hash algorithm id)
	 * | key length    | (in bytes)
	 * | key data      | (variable)
	 **/
	uint32 this_version_magic = m->decode32();
	uint8 type = m->decode8();
	uint32 totallen     = m->decode32();
	uint32 keyid        = m->decode32();
	uint16 transformid  = m->decode16(); // currently ignored, default is used
	uint32 keylen       = m->decode32();

	if (this_version_magic != UDS_API::version_magic)
		ERRLog(param.name, "Magic number mismatch - different UDS API version? Seen: 0x" << hex << this_version_magic << " Supported: 0x" << UDS_API::version_magic << dec );

	if (type !=  SET_HMAC_KEY)
	{
		ERRLog(param.name, "internal error - expected message of type SET_HMAC_KEY, but got " << (int) type << ", ignoring.");
		return;
	}

	DLog(param.name,"set_hmac_key: keyid=" << hex << keyid << dec << ", key length:" << keylen);
	nslp_auth::sign_key_registry.storeKey(keyid, m->get_buffer()+m->get_pos(), keylen);
}

void 
UDS_API::del_hmac_key(NetMsg* m)
{
	m->to_start();

	uint32 this_version_magic = m->decode32();
	uint8 type = m->decode8();
	uint32 totallen     = m->decode32();
	uint32 keyid        = m->decode32();

	if (this_version_magic != UDS_API::version_magic)
		ERRLog(param.name, "Magic number mismatch - different UDS API version? Seen: 0x" << hex << this_version_magic << " Supported: 0x" << UDS_API::version_magic << dec );
	if (type !=  DEL_HMAC_KEY)
	{
		ERRLog(param.name, "internal error - expected message of type DEL_HMAC_KEY, but got " << (int) type << ", ignoring.");
		return;
	}
	if (totallen != 4)
	{
		ERRLog(param.name,"del_hmac_key: length parameter expected to be 4, but is " << totallen );
	}
	DLog(param.name,"del_hmac_key: keyid=" << hex << keyid << dec);
	nslp_auth::sign_key_registry.deleteKey(keyid);
}

} // end namespace qos_nslp
