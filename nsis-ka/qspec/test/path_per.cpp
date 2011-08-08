/*
 * path_per.cpp - Test the QSPEC Path PER Parameter
 *
 * $Id: path_per.cpp 504 2005-11-01 10:18:20Z stud-matfried $
 * $HeadURL: https://svn.tm.kit.edu/nsis/qspec/trunk/test/path_per.cpp $
 *
 */
#include "generic_param_test.h"

using namespace qspec;


class PathPerTest : public GenericParamTest {

	CPPUNIT_TEST_SUITE( PathPerTest );

	QSPEC_PARAM_DEFAULT_TESTS();
	CPPUNIT_TEST( testValues );

	CPPUNIT_TEST_SUITE_END();

  public:
	virtual qspec_param *createInstance1() const {
		return new path_per();
	}

	virtual qspec_param *createInstance2() const {
		return new path_per(500000.0);
	}

	virtual void mutate1(qspec_param *p) const {
		path_per *b = dynamic_cast<path_per *>(p);
		b->set_value(1024.0);
	}

	void testValues() {
		path_per *b = new path_per(0.0);
		CPPUNIT_ASSERT( b->check() == true );

		b->set_value(-1.0);
		CPPUNIT_ASSERT( b->check() == false );

		b->set_value(10000.0);
		CPPUNIT_ASSERT( b->check() == true );

		delete b;
	}
};

CPPUNIT_TEST_SUITE_REGISTRATION( PathPerTest );

// EOF
