/*
 * utils.cpp - Helper functions
 *
 * $Id: utils.cpp 4118 2009-07-16 16:13:10Z bless $
 * $HeadURL: https://svn.tm.kit.edu/nsis/natfw-nslp/trunk/eval/utils.cpp $
 *
 */
#include "utils.h"
#include "msg/natfw_msg.h"
#include "msg/information_code.h"
#include "session_id.h"

namespace ntlp {
// configuration class
gistconf gconf;
}

namespace natfw {
  using namespace natfw::msg;


protlib::NetMsg *build_create_msg(uint32 msn, uint32 lifetime) {

	natfw_create create(false);
	create.set_session_lifetime(lifetime);
	create.set_extended_flow_info(extended_flow_info::ra_allow, 0);
	create.set_msg_sequence_number(msn);

	/*
	 * Variables.
	 */
	NetMsg *msg = new NetMsg(create.get_serialized_size(IE::protocol_v1));
	uint32 bytes_written;		// the number of bytes written

	/*
	 * Write the QSPEC into the NetMsg.
	 */
	try {
		create.serialize(*msg, IE::protocol_v1, bytes_written);
	}
	catch ( IEError &err ) {
		std::cout << "Error: " << err << std::endl;
		exit(1);
	}

	return msg;
}


protlib::NetMsg *build_response_msg() {

	natfw_response resp;
	resp.set_information_code(information_code::sc_success,
		information_code::suc_successfully_processed, 0);
	resp.set_msg_sequence_number(0xdeadbeef);

	/*
	 * Variables.
	 */
	NetMsg *msg = new NetMsg(resp.get_serialized_size(IE::protocol_v1));
	uint32 bytes_written;		// the number of bytes written

	/*
	 * Write the QSPEC into the NetMsg.
	 */
	try {
		resp.serialize(*msg, IE::protocol_v1, bytes_written);
	}
	catch ( IEError &err ) {
		std::cout << "Error: " << err << std::endl;
		exit(1);
	}

	return msg;
}


static ntlp::mri *build_mri(const appladdress &nr, bool downstream) {
	using namespace ntlp;

	hostaddress source("10.38.1.191");

	mri_pathcoupled *mr = new mri_pathcoupled(
		source, 32, nr, 32, downstream);

	mr->set_protocol(nr.get_protocol());
	mr->set_sourceport(nr.get_port()-1);
	mr->set_destport(nr.get_port());

	return mr;
}


/**
 * Build an NTLP message containing a NATFW CREATE message.
 */
ntlp::APIMsg *build_api_create_msg(const appladdress &nr, bool final_hop) {
	using namespace ntlp;

	APIMsg *msg = new APIMsg();

	NetMsg *tmp = build_create_msg();
	nslpdata *data = new nslpdata(tmp->get_buffer(), tmp->get_size());
	delete tmp;

	uint32 nslp_id = 2;

	static uint32 counter = 0;
	sessionid *sid = new sessionid(0, 0, 0, counter++);
	mri *mr = build_mri(nr, true);

	bool adjacency_check = false;
	uint32 sii_handle = 0;
	tx_attr_t tx_attr;
	tx_attr.reliable = true;
	tx_attr.secure = false;
	tx_attr.final_hop = final_hop;
	uint16 ip_ttl = 62;
	uint16 ip_distance = 2;
	uint32 ghc = 1;
	
	msg->set_recvmessage(data, nslp_id, sid, mr, adjacency_check,
		sii_handle, tx_attr, ip_ttl, ip_distance, ghc);

	return msg;
}


/**
 * Build an NTLP message containing a NATFW CREATE(lt=0) message.
 */
ntlp::APIMsg *build_api_teardown_msg(const appladdress &nr, bool final_hop) {
	using namespace ntlp;

	APIMsg *msg = new APIMsg();

	NetMsg *tmp = build_create_msg(0xdeadbef0, 0);
	nslpdata *data = new nslpdata(tmp->get_buffer(), tmp->get_size());
	delete tmp;

	uint32 nslp_id = 2;

	static uint32 counter = 0;
	sessionid *sid = new sessionid(0, 0, 0, counter++);
	mri *mr = build_mri(nr, true);

	bool adjacency_check = false;
	uint32 sii_handle = 0;
	tx_attr_t tx_attr;
	tx_attr.reliable = true;
	tx_attr.secure = false;
	tx_attr.final_hop = final_hop;
	uint16 ip_ttl = 62;
	uint16 ip_distance = 2;
	uint32 ghc = 1;
	
	msg->set_recvmessage(data, nslp_id, sid, mr, adjacency_check,
		sii_handle, tx_attr, ip_ttl, ip_distance, ghc);

	return msg;
}


/**
 * Build an NTLP message containing a NATFW RESPONSE message.
 */
ntlp::APIMsg *build_api_response_msg(const appladdress &nr, bool final_hop) {
	using namespace ntlp;

	APIMsg *msg = new APIMsg();

	NetMsg *tmp = build_response_msg();
	nslpdata *data = new nslpdata(tmp->get_buffer(), tmp->get_size());
	delete tmp;

	uint32 nslp_id = 2;

	static uint32 counter = 0;
	sessionid *sid = new sessionid(0, 0, 0, counter++);
	mri *mr = build_mri(nr, false);

	bool adjacency_check = false;
	uint32 sii_handle = 0;
	tx_attr_t tx_attr;
	tx_attr.reliable = true;
	tx_attr.secure = false;
	tx_attr.final_hop = final_hop;
	uint16 ip_ttl = 62;
	uint16 ip_distance = 2;
	uint32 ghc = 1;
	
	msg->set_recvmessage(data, nslp_id, sid, mr, adjacency_check,
		sii_handle, tx_attr, ip_ttl, ip_distance, ghc);

	return msg;
}


void nop_dispatcher::send_message(msg::ntlp_msg *msg) throw () {
	DLog("nop_dispatcher", "sending " << *msg);

	ntlp::APIMsg *apimsg = mapper.create_api_msg(msg);
	apimsg->get_ghc();	// no-op to prevent compiler warning

	// we don't really send the message, this is just a dummy method!

	delete msg;
}


id_t nop_dispatcher::start_timer(const session *s, int seconds) throw () {
	DLog("nop_dispatcher", "starting timer: " << seconds << "s from now");
	return 0;
}


} // namespace natfw

// EOF
