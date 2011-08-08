/*
 * rph_priority.cpp - Test the RPH Priority parameter
 *
 * $Id: rph_priority.cpp 1670 2006-02-16 13:03:56Z stud-matfried $
 * $HeadURL: https://svn.tm.kit.edu/nsis/qspec/trunk/test/rph_priority.cpp $
 *
 */
#include "generic_param_test.h"

using namespace qspec;


class RphPriorityTest : public GenericParamTest {

	CPPUNIT_TEST_SUITE( RphPriorityTest );

	QSPEC_PARAM_DEFAULT_TESTS();

	CPPUNIT_TEST_SUITE_END();

  public:
	virtual qspec_param *createInstance1() const {
		return new rph_priority();
	}

	virtual qspec_param *createInstance2() const {
		return new rph_priority(rn_dsn, dsn_immediate);
	}

	virtual void mutate1(qspec_param *p) const {
		rph_priority *pp
			= dynamic_cast<rph_priority *>(p);

		pp->set_rpn_priority(dsn_flash);
	}
};

CPPUNIT_TEST_SUITE_REGISTRATION( RphPriorityTest );

// EOF
