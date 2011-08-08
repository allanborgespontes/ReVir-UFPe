/*
 * Test the nf_non_edge_ext_session class.
 *
 * $Id: nf_non_edge_ext_session.cpp 2438 2006-12-08 15:07:34Z stud-matfried $
 * $HeadURL: https://svn.tm.kit.edu/nsis/natfw-nslp/trunk/test/nf_non_edge_ext_session.cpp $
 */
#include <cppunit/TestCase.h>
#include <cppunit/extensions/HelperMacros.h>

#include "events.h"
#include "session.h"

#include "utils.h" // custom assertions

using namespace natfw;
using namespace natfw::msg;


class ExtNonEdgeForwarderTest;


/*
 * We use a subclass for testing and make the test case a friend. This
 * way the test cases have access to protected methods and they don't have
 * to be public in the base class.
 */
class nf_non_edge_ext_session_test : public nf_non_edge_ext_session {
  public:
	nf_non_edge_ext_session_test(state_t state, const natfw_config *conf,
			uint32 start_msn=0)
			: nf_non_edge_ext_session(state, conf) {

		set_msg_sequence_number(start_msn);
	}

	friend class ExtNonEdgeForwarderTest;
};


class ExtNonEdgeForwarderTest : public CppUnit::TestCase {

	CPPUNIT_TEST_SUITE( ExtNonEdgeForwarderTest );

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

	void process(nf_non_edge_ext_session_test &s, event *evt);

	msg::ntlp_msg *create_natfw_ext(
		uint32 msn=START_MSN, uint32 lifetime=30) const;
	msg::ntlp_msg *create_natfw_response(uint8 severity,
		uint8 response_code, uint16 msg_type, uint32 msn) const;

	mock_natfw_config *conf;
	mock_dispatcher *d;
	nat_manager *nat_mgr;
	hostaddress source;
	hostaddress destination;
};

CPPUNIT_TEST_SUITE_REGISTRATION( ExtNonEdgeForwarderTest );


/*
 * Call the event handler and delete the event afterwards.
 */
void ExtNonEdgeForwarderTest::process(
		nf_non_edge_ext_session_test &s, event *evt) {

	d->clear();
	s.process(d, evt);
	delete evt;
}

void ExtNonEdgeForwarderTest::setUp() {
	nat_mgr = new nat_manager(hostaddress("141.3.70.4"), 20000, 30000);

	conf = new mock_natfw_config();
	conf->set_nf_is_nat(false);
	conf->set_nf_is_edge_nat(false);
	conf->set_nf_is_firewall(true);
	conf->set_nf_is_edge_firewall(false);

	d = new mock_dispatcher(NULL, nat_mgr, NULL, conf);

	source = hostaddress("141.3.70.5");
	destination = hostaddress("141.3.70.6");
}

void ExtNonEdgeForwarderTest::tearDown() {
	delete nat_mgr;
	delete d;
	delete conf;
}


msg::ntlp_msg *ExtNonEdgeForwarderTest::create_natfw_ext(
		uint32 msn, uint32 lifetime) const {

	natfw_ext *ext = new natfw_ext();
	ext->set_msg_sequence_number(msn);
	ext->set_session_lifetime(lifetime);

	ntlp::mri *ntlp_mri = new ntlp::mri_pathcoupled(
		hostaddress("192.168.0.4"), 32, 0,
		hostaddress("192.168.0.5"), 32, 0,
		"tcp", 0, 0, 0, false
	);

	return new ntlp_msg(session_id(), ext, ntlp_mri, 0);
}


msg::ntlp_msg *ExtNonEdgeForwarderTest::create_natfw_response(uint8 severity,
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


void ExtNonEdgeForwarderTest::testIdle() {
	/*
	 * IDLE ---[rx_EXT(Lifetime > 0) ---> WAITRESP
	 */
	nf_non_edge_ext_session_test s1(
		nf_non_edge_ext_session::STATE_IDLE, conf);
	event *e2 = new msg_event(NULL, create_natfw_ext());

	process(s1, e2);
	ASSERT_STATE(s1, nf_non_edge_ext_session::STATE_WAITRESP);
	ASSERT_EXT_MESSAGE_SENT(d);
	ASSERT_TIMER_STARTED(d, s1.get_state_timer());
}


void ExtNonEdgeForwarderTest::testWaitresp() {
	/*
	 * WAITRESP ---[rx_RESPONSE(SUCCESS,EXT)]---> SESSION
	 */
	nf_non_edge_ext_session_test s1(
		nf_non_edge_ext_session::STATE_WAITRESP, conf);
	s1.set_last_ext_message(create_natfw_ext(START_MSN));

	ntlp_msg *resp1 = create_natfw_response(information_code::sc_success,
		information_code::suc_successfully_processed,
		natfw_ext::MSG_TYPE, START_MSN);

	event *e1 = new msg_event(new session_id(s1.get_id()), resp1);

	process(s1, e1);
	ASSERT_STATE(s1, nf_non_edge_ext_session::STATE_SESSION);
	ASSERT_RESPONSE_MESSAGE_SENT(d, information_code::sc_success);
	ASSERT_TIMER_STARTED(d, s1.get_state_timer());


	/*
	 * WAITRESP ---[rx_RESPONSE(ERROR,EXT)]---> FINAL
	 */
	nf_non_edge_ext_session_test s2(
		nf_non_edge_ext_session::STATE_WAITRESP, conf);
	s2.set_last_ext_message(create_natfw_ext());

	ntlp_msg *resp2 = create_natfw_response(
		information_code::sc_permanent_failure, 0,
		natfw_ext::MSG_TYPE, START_MSN);

	event *e2 = new msg_event(new session_id(s2.get_id()), resp2);

	process(s2, e2);
	ASSERT_STATE(s2, nf_non_edge_ext_session::STATE_FINAL);
	ASSERT_RESPONSE_MESSAGE_SENT(d, information_code::sc_permanent_failure);
	ASSERT_NO_TIMER(d);


	/*
	 * WAITRESP ---[rx_EXT(Lifetime == 0) ]---> FINAL
	 */
	nf_non_edge_ext_session_test s3(
		nf_non_edge_ext_session::STATE_WAITRESP, conf);
	s3.set_last_ext_message(create_natfw_ext());

	event *e3 = new msg_event(new session_id(s3.get_id()),
			create_natfw_ext(START_MSN+1, 0));

	process(s3, e3);
	ASSERT_STATE(s3, nf_non_edge_ext_session::STATE_FINAL);
	ASSERT_EXT_MESSAGE_SENT(d);
	ASSERT_NO_TIMER(d);


	/*
	 * WAITRESP ---[STATE_TIMEOUT]---> FINAL
	 */
	nf_non_edge_ext_session_test s4(
		nf_non_edge_ext_session::STATE_WAITRESP, conf);
	s4.get_state_timer().set_id(47);
	s4.set_last_ext_message(create_natfw_ext());
	timer_event *e4 = new timer_event(NULL, 47);

	process(s4, e4);
	ASSERT_STATE(s4, nf_non_edge_ext_session::STATE_FINAL);
	ASSERT_RESPONSE_MESSAGE_SENT(d, information_code::sc_permanent_failure);
	ASSERT_NO_TIMER(d);
}


void ExtNonEdgeForwarderTest::testSession() {
	/*
	 * SESSION ---[rx_EXT && EXT(Lifetime > 0) ]---> SESSION
	 */
	nf_non_edge_ext_session_test s1(
		nf_non_edge_ext_session::STATE_SESSION, conf, START_MSN);
	event *e1 = new msg_event(NULL, create_natfw_ext(START_MSN+1));

	process(s1, e1);
	ASSERT_STATE(s1, nf_non_edge_ext_session::STATE_SESSION);
	CPPUNIT_ASSERT( s1.get_msg_sequence_number() == START_MSN+1 );
	ASSERT_EXT_MESSAGE_SENT(d);
	ASSERT_TIMER_STARTED(d, s1.get_response_timer());


	/*
	 * SESSION ---[rx_EXT && EXT(Lifetime > MAX) ]---> SESSION
	 */
	nf_non_edge_ext_session_test s2(
		nf_non_edge_ext_session::STATE_SESSION, conf, START_MSN);
	ntlp_msg *m2 = create_natfw_ext();
	natfw_ext *c2 = dynamic_cast<natfw_ext *>(m2->get_natfw_msg());
	c2->set_session_lifetime(1000000); // more than allowed
	event *e2 = new msg_event(NULL, m2);

	process(s2, e2);
	ASSERT_STATE(s2, nf_non_edge_ext_session::STATE_SESSION);
	ASSERT_RESPONSE_MESSAGE_SENT(d,
		information_code::sc_signaling_session_failures);
	ASSERT_NO_TIMER(d);


	/*
	 * SESSION ---[rx_EXT && EXT(Lifetime == 0) ]---> FINAL
	 */
	nf_non_edge_ext_session_test s3(
		nf_non_edge_ext_session::STATE_SESSION, conf, START_MSN);
	ntlp_msg *m3 = create_natfw_ext(START_MSN+1);
	natfw_ext *c3 = dynamic_cast<natfw_ext *>(m3->get_natfw_msg());
	c3->set_session_lifetime(0);
	event *e3 = new msg_event(NULL, m3);

	process(s3, e3);
	ASSERT_STATE(s3, nf_non_edge_ext_session::STATE_FINAL);
	ASSERT_EXT_MESSAGE_SENT(d);
	ASSERT_NO_TIMER(d);


	/*
	 * SESSION ---[rx_EXT && MSN too low ]---> SESSION
	 */
	nf_non_edge_ext_session_test s4(
		nf_non_edge_ext_session::STATE_SESSION, conf, START_MSN);
	ntlp_msg *m4 = create_natfw_ext(START_MSN);
	natfw_ext *c4 = dynamic_cast<natfw_ext *>(m4->get_natfw_msg());
	c4->set_msg_sequence_number(START_MSN);
	event *e4 = new msg_event(NULL, m4);

	process(s4, e4);
	ASSERT_STATE(s4, nf_non_edge_ext_session::STATE_SESSION);
	ASSERT_NO_MESSAGE(d);
	ASSERT_NO_TIMER(d);
}


// EOF
