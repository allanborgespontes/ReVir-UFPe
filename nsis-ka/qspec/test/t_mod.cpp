/*
 * t_mod.cpp - Test the QSPEC Token Bucket Parameters TMOD
 *
 * $Id: t_mod.cpp 3236 2008-07-28 10:05:05Z bless $
 * $HeadURL: https://svn.tm.kit.edu/nsis/qspec/trunk/test/t_mod.cpp $
 *
 */
#include "generic_param_test.h"

using namespace qspec;


class T_ModTest : public GenericParamTest {

	CPPUNIT_TEST_SUITE( T_ModTest );

	QSPEC_PARAM_DEFAULT_TESTS();

	CPPUNIT_TEST_SUITE_END();

  public:
	virtual qspec_param *createInstance1() const {
		return new t_mod();
	}

	virtual qspec_param *createInstance2() const {
		return new t_mod(10.0, 100.0, 90.0, 500);
	}

	virtual void mutate1(qspec_param *p) const {
		t_mod *tb = dynamic_cast<t_mod *>(p);
		tb->set_bucket_depth(1000.0);
	}
};

CPPUNIT_TEST_SUITE_REGISTRATION( T_ModTest );

// EOF
