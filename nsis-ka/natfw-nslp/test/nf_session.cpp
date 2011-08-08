/*
 * Test the nf_session class.
 *
 * $Id: nf_session.cpp 1936 2006-07-25 15:13:45Z stud-matfried $
 * $HeadURL: https://svn.tm.kit.edu/nsis/natfw-nslp/trunk/test/nf_session.cpp $
 */
#include <cppunit/TestCase.h>
#include <cppunit/extensions/HelperMacros.h>

#include "events.h"
#include "session.h"
#include "dispatcher.h"
#include "policy_rule.h"

#include "utils.h" // custom assertions

using namespace natfw;
using namespace natfw::msg;

class ForwarderTest;

/*
 * We use a subclass for testing and make the test case a friend. This
 * way the test cases have access to protected methods and they don't have
 * to be public in nf_session.
 */
class nf_session_test : public nf_session {
  public:
	nf_session_test(state_t state, natfw_config *conf)
		: nf_session(state, conf) { }

	friend class ForwarderTest;
};


class ForwarderTest : public CppUnit::TestCase {

	CPPUNIT_TEST_SUITE( ForwarderTest );

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
	void process(nf_session_test &s, event *evt);

	static const uint32 START_MSN = 77;
	static const uint32 SESSION_LIFETIME = 30;

	msg::ntlp_msg *create_natfw_create(uint32 msn=START_MSN,
		uint32 session_lifetime=SESSION_LIFETIME) const;

	msg::ntlp_msg *create_natfw_response(
		uint8 severity, uint8 response_code, uint16 msg_type,
		uint32 msn=START_MSN) const;

	fw_policy_rule *create_fw_policy_rule() const;

	mock_natfw_config *conf;
	policy_rule_installer *rule_installer;
	mock_dispatcher *d;
	hostaddress destination;
};

CPPUNIT_TEST_SUITE_REGISTRATION( ForwarderTest );


/*
 * Call the event handler and delete the event afterwards.
 */
void ForwarderTest::process(nf_session_test &s, event *evt) {
	d->clear();
	s.process(d, evt);
	delete evt;
}

void ForwarderTest::setUp() {
	conf = new mock_natfw_config();
	conf->set_nf_is_firewall(true);
	rule_installer = new nop_policy_rule_installer(conf);
	d = new mock_dispatcher(NULL, NULL, rule_installer, conf);
	destination = hostaddress("141.3.70.4");
}

void ForwarderTest::tearDown() {
	delete d;
	delete rule_installer;
	delete conf;
}


msg::ntlp_msg *ForwarderTest::create_natfw_create(uint32 msn, uint32 lt) const {

	msg::natfw_create *create = new natfw_create();
	create->set_msg_sequence_number(msn);
	create->set_session_lifetime(lt);

	ntlp::mri *ntlp_mri = new ntlp::mri_pathcoupled(
		hostaddress("192.168.0.4"), 32, 0,
		hostaddress("192.168.0.5"), 32, 0,
		"tcp", 0, 0, 0, true
	);

	return new msg::ntlp_msg(session_id(), create, ntlp_mri, 0);
}


msg::ntlp_msg *ForwarderTest::create_natfw_response(uint8 severity,
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


fw_policy_rule *ForwarderTest::create_fw_policy_rule() const {
	return new fw_policy_rule(fw_policy_rule::ACTION_ALLOW,
		hostaddress("141.3.70.4"), 32, 0,
		hostaddress("141.3.70.1"), 32, 0, 0);
}

void ForwarderTest::testIdle() {
	/*
	 * IDLE ---[rx_CREATE && CREATE(Lifetime>0) ]---> WAITRESP
	 */
	nf_session_test s1(nf_session::STATE_IDLE, conf);
	event *e1 = new msg_event(new session_id(s1.get_id()),
		create_natfw_create());

	process(s1, e1);
	ASSERT_STATE(s1, nf_session::STATE_WAITRESP);
	ASSERT_CREATE_MESSAGE_SENT(d);
	ASSERT_TIMER_STARTED(d, s1.get_state_timer());

	/*
	 * IDLE ---[rx_CREATE && CREATE(Lifetime > MAX) ]---> FINAL
	 */
	nf_session_test s2(nf_session::STATE_IDLE, conf);
	event *e2 = new msg_event(new session_id(s2.get_id()),
		create_natfw_create(START_MSN, 1000000)); // more than allowed

	process(s2, e2);
	ASSERT_STATE(s2, nf_session::STATE_FINAL);
	ASSERT_RESPONSE_MESSAGE_SENT(d,
		information_code::sc_signaling_session_failures);
	ASSERT_NO_TIMER(d);
}


void ForwarderTest::testWaitresp() {
	/*
	 * WAITRESP ---[rx_RESPONSE(SUCCESS,CREATE)]---> SESSION
	 */
	nf_session_test s1(nf_session::STATE_WAITRESP, conf);
	s1.set_fw_policy_rule(create_fw_policy_rule());
	s1.set_last_create_message(create_natfw_create());

	ntlp_msg *resp1 = create_natfw_response(information_code::sc_success,
		information_code::suc_successfully_processed,
		natfw_create::MSG_TYPE, START_MSN);

	event *e1 = new msg_event(new session_id(s1.get_id()), resp1);

	process(s1, e1);
	ASSERT_STATE(s1, nf_session::STATE_SESSION);
	ASSERT_RESPONSE_MESSAGE_SENT(d, information_code::sc_success);
	ASSERT_TIMER_STARTED(d, s1.get_state_timer());


	/*
	 * WAITRESP ---[rx_RESPONSE(ERROR,CREATE)]---> FINAL
	 */
	nf_session_test s2(nf_session::STATE_WAITRESP, conf);
	s2.set_fw_policy_rule(create_fw_policy_rule());
	s2.set_last_create_message(create_natfw_create());

	ntlp_msg *resp2 = create_natfw_response(
		information_code::sc_permanent_failure, 0,
		natfw_create::MSG_TYPE, START_MSN);

	event *e2 = new msg_event(new session_id(s2.get_id()), resp2);

	process(s2, e2);
	ASSERT_STATE(s2, nf_session::STATE_FINAL);
	ASSERT_RESPONSE_MESSAGE_SENT(d, information_code::sc_permanent_failure);
	ASSERT_NO_TIMER(d);


	/*
	 * WAITRESP ---[rx_CREATE && CREATE(Lifetime == 0) ]---> FINAL
	 */
	nf_session_test s3(nf_session::STATE_WAITRESP, conf);
	s3.set_fw_policy_rule(create_fw_policy_rule());
	s3.set_last_create_message(create_natfw_create());

	event *e3 = new msg_event(new session_id(s3.get_id()),
			create_natfw_create(START_MSN+1, 0));

	process(s3, e3);
	ASSERT_STATE(s3, nf_session::STATE_FINAL);
	ASSERT_CREATE_MESSAGE_SENT(d);
	ASSERT_NO_TIMER(d);

	/*
	 * WAITRESP ---[STATE_TIMEOUT]---> FINAL
	 */
	nf_session_test s4(nf_session::STATE_WAITRESP, conf);
	s4.set_fw_policy_rule(create_fw_policy_rule());
	s4.get_state_timer().set_id(47);
	s4.set_last_create_message(create_natfw_create());
	timer_event *e4 = new timer_event(NULL, 47);

	process(s4, e4);
	ASSERT_STATE(s4, nf_session::STATE_FINAL);
	ASSERT_RESPONSE_MESSAGE_SENT(d, information_code::sc_permanent_failure);
	ASSERT_NO_TIMER(d);
}


void ForwarderTest::testSession() {
	/*
	 * SESSION ---[RESPONSE_TIMEOUT]---> FINAL
	 */
	nf_session_test s1(nf_session::STATE_SESSION, conf);
	s1.set_fw_policy_rule(create_fw_policy_rule());
	s1.set_last_create_message(create_natfw_create());
	s1.get_response_timer().set_id(47);

	timer_event *e1 = new timer_event(NULL, 47);

	process(s1, e1);
	ASSERT_STATE(s1, nf_session::STATE_FINAL);
	ASSERT_RESPONSE_MESSAGE_SENT(d, information_code::sc_permanent_failure);
	ASSERT_NO_TIMER(d);


	/*
	 * SESSION ---[rx_RESPONSE(SUCCESS,CREATE)]---> SESSION
	 */
	nf_session_test s2(nf_session::STATE_WAITRESP, conf);
	s2.set_fw_policy_rule(create_fw_policy_rule());
	s2.set_last_create_message(create_natfw_create());

	ntlp_msg *resp2 = create_natfw_response(information_code::sc_success,
		information_code::suc_successfully_processed,
		natfw_create::MSG_TYPE);

	event *e2 = new msg_event(new session_id(s2.get_id()), resp2);

	process(s2, e2);
	ASSERT_STATE(s2, nf_session::STATE_SESSION);
	ASSERT_RESPONSE_MESSAGE_SENT(d, information_code::sc_success);
	ASSERT_TIMER_STARTED(d, s2.get_state_timer());


	/*
	 * SESSION ---[rx_CREATE && CREATE(Lifetime == 0) ]---> FINAL
	 */
	nf_session_test s3(nf_session::STATE_SESSION, conf);
	s3.set_fw_policy_rule(create_fw_policy_rule());
	s3.set_last_create_message(create_natfw_create());

	event *e3 = new msg_event(new session_id(s3.get_id()),
		create_natfw_create(START_MSN+1, 0));

	process(s3, e3);
	ASSERT_STATE(s3, nf_session::STATE_FINAL);
	ASSERT_CREATE_MESSAGE_SENT(d);
	ASSERT_NO_TIMER(d);


	/*
	 * SESSION ---[STATE_TIMEOUT]---> FINAL
	 */
	nf_session_test s4(nf_session::STATE_SESSION, conf);
	s4.set_fw_policy_rule(create_fw_policy_rule());
	s4.set_last_create_message(create_natfw_create());
	s4.get_state_timer().set_id(47);

	timer_event *e4 = new timer_event(NULL, 47);

	process(s4, e4);
	ASSERT_STATE(s4, nf_session::STATE_FINAL);
	ASSERT_NO_MESSAGE(d);
	ASSERT_NO_TIMER(d);


	/*
	 * SESSION ---[rx_CREATE && CREATE(Lifetime > 0) ]---> SESSION
	 */
	nf_session_test s5(nf_session::STATE_SESSION, conf);
	s5.set_fw_policy_rule(create_fw_policy_rule());
	s5.set_last_create_message(create_natfw_create());

	event *e5 = new msg_event(new session_id(s5.get_id()),
		create_natfw_create(START_MSN+1, 20));

	process(s5, e5);
	ASSERT_STATE(s5, nf_session::STATE_SESSION);
	ASSERT_CREATE_MESSAGE_SENT(d);
	ASSERT_TIMER_STARTED(d, s5.get_response_timer());
}


// EOF
