#include "utils.h"
#include "hmac_keyregistry.h"
#include "ntlp_pdu.h"
#include "reservemsg.h"
#include "mri.h"
#include "mri_pc.h"
#include "mri_le.h"
#include "mri_est.h"
#include "sessionid.h"
#include "nli.h"
#include "nslpdata.h"
#include "query.h"
#include "response.h"
#include "confirm.h"
#include "hello.h"
#include "ntlp_error.h"
#include "data.h"

#include "auth_attr_ie.h"

#include "benchmark_journal.h"

namespace nslp_auth {

using namespace qos_nslp;
using namespace ntlp;
using namespace protlib;
using namespace qspec;

extern class benchmark_journal journal;

auth_session_object* createAuthObj(uint32 keyid) {
		auth_attr_addr *a1 = new auth_attr_addr(DEST_ADDR,IPV4_ADDRESS);

		hostaddress ip("192.168.2.1");
		a1->set_ip(ip);		

		auth_attr_addr_spi *a2 = new auth_attr_addr_spi();
		a2->set_spi(2);		

		auth_attr_data *a3 = new auth_attr_data();
		a3->set_key_id(keyid);
		uint8* hmac = new uint8[auth_session_object::HMAC_size];
		// use memset to zero all bytes in the HMAC (this will be faster than an explicit for loop)
		memset(hmac, auth_session_object::HMAC_size, 0);
		a3->set_hmac(hmac, auth_session_object::HMAC_size);

		auth_attr_hmac_trans_id *a4 = new auth_attr_hmac_trans_id();
		a4->set_id(2);

		auth_attr_portlist *a5 = new auth_attr_portlist();
		a5->insert(4);
		a5->insert(5);
		a5->insert(6);

		auth_attr_string *a6 = new auth_attr_string();
		uchar *uc = new uchar[2];
		uc[0]='b';
		uc[1]=0;
		a6->set_string(uc,2);
		auth_attr_time *a7 = new auth_attr_time();
		a7->set_value(time(NULL)+6000);

		auth_nslp_object_list *a8 = new auth_nslp_object_list();
		a8->insert(known_nslp_object::AUTH_SESSION);

		auth_session_object *auth_obj = new auth_session_object();
		auth_obj->insert_attr(a1);
		auth_obj->insert_attr(a2);
		auth_obj->insert_attr(a3);
		auth_obj->insert_attr(a4);
		auth_obj->insert_attr(a5);
		auth_obj->insert_attr(a6);
		auth_obj->insert_attr(a7);
		auth_obj->insert_attr(a8);

		return auth_obj;
}

auth_session_object* createAuthObj() {
	return createAuthObj(1);
}

qos_nslp::reservereq* create_NSLP_PDU(bool with_sessionauth, uint32 keyid, bool storekey, uint16 qspec_param_count) {

	if (with_sessionauth && storekey)
	{
		// register shared key
		uint32 keywords[8]={0x12345678,0xffab3450,0xa123bfda,0x3efe3ead,0xa503412c,0xc4f20fe0,0x8af4cdef,0x0001abc};
		const char* key=(const char*) keywords;
		nslp_auth::sign_key_registry.storeKey(keyid, key, sizeof(keywords), 2);
	}

	// create QoS NSLP RESERVE PDU
	reservereq* new_res = new reservereq();

	if (with_sessionauth)
	{
		auth_session_object *p1 = createAuthObj(keyid);
		auth_nslp_object_list *nslp_obj_list =dynamic_cast<auth_nslp_object_list*>(p1->get_attr(AUTH_NSLP_OBJECT_LIST, 0));
		nslp_obj_list->insert(known_nslp_object::RII);
		nslp_obj_list->insert(known_nslp_object::RSN);
		nslp_obj_list->insert(known_nslp_object::PACKET_CLASSIFIER);
		nslp_obj_list->insert(known_nslp_object::QSPEC);
		new_res->set_authsession(p1);
	}
    
  packet_classifier* new_pc = new packet_classifier();
  new_pc->set_spi_flag();
  new_res->set_packet_classifier(new_pc);

  rsn* new_rsn = new rsn();
  new_rsn->new_instance();
  new_res->set_rsn(new_rsn);
  
  rii* new_rii = new rii();
  new_rii->generate_new();
  new_rii->set_own(true);
  new_res->set_rii(new_rii);

  qspec::qspec_pdu *q_pdu = new qspec_pdu(ms_resource_queries, 0, 15);

  qspec::qspec_object *o1 = new qspec::qspec_object(ot_qos_desired);
  o1->set_parameter(new phb_class((uint8) 42));
  q_pdu->set_object(o1);

  qspec::qspec_object *o2 = new qspec::qspec_object(ot_minimum_qos);

  for(uint16 param_id = 15; param_id < qspec_param_count + 15 && param_id < 4096; param_id++ ) {
    qspec::qspec_param_raw *tmp = new qspec::qspec_param_raw();
    tmp->set_parameter_id(param_id);
    o2->set_parameter(tmp);
  }
  q_pdu->set_object(o2);

  qos_nslp::qspec_object *new_qspec = new qos_nslp::qspec_object(q_pdu);
  new_res->set_qspec(new_qspec);
  new_res->set_originator(true);
 
  return new_res;
}

qos_nslp::reservereq* create_NSLP_PDU(bool with_sessionauth) {
  return create_NSLP_PDU(with_sessionauth, 1, true, 1);
}

ntlp::data* create_NTLP_PDU(bool with_sessionauth, uint32 keyid, bool storekey, uint16 qspec_param_count) {

	// create QoS NSLP RESERVE PDU
	reservereq* new_res = create_NSLP_PDU(with_sessionauth, keyid, storekey, qspec_param_count);
	uint32 written_bytes= 0;
	NetMsg* nslpmsg= new NetMsg(new_res->get_serialized_size(IE::protocol_v1));

 
	// serialize NSLP PDU
	//MP(benchmark_journal::PRE_SERIALIZE_NSLP); /// ** measurement point **
	// serialize NSLP PDU
	new_res->serialize(*nslpmsg,IE::protocol_v1,written_bytes);
	// this is then the SDU of NTLP
	nslpdata* nslpsdu= new nslpdata(nslpmsg->get_buffer(), written_bytes);
	//MP(benchmark_journal::POST_SERIALIZE_NSLP); /// ** measurement point **

	// create NTLP PDU
	hostaddress sourceaddress("1.2.3.4");
	hostaddress destaddress("4.3.2.1");
	mri* testmri= new mri_pathcoupled(sourceaddress, 32, destaddress, 32, true);
	for(uint16 i= 0; i<10000; i++){
	
	}
	sessionid* sid= new sessionid(0x1234,0xabcd,0xfedc,0x4321);
	netaddress na;
	nli* testnli= new nli(0,588,new peer_identity(),na);

	ntlp::data* ntlpdata_pdu = new ntlp::data(testmri, sid, testnli, nslpsdu);
	
	//cout<<"PDU length : " << ntlpdata_pdu->get_serialized_size(IE::protocol_v1)<<endl;
	return ntlpdata_pdu;

}

ntlp::data* create_NTLP_PDU(bool with_sessionauth) {
	return create_NTLP_PDU(with_sessionauth, 1, true, 1);
}

void register_NTLP_ies() {
  NTLP_IEManager::clear();
  NTLP_IE *tmp;
  tmp = new query;
  NTLP_IEManager::instance()->register_ie(tmp);
  tmp = new mri_pathcoupled;
  NTLP_IEManager::instance()->register_ie(tmp);
  tmp = new mri_looseend;
  NTLP_IEManager::instance()->register_ie(tmp);
  tmp = new mri_explicitsigtarget;
  NTLP_IEManager::instance()->register_ie(tmp);
  tmp = new nattraversal;
  NTLP_IEManager::instance()->register_ie(tmp);
  tmp = new ntlp::sessionid;
  NTLP_IEManager::instance()->register_ie(tmp);
  tmp = new nslpdata;
  NTLP_IEManager::instance()->register_ie(tmp);
  tmp = new nli;
  NTLP_IEManager::instance()->register_ie(tmp);
  tmp = new querycookie;
  NTLP_IEManager::instance()->register_ie(tmp);
  tmp = new stack_conf_data;
  NTLP_IEManager::instance()->register_ie(tmp);
  tmp = new stackprop;
  NTLP_IEManager::instance()->register_ie(tmp);
  tmp = new response;
  NTLP_IEManager::instance()->register_ie(tmp);
  tmp = new confirm;
  NTLP_IEManager::instance()->register_ie(tmp);
  tmp = new hello;
  NTLP_IEManager::instance()->register_ie(tmp);
  tmp = new error;
  NTLP_IEManager::instance()->register_ie(tmp); 
  tmp = new helloid;
  NTLP_IEManager::instance()->register_ie(tmp); 
  tmp = new respcookie;
  NTLP_IEManager::instance()->register_ie(tmp); 
  tmp = new data;
  NTLP_IEManager::instance()->register_ie(tmp);
  tmp = new errorobject;
  NTLP_IEManager::instance()->register_ie(tmp);
}

void register_auth_attr_ies() {
  AUTH_ATTR_IEManager::clear();
  AUTH_ATTR_IEManager::instance()->register_known_ies();
}

}
