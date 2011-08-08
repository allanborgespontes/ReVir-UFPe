/*
 * Test the dispatcher class.
 *
 * $Id: dispatcher.cpp 1936 2006-07-25 15:13:45Z stud-matfried $
 * $HeadURL: https://svn.tm.kit.edu/nsis/natfw-nslp/trunk/test/dispatcher.cpp $
 */
#include <cppunit/TestCase.h>
#include <cppunit/extensions/HelperMacros.h>

#include "address.h"

#include "dispatcher.h"
#include "events.h"

#include "utils.h"

using namespace natfw;
using namespace protlib;


class DispatcherTest : public CppUnit::TestCase {

	CPPUNIT_TEST_SUITE( DispatcherTest );

	CPPUNIT_TEST( testBasic );

	CPPUNIT_TEST_SUITE_END();

  public:
	void setUp();
	void testBasic();
	void tearDown();

	mock_natfw_config *conf;
	session_manager *mgr;
	nat_manager *nat_mgr;
	mock_dispatcher *d;
};

CPPUNIT_TEST_SUITE_REGISTRATION( DispatcherTest );

void DispatcherTest::setUp() {
	conf = new mock_natfw_config();
	mgr = new session_manager(conf);
	nat_mgr = new nat_manager(conf);
	d = new mock_dispatcher(mgr, nat_mgr, NULL, conf);
}

void DispatcherTest::tearDown() {
	delete d;
	delete nat_mgr;
	delete mgr;
	delete conf;
}

void DispatcherTest::testBasic() {
	event *e = new api_create_event(hostaddress("141.3.70.4"),
		hostaddress("141.3.70.5"), 1234, 4321, 6,
		std::list<uint8>(), 40);
	CPPUNIT_ASSERT( e->get_session_id() == NULL );

	d->process(e);
	ASSERT_CREATE_MESSAGE_SENT(d);
}

// EOF
