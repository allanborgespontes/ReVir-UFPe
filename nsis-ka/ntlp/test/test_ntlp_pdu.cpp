#include <cppunit/TestCase.h>
#include <cppunit/extensions/HelperMacros.h>

#include "query.h"
#include "capability.h"
#include "mri.h"
#include "mri_pc.h"
#include "mri_le.h"
#include "mri_est.h"
#include "sessionid.h"
#include "nli.h"
#include "response.h"
#include "confirm.h"
#include "hello.h"
#include "ntlp_error.h"
#include "data.h"
#include "gist_conf.h"

using namespace ntlp;
using namespace protlib;


class NTLP_PDU_Test : public CppUnit::TestCase {

	CPPUNIT_TEST_SUITE( NTLP_PDU_Test );

	CPPUNIT_TEST( setUp );
	CPPUNIT_TEST( testReadWritePDU );
	
	CPPUNIT_TEST_SUITE_END();

  public:
	void setUp();

	void testReadWritePDU();

private:
	void register_NTLP_ies();
};

void 
NTLP_PDU_Test::setUp() {
	register_NTLP_ies();
	gconf.repository_init();
}

void 
NTLP_PDU_Test::testReadWritePDU() {

  hostaddress sourceaddress("1.2.3.4");
  hostaddress destaddress("4.3.2.1");
  mri* testmri= new mri_pathcoupled(sourceaddress, 32, destaddress, 32, true);
  sessionid* sid= new sessionid(0x1234,0xabcd,0xfedc,0x4321);
  netaddress na;
  nli* testnli= new nli(0,588,new peer_identity(),na);

  
  querycookie* myqc= new querycookie();


  capability cap(270,271,272,273);

  stackprop* sp= cap.query_stackprop(false);

  stack_conf_data* sc= cap.query_stackconf(false);

  nslpdata* test_nslpdata= new nslpdata;

  query* test_query_pdu= new query(testmri, sid, testnli, myqc, sp, sc, test_nslpdata);


  uint32 expected_size= test_query_pdu->get_serialized_size(IE::protocol_v1);
  CPPUNIT_ASSERT( expected_size > 0 );

  NetMsg* testbuf= new NetMsg(expected_size);

  // serialize
  uint32 written_bytes_ntlp= 0;
  test_query_pdu->serialize(*testbuf, IE::protocol_v1, written_bytes_ntlp);

  // check for expected length equals actual length
  CPPUNIT_ASSERT( expected_size == written_bytes_ntlp);

  // deserialize
  IEErrorList errlist;
  uint32 bytes_read_ntlp= 0;
  uint32 bytes_read_nslp= 0;
  testbuf->set_pos(0);

  IE* tmp_ie = NTLP_IEManager::instance()->deserialize(*testbuf, NTLP_IE::cat_known_pdu, IE::protocol_v1, errlist, bytes_read_ntlp, false);

  CPPUNIT_ASSERT( tmp_ie != NULL );
  query* rcv_query_pdu= dynamic_cast<query*>(tmp_ie);
  CPPUNIT_ASSERT( rcv_query_pdu );
  CPPUNIT_ASSERT( *test_query_pdu == *rcv_query_pdu );
  CPPUNIT_ASSERT( errlist.is_empty() == true );  
  CPPUNIT_ASSERT( written_bytes_ntlp == bytes_read_ntlp );

  return;

  delete test_query_pdu;
  delete testbuf;
  delete myqc;
  delete testmri;
  delete sid;
  delete testnli;
}


void 
NTLP_PDU_Test::register_NTLP_ies() {
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



CPPUNIT_TEST_SUITE_REGISTRATION( NTLP_PDU_Test );

