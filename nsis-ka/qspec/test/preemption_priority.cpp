/*
 * preemption_priority.cpp - Test the Preemption/Defending Priority parameter
 *
 * $Id: preemption_priority.cpp 349 2005-10-24 11:45:49Z stud-matfried $
 * $HeadURL: https://svn.tm.kit.edu/nsis/qspec/trunk/test/preemption_priority.cpp $
 *
 */
#include "generic_param_test.h"

using namespace qspec;


class PreemptionPriorityTest : public GenericParamTest {

	CPPUNIT_TEST_SUITE( PreemptionPriorityTest );

	QSPEC_PARAM_DEFAULT_TESTS();

	CPPUNIT_TEST_SUITE_END();

  public:
	virtual qspec_param *createInstance1() const {
		return new preemption_priority();
	}

	virtual qspec_param *createInstance2() const {
		return new preemption_priority(42, 77);
	}

	virtual void mutate1(qspec_param *p) const {
		preemption_priority *pp
			= dynamic_cast<preemption_priority *>(p);

		pp->set_defending_priority(9);
	}
};

CPPUNIT_TEST_SUITE_REGISTRATION( PreemptionPriorityTest );

// EOF
