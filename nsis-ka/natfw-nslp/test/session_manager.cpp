/*
 * Test the session_manager class.
 *
 * $Id: session_manager.cpp 1842 2006-06-28 11:43:50Z stud-matfried $
 * $HeadURL: https://svn.tm.kit.edu/nsis/natfw-nslp/trunk/test/session_manager.cpp $
 */
#include <cppunit/TestCase.h>
#include <cppunit/extensions/HelperMacros.h>

#include "session_manager.h"
#include "dispatcher.h"

#include "utils.h"

using namespace natfw;


class SessionManagerTest : public CppUnit::TestCase {

	CPPUNIT_TEST_SUITE( SessionManagerTest );

	CPPUNIT_TEST( testGetRetrieve );
	CPPUNIT_TEST( testRemove );

	CPPUNIT_TEST_SUITE_END();

  public:
	void testGetRetrieve();
	void testRemove();
};

CPPUNIT_TEST_SUITE_REGISTRATION( SessionManagerTest );

void SessionManagerTest::testGetRetrieve() {
	mock_natfw_config conf;
	session_manager mgr = session_manager(&conf);

	session *s1 = mgr.create_ni_session();
	session *s2 = mgr.create_ni_session();

	session *s3 = mgr.get_session(s1->get_id());
	CPPUNIT_ASSERT( s1 == s3 ); // same objects

	session *s4 = mgr.get_session(s2->get_id());
	CPPUNIT_ASSERT( s2 == s4 ); // same objects
}

void SessionManagerTest::testRemove() {
	mock_natfw_config conf;
	session_manager mgr = session_manager(&conf);

	session_id id = session_id();
	CPPUNIT_ASSERT( mgr.get_session(id) == NULL );

	session *s1 = mgr.create_ni_session();
	CPPUNIT_ASSERT( mgr.get_session(s1->get_id()) == s1 );

	mgr.remove_session(s1->get_id());
	CPPUNIT_ASSERT( mgr.get_session(s1->get_id()) == NULL );
}

// EOF
