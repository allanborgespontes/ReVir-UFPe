/*
 * dste_class_type.cpp - The QSPEC DSTE Class Type Parameter
 *
 * $Id: dste_class_type.cpp 345 2005-10-24 10:07:47Z stud-matfried $
 * $HeadURL: https://svn.tm.kit.edu/nsis/qspec/trunk/test/dste_class_type.cpp $
 *
 */
#include "generic_param_test.h"

using namespace qspec;


class DsteClassTypeTest : public GenericParamTest {

	CPPUNIT_TEST_SUITE( DsteClassTypeTest );

	QSPEC_PARAM_DEFAULT_TESTS();
	CPPUNIT_TEST( testInvalidClass );

	CPPUNIT_TEST_SUITE_END();

  public:
	virtual qspec_param *createInstance1() const {
		return new dste_class_type();
	}

	virtual qspec_param *createInstance2() const {
		return new dste_class_type(3);
	}

	virtual void mutate1(qspec_param *p) const {
		dste_class_type *d = dynamic_cast<dste_class_type *>(p);
		d->set_value(5);
	}

	virtual void testInvalidClass() {
		// Error: Only 0 to 7 are allowed
		dste_class_type *d = new dste_class_type(8);
		CPPUNIT_ASSERT( d->check() == false );
	}
};

CPPUNIT_TEST_SUITE_REGISTRATION( DsteClassTypeTest );

// EOF
