/*
 * Test the nr_ext_session_test class.
 *
 * $Id: nr_ext_session.cpp 2438 2006-12-08 15:07:34Z stud-matfried $
 * $HeadURL: https://svn.tm.kit.edu/nsis/natfw-nslp/trunk/test/nr_ext_session.cpp $
 */
#include <cppunit/TestCase.h>
#include <cppunit/extensions/HelperMacros.h>

#include "events.h"
#include "session.h"
#include "dispatcher.h"

#include "utils.h" // custom assertions

using namespace natfw;
using namespace natfw::msg;


class ExtResponderTest;


/*
 * We use a subclass for testing and make the test case a friend. This
 * way the test cases have access to protected methods and they don't have
 * to be public in the base class.
 */
class nr_ext_session_test : public nr_ext_session {
  public:
	nr_ext_session_test(state_t state=STATE_IDLE)
		: nr_ext_session(state) { }

	friend class ExtResponderTest;
};


class ExtResponderTest : public CppUnit::TestCase {

	CPPUNIT_TEST_SUITE( ExtResponderTest );

	CPPUNIT_TEST( testIdle );
	CPPUNIT_TEST( testWaitresp );
	CPPUNIT_TEST( testSession );

	CPPUNIT_TEST_SUITE_END();

  public:
	void setUp();
	void tearDown();

	void testIdle();
	void testWaitresp();
	void testSession();

  private:
	static const uint32 START_MSN = 77;

	void process(nr_ext_session_test &s, event *evt);

	msg::ntlp_msg *create_natfw_ext() const;

	msg::ntlp_msg *create_natfw_response(
		uint8 severity, uint8 response_code, uint16 msg_type,
		uint32 msn=START_MSN) const;

	mock_natfw_config *conf;
	mock_dispatcher *d;
	hostaddress source;
	hostaddress destination;
};

CPPUNIT_TEST_SUITE_REGISTRATION( ExtResponderTest );


/*
 * Call the event handler and delete the event afterwards.
 */
void ExtResponderTest::process(nr_ext_session_test &s, event *evt) {
	d->clear();
	s.process(d, evt);
	delete evt;
}

void ExtResponderTest::setUp() {
	conf = new mock_natfw_config();
	d = new mock_dispatcher(NULL, NULL, NULL, conf);
	source = hostaddress("141.3.70.5");
	destination = hostaddress("141.3.70.4");
}

void ExtResponderTest::tearDown() {
	delete d;
	delete conf;
}


msg::ntlp_msg *ExtResponderTest::create_natfw_ext() const {

	msg::natfw_ext *ext = new natfw_ext();
	ext->set_msg_sequence_number(START_MSN);

	msg::ntlp_msg *msg = new msg::ntlp_msg(
		session_id(), ext,
		new ntlp::mri_pathcoupled(), 0
	);

	return msg;
}


msg::ntlp_msg *ExtResponderTest::create_natfw_response(uint8 severity,
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


void ExtResponderTest::testIdle() {
	/*
	 * IDLE ---[tg_EXT]---> WAITRESP
	 */
	nr_ext_session_test s1(nr_ext_session::STATE_IDLE);
	event *e1 = new api_ext_event(fw_policy_rule::ACTION_ALLOW,
		source, 32, 1234, destination, 32, 4321, 17);

	process(s1, e1);
	ASSERT_STATE(s1, nr_ext_session::STATE_WAITRESP);
	ASSERT_EXT_MESSAGE_SENT(d);
	ASSERT_TIMER_STARTED(d, s1.get_response_timer());
}


void ExtResponderTest::testWaitresp() {
	/*
	 * WAITRESP ---[rx_RESPONSE(SUCCESS,EXT)]---> EXT
	 */
	nr_ext_session_test s1(nr_ext_session::STATE_WAITRESP);
	s1.get_refresh_timer().set_id(47);
	s1.set_last_ext_message(create_natfw_ext());

	ntlp_msg *resp1 = create_natfw_response(information_code::sc_success,
		information_code::suc_successfully_processed,
		natfw_ext::MSG_TYPE);

	event *e1 = new msg_event(NULL, resp1);

	process(s1, e1);
	ASSERT_STATE(s1, nr_ext_session::STATE_SESSION);
	ASSERT_NO_MESSAGE(d);
	ASSERT_TIMER_STARTED(d, s1.get_refresh_timer());


	/*
	 * WAITRESP ---[rx_RESPONSE(ERROR,EXT)]---> FINAL
	 */
	nr_ext_session_test s2(nr_ext_session::STATE_WAITRESP);
	s2.set_last_ext_message(create_natfw_ext());

	ntlp_msg *resp2 = create_natfw_response(
		information_code::sc_permanent_failure, 0,
		natfw_ext::MSG_TYPE);

	event *e2 = new msg_event(NULL, resp2);

	process(s2, e2);
	ASSERT_STATE(s2, nr_ext_session::STATE_FINAL);
	ASSERT_NO_MESSAGE(d);
	ASSERT_NO_TIMER(d);


	/*
	 * WAITRESP ---[RESPONSE_TIMEOUT, retry]---> WAITRESP
	 */
	nr_ext_session_test s3(nr_ext_session::STATE_WAITRESP);
	// fake a previously sent EXT message
	s3.set_last_ext_message(create_natfw_ext());
	s3.get_response_timer().set_id(47);

	timer_event *e3 = new timer_event(NULL, 47);

	process(s3, e3);
	ASSERT_STATE(s3, nr_ext_session::STATE_WAITRESP);
	ASSERT_EXT_MESSAGE_SENT(d);
	ASSERT_TIMER_STARTED(d, s3.get_response_timer());


	/*
	 * WAITRESP ---[RESPONSE_TIMEOUT, no retry]---> FINAL
	 */
	nr_ext_session_test s4(nr_ext_session::STATE_WAITRESP);
	s4.set_ext_counter(1000);
	s4.get_response_timer().set_id(47);
	timer_event *e4 = new timer_event(NULL, 47);

	process(s4, e4);
	ASSERT_STATE(s4, nr_ext_session::STATE_FINAL);
	ASSERT_NO_MESSAGE(d);
	ASSERT_NO_TIMER(d);


	/*
	 * WAITRESP ---[tg_TEARDOWN]---> FINAL
	 */
	nr_ext_session_test s5(nr_ext_session::STATE_WAITRESP);
	s5.set_last_ext_message(create_natfw_ext());
	event *e5 = new api_teardown_event(NULL);

	process(s5, e5);
	ASSERT_STATE(s5, nr_ext_session::STATE_FINAL);
	ASSERT_EXT_MESSAGE_SENT(d);
	ASSERT_NO_TIMER(d);
}


void ExtResponderTest::testSession() {
	/*
	 * SESSION ---[rx_RESPONSE(SUCCESS,EXT)]---> SESSION
	 */
	nr_ext_session_test s1(nr_ext_session::STATE_SESSION);
	s1.set_last_ext_message(create_natfw_ext());
	s1.get_refresh_timer().set_id(47);

	ntlp_msg *resp1 = create_natfw_response(information_code::sc_success,
		information_code::suc_successfully_processed,
		natfw_ext::MSG_TYPE);

	event *e1 = new msg_event(NULL, resp1);

	process(s1, e1);
	ASSERT_STATE(s1, nr_ext_session::STATE_SESSION);
	ASSERT_NO_MESSAGE(d);
	ASSERT_TIMER_STARTED(d, s1.get_refresh_timer());


	/*
	 * SESSION ---[rx_RESPONSE(ERROR,EXT)]---> FINAL
	 */
	nr_ext_session_test s2(nr_ext_session::STATE_SESSION);
	s2.set_last_ext_message(create_natfw_ext());

	ntlp_msg *resp2 = create_natfw_response(
		information_code::sc_permanent_failure, 0,
		natfw_ext::MSG_TYPE);

	event *e2 = new msg_event(NULL, resp2);

	process(s2, e2);
	ASSERT_STATE(s2, nr_ext_session::STATE_FINAL);
	ASSERT_NO_MESSAGE(d);
	ASSERT_NO_TIMER(d);


	/*
	 * SESSION ---[RESPONSE_TIMEOUT, retry]---> SESSION
	 */
	nr_ext_session_test s3(nr_ext_session::STATE_SESSION);
	// fake a previously sent EXT message
	s3.set_last_ext_message(create_natfw_ext());
	s3.get_response_timer().set_id(47);

	timer_event *e3 = new timer_event(NULL, 47);

	process(s3, e3);
	ASSERT_STATE(s3, nr_ext_session::STATE_SESSION);
	ASSERT_EXT_MESSAGE_SENT(d);
	ASSERT_TIMER_STARTED(d, s3.get_response_timer());


	/*
	 * SESSION ---[RESPONSE_TIMEOUT, no retry]---> FINAL
	 */
	nr_ext_session_test s4(nr_ext_session::STATE_WAITRESP);
	s4.set_ext_counter(1000);
	s4.get_response_timer().set_id(47);
	timer_event *e4 = new timer_event(NULL, 47);

	process(s4, e4);
	ASSERT_STATE(s4, nr_ext_session::STATE_FINAL);
	ASSERT_NO_MESSAGE(d);
	ASSERT_NO_TIMER(d);


	/*
	 * SESSION ---[tg_TEARDOWN]---> FINAL
	 */
	nr_ext_session_test s5(nr_ext_session::STATE_SESSION);
	s5.set_last_ext_message(create_natfw_ext());
	event *e5 = new api_teardown_event(NULL);

	process(s5, e5);
	ASSERT_STATE(s5, nr_ext_session::STATE_FINAL);
	ASSERT_EXT_MESSAGE_SENT(d);
	ASSERT_NO_TIMER(d);


	/*
	 * SESSION ---[REFRESH_TIMEOUT]---> SESSION
	 */
	nr_ext_session_test s6(nr_ext_session::STATE_SESSION);
	s6.set_last_ext_message(create_natfw_ext());
	s6.get_refresh_timer().set_id(0xABCD);
	event *e6 = new timer_event(NULL, 0xABCD);

	process(s6, e6);
	ASSERT_STATE(s6, nr_ext_session::STATE_SESSION);
	ASSERT_EXT_MESSAGE_SENT(d);
	ASSERT_TIMER_STARTED(d, s6.get_response_timer());
}


// EOF
