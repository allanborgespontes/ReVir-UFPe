/*
 * basic.cpp - Check cppunit and protlib
 *
 * $Id: basic.cpp 4107 2009-07-16 13:49:52Z bless $
 * $HeadURL: https://svn.tm.kit.edu/nsis/protlib/trunk/test/basic.cpp $
 *
 */
#include <cppunit/TestCase.h>
#include <cppunit/extensions/HelperMacros.h>

#include "ie.h"
#include "network_message.h"
#include "logfile.h"
#include "address.h"

using namespace protlib;
using namespace protlib::log;


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
	}
		
	void testProtlib() {
		NetMsg msg(10);

		Log(ERROR_LOG, LOG_CRIT, "basic", "Test");

		hostaddress host("127.0.0.1");
		appladdress addr(host, 10, 80);
	}

	void tearDown() {
		// Executed after each of the test methods.
	}
};

CPPUNIT_TEST_SUITE_REGISTRATION( BasicTest );
