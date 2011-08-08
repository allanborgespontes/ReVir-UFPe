/*
 * Test the ni_session_test class.
 *
 * $Id: ni_session.cpp 1936 2006-07-25 15:13:45Z stud-matfried $
 * $HeadURL: https://svn.tm.kit.edu/nsis/natfw-nslp/trunk/test/ni_session.cpp $
 */
#include <cppunit/TestCase.h>
#include <cppunit/extensions/HelperMacros.h>

#include "events.h"
#include "session.h"
#include "dispatcher.h"

#include "utils.h" // custom assertions

using namespace natfw;
using namespace natfw::msg;


class InitiatorTest;


/*
 * We use a subclass for testing and make the test case a friend. This
 * way the test cases have access to protected methods and they don't have
 * to be public in ni_session.
 */
class ni_session_test : public ni_session {
  public:
	ni_session_test(state_t state=STATE_IDLE) : ni_session(state) { }

	friend class InitiatorTest;
};


class InitiatorTest : public CppUnit::TestCase {

	CPPUNIT_TEST_SUITE( InitiatorTest );

	CPPUNIT_TEST( testIdle );
	CPPUNIT_TEST( testWaitresp );
	CPPUNIT_TEST( testSession );


	CPPUNIT_TEST( testWaitrespErrors );

	CPPUNIT_TEST_SUITE_END();

  public:
	void setUp();
	void tearDown();

	void testIdle();
	void testWaitresp();
	void testSession();

	void testWaitrespErrors();

  private:
	static const uint32 START_MSN = 77;

	void process(ni_session_test &s, event *evt);

	msg::ntlp_msg *create_natfw_create() const;

	msg::ntlp_msg *create_natfw_response(
		uint8 severity, uint8 response_code, uint16 msg_type,
		uint32 msn=START_MSN) const;

	mock_natfw_config *conf;
	mock_dispatcher *d;
	hostaddress source;
	hostaddress destination;
};

CPPUNIT_TEST_SUITE_REGISTRATION( InitiatorTest );


/*
 * Call the event handler and delete the event afterwards.
 */
void InitiatorTest::process(ni_session_test &s, event *evt) {
	d->clear();
	s.process(d, evt);
	delete evt;
}

void InitiatorTest::setUp() {
	conf = new mock_natfw_config();
	d = new mock_dispatcher(NULL, NULL, NULL, conf);
	source = hostaddress("141.3.70.5");
	destination = hostaddress("141.3.70.4");
}

void InitiatorTest::tearDown() {
	delete d;
	delete conf;
}


msg::ntlp_msg *InitiatorTest::create_natfw_create() const {

	msg::natfw_create *create = new natfw_create();
	create->set_msg_sequence_number(START_MSN);

	msg::ntlp_msg *msg = new msg::ntlp_msg(
		session_id(), create,
		new ntlp::mri_pathcoupled(), 0
	);

	return msg;
}


msg::ntlp_msg *InitiatorTest::create_natfw_response(uint8 severity,
		uint8 response_code, uint16 msg_type, uint32 msn) const {

	natfw_response *resp = new natfw_response();
	resp->set_information_code(severity, response_code, msg_type);
	resp->set_msg_sequence_number(msn);

	ntlp::mri *ntlp_mri = new ntlp::mri_pathcoupled(
		hostaddress("192.168.0.4"), 32, 0,
		hostaddress("192.168.0.5"), 32, 0,
		"tcp", 0, 0, 0, true
	);

	return new msg::ntlp_msg(session_id(), resp, ntlp_mri, 0);
}


void InitiatorTest::testIdle() {
	/*
	 * IDLE ---[tg_CREATE]---> WAITRESP
	 */
	ni_session_test s1(ni_session::STATE_IDLE);
	event *e1 = new api_create_event(source, destination);

	process(s1, e1);
	ASSERT_STATE(s1, ni_session::STATE_WAITRESP);
	ASSERT_CREATE_MESSAGE_SENT(d);
	ASSERT_TIMER_STARTED(d, s1.get_response_timer());
}


void InitiatorTest::testWaitresp() {
	/*
	 * WAITRESP ---[rx_RESPONSE(SUCCESS,CREATE)]---> SESSION
	 */
	ni_session_test s1(ni_session::STATE_WAITRESP);
	s1.set_last_create_message(create_natfw_create());

	ntlp_msg *resp1 = create_natfw_response(information_code::sc_success,
		information_code::suc_successfully_processed,
		natfw_create::MSG_TYPE);

	event *e1 = new msg_event(NULL, resp1);

	process(s1, e1);
	ASSERT_STATE(s1, ni_session::STATE_SESSION);
	ASSERT_NO_MESSAGE(d);
	ASSERT_TIMER_STARTED(d, s1.get_refresh_timer());


	/*
	 * WAITRESP ---[rx_RESPONSE(ERROR,CREATE)]---> FINAL
	 */
	ni_session_test s2(ni_session::STATE_WAITRESP);
	s2.set_last_create_message(create_natfw_create());

	ntlp_msg *resp2 = create_natfw_response(
		information_code::sc_permanent_failure, 0,
		natfw_create::MSG_TYPE);

	event *e2 = new msg_event(NULL, resp2);

	process(s2, e2);
	ASSERT_STATE(s2, ni_session::STATE_FINAL);
	ASSERT_NO_MESSAGE(d);
	ASSERT_NO_TIMER(d);


	/*
	 * WAITRESP ---[RESPONSE_TIMEOUT, retry]---> WAITRESP
	 */
	ni_session_test s3(ni_session::STATE_WAITRESP);
	// fake a previously sent CREATE message
	s3.set_last_create_message(create_natfw_create());
	s3.get_response_timer().set_id(47);

	timer_event *e3 = new timer_event(NULL, 47);

	process(s3, e3);
	ASSERT_STATE(s3, ni_session::STATE_WAITRESP);
	ASSERT_CREATE_MESSAGE_SENT(d);
	ASSERT_TIMER_STARTED(d, s3.get_response_timer());


	/*
	 * WAITRESP ---[RESPONSE_TIMEOUT, no retry]---> FINAL
	 */
	ni_session_test s4(ni_session::STATE_WAITRESP);
	s4.set_create_counter(1000);
	s4.get_response_timer().set_id(47);
	timer_event *e4 = new timer_event(NULL, 47);

	process(s4, e4);
	ASSERT_STATE(s4, ni_session::STATE_FINAL);
	ASSERT_NO_MESSAGE(d);
	ASSERT_NO_TIMER(d);


	/*
	 * WAITRESP ---[tg_TEARDOWN]---> FINAL
	 */
	ni_session_test s5(ni_session::STATE_WAITRESP);
	s5.set_last_create_message(create_natfw_create());
	event *e5 = new api_teardown_event(NULL);

	process(s5, e5);
	ASSERT_STATE(s5, ni_session::STATE_FINAL);
	ASSERT_CREATE_MESSAGE_SENT(d);
	ASSERT_NO_TIMER(d);


	/*
	 * WAITRESP ---[tg_TEARDOWN]---> FINAL
	 */
	ni_session_test s6(ni_session::STATE_WAITRESP);
	s6.set_last_create_message(create_natfw_create());
	event *e6 = new no_next_node_found_event(NULL);

	process(s6, e6);
	ASSERT_STATE(s6, ni_session::STATE_FINAL);
	ASSERT_NO_MESSAGE(d);
	ASSERT_NO_TIMER(d);
}


void InitiatorTest::testSession() {
	/*
	 * SESSION ---[rx_RESPONSE(SUCCESS,CREATE)]---> SESSION
	 */
	ni_session_test s1(ni_session::STATE_SESSION);
	s1.set_last_create_message(create_natfw_create());

	ntlp_msg *resp1 = create_natfw_response(information_code::sc_success,
		information_code::suc_successfully_processed,
		natfw_create::MSG_TYPE);

	event *e1 = new msg_event(NULL, resp1);

	process(s1, e1);
	ASSERT_STATE(s1, ni_session::STATE_SESSION);
	ASSERT_NO_MESSAGE(d);
	ASSERT_TIMER_STARTED(d, s1.get_refresh_timer());


	/*
	 * SESSION ---[rx_RESPONSE(ERROR,CREATE)]---> FINAL
	 */
	ni_session_test s2(ni_session::STATE_SESSION);
	s2.set_last_create_message(create_natfw_create());

	ntlp_msg *resp2 = create_natfw_response(
		information_code::sc_permanent_failure, 0,
		natfw_create::MSG_TYPE);

	event *e2 = new msg_event(NULL, resp2);

	process(s2, e2);
	ASSERT_STATE(s2, ni_session::STATE_FINAL);
	ASSERT_NO_MESSAGE(d);
	ASSERT_NO_TIMER(d);


	/*
	 * SESSION ---[RESPONSE_TIMEOUT, retry]---> SESSION
	 */
	ni_session_test s3(ni_session::STATE_SESSION);
	s3.set_last_create_message(create_natfw_create());
	s3.get_response_timer().set_id(47);

	timer_event *e3 = new timer_event(NULL, 47);

	process(s3, e3);
	ASSERT_STATE(s3, ni_session::STATE_SESSION);
	ASSERT_CREATE_MESSAGE_SENT(d);
	ASSERT_TIMER_STARTED(d, s3.get_response_timer());


	/*
	 * SESSION ---[RESPONSE_TIMEOUT, no retry]---> FINAL
	 */
	ni_session_test s4(ni_session::STATE_WAITRESP);
	s4.set_create_counter(1000);
	s4.get_response_timer().set_id(47);
	timer_event *e4 = new timer_event(NULL, 47);

	process(s4, e4);
	ASSERT_STATE(s4, ni_session::STATE_FINAL);
	ASSERT_NO_MESSAGE(d);
	ASSERT_NO_TIMER(d);


	/*
	 * SESSION ---[tg_TEARDOWN]---> FINAL
	 */
	ni_session_test s5(ni_session::STATE_SESSION);
	s5.set_last_create_message(create_natfw_create());
	event *e5 = new api_teardown_event(NULL);

	process(s5, e5);
	ASSERT_STATE(s5, ni_session::STATE_FINAL);
	ASSERT_CREATE_MESSAGE_SENT(d);
	ASSERT_NO_TIMER(d);


	/*
	 * SESSION ---[REFRESH_TIMEOUT]---> SESSION
	 */
	ni_session_test s6(ni_session::STATE_SESSION);
	s6.set_last_create_message(create_natfw_create());
	s6.get_refresh_timer().set_id(0xABCD);
	event *e6 = new timer_event(NULL, 0xABCD);

	process(s6, e6);
	ASSERT_STATE(s6, ni_session::STATE_SESSION);
	ASSERT_CREATE_MESSAGE_SENT(d);
	ASSERT_TIMER_STARTED(d, s6.get_response_timer());
}


void InitiatorTest::testWaitrespErrors() {
	/*
	 * WAITRESP ---[rx_RESPONSE(SUCCESS, invalid CREATE MSN)]---> WAITRESP
	 */
	ni_session_test s1(ni_session::STATE_WAITRESP);
	s1.set_last_create_message(create_natfw_create());

	ntlp_msg *resp1 = create_natfw_response(information_code::sc_success,
		information_code::suc_successfully_processed,
		natfw_create::MSG_TYPE, START_MSN-1);

	event *e1 = new msg_event(NULL, resp1);

	process(s1, e1);
	ASSERT_STATE(s1, ni_session::STATE_WAITRESP);
	ASSERT_NO_MESSAGE(d);
	ASSERT_NO_TIMER(d);
}

// EOF
