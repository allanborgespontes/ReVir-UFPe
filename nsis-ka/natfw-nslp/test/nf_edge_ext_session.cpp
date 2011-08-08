/*
 * Test the nf_edge_ext_session class.
 *
 * $Id: nf_edge_ext_session.cpp 2438 2006-12-08 15:07:34Z stud-matfried $
 * $HeadURL: https://svn.tm.kit.edu/nsis/natfw-nslp/trunk/test/nf_edge_ext_session.cpp $
 */
#include <cppunit/TestCase.h>
#include <cppunit/extensions/HelperMacros.h>

#include "events.h"
#include "session.h"

#include "utils.h" // custom assertions

using namespace natfw;
using namespace natfw::msg;


class ExtEdgeForwarderTest;


/*
 * We use a subclass for testing and make the test case a friend. This
 * way the test cases have access to protected methods and they don't have
 * to be public in the base class.
 */
class nf_edge_ext_session_test : public nf_edge_ext_session {
  public:
	nf_edge_ext_session_test(state_t state, const natfw_config *conf,
			uint32 start_msn=0) : nf_edge_ext_session(state, conf) {
		set_msg_sequence_number(start_msn);
	}

	friend class ExtEdgeForwarderTest;
};


class ExtEdgeForwarderTest : public CppUnit::TestCase {

	CPPUNIT_TEST_SUITE( ExtEdgeForwarderTest );

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

	void process(nf_edge_ext_session_test &s, event *evt);

	msg::ntlp_msg *create_natfw_ext() const;

	mock_natfw_config *conf;
	mock_dispatcher *d;
	nat_manager *nat_mgr;
	policy_rule_installer *rule_installer;
	hostaddress source;
	hostaddress destination;
};

CPPUNIT_TEST_SUITE_REGISTRATION( ExtEdgeForwarderTest );


/*
 * Call the event handler and delete the event afterwards.
 */
void ExtEdgeForwarderTest::process(nf_edge_ext_session_test &s, event *evt) {
	d->clear();
	s.process(d, evt);
	delete evt;
}

void ExtEdgeForwarderTest::setUp() {
	conf = new mock_natfw_config();
	conf->set_nf_is_nat(false);
	conf->set_nf_is_edge_nat(false);
	conf->set_nf_is_firewall(true);
	conf->set_nf_is_edge_firewall(true);

	nat_mgr = new nat_manager(hostaddress("141.3.70.4"), 20000, 30000);
	rule_installer = new nop_policy_rule_installer(conf);
	d = new mock_dispatcher(NULL, nat_mgr, rule_installer, conf);
	source = hostaddress("141.3.70.5");
	destination = hostaddress("141.3.70.6");
}

void ExtEdgeForwarderTest::tearDown() {
	delete d;
	delete rule_installer;
	delete nat_mgr;
	delete conf;
}


msg::ntlp_msg *ExtEdgeForwarderTest::create_natfw_ext() const {
	natfw_ext *ext = new natfw_ext();
	ext->set_msg_sequence_number(START_MSN+1);
	ext->set_session_lifetime(30);

	ntlp::mri *ntlp_mri = new ntlp::mri_pathcoupled(
		hostaddress("192.168.0.4"), 32, 0,
		hostaddress("192.168.0.5"), 32, 0,
		"tcp", 0, 0, 0, false
	);

	return new ntlp_msg(session_id(), ext, ntlp_mri, 0);
}


void ExtEdgeForwarderTest::testIdle() {
	/*
	 * IDLE ---[rx_EXT && !IS_PUBLICSIDE && IS_EDGE 
	 *                 && CHECK_AA && CREATE(Lifetime>0) ]---> SESSION
	 */
	nf_edge_ext_session_test s1(nf_edge_ext_session::STATE_IDLE, conf);
	event *e1 = new msg_event(NULL, create_natfw_ext());

	process(s1, e1);
	ASSERT_STATE(s1, nf_edge_ext_session::STATE_SESSION);
	ASSERT_RESPONSE_MESSAGE_SENT(d, information_code::sc_success);
	ASSERT_TIMER_STARTED(d, s1.get_state_timer());
}


void ExtEdgeForwarderTest::testSession() {
	/*
	 * SESSION ---[rx_EXT && CHECK_AA && EXT(Lifetime > 0) ]---> SESSION
	 */
	nf_edge_ext_session_test s1(
		nf_edge_ext_session::STATE_SESSION, conf, START_MSN);
	event *e1 = new msg_event(NULL, create_natfw_ext());

	process(s1, e1);
	ASSERT_STATE(s1, nf_edge_ext_session::STATE_SESSION);
	CPPUNIT_ASSERT( s1.get_msg_sequence_number() == START_MSN+1 );
	ASSERT_RESPONSE_MESSAGE_SENT(d, information_code::sc_success);
	ASSERT_TIMER_STARTED(d, s1.get_state_timer());


	/*
	 * SESSION ---[rx_EXT && EXT(Lifetime > MAX) ]---> SESSION
	 */
	nf_edge_ext_session_test s2(
		nf_edge_ext_session::STATE_SESSION, conf, START_MSN);
	ntlp_msg *m2 = create_natfw_ext();
	natfw_ext *c2 = dynamic_cast<natfw_ext *>(m2->get_natfw_msg());
	c2->set_session_lifetime(1000000); // more than allowed
	event *e2 = new msg_event(NULL, m2);

	process(s2, e2);
	ASSERT_STATE(s2, nf_edge_ext_session::STATE_SESSION);
	ASSERT_RESPONSE_MESSAGE_SENT(d,
		information_code::sc_signaling_session_failures);
	ASSERT_NO_TIMER(d);


	/*
	 * SESSION ---[rx_EXT && EXT(Lifetime == 0) ]---> FINAL
	 */
	nf_edge_ext_session_test s3(
		nf_edge_ext_session::STATE_SESSION, conf, START_MSN);
	ntlp_msg *m3 = create_natfw_ext();
	natfw_ext *c3 = dynamic_cast<natfw_ext *>(m3->get_natfw_msg());
	c3->set_session_lifetime(0);
	event *e3 = new msg_event(NULL, m3);

	process(s3, e3);
	ASSERT_STATE(s3, nf_edge_ext_session::STATE_FINAL);
	ASSERT_NO_MESSAGE(d);
	ASSERT_NO_TIMER(d);


	/*
	 * SESSION ---[rx_EXT && MSN too low ]---> SESSION
	 */
	nf_edge_ext_session_test s4(
		nf_edge_ext_session::STATE_SESSION, conf, START_MSN);
	ntlp_msg *m4 = create_natfw_ext();
	natfw_ext *c4 = dynamic_cast<natfw_ext *>(m4->get_natfw_msg());
	c4->set_msg_sequence_number(START_MSN);
	event *e4 = new msg_event(NULL, m4);

	process(s4, e4);
	ASSERT_STATE(s4, nf_edge_ext_session::STATE_SESSION);
	ASSERT_NO_MESSAGE(d);
	ASSERT_NO_TIMER(d);
}


// EOF
