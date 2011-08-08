/*
 * Test the sessionid class.
 *
 * $Id: sessionid.cpp 3598 2008-10-31 09:53:30Z bless $
 * $HeadURL: https://svn.tm.kit.edu/nsis/ntlp/trunk/test/sessionid.cpp $
 */
#include "test_suite.h"

#include "sessionid.h"

using namespace ntlp;


class sessionid_test : public CppUnit::TestCase {

  CPPUNIT_TEST_SUITE( sessionid_test );

  CPPUNIT_TEST( test_equals );
  CPPUNIT_TEST( test_basics );
  CPPUNIT_TEST( test_streams );
  CPPUNIT_TEST( test_random );
  CPPUNIT_TEST( test_uint128 );
  // Add more tests here.

  CPPUNIT_TEST_SUITE_END();

  public:
	// Check if the == operator works. Required for test_basics() below.
	void test_equals() {
		sessionid sid1 = sessionid(0x12345678, 0, 0, 0xdeadbeef);
		sessionid sid2 = sessionid(0x12345678, 0, 0, 0xdeadbeef);
		CPPUNIT_ASSERT( sid1 == sid2 );

		sessionid sid3 = sessionid(1, 2, 3, 4);
		CPPUNIT_ASSERT( sid1 != sid3 );
		CPPUNIT_ASSERT( sid2 != sid3 );
	}

	void test_basics() {
		sessionid sid1;
		ASSERT_BASICS_WORK( sid1 );

		sessionid sid2 = sessionid(0, 0, 0, 0);
		ASSERT_BASICS_WORK( sid2 );

		sessionid sid3 = sessionid(0x12345678, 0xbad00, 0xc0ffee, 0xdeadbeef);
		ASSERT_BASICS_WORK( sid3 );
	}

	void test_random() {
		sessionid sid1 = sessionid(0x12345678, 0, 0, 0xdeadbeef);
		sessionid sid2 = sessionid(0x12345678, 0, 0, 0xdeadbeef);
		CPPUNIT_ASSERT( sid1 == sid2 );

		sid1.generate_random();
		sid2.generate_random();
		CPPUNIT_ASSERT( sid1 != sid2 );

		sessionid sid3 = sessionid(1, 2, 3, 4);
		sid3.generate_random();

		CPPUNIT_ASSERT( sid1 != sid3 );
		CPPUNIT_ASSERT( sid2 != sid3 );
	}

	void test_uint128() {
	  uint128 sid_uint(1,2,3,4);
	  
	  sessionid sid1(sid_uint);
	  ASSERT_BASICS_WORK( sid1 );

	  // test uint128 conversion operator
	  CPPUNIT_ASSERT( (uint128) sid1 == sid_uint );
	}


  void test_streams()
  {
    sessionid sid= sessionid(0x12345678, 0xbad00, 0xc0ffee, 0xdeadbeef);
    
    stringstream tests;
    // test output
    tests << sid.to_string();
    // test input
    sessionid newsid;
    
    tests >> newsid;

    // cout << "sid:" << sid << " newsid:" << newsid << endl;
    CPPUNIT_ASSERT( newsid == sid );
  }
};

CPPUNIT_TEST_SUITE_REGISTRATION( sessionid_test );
