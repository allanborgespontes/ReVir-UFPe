/*
 * basic.cpp - Check cppunit and protlib.
 *
 * $Id: basic.cpp 1838 2006-06-27 15:28:11Z stud-matfried $
 * $HeadURL: https://svn.tm.kit.edu/nsis/natfw-nslp/trunk/test/basic.cpp $
 */
#include <cppunit/TestCase.h>
#include <cppunit/extensions/HelperMacros.h>

#include "ie.h"
#include "address.h"
#include "network_message.h"

using namespace protlib;


class BasicTest : public CppUnit::TestCase {

	CPPUNIT_TEST_SUITE( BasicTest );

	CPPUNIT_TEST( testCppUnit );
	CPPUNIT_TEST( testProtlib );

	CPPUNIT_TEST_SUITE_END();

  public:
	void setUp() {
		// Executed before each of the test methods.
	}

	void testCppUnit() {
		CPPUNIT_ASSERT( 1 == 1 );
		CPPUNIT_ASSERT_MESSAGE("error message", 1 == 1 );
		CPPUNIT_ASSERT_THROW(
			throw std::exception(), std::exception );
	}
		
	void testProtlib() {
		NetMsg msg(100);

		netaddress net("10.38.2.0/24");
		CPPUNIT_ASSERT( net.get_pref_len() == 24 );

		hostaddress addr1("10.38.2.222");
		CPPUNIT_ASSERT( addr1.match_against(net) == 24 );

		hostaddress addr2("10.38.1.191");
		CPPUNIT_ASSERT( addr2.match_against(net) != 24 );
	}

	void tearDown() {
		// Executed after each of the test methods.
	}
};

CPPUNIT_TEST_SUITE_REGISTRATION( BasicTest );
