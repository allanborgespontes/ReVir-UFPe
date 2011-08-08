/*
 * path_plr.cpp - Test the QSPEC Path PLR Parameter
 *
 * $Id: path_plr.cpp 399 2005-10-27 16:02:28Z stud-matfried $
 * $HeadURL: https://svn.tm.kit.edu/nsis/qspec/trunk/test/path_plr.cpp $
 *
 */
#include "generic_param_test.h"

using namespace qspec;


class PathPlrTest : public GenericParamTest {

	CPPUNIT_TEST_SUITE( PathPlrTest );

	QSPEC_PARAM_DEFAULT_TESTS();
	CPPUNIT_TEST( testValues );

	CPPUNIT_TEST_SUITE_END();

  public:
	virtual qspec_param *createInstance1() const {
		return new path_plr();
	}

	virtual qspec_param *createInstance2() const {
		return new path_plr(500000.0);
	}

	virtual void mutate1(qspec_param *p) const {
		path_plr *b = dynamic_cast<path_plr *>(p);
		b->set_value(1024.0);
	}

	void testValues() {
		path_plr *b = new path_plr(0.0);
		CPPUNIT_ASSERT( b->check() == true );

		b->set_value(-1.0);
		CPPUNIT_ASSERT( b->check() == false );

		b->set_value(10000.0);
		CPPUNIT_ASSERT( b->check() == true );

		delete b;
	}
};

CPPUNIT_TEST_SUITE_REGISTRATION( PathPlrTest );

// EOF
