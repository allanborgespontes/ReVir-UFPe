/*
 * Test the natfw_create class.
 *
 * $Id: natfw_create.cpp 1730 2006-05-15 12:57:33Z stud-matfried $
 * $HeadURL: https://svn.tm.kit.edu/nsis/natfw-nslp/trunk/test/natfw_create.cpp $
 */
#include <cppunit/TestCase.h>
#include <cppunit/extensions/HelperMacros.h>

#include "network_message.h"

#include "msg/natfw_create.h"
#include "msg/natfw_object.h"

using namespace natfw::msg;


class NatFwCreateTest : public CppUnit::TestCase {

	CPPUNIT_TEST_SUITE( NatFwCreateTest );

	CPPUNIT_TEST( testBasics );
	CPPUNIT_TEST( testComparison );
	CPPUNIT_TEST( testCopying );
	//CPPUNIT_TEST( testObjectFlags );

	CPPUNIT_TEST_SUITE_END();

  public:
	void testBasics();
	void testComparison();
	void testCopying();
	void testObjectFlags();
};

CPPUNIT_TEST_SUITE_REGISTRATION( NatFwCreateTest );


void NatFwCreateTest::testBasics() {
	natfw_create c1;

	c1.set_session_lifetime(27);
	CPPUNIT_ASSERT( c1.get_session_lifetime() == 27 );

	c1.set_msg_sequence_number(42000);
	CPPUNIT_ASSERT( c1.get_msg_sequence_number() == 42000 );

	c1.set_extended_flow_info(extended_flow_info::ra_allow, 1);
	CPPUNIT_ASSERT( c1.get_rule_action() == extended_flow_info::ra_allow );
	CPPUNIT_ASSERT( c1.get_subsequent_ports() == 1 );
}


void NatFwCreateTest::testComparison() {
	natfw_create *c1 = new natfw_create();
	natfw_create *c2 = new natfw_create(true);
	c2->set_msg_sequence_number(7777);

	CPPUNIT_ASSERT( *c1 == *c1 );
	CPPUNIT_ASSERT( *c2 == *c2 );
	CPPUNIT_ASSERT( *c1 != *c2 );
	CPPUNIT_ASSERT( *c2 != *c1 );

	c1->set_proxy_mode(true);
	c1->set_msg_sequence_number(7777);
	CPPUNIT_ASSERT( *c1 == *c2 );
	CPPUNIT_ASSERT( *c2 == *c1 );

	delete c2;
	delete c1;
}

void NatFwCreateTest::testCopying() {
	natfw_create *c1 = new natfw_create(true);
	c1->set_nonce(42);

	natfw_create *c2 = c1->copy();
	CPPUNIT_ASSERT( c1 != c2 );
	CPPUNIT_ASSERT( *c1 == *c2 );

	delete c2;
	delete c1;
}

/*
 * NATFW Object treatments are currently not part of the public interface,
 * so this test doesn't work. I'm currently not sure if it is needed.
 *
void NatFwCreateTest::testObjectFlags() {
	natfw_create *c1 = new natfw_create(true); // proxy mode
	c1->set_nonce(42);

	natfw_object *o1 = c1->get_object(nonce::OBJECT_TYPE);
	CPPUNIT_ASSERT( o1->get_treatment() == natfw_object::tr_mandatory );

	delete c1;


	natfw_create *c2 = new natfw_create(false);
	c2->set_nonce(42);

	natfw_object *o2 = c2->get_object(nonce::OBJECT_TYPE);
	CPPUNIT_ASSERT( o2->get_treatment() == natfw_object::tr_optional );

	delete c2;
}
*/

// EOF
