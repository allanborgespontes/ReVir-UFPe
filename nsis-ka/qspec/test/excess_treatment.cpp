/*
 * excess_treatment.cpp - Test cases for the Excess Treatment parameter
 *
 * $Id: excess_treatment.cpp 3236 2008-07-28 10:05:05Z bless $
 * $HeadURL: https://svn.tm.kit.edu/nsis/qspec/trunk/test/excess_treatment.cpp $
 *
 */
#include "generic_param_test.h"

using namespace qspec;


class ExcessTreatmentTest : public GenericParamTest {

	CPPUNIT_TEST_SUITE( ExcessTreatmentTest );

	QSPEC_PARAM_DEFAULT_TESTS();

	CPPUNIT_TEST_SUITE_END();

  public:
	virtual qspec_param *createInstance1() const {
		return new excess_treatment();
	}

	virtual qspec_param *createInstance2() const {
	  return new excess_treatment(et_remark, 42);
	}

	virtual void mutate1(qspec_param *p) const {
		excess_treatment *e = dynamic_cast<excess_treatment *>(p);
		e->set_value(et_shape);
	}
};

CPPUNIT_TEST_SUITE_REGISTRATION( ExcessTreatmentTest );

// EOF
