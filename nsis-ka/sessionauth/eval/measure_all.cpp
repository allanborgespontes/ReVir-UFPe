#include "measure_all.h"
#include <iostream>
#include "../include/benchmark_journal.h"
#include <cstdlib>
#include "hmac_keyregistry.h"

namespace nslp_auth {
  using namespace std;

extern class benchmark_journal journal;

measure_all::measure_all(uint32 keyid_count, uint16 qspec_param_count) : keyid_count(keyid_count?keyid_count:1), cur_keyid(1), qspec_param_count(qspec_param_count), expected_size(0), data_pdu(create_NTLP_PDU(true, 1, (keyid_count<=1?true:false), qspec_param_count)), data_pdu_noauth(create_NTLP_PDU(false)), msg(new protlib::NetMsg(20000)) {
	expected_size = data_pdu->get_serialized_size(protlib::IE::protocol_v1);
	register_NTLP_ies();
	srand( time(NULL) );
	if(keyid_count>1) {
	  for(uint32 keyid = 1; keyid<=keyid_count;keyid++) {
	    // register shared key
	    int32 keywords[8];
	    for(uint8 i = 0; i<8; i++ ) keywords[i] = rand();
	    const char* key=(const char*) keywords;
	    nslp_auth::sign_key_registry.storeKey(keyid, key, sizeof(keywords), 2);
	  }
	}
}

measure_all::~measure_all() {
	delete data_pdu;
	delete data_pdu_noauth;
	delete msg;
}

void measure_all::performTask() {
	msg->set_pos(0);
	uint32 bytes_written = 0;
	MP(benchmark_journal::PRE_SERIALIZE_NTLP);
	data_pdu->serialize(*msg,protlib::IE::protocol_v1,bytes_written);
	MP(benchmark_journal::POST_SERIALIZE_NTLP);
	if( bytes_written != expected_size )
		cout<<"Error: unexpected pdu size"<<endl;


	uint32 bytes_read = 0;
	IEErrorList err;
	
	MP(benchmark_journal::PRE_HMAC_CREATE);
	auth_session_object::serialize_hmac(*msg, protlib::IE::protocol_v1);
	MP(benchmark_journal::POST_HMAC_CREATE);
	
	msg->set_pos(0);
	MP(benchmark_journal::PRE_DESERIALIZE_NTLP);
	IE* ie = ntlp::NTLP_IEManager::instance()->deserialize(*msg, ntlp::NTLP_IE::cat_known_pdu, protlib::IE::protocol_v1, err, bytes_read, false);
	MP(benchmark_journal::POST_DESERIALIZE_NTLP);
	delete ie;
	
	msg->set_pos(0);
	MP(benchmark_journal::PRE_HMAC_VERIFY);
	auth_session_object::check_hmac(*msg, protlib::IE::protocol_v1,false);
	MP(benchmark_journal::POST_HMAC_VERIFY);
/*
	const TLP_list::position_list_t *entry = msg->get_TLP_list()->get_list(ntlp::ntlp_pdu::tlplist_nslp_object_type,nslp_auth::auth_session_object::NSLP_TYPE);
	uint8 *in_hmac = msg->get_buffer() + entry->front() + ntlp::ntlp_object::getLengthFromTLVHeader(msg->get_buffer()+entry->front()) - 10;
	if(*in_hmac) *in_hmac = 0;
	else *in_hmac = 1;
*/
	msg->set_pos(0);
	MP(benchmark_journal::PRE_DESERIALIZE_NTLP_HMAC_FAIL);
	auth_session_object::check_hmac(*msg, protlib::IE::protocol_v1,true);
//	ie = ntlp::NTLP_IEManager::instance()->deserialize(*msg, ntlp::NTLP_IE::cat_known_pdu, protlib::IE::protocol_v1, err, bytes_read, false);
	MP(benchmark_journal::POST_DESERIALIZE_NTLP_HMAC_FAIL);
//	delete ie; // unnecessary

//	cout<<*msg->get_TLP_list();

	msg->set_pos(0);
	MP(benchmark_journal::PRE_SERIALIZE_NTLP_NOAUTH);
	data_pdu_noauth->serialize(*msg,protlib::IE::protocol_v1,bytes_written);
	MP(benchmark_journal::POST_SERIALIZE_NTLP_NOAUTH);

	msg->set_pos(0);
	bytes_read = 0;
	MP(benchmark_journal::PRE_DESERIALIZE_NTLP_NOAUTH);
	ie = ntlp::NTLP_IEManager::instance()->deserialize(*msg, ntlp::NTLP_IE::cat_known_pdu, protlib::IE::protocol_v1, err, bytes_read, false);
	MP(benchmark_journal::POST_DESERIALIZE_NTLP_NOAUTH);
	delete ie;
	
	MP(benchmark_journal::PRE_GET_KEY_FROM_REGISTRY);
	nslp_auth::sign_key_registry.getKey(cur_keyid);
	MP(benchmark_journal::POST_GET_KEY_FROM_REGISTRY);
	
	if(keyid_count>1) {
	  cur_keyid = rand();
	  cur_keyid %= keyid_count;
	  cur_keyid++;
	  delete data_pdu;
	  data_pdu = create_NTLP_PDU(true, cur_keyid, false, qspec_param_count);
	}	
}

}

