/*
 * slack_term.cpp - Test the QSPEC Slack Term Parameter
 *
 * $Id: slack_term.cpp 344 2005-10-24 09:42:10Z stud-matfried $
 * $HeadURL: https://svn.tm.kit.edu/nsis/qspec/trunk/test/slack_term.cpp $
 *
 */
#include "generic_param_test.h"

using namespace qspec;


class SlackTermTest : public GenericParamTest {

	CPPUNIT_TEST_SUITE( SlackTermTest );

	QSPEC_PARAM_DEFAULT_TESTS();

	CPPUNIT_TEST_SUITE_END();

  public:
	virtual qspec_param *createInstance1() const {
		return new slack_term();
	}

	virtual qspec_param *createInstance2() const {
		return new slack_term(42);
	}

	virtual void mutate1(qspec_param *p) const {
		slack_term *s = dynamic_cast<slack_term *>(p);
		s->set_value(13);
	}
};

CPPUNIT_TEST_SUITE_REGISTRATION( SlackTermTest );

// EOF
