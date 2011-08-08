/*
 * Test the natfw_response class.
 *
 * $Id: natfw_response.cpp 1777 2006-06-12 07:42:29Z stud-matfried $
 * $HeadURL: https://svn.tm.kit.edu/nsis/natfw-nslp/trunk/test/natfw_response.cpp $
 */
#include <cppunit/TestCase.h>
#include <cppunit/extensions/HelperMacros.h>

#include "network_message.h"

#include "msg/natfw_msg.h"

using namespace natfw::msg;


class NatFwResponseTest : public CppUnit::TestCase {

	CPPUNIT_TEST_SUITE( NatFwResponseTest );

	CPPUNIT_TEST( testBasic );
	CPPUNIT_TEST( testComparison );
	CPPUNIT_TEST( testCreateResponse );

	CPPUNIT_TEST_SUITE_END();

  public:
	void testBasic();
	void testComparison();
	void testCreateResponse();
};

CPPUNIT_TEST_SUITE_REGISTRATION( NatFwResponseTest );


void NatFwResponseTest::testBasic() {
	natfw_response r;

	CPPUNIT_ASSERT( r.get_msg_type() == natfw_response::MSG_TYPE );

	hostaddress h1 = hostaddress("141.3.70.4");
	r.set_external_address(h1, 12345);
	CPPUNIT_ASSERT( r.get_external_address() == h1 );
	CPPUNIT_ASSERT( r.get_external_port() == 12345 );

	r.set_information_code(information_code::sc_success,
		information_code::suc_successfully_processed);
	CPPUNIT_ASSERT( r.get_severity_class()
		== information_code::sc_success );
}


void NatFwResponseTest::testComparison() {
	natfw_response *r1 = new natfw_response();
	natfw_response *r2 = new natfw_response();
	r2->set_session_lifetime(20);
	r2->set_msg_sequence_number(7777);
	r2->set_information_code(information_code::sc_success,
			information_code::suc_successfully_processed);

	CPPUNIT_ASSERT( r2->is_success() );
	CPPUNIT_ASSERT( *r1 == *r1 );
	CPPUNIT_ASSERT( *r2 == *r2 );
	CPPUNIT_ASSERT( *r1 != *r2 );
	CPPUNIT_ASSERT( *r2 != *r1 );

	r1->set_session_lifetime(20);
	r1->set_msg_sequence_number(7777);
	r1->set_information_code(information_code::sc_success,
			information_code::suc_successfully_processed);
	CPPUNIT_ASSERT( *r1 == *r2 );
	CPPUNIT_ASSERT( *r2 == *r1 );

	delete r2;
	delete r1;
}


void NatFwResponseTest::testCreateResponse() {
	natfw_create c1;

	c1.set_session_lifetime(30);
	c1.set_extended_flow_info(extended_flow_info::ra_allow, 1);
	c1.set_msg_sequence_number(0xFABC);

	//std::cout << "CREATE: " << c1 << std::endl;

	natfw_response r1(&c1, information_code::sc_success,
		information_code::suc_successfully_processed);

	//std::cout << "RESPONSE: " << r1 << std::endl;

	CPPUNIT_ASSERT( r1.is_response_to(&c1) );


	c1.set_msg_sequence_number(47);

	CPPUNIT_ASSERT( ! r1.is_response_to(&c1) );
}

// EOF
