/*
 * generic_attr_test.h - A generic test case for session auth attributes
 *
 * $Id:$
 * $HeadURL:$
 *
 */
#include <cppunit/TestCase.h>
#include <cppunit/extensions/HelperMacros.h>

#include "auth_attr.h"

using namespace nslp_auth;


/**
 * Helper macro to register test cases in child classes.
 */
#define AUTH_ATTR_DEFAULT_TESTS() \
	CPPUNIT_TEST( testMyself ); \
	CPPUNIT_TEST( testComparison ); \
	CPPUNIT_TEST( testCopying ); \
	CPPUNIT_TEST( testSerialize ); \
	CPPUNIT_TEST( testReadWrite );


/**
 * The abstract base class for all attribute tests.
 *
 * This class uses the Strategy design pattern to run a set of tests which
 * are the same for all auth_attr child classes. Additional to those
 * tests, child classes may define more specific test cases.
 */
class GenericAttributeTest : public CppUnit::TestCase {

  public:
  	/**
	 * Create an instance using the default constructor.
	 *
	 * @return an instance created using the default constructor
	 */
	virtual auth_attr *createInstance1() const = 0;

	/**
	 * Create an instance that is not equal to an instance created
	 * by the default constructor or created by createInstance1().
	 *
	 * @return an instance different from that created by createInstance1()
	 */
	virtual auth_attr *createInstance2() const = 0;

	/**
	 * Modify the given attribute.
	 *
	 * An instance created by createInstance1() that is modified by
	 * this method must no longer be equal to that newly created instance.
	 *
	 * @param param the attribute to modify
	 */
	virtual void mutate1(auth_attr *attribute) const = 0;

	// Note: Adding a "const" modifier breaks cppunit.
	virtual void testMyself();
	virtual void testComparison();
	virtual void testCopying();
	virtual void testSerialize();
	virtual void testReadWrite();
};

// EOF
