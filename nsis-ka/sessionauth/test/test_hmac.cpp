/*
 * test_hmac - test the HMAC
 *
 * $Id:$
 * $HeadURL:$
 *
 */
#include <cppunit/TestCase.h>
#include <cppunit/extensions/HelperMacros.h>

#include "session_auth_object.h"
#include "../../qos-nslp/include/reservemsg.h"

#include "mri.h"
#include "mri_pc.h"
#include "sessionid.h"
#include "../src/pdu/nli.h"
#include "../src/pdu/data.h"
#include "nslpdata.h"

#include "hmac_keyregistry.h"
#include "openssl/evp.h"

using namespace nslp_auth;
using namespace qos_nslp;
using namespace ntlp;
using namespace protlib;

class SessionAuthObjectTest : public CppUnit::TestCase {

	CPPUNIT_TEST_SUITE( SessionAuthObjectTest );

	CPPUNIT_TEST( test_hmac );

	CPPUNIT_TEST_SUITE_END();

  public:
	void test_hmac();
};

CPPUNIT_TEST_SUITE_REGISTRATION( SessionAuthObjectTest );


void
SessionAuthObjectTest::test_hmac()
{
  // register at least two keys
  uint32 key_id1=0xabcd1234;
  const char *key1= "ASJDFHASLJDFKHRC)PQreiuqqnriuw$%§$&prunweiopurcnwo";
  uint32 key_id2=0xdeadbeef;
  const char *key2= "SDFLKJAHSETRKLWNFLKASUERPWIOEUTRNPOIEURVPQNUERPOUN";

  sign_key_registry.storeKey(key_id1, key1, strlen(key1), EVP_MD_nid(EVP_sha1()));
  sign_key_registry.storeKey(key_id2, key2, strlen(key2), EVP_MD_nid(EVP_sha1()));

  reservereq* new_res = new reservereq();

  packet_classifier* new_pc = new packet_classifier();
  new_pc->set_spi_flag();
  new_res->set_packet_classifier(new_pc);

  rsn* new_rsn = new rsn();
  new_res->set_rsn(new_rsn);
  
  rii* new_rii = new rii();
  new_rii->generate_new();
  new_rii->set_own(true);
  new_res->set_rii(new_rii);

  qspec::qspec_pdu* q_pdu = new qspec::qspec_pdu;
  qos_nslp::qspec_object* new_qspec = new qos_nslp::qspec_object(q_pdu);
  new_res->set_qspec(new_qspec);
  new_res->set_originator(true);

  hostaddress sourceaddress("1.2.3.4");
  hostaddress destaddress("4.3.2.1");
  mri* testmri= new mri_pathcoupled(sourceaddress, 32, destaddress, 32, true);
  sessionid* sid= new sessionid(0x1234,0xabcd,0xfedc,0x4321);
  nli* testnli= new nli();
  
  uint32 written_bytes= 0;
  NetMsg* nslpmsg= new NetMsg(new_res->get_serialized_size(IE::protocol_v1));
  
  // serialize NSLP PDU
  new_res->serialize(*nslpmsg,IE::protocol_v1,written_bytes);

  nslpdata* nslpsdu= new nslpdata(nslpmsg->get_buffer(), written_bytes);
  
  ntlp::data ntlpdata_pdu(testmri, sid, testnli, nslpsdu);


}
