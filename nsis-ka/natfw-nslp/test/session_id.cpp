/*
 * Test the session_id class.
 *
 * $Id: session_id.cpp 6282 2011-06-17 13:31:57Z bless $
 * $HeadURL: https://svn.tm.kit.edu/nsis/natfw-nslp/trunk/test/session_id.cpp $
 */
#include <cppunit/TestCase.h>
#include <cppunit/extensions/HelperMacros.h>

#include "hashmap"
#include "protlib_types.h"
#include "session.h"

using namespace natfw;


class SessionIdTest : public CppUnit::TestCase {

	CPPUNIT_TEST_SUITE( SessionIdTest );

	CPPUNIT_TEST( testComparison );
	CPPUNIT_TEST( testAssignment );

	CPPUNIT_TEST_SUITE_END();

  public:
	void testComparison() {
		session_id id1 = session_id();
		session_id id2 = session_id();

		CPPUNIT_ASSERT( id1 == id1 );
		CPPUNIT_ASSERT( id1 == id1 );
		CPPUNIT_ASSERT( id1 != id2 );

		uint128 val = id1.get_id();
		session_id id3 = session_id(val);

		CPPUNIT_ASSERT( id1 == id3 );
		CPPUNIT_ASSERT( id3 == id1 );
	}

	void testAssignment() {
		session_id id1 = session_id();
		session_id id2 = id1;

		CPPUNIT_ASSERT( id1 == id2 );
		CPPUNIT_ASSERT( &id1 != &id2 );
	}
};

CPPUNIT_TEST_SUITE_REGISTRATION( SessionIdTest );
