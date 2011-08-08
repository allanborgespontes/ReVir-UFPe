/*
 * This file is a template which can be copied to create a new test case file.
 *
 * $Id: test_types.cpp 4107 2009-07-16 13:49:52Z bless $
 * $HeadURL: https://svn.tm.kit.edu/nsis/protlib/trunk/test/test_types.cpp $
 */
#include <cppunit/TestCase.h>
#include <cppunit/extensions/HelperMacros.h>

#include "protlib_types.h"

using namespace protlib;

class test_types : public CppUnit::TestCase {

	CPPUNIT_TEST_SUITE( test_types );

	CPPUNIT_TEST( test_sizes );

	CPPUNIT_TEST_SUITE_END();

  public:
	void setUp() {
		// Executed before each of the test methods.
	}

	void test_sizes() {
	  CPPUNIT_ASSERT( sizeof(int8)   == 1 );
	  CPPUNIT_ASSERT( sizeof(int16)  == 2 );
	  CPPUNIT_ASSERT( sizeof(int32)  == 4 );
	  CPPUNIT_ASSERT( sizeof(int64)  == 8 );

	  CPPUNIT_ASSERT( sizeof(uint8)   ==  1 );
	  CPPUNIT_ASSERT( sizeof(uint16)  ==  2 );
	  CPPUNIT_ASSERT( sizeof(uint32)  ==  4 );
	  CPPUNIT_ASSERT( sizeof(uint64)  ==  8 );
	  CPPUNIT_ASSERT( sizeof(uint128) == 16 );
	}
	
	void tearDown() {
		// Executed after each of the test methods.
	}
};

CPPUNIT_TEST_SUITE_REGISTRATION( test_types );
