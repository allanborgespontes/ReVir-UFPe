/*
 * admission_priority.cpp - Test cases for the Admission Priority parameter
 *
 * $Id: admission_priority.cpp 1669 2006-02-16 12:42:55Z stud-matfried $
 * $HeadURL: https://svn.tm.kit.edu/nsis/qspec/trunk/test/admission_priority.cpp $
 *
 */
#include "generic_param_test.h"

using namespace qspec;


class AdmissionPriorityTest : public GenericParamTest {

	CPPUNIT_TEST_SUITE( AdmissionPriorityTest );

	QSPEC_PARAM_DEFAULT_TESTS();

	CPPUNIT_TEST_SUITE_END();

  public:
	virtual qspec_param *createInstance1() const {
		return new admission_priority();
	}

	virtual qspec_param *createInstance2() const {
		return new admission_priority(ap_high_priority_flow);
	}

	virtual void mutate1(qspec_param *p) const {
		admission_priority *e = dynamic_cast<admission_priority *>(p);
		e->set_value(ap_normal_priority_flow);
	}
};

CPPUNIT_TEST_SUITE_REGISTRATION( AdmissionPriorityTest );

// EOF
