/*
 * Test the natfw_ext class.
 *
 * $Id: natfw_ext.cpp 2438 2006-12-08 15:07:34Z stud-matfried $
 * $HeadURL: https://svn.tm.kit.edu/nsis/natfw-nslp/trunk/test/natfw_ext.cpp $
 */
#include <cppunit/TestCase.h>
#include <cppunit/extensions/HelperMacros.h>

#include "network_message.h"

#include "msg/natfw_ext.h"
#include "msg/natfw_object.h"

using namespace natfw::msg;


class NatFwExtTest : public CppUnit::TestCase {

	CPPUNIT_TEST_SUITE( NatFwExtTest );

	CPPUNIT_TEST( testBasics );

	CPPUNIT_TEST_SUITE_END();

  public:
	void testBasics();
};

CPPUNIT_TEST_SUITE_REGISTRATION( NatFwExtTest );


void NatFwExtTest::testBasics() {
	natfw_ext c1;

	c1.set_session_lifetime(27);
	CPPUNIT_ASSERT( c1.get_session_lifetime() == 27 );

	c1.set_msg_sequence_number(42000);
	CPPUNIT_ASSERT( c1.get_msg_sequence_number() == 42000 );

	c1.set_extended_flow_info(extended_flow_info::ra_allow, 1);
	CPPUNIT_ASSERT( c1.get_rule_action() == extended_flow_info::ra_allow );
	CPPUNIT_ASSERT( c1.get_subsequent_ports() == 1 );

	hostaddress addr = hostaddress("127.0.0.1");
	c1.set_data_terminal_info(addr, 24);
	CPPUNIT_ASSERT( c1.get_ds_address() == addr);
}

// EOF
