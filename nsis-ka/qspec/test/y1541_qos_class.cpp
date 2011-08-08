/*
 * y1541_qos_class.cpp - The QSPEC Y.1541 QoS Class Parameter
 *
 * $Id: y1541_qos_class.cpp 345 2005-10-24 10:07:47Z stud-matfried $
 * $HeadURL: https://svn.tm.kit.edu/nsis/qspec/trunk/test/y1541_qos_class.cpp $
 *
 */
#include "generic_param_test.h"

using namespace qspec;


class Y1541QoSClassTest : public GenericParamTest {

	CPPUNIT_TEST_SUITE( Y1541QoSClassTest );

	QSPEC_PARAM_DEFAULT_TESTS();
	CPPUNIT_TEST( testInvalidClass );

	CPPUNIT_TEST_SUITE_END();

  public:
	virtual qspec_param *createInstance1() const {
		return new y1541_qos_class();
	}

	virtual qspec_param *createInstance2() const {
		return new y1541_qos_class(3);
	}

	virtual void mutate1(qspec_param *p) const {
		y1541_qos_class *phb = dynamic_cast<y1541_qos_class *>(p);
		phb->set_value(5);
	}

	virtual void testInvalidClass() {
		// Error: Only 0 to 5 are allowed
		y1541_qos_class *p = new y1541_qos_class(7);
		CPPUNIT_ASSERT( p->check() == false );
	}
};

CPPUNIT_TEST_SUITE_REGISTRATION( Y1541QoSClassTest );

// EOF
