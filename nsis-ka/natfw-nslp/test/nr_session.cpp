/*
 * Test the nr_session_test class.
 *
 * $Id: nr_session.cpp 2133 2006-10-04 08:14:10Z stud-matfried $
 * $HeadURL: https://svn.tm.kit.edu/nsis/natfw-nslp/trunk/test/nr_session.cpp $
 */
#include <cppunit/TestCase.h>
#include <cppunit/extensions/HelperMacros.h>

#include "mri.h"	// from NTLP

#include "events.h"
#include "session.h"
#include "dispatcher.h"

#include "utils.h" // custom assertions

using namespace natfw;
using namespace natfw::msg;


class ResponderTest;


/*
 * We use a subclass for testing and make the test case a friend. This
 * way the test cases have access to protected methods and they don't have
 * to be public in nr_session.
 */
class nr_session_test : public nr_session {
  public:
	nr_session_test(state_t state=STATE_IDLE, uint32 msn=0)
		: nr_session(state, msn) { }

	friend class ResponderTest;
};


class ResponderTest : public CppUnit::TestCase {

	CPPUNIT_TEST_SUITE( ResponderTest );

	CPPUNIT_TEST( testIdle );
	CPPUNIT_TEST( testSession );

	CPPUNIT_TEST_SUITE_END();

  public:
	void setUp();
	void tearDown();

	void testIdle();
	void testSession();

  private:
	static const uint32 START_MSN = 77;
	static const uint32 SESSION_LIFETIME = 30;

	void process(nr_session_test &s, event *evt);

	msg::ntlp_msg *create_natfw_create(uint32 msn=START_MSN,
		uint32 session_lifetime=SESSION_LIFETIME) const;

	mock_dispatcher *d;
	hostaddress destination;
};

CPPUNIT_TEST_SUITE_REGISTRATION( ResponderTest );


/*
 * Call the event handler and delete the event afterwards.
 */
void ResponderTest::process(nr_session_test &s, event *evt) {
	d->clear();
	s.process(d, evt);
	delete evt;
}

void ResponderTest::setUp() {
	d = new mock_dispatcher();
}

void ResponderTest::tearDown() {
	delete d;
}


msg::ntlp_msg *ResponderTest::create_natfw_create(uint32 msn, uint32 lt) const {

	msg::natfw_create *create = new natfw_create();
	create->set_msg_sequence_number(msn);
	create->set_session_lifetime(lt);

	ntlp::mri *ntlp_mri = new ntlp::mri_pathcoupled(
		hostaddress("192.168.0.4"), 32, 0,
		hostaddress("192.168.0.5"), 32, 0,
		"tcp", 0, 0, 0, false
	);

	return new msg::ntlp_msg(session_id(), create, ntlp_mri, 0);
}


void ResponderTest::testIdle() {
	/*
	 * IDLE ---[rx_CREATE && CHECK_AA && CREATE(Lifetime>0) ]---> SESSION
	 */
	nr_session_test s1(nr_session::STATE_IDLE);
	event *e1 = new msg_event(NULL, create_natfw_create(), true);

	process(s1, e1);
	ASSERT_STATE(s1, nr_session::STATE_SESSION);
	ASSERT_RESPONSE_MESSAGE_SENT(d, information_code::sc_success);
	ASSERT_TIMER_STARTED(d, s1.get_state_timer());


	/*
	 * IDLE ---[rx_CREATE && CREATE(Lifetime > MAX) ]---> FINAL
	 */
	nr_session_test s2(nr_session::STATE_IDLE);
	event *e2 = new msg_event(NULL,
		create_natfw_create(START_MSN+1, 100000), true);

	process(s2, e2);
	ASSERT_STATE(s2, nr_session::STATE_FINAL);
	ASSERT_RESPONSE_MESSAGE_SENT(d,
		information_code::sc_signaling_session_failures);
	ASSERT_NO_TIMER(d);
}


void ResponderTest::testSession() {
	/*
	 * SESSION ---[rx_CREATE && CHECK_AA && CREATE(Lifetime>0) ]---> SESSION
	 */
	nr_session_test s1(nr_session::STATE_SESSION, START_MSN);
	event *e1 = new msg_event(NULL, create_natfw_create(START_MSN+1), true);

	process(s1, e1);
	ASSERT_STATE(s1, nr_session::STATE_SESSION);
	CPPUNIT_ASSERT( s1.get_msg_sequence_number() == START_MSN+1 );
	ASSERT_RESPONSE_MESSAGE_SENT(d, information_code::sc_success);
	ASSERT_TIMER_STARTED(d, s1.get_state_timer());


	/*
	 * SESSION ---[rx_CREATE && CREATE(Lifetime > MAX) ]---> SESSION
	 */
	nr_session_test s2(nr_session::STATE_SESSION, START_MSN);
	event *e2 = new msg_event(NULL,
		create_natfw_create(START_MSN+1, 100000), true);

	process(s2, e2);
	ASSERT_STATE(s2, nr_session::STATE_SESSION);
	ASSERT_RESPONSE_MESSAGE_SENT(d,
		information_code::sc_signaling_session_failures);
	ASSERT_NO_TIMER(d);


	/*
	 * SESSION ---[rx_CREATE && CREATE(Lifetime == 0) ]---> FINAL
	 */
	nr_session_test s3(nr_session::STATE_SESSION, START_MSN);
	event *e3 = new msg_event(NULL,
			create_natfw_create(START_MSN+1, 0), true);

	process(s3, e3);
	ASSERT_STATE(s3, nr_session::STATE_FINAL);
	ASSERT_NO_MESSAGE(d);
	ASSERT_NO_TIMER(d);


	/*
	 * SESSION ---[rx_CREATE && MSN too low ]---> SESSION
	 */
	nr_session_test s4(nr_session::STATE_SESSION, START_MSN);
	event *e4 = new msg_event(NULL, create_natfw_create(START_MSN), true);

	process(s4, e4);
	ASSERT_STATE(s4, nr_session::STATE_SESSION);
	ASSERT_NO_MESSAGE(d);
	ASSERT_NO_TIMER(d);
}


// EOF
