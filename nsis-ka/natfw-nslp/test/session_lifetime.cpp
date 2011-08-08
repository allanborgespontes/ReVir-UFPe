/*
 * Test the Session Lifetime Object
 *
 * $Id: session_lifetime.cpp 1707 2006-03-31 11:40:41Z stud-matfried $
 * $HeadURL: https://svn.tm.kit.edu/nsis/natfw-nslp/trunk/test/session_lifetime.cpp $
 */
#include "generic_object_test.h"

using namespace natfw::msg;


class SessionLifetimeTest : public GenericObjectTest {

	CPPUNIT_TEST_SUITE( SessionLifetimeTest );

	NATFW_OBJECT_DEFAULT_TESTS();

	CPPUNIT_TEST_SUITE_END();

  public:
	virtual natfw_object *createInstance1() const {
		return new session_lifetime();
	}

	virtual natfw_object *createInstance2() const {
		return new session_lifetime(30);
	}

	virtual void mutate1(natfw_object *o) const {
		session_lifetime *lt = dynamic_cast<session_lifetime *>(o);
		lt->set_value(50);
	}
};

CPPUNIT_TEST_SUITE_REGISTRATION( SessionLifetimeTest );

// EOF
