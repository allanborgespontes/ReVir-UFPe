/*
 * path_jitter.cpp - Test the QSPEC Path Jitter Parameter
 *
 * $Id: path_jitter.cpp 383 2005-10-26 09:37:08Z stud-matfried $
 * $HeadURL: https://svn.tm.kit.edu/nsis/qspec/trunk/test/path_jitter.cpp $
 *
 */
#include "generic_param_test.h"

using namespace qspec;


class PathJitterTest : public GenericParamTest {

	CPPUNIT_TEST_SUITE( PathJitterTest );

	QSPEC_PARAM_DEFAULT_TESTS();
	CPPUNIT_TEST( testMisc );

	CPPUNIT_TEST_SUITE_END();

  public:
	virtual qspec_param *createInstance1() const {
		return new path_jitter();
	}

	virtual qspec_param *createInstance2() const {
		return new path_jitter(200000, 30000, 4000);
	}

	virtual void mutate1(qspec_param *p) const {
		path_jitter *b = dynamic_cast<path_jitter *>(p);
		b->set_value1(10000);
	}

	virtual void testMisc() {
		path_jitter *pj1 = new path_jitter(200000, 30000, 4000);
		CPPUNIT_ASSERT( pj1->get_value1() == 200000 );
		CPPUNIT_ASSERT( pj1->get_value2() == 30000 );
		CPPUNIT_ASSERT( pj1->get_value3() == 4000 );

		pj1->set_value1(42);
		pj1->set_value2(43);
		pj1->set_value3(44);

		CPPUNIT_ASSERT( pj1->get_value1() == 42 );
		CPPUNIT_ASSERT( pj1->get_value2() == 43 );
		CPPUNIT_ASSERT( pj1->get_value3() == 44 );

		path_jitter *pj2 = new path_jitter(1, 2, 3);
		CPPUNIT_ASSERT( *pj1 != *pj2 );

		pj1->set_value1(1);
		CPPUNIT_ASSERT( *pj1 != *pj2 );

		pj1->set_value2(2);
		pj1->set_value3(3);
		CPPUNIT_ASSERT( *pj1 == *pj2 );

		delete pj2;
		delete pj1;
	}
};

CPPUNIT_TEST_SUITE_REGISTRATION( PathJitterTest );

// EOF
