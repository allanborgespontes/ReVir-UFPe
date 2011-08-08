/*
 * basic.cpp - Check cppunit and protlib
 *
 * $Id: basic.cpp 901 2005-11-17 17:11:42Z stud-matfried $
 * $HeadURL: https://svn.tm.kit.edu/nsis/qspec/trunk/test/basic.cpp $
 *
 */
#include <cppunit/TestCase.h>
#include <cppunit/extensions/HelperMacros.h>

#include "ie.h"
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
	}

	void tearDown() {
		// Executed after each of the test methods.
	}
};

CPPUNIT_TEST_SUITE_REGISTRATION( BasicTest );
