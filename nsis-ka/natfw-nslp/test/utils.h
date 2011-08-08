/*
 * Utilities to make testing easier.
 *
 * $Id: utils.h 4118 2009-07-16 16:13:10Z bless $
 * $HeadURL: https://svn.tm.kit.edu/nsis/natfw-nslp/trunk/test/utils.h $
 */
#ifndef TESTSUITE_UTILS_H
#define TESTSUITE_UTILS_H

#include <cppunit/TestCase.h>
#include <cppunit/extensions/HelperMacros.h>

#include "protlib_types.h"
#include "dispatcher.h"
#include "gist_conf.h"

class mock_config;
class mock_dispatcher;
class mock_policy_rule_installer;


#define ASSERT_STATE(session, state) \
  CPPUNIT_ASSERT_MESSAGE("state mismatch", session.get_state() == state)

#define ASSERT_NO_MESSAGE(d) \
  CPPUNIT_ASSERT_MESSAGE("message has been sent", d->get_message() == NULL)

#define ASSERT_NO_TIMER(d) \
  CPPUNIT_ASSERT_MESSAGE("timer has been started", d->get_timer() == 0)

#define ASSERT_CREATE_MESSAGE_SENT(d) \
  CPPUNIT_ASSERT_MESSAGE("no create message sent", \
    ( d->get_message() != NULL && dynamic_cast<natfw::msg::natfw_create *>( \
	d->get_message()->get_natfw_msg()) != NULL ) )

#define ASSERT_EXT_MESSAGE_SENT(d) \
  CPPUNIT_ASSERT_MESSAGE("no rea message sent", \
    ( d->get_message() != NULL && dynamic_cast<natfw::msg::natfw_ext *>( \
	d->get_message()->get_natfw_msg()) != NULL ) )

#define ASSERT_TIMER_STARTED(d, timer) \
  CPPUNIT_ASSERT_MESSAGE("no timer started", d->get_timer() == timer.get_id())


void checkResponse(mock_dispatcher *d, protlib::uint8 severity,
	CppUnit::SourceLine line);

#define ASSERT_RESPONSE_MESSAGE_SENT(d, severity) \
  checkResponse(d, severity, CPPUNIT_SOURCELINE())


using namespace natfw;

/**
 * A fake naftw_config class for testing.
 */
class mock_natfw_config : public natfw_config {
  public:
	mock_natfw_config() { 
		repository_init(); 
		setRepository();	
		// register all GIST configuration parameters at the registry
		ntlp::gconf.setRepository();
	}

	void set_nf_is_nat(bool val) { setpar(natfwconf_nf_is_nat,val); }
	void set_nf_is_firewall(bool val) { setpar(natfwconf_nf_is_firewall, val); }
	void set_nf_is_edge_nat(bool val) { setpar(natfwconf_nf_is_edge_nat, val); }
	void set_nf_is_edge_firewall(bool val) { setpar(natfwconf_nf_is_edge_firewall,val); }
};


/**
 * A fake dispatcher class for testing.
 */
class mock_dispatcher : public dispatcher {
  public:
	mock_dispatcher(session_manager *m=NULL, nat_manager *n=NULL,
			policy_rule_installer *p=NULL, natfw_config *conf=NULL)
		: dispatcher(m, n, p, conf ? conf : new mock_natfw_config()),
		  message(NULL), timer(0), next_timer_id(1) { }
	~mock_dispatcher() { clear(); }

	/*
	 * Inherited from parent:
	 */
	virtual void send_message(msg::ntlp_msg *msg) throw ();
	virtual id_t start_timer(const session *s, int secs) throw ();
	virtual bool is_from_private_side(
			const hostaddress &addr) const throw();

	/*
	 * New methods:
	 */
	void clear() throw ();
	msg::ntlp_msg *get_message() throw ();
	id_t get_timer() throw ();

  private:
	msg::ntlp_msg *message;

	id_t timer;
	id_t next_timer_id;
};


#endif // TESTSUITE_UTILS_H
