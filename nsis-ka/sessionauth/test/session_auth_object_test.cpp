/*
 * session_auth_object.cpp - Test SESSION_AUTH_OBJECTs
 *
 */
#define _DEBUG_IE

#include <cppunit/TestCase.h>
#include <cppunit/extensions/HelperMacros.h>

#include "session_auth_object.h"
#include "attr_test.h"
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
#include "qos_nslp.h"

using namespace nslp_auth;
using namespace qos_nslp;
using namespace ntlp;
using namespace protlib;
using namespace qspec;

const string psk1("12345678901234567890123456789012");
const string psk2("abcdefghijklmnopqrstuvwxyz-.,+%$");
const string psk3("xabcdefghijklmnopqrstuvwxyz.,+$x");
nslp_auth::key_id_t pskey_id1= 0x1;
nslp_auth::key_id_t pskey_id2= 0x2;
nslp_auth::key_id_t pskey_id3= 0x1a1a1b1bL;

class SessionAuthObjectTest : public CppUnit::TestCase {

	CPPUNIT_TEST_SUITE( SessionAuthObjectTest );

	CPPUNIT_TEST( testComparisonObj );
	CPPUNIT_TEST( testComparison );
	CPPUNIT_TEST( testCopyingObj );
	CPPUNIT_TEST( testCopying );
	CPPUNIT_TEST( testSerialize );
	CPPUNIT_TEST( testDeserialize );
	CPPUNIT_TEST( testReadWrite );
	CPPUNIT_TEST( testReadWriteWithObjects );
	CPPUNIT_TEST( testReadWriteWithPDU );
	
	CPPUNIT_TEST_SUITE_END();

  public:
	void setUp();
	void tearDown();

	void testComparisonObj();
	void testComparison();
	void testCopyingObj();
	void testCopying();
	void testSerialize();
	void testDeserialize();
	void testReadWrite();
	void testReadWriteWithObjects();
	void testReadWriteWithPDU();
	
        void insert_all_attr(session_auth_object *obj, key_id_t key_id);
	void register_NTLP_ies();

  private:
	void testComparisonWith(GenericAttributeTest *test);
	void testCopyingWith(GenericAttributeTest *test);


};

CPPUNIT_TEST_SUITE_REGISTRATION( SessionAuthObjectTest );


// executed before each test method
void SessionAuthObjectTest::setUp() {
	AUTH_ATTR_IEManager::clear();
	register_NTLP_ies();
	NSLP_IEManager::clear();
	qos_nslp::register_ies();
	AUTH_ATTR_IEManager::register_known_ies();

	// register keys
	nslp_auth::sign_key_registry.storeKey(pskey_id1, psk1.c_str(), psk1.length(), 2);
	nslp_auth::sign_key_registry.storeKey(pskey_id2, psk2.c_str(), psk2.length(), 2);
	nslp_auth::sign_key_registry.storeKey(pskey_id3, psk3.c_str(), psk3.length(), 2);
}

void SessionAuthObjectTest::tearDown() {
	// unregister keys
	nslp_auth::sign_key_registry.deleteKey(pskey_id1);
	nslp_auth::sign_key_registry.deleteKey(pskey_id2);
	nslp_auth::sign_key_registry.deleteKey(pskey_id3);
}


void SessionAuthObjectTest::testComparison() {
	GenericAttributeTest *test = new AuthAttrAddrTest();
	testComparisonWith(test);
	delete test;
	test = new AuthAttrAddrSPITest();
	testComparisonWith(test);
	delete test;
	test = new AuthAttrDataTest();
	testComparisonWith(test);
	delete test;
	test = new AuthAttrHmacTransIdTest();
	testComparisonWith(test);
	delete test;
	test = new AuthAttrPortlistTest();
	testComparisonWith(test);
	delete test;
	test = new AuthAttrStringTest();
	testComparisonWith(test);
	delete test;
	test = new AuthAttrTimeTest();
	testComparisonWith(test);
	delete test;
	test = new AuthNslpObjectListTest();
	testComparisonWith(test);
	delete test;
}

void SessionAuthObjectTest::testComparisonObj() {
	session_auth_object *p1 = new session_auth_object();
	session_auth_object *p2 = new session_auth_object(known_nslp_object::ignore, known_nslp_object::SESSION_AUTH_OBJECT);

	CPPUNIT_ASSERT( *p1 != *p2 );

	p1->set_ignore();

	CPPUNIT_ASSERT( *p1 == *p1 );
	CPPUNIT_ASSERT( *p2 == *p2 );
	CPPUNIT_ASSERT( *p1 == *p2 );

	p2->set_mandatory();

	CPPUNIT_ASSERT( *p1 != *p2 );

	delete p1; 
	delete p2;
}

void SessionAuthObjectTest::testComparisonWith(GenericAttributeTest *test) {
	session_auth_object *p1 = new session_auth_object();
	session_auth_object *p2 = new session_auth_object();
	
	insert_all_attr(p1, 1);

	CPPUNIT_ASSERT( *p1 != *p2 );

	insert_all_attr(p2, 1);

	CPPUNIT_ASSERT( *p1 == *p1 );
	CPPUNIT_ASSERT( *p2 == *p2 );
	CPPUNIT_ASSERT( *p1 == *p2 );

	auth_attr *attr1,*attr2;

	attr1 = test->createInstance2();

	attr2 = p2->get_attr(attr1->get_xtype(), attr1->get_subtype());
	test->mutate1(attr2);
	
	CPPUNIT_ASSERT( *p1 != *p2 );

	attr2 = p1->get_attr(attr1->get_xtype(), attr1->get_subtype());
	test->mutate1(attr2);
	
	CPPUNIT_ASSERT( *p1 == *p2 );

	delete attr1;	
	delete p1; 
	delete p2;
}

void SessionAuthObjectTest::insert_all_attr(session_auth_object *obj, key_id_t key_id){
	GenericAttributeTest *test = new AuthAttrAddrTest();
	obj->insert_attr(test->createInstance2());
	delete test;
	test = new AuthAttrAddrSPITest();
	obj->insert_attr(test->createInstance2());
	delete test;
	test = new AuthAttrDataTest();
	auth_attr_data *attr_dataptr= static_cast<auth_attr_data*>(test->createInstance2());
	attr_dataptr->set_key_id(key_id);
	obj->insert_attr(attr_dataptr);
	delete test;
	test = new AuthAttrHmacTransIdTest();
	obj->insert_attr(test->createInstance2());
	delete test;
	test = new AuthAttrPortlistTest();
	obj->insert_attr(test->createInstance2());
	delete test;
	test = new AuthAttrStringTest();
	obj->insert_attr(test->createInstance2());
	delete test;
	test = new AuthAttrTimeTest();
	obj->insert_attr(test->createInstance2());
	delete test;
	test = new AuthNslpObjectListTest();
	obj->insert_attr(test->createInstance2());
	delete test;
}

void SessionAuthObjectTest::register_NTLP_ies() {
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

void SessionAuthObjectTest::testCopyingObj() {
	session_auth_object *p1 = new session_auth_object();

	insert_all_attr(p1, pskey_id1);

	session_auth_object *p2 = p1->copy();

	CPPUNIT_ASSERT( *p1 == *p2 );

	// now make sure that this was a deep copy

	CPPUNIT_ASSERT( p1->get_vec_length() == p2->get_vec_length() );
	CPPUNIT_ASSERT( p1 != p2 );

	delete p1;
	delete p2;
}

void SessionAuthObjectTest::testCopyingWith(GenericAttributeTest *test) {
	session_auth_object *p1 = new session_auth_object();

	insert_all_attr(p1, pskey_id1);

	session_auth_object *p2 = p1->copy();

	CPPUNIT_ASSERT( *p1 == *p2 );

	// now make sure that this was a deep copy

	CPPUNIT_ASSERT( p1->get_vec_length() == p2->get_vec_length() );
	CPPUNIT_ASSERT( p1 != p2 );
	
	auth_attr *attr1,*attr2;

	attr1 = test->createInstance2();

	attr2 = p2->get_attr(attr1->get_xtype(), attr1->get_subtype());
	test->mutate1(attr2);

	CPPUNIT_ASSERT( *p1 != *p2 );

	delete p1;
	delete p2;
}

void SessionAuthObjectTest::testCopying() {
	GenericAttributeTest *test = new AuthAttrAddrTest();
	testCopyingWith(test);
	delete test;
	test = new AuthAttrAddrSPITest();
	testCopyingWith(test);
	delete test;
	test = new AuthAttrDataTest();
	testCopyingWith(test);
	delete test;
	test = new AuthAttrHmacTransIdTest();
	testCopyingWith(test);
	delete test;
	test = new AuthAttrPortlistTest();
	testCopyingWith(test);
	delete test;
	test = new AuthAttrStringTest();
	testCopyingWith(test);
	delete test;
	test = new AuthAttrTimeTest();
	testCopyingWith(test);
	delete test;
	test = new AuthNslpObjectListTest();
	testCopyingWith(test);
	delete test;
}


void SessionAuthObjectTest::testSerialize() {
	/*
	 * Error: action is larger than 2 bit.
	 */
    session_auth_object *p1 = new session_auth_object((known_nslp_object::action_type_t)4, known_nslp_object::SESSION_AUTH_OBJECT);
    insert_all_attr(p1, pskey_id1);
	
	CPPUNIT_ASSERT( p1->get_type() == nslp_auth::session_auth_object::NSLP_TYPE);
	// expect that check fails
	CPPUNIT_ASSERT( p1->check() ==  false);

	NetMsg msg1(p1->get_serialized_size(protlib::IE::protocol_v1)+(nslp_auth::session_auth_object::HEADER_LENGTH));
	uint32 bytes_written1 = 42;

	CPPUNIT_ASSERT_THROW(
		p1->serialize(msg1, IE::protocol_v1, bytes_written1), IEError);

	CPPUNIT_ASSERT( bytes_written1 == 0 );

	delete p1;

	/*
	 * Error: object type is set wrong.
	 */
	p1 = new session_auth_object(known_nslp_object::refresh, (known_nslp_object::type_t)(nslp_auth::session_auth_object::NSLP_TYPE+1));
	insert_all_attr(p1, pskey_id1);
	
	CPPUNIT_ASSERT( p1->get_type() == nslp_auth::session_auth_object::NSLP_TYPE+1);
	// expect that check fails
	CPPUNIT_ASSERT( p1->check() ==  false);

	NetMsg msg4(p1->get_serialized_size(protlib::IE::protocol_v1)+(nslp_auth::session_auth_object::HEADER_LENGTH));
	bytes_written1 = 42;

	CPPUNIT_ASSERT_THROW(
		p1->serialize(msg4, IE::protocol_v1, bytes_written1), IEError);

	CPPUNIT_ASSERT( bytes_written1 == 0 );

	delete p1;

	/*
	 * Error: Not enough space in NetMsg.
	 */
	session_auth_object *p2 = new session_auth_object();
	insert_all_attr(p2, pskey_id1);
	
	NetMsg msg2(2);
	uint32 bytes_written2 = 42;

	CPPUNIT_ASSERT_THROW(
		p2->serialize(msg2, IE::protocol_v1, bytes_written2), IEError);

	CPPUNIT_ASSERT( bytes_written2 == 0 );

	delete p2;


	/*
	 * Error: Empty AUTH SESSION OBJECT (without attributes).
	 */
	session_auth_object *p3 = new session_auth_object();

	NetMsg msg3(100);
	uint32 bytes_written3 = 42;

	CPPUNIT_ASSERT_THROW(
		p3->serialize(msg3, IE::protocol_v1, bytes_written3), IEError
	);

	delete p3;

	// for a serialization without errors see testReadWrite()
}


void SessionAuthObjectTest::testDeserialize() {
	AUTH_ATTR_IEManager::clear();
	AUTH_ATTR_IEManager::register_known_ies();

	AUTH_ATTR_IEManager *iem = AUTH_ATTR_IEManager::instance();

	session_auth_object *p1 = new session_auth_object();

	/*
	 * Test for empty NetMsg;
	 */
	NetMsg msg1(1);
	IEErrorList errlist1;
	uint32 bytes_read1;


	IE *ie1 = iem->deserialize_auth_attr(msg1, IE::protocol_v1, errlist1, bytes_read1, false);

	CPPUNIT_ASSERT( ie1 == NULL );
	CPPUNIT_ASSERT( errlist1.is_empty() == false );

	IEError *err1 = errlist1.get();
	CPPUNIT_ASSERT( dynamic_cast<IEMsgTooShort *>(err1) != NULL );
	delete err1;

	CPPUNIT_ASSERT( errlist1.is_empty() == true );


	/*
	 * Test for AUTH SESSION OBJECT without attributes.
	 */
	NetMsg msg2(4);
	msg2.encode32(0x800A0000);
	msg2.set_pos(0);

	IEErrorList errlist2;
	uint32 bytes_read2;

	IE *ie2 = p1->deserialize(msg2, IE::protocol_v1, errlist2, bytes_read2, true);

	CPPUNIT_ASSERT( ie2 != NULL );
	CPPUNIT_ASSERT( errlist2.is_empty() == true );

}

void SessionAuthObjectTest::testReadWrite() {

    session_auth_object *p1 = new session_auth_object();

    insert_all_attr(p1, pskey_id1);
			
	session_auth_object *p2 = new session_auth_object();

	uint32 expected_size= p1->get_serialized_size(IE::protocol_v1);
	NetMsg msg(expected_size+20);
	uint32 bytes_written = 0;

	msg.encode32(0x4e04bda5); // GIST magic
	msg.encode32(0x0100001A); // NTLP HEADER  = version | Gist Hops | Message Length 
	msg.encode32(0x12340000); // NSLPID & offcut of the NSLP HEADER
	msg.encode16(0x0008); // NSLPDATA_Object = Action & Type
	msg.encode16((expected_size/4)+1); // NSLPDATA_Object = RSV & length (in 32 bit)
	msg.encode32(0x01000000); // NSLP HEADER ?
	p1->serialize(msg, IE::protocol_v1, bytes_written);
	
//	std::cout <<"bytes_written : "<<bytes_written<<"\nexpected_size :"<<expected_size<<endl;
	
	CPPUNIT_ASSERT( bytes_written == expected_size );


	/*
	 * Now read the serialized object.
	 */
	AUTH_ATTR_IEManager::clear();
	AUTH_ATTR_IEManager::register_known_ies();
	IEErrorList errlist;
	uint32 bytes_read= 0;

	msg.to_start();
	msg.set_pos_r(4); // skip magic number
	bool serialize_hmac_correct=session_auth_object::serialize_hmac(msg, IE::protocol_v1, true);
	
	CPPUNIT_ASSERT( serialize_hmac_correct==true );

//	std::cout << msg.hexdump(cout) << '\n';

	bool hmac_correct=session_auth_object::check_hmac(msg, IE::protocol_v1);
	
	CPPUNIT_ASSERT( hmac_correct==true );

	msg.set_pos(20); // skip faked NTLP header
	p2->deserialize(msg, IE::protocol_v1, errlist, bytes_read, false);

	CPPUNIT_ASSERT( errlist.is_empty() );
	CPPUNIT_ASSERT( p2 != NULL );
	CPPUNIT_ASSERT( bytes_read == bytes_written );

	// p1 != p2 , because hmac is in p2, but not in p1 because it is calced in ntlp
	auth_attr_data *auth_data_p1 = dynamic_cast<auth_attr_data*>(p1->get_attr(AUTHENTICATION_DATA, 0));
	auth_attr_data *auth_data_p2 = dynamic_cast<auth_attr_data*>(p2->get_attr(AUTHENTICATION_DATA, 0));
	auth_data_p1->set_hmac(auth_data_p2->get_hmac(),auth_data_p2->get_hmac_len());
	
	CPPUNIT_ASSERT( *p1 == *p2 );

	// serializing p2 again should yield an identical bytes stream
	NetMsg msg_cmp(expected_size+20);
	msg_cmp.encode32(0x4e04bda5); // GIST magic
	msg_cmp.encode32(0x0100001A); // NSLP HEADER  = version | Gist Hops | Message Length 
	msg_cmp.encode32(0x12340000); // NSLPID & offcut of the NSLP HEADER
	msg_cmp.encode16(0x0008); // NSLPDATA_Object = Action & Type
	msg_cmp.encode16((expected_size/4)+1); // NSLPDATA_Object = RSV & length (in 32 bit)
	msg_cmp.encode32(0x01000000); // NSLP HEADER ?

	p2->serialize(msg_cmp, IE::protocol_v1, bytes_written);
	
	// byte stream the same now? 
	CPPUNIT_ASSERT( memcmp(msg.get_buffer(), msg_cmp.get_buffer(), bytes_written+20 ) == 0 );
	
	uint32 auth_pos = *(msg.get_TLP_list()->get_list(ntlp_pdu::tlplist_nslp_object_type, session_auth_object::NSLP_TYPE)->begin());
	uint32 hmac_pos = auth_pos + p1->get_serialized_size(IE::protocol_v1) - session_auth_object::HMAC_size;

	// set hmac wrong
	msg.get_buffer()[hmac_pos+8]=0;
	
	msg.to_start();
	hmac_correct=session_auth_object::check_hmac(msg, IE::protocol_v1);
	
	CPPUNIT_ASSERT( hmac_correct==false );
	
	delete p1;
	delete p2;


	session_auth_object *sauth_obj_token= new session_auth_object;
	auth_attr_string *token_auth_ent_id = new auth_attr_string(AUTH_ENT_ID, X509_V3_CERT);
	token_auth_ent_id->set_string("AAA@tm.kit.edu");
	sauth_obj_token->insert_attr(token_auth_ent_id);

	appladdress src("141.3.70.4","udp",80);
	auth_attr_addr *token_src_addr = new auth_attr_addr(SOURCE_ADDR,IPV4_ADDRESS);
	token_src_addr->set_ip(src);
	sauth_obj_token->insert_attr(token_src_addr);

	appladdress dst("141.3.70.5","udp",80);
	auth_attr_addr *token_dst_addr = new auth_attr_addr(DEST_ADDR,IPV4_ADDRESS);
	token_dst_addr->set_ip(dst);
	sauth_obj_token->insert_attr(token_dst_addr);

	auth_attr_time *token_timestamp_attr = new auth_attr_time();
	token_timestamp_attr->set_value(time(NULL)+6000);
	sauth_obj_token->insert_attr(token_timestamp_attr);

	auth_attr_data *token_auth_dataattr = new auth_attr_data();
	string auth_token("this is a test token: X-C;.VASDG:FATRR!GASD2A4SRQWTG2DSRG5KEW4T99ZH$=O§RASGFSR%ERGAF");
	token_auth_dataattr->set_auth_data(reinterpret_cast<const uchar*>(auth_token.c_str()),auth_token.length());
	sauth_obj_token->insert_attr(token_auth_dataattr);

	expected_size= sauth_obj_token->get_serialized_size(IE::protocol_v1);
	NetMsg msg_attr_data(expected_size+20);
	bytes_written = 0;
	
	msg.encode32(0x4e04bda5); // GIST magic
	msg.encode32(0x0100001A); // NTLP HEADER  = version | Gist Hops | Message Length 
	msg.encode32(0x12340000); // NSLPID & offcut of the NSLP HEADER
	msg.encode16(0x0008); // NSLPDATA_Object = Action & Type
	msg.encode16((expected_size/4)+1); // NSLPDATA_Object = RSV & length (in 32 bit)
	msg.encode32(0x01000000); // NSLP HEADER ?
	sauth_obj_token->serialize(msg_attr_data, IE::protocol_v1, bytes_written);
	
	//std::cout <<"bytes_written : "<<bytes_written<<"\nexpected_size :"<<expected_size<<endl;
	
	CPPUNIT_ASSERT( bytes_written == expected_size );
	
	delete sauth_obj_token;
}

void SessionAuthObjectTest::testReadWriteWithObjects() {

    session_auth_object *p1 = new session_auth_object();

    insert_all_attr(p1, pskey_id1);
	
		
	session_auth_object *p2 = new session_auth_object();
	session_auth_object *p3 = new session_auth_object();
	
	const uint32 expected_size= p1->get_serialized_size(IE::protocol_v1);
	
	NetMsg msg(2*expected_size+16);
	uint32 bytes_written1 = 42;
	uint32 bytes_written2 = 42;

	msg.encode32(0x01000032); // NSLP HEADER
	msg.encode32(0x12340000); // NSLPID & offcut of the NSLP HEADER
	msg.encode16(0x0008); // NSLPDATA_Object = Action & Type
	msg.encode16((2*expected_size/4)+1); // NSLPDATA_Object = RSV & length (in 32 bit)
	msg.encode32(0x01000000); // NSLP HEADER ?

	p1->serialize(msg, IE::protocol_v1, bytes_written1);
	p1->serialize(msg, IE::protocol_v1, bytes_written2);
	
	CPPUNIT_ASSERT( bytes_written1 == expected_size );
	CPPUNIT_ASSERT( bytes_written2 == expected_size );


	/*
	 * Now read the serialized object.
	 */
	AUTH_ATTR_IEManager::clear();
	AUTH_ATTR_IEManager::register_known_ies();

	IEErrorList errlist1;
	uint32 bytes_read1= 0;
	IEErrorList errlist2;
	uint32 bytes_read2= 0;

//	std::cout << msg.hexdump(cout) << '\n';

	
	msg.to_start();
	bool serialize_hmac_correct=session_auth_object::serialize_hmac(msg, IE::protocol_v1, true);
	
	CPPUNIT_ASSERT( serialize_hmac_correct==true );

	bool hmac_correct=session_auth_object::check_hmac(msg, IE::protocol_v1);
	
	CPPUNIT_ASSERT( hmac_correct==true );

	msg.set_pos(16); // skip faked NTLP header
	p2->deserialize(msg, IE::protocol_v1, errlist1, bytes_read1, false);
	p3->deserialize(msg, IE::protocol_v1, errlist2, bytes_read2, false);
	
	CPPUNIT_ASSERT( errlist1.is_empty() );
	CPPUNIT_ASSERT( p2 != NULL );
	CPPUNIT_ASSERT( bytes_read1 == bytes_written1 );
	CPPUNIT_ASSERT( errlist2.is_empty() );
	CPPUNIT_ASSERT( p3 != NULL );
	CPPUNIT_ASSERT( bytes_read2 == bytes_written2 );

/*	cout<<endl<<"p1: "<<endl;
	p1->print(cout,1,2);
	cout<<endl<<"p2: "<<endl;
	p2->print(cout,1,2);*/

	// p1 != p2 , because hmac is in p2, but not in p1 because it will be set in ntlp
	auth_attr_data *auth_data_p1 = dynamic_cast<auth_attr_data*>(p1->get_attr(AUTHENTICATION_DATA, 0));
	auth_attr_data *auth_data_p2 = dynamic_cast<auth_attr_data*>(p2->get_attr(AUTHENTICATION_DATA, 0));
	auth_data_p1->set_hmac(auth_data_p2->get_hmac(),auth_data_p2->get_hmac_len());

	CPPUNIT_ASSERT( *p1 == *p2 );
	CPPUNIT_ASSERT( *p2 == *p3 );
	CPPUNIT_ASSERT( *p3 == *p2 );

	// serializing p2 again should yield an identical bytes stream
	NetMsg msg_cmp(bytes_written1+bytes_written2+16);
	msg_cmp.encode32(0x01000032); // NSLP HEADER
	msg_cmp.encode32(0x12340000); // NSLPID & offcut of the NSLP HEADER
	msg_cmp.encode16(0x0008); // NSLPDATA_Object = Action & Type
	msg_cmp.encode16((2*expected_size/4)+1); // NSLPDATA_Object = RSV & length (in 32 bit)
	msg_cmp.encode32(0x01000000); // NSLP HEADER ?

	p2->serialize(msg_cmp, IE::protocol_v1, bytes_written1);
	p3->serialize(msg_cmp, IE::protocol_v1, bytes_written2);
	
	// std::cout << msg_cmp.hexdump(cout) << '\n';

	// byte stream the same now? 
	CPPUNIT_ASSERT( memcmp(msg.get_buffer(), msg_cmp.get_buffer(), bytes_written1+bytes_written2+8 ) == 0 );
	
	delete p1;
	delete p2;
	delete p3;
}

void SessionAuthObjectTest::testReadWriteWithPDU() {

	DLog("testReadWriteWithPDU","start");

	const string pskx("xxxxabcdefghijklmnopqrstuvwxyz-.,+%$");
	nslp_auth::key_id_t pskey_idx= 0x2a2a1f1fL;
	nslp_auth::sign_key_registry.storeKey(pskey_idx, pskx.c_str(), pskx.length(), 2);
	    
	rsn* new_rsn = new rsn();
  
	rii* new_rii = new rii();
	new_rii->generate_new();
	new_rii->set_own(true);

	packet_classifier* new_pc = new packet_classifier();
	new_pc->set_spi_flag();


	// qspec
	qspec::qspec_pdu *q_pdu = new qspec_pdu(ms_resource_queries, 0, 15);

	qspec::qspec_object *o1 = new qspec::qspec_object(ot_qos_desired);
	o1->set_parameter(new phb_class((uint8) 42));
	q_pdu->set_object(o1);

	qspec::qspec_object *o2 = new qspec::qspec_object(ot_minimum_qos);
	o2->set_parameter(new t_mod(2048000.0, 576.0, 100000.0, 40));
	q_pdu->set_object(o2);

	qos_nslp::qspec_object *new_qspec = new qos_nslp::qspec_object(q_pdu);

	// MRI
	hostaddress sourceaddress("1.2.3.4");
	hostaddress destaddress("4.3.2.1");
	mri* testmri= new mri_pathcoupled(sourceaddress, 32, destaddress, 32, true);

	// session id
	sessionid* sid= new sessionid(0x1234,0xabcd,0xfedc,0x4321);

	netaddress na;
	nli* testnli= new nli(0,588,new peer_identity(),na);

	reservereq* new_res = new reservereq(new_rsn,new_pc,new_rii);
	new_res->set_qspec(new_qspec);
	DLog("testReadWriteWithPDU","qspec object size=" << new_qspec->get_serialized_size(IE::protocol_v1));

	new_res->set_originator(true);

	session_auth_object *s_auth_obj = new session_auth_object();
	insert_all_attr(s_auth_obj, pskey_id2);

	auth_nslp_object_list *nslp_obj_list = dynamic_cast<auth_nslp_object_list*>(s_auth_obj->get_attr(AUTH_NSLP_OBJECT_LIST, 0));
	nslp_obj_list->insert(known_nslp_object::RII);
	nslp_obj_list->insert(known_nslp_object::RSN);
	nslp_obj_list->insert(known_nslp_object::PACKET_CLASSIFIER);
	nslp_obj_list->insert(known_nslp_object::QSPEC);

	new_res->set_sessionauth(s_auth_obj);

	DLog("testReadWriteWithPDU","sessionauth object size=" << s_auth_obj->get_serialized_size(IE::protocol_v1));

	NetMsg* serauthobj= new NetMsg(s_auth_obj->get_serialized_size(IE::protocol_v1));
	uint32 written_bytes_sauth= 0;
	s_auth_obj->serialize(*serauthobj,IE::protocol_v1,written_bytes_sauth);
	DLog("testReadWriteWithPDU","serialized sessionauth object size=" << written_bytes_sauth);

	IEErrorList errlist;
	uint32 bytes_read_sauth= 0;
	serauthobj->to_start();

	session_auth_object *s_auth_obj_rcv = new session_auth_object();	
	s_auth_obj_rcv->deserialize(*serauthobj, IE::protocol_v1, errlist, bytes_read_sauth, false);
	DLog("testReadWriteWithPDU","deserialized sessionauth object size=" << bytes_read_sauth);
	CPPUNIT_ASSERT( written_bytes_sauth == bytes_read_sauth );
	CPPUNIT_ASSERT( bytes_read_sauth == s_auth_obj_rcv->get_serialized_size(IE::protocol_v1) );

	uint32 written_bytes_ntlp= 0;
	uint32 written_bytes_nslp= 0;
	NetMsg* nslpmsg= new NetMsg(new_res->get_serialized_size(IE::protocol_v1));

	DLog("testReadWriteWithPDU","before serialize, NSLP PDU size=" << new_res->get_serialized_size(IE::protocol_v1));
 
	// serialize NSLP PDU
	new_res->serialize(*nslpmsg,IE::protocol_v1,written_bytes_nslp);

	DLog("testReadWriteWithPDU","after serialize, NSLP PDU written bytes=" << written_bytes_nslp); 

	// NSLP data object
	nslpdata* nslpsdu= new nslpdata(nslpmsg->get_buffer(), written_bytes_nslp);
	CPPUNIT_ASSERT( written_bytes_nslp == nslpmsg->get_size() );

	ntlp::data ntlpdata_pdu(testmri, sid, testnli, nslpsdu);
  
	NetMsg msg(ntlpdata_pdu.get_serialized_size(IE::protocol_v1));

	ntlpdata_pdu.serialize(msg,IE::protocol_v1,written_bytes_ntlp);

	CPPUNIT_ASSERT( ntlpdata_pdu.get_serialized_size(IE::protocol_v1) == written_bytes_ntlp);

	uint32 bytes_read_ntlp= 0;
	msg.to_start();

	DLog("testReadWriteWithPDU","before deserialize NTLP PDU");

	IE* tmp_ie = NTLP_IEManager::instance()->deserialize(msg, NTLP_IE::cat_known_pdu, IE::protocol_v1, errlist, bytes_read_ntlp, false);

	DLog("testReadWriteWithPDU","after deserialize NTLP PDU");

	CPPUNIT_ASSERT( tmp_ie != NULL );
	CPPUNIT_ASSERT( errlist.is_empty() == true );  
	CPPUNIT_ASSERT( written_bytes_ntlp == bytes_read_ntlp );
	// is TLP_list filled?
	CPPUNIT_ASSERT(msg.get_TLP_list());
	CPPUNIT_ASSERT(msg.get_TLP_list()->get_list(ntlp_pdu::tlplist_ntlp_object_type,known_ntlp_object::Nslpdata) != NULL);
	CPPUNIT_ASSERT(msg.get_TLP_list()->get_list(ntlp_pdu::tlplist_ntlp_object_type,known_ntlp_object::Nslpdata)->empty() == false);
	
	list<uint32>::const_iterator list_it = msg.get_TLP_list()->get_list(ntlp_pdu::tlplist_ntlp_object_type,known_ntlp_object::Nslpdata)->begin();
	uint32 nslpdata_pos = *list_it;
	msg.set_pos(nslpdata_pos);

	DLog("testReadWriteWithPDU","before deserialize NSLP PDU");
	uint32 bytes_read_nslp= 0;
	IE* tmp_ie2 = NSLP_IEManager::instance()->deserialize(msg, NSLP_IE::cat_known_nslp_pdu, IE::protocol_v1, errlist, bytes_read_nslp, false);

	DLog("testReadWriteWithPDU","after deserialize NSLP PDU - bytes read: " << bytes_read_nslp << (errlist.is_empty() ? ", ok" : ", ERRORs occured"));

        while ( errlist.is_empty() == false )
	{
		IEError* ie_err= errlist.get();
		if (ie_err)
			ERRLog("testReadWriteWithPDU", "parsing error occured: " << ie_err->what());
	}

	CPPUNIT_ASSERT( tmp_ie2 != NULL );
	reservereq* rcvd_res= dynamic_cast<reservereq*>(tmp_ie2);
	CPPUNIT_ASSERT( rcvd_res != NULL );

        CPPUNIT_ASSERT( written_bytes_nslp == bytes_read_nslp );  

	DLog("testReadWriteWithPDU()","nslp data position: " << nslpdata_pos << ", bytes read: " << bytes_read_nslp << " " << *(msg.get_TLP_list()) );

	CPPUNIT_ASSERT( *new_res == *rcvd_res );

	PDUSyntaxError* ieerr = NULL;
	while( (ieerr = dynamic_cast<PDUSyntaxError*>(errlist.get())) )
		cerr <<"ERR : "<< ieerr->getstr() << " " << ieerr->message <<endl;
	
	CPPUNIT_ASSERT( errlist.is_empty() == true );  
	CPPUNIT_ASSERT(msg.get_TLP_list()->get_list(ntlp_pdu::tlplist_nslp_object_type,session_auth_object::NSLP_TYPE) != NULL);

	uint32 auth_pos = *(msg.get_TLP_list()->get_list(ntlp_pdu::tlplist_nslp_object_type, session_auth_object::NSLP_TYPE)->begin());
	uint32 hmac_pos = auth_pos + s_auth_obj->get_serialized_size(IE::protocol_v1) - session_auth_object::HMAC_size;

	// set hmac wrong
	msg.get_buffer()[hmac_pos+8]=0;

	msg.to_start();
	bool hmac_correct = session_auth_object::check_hmac(msg, IE::protocol_v1);
  
	CPPUNIT_ASSERT( hmac_correct==false );

	delete tmp_ie;
	delete tmp_ie2;
} 
// EOF
