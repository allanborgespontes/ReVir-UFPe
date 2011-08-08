/*
 * generic_param_test.h - A generic test case for QSPEC parameters
 *
 * $Id: generic_param_test.h 280 2005-10-20 14:43:41Z stud-matfried $
 * $HeadURL: https://svn.tm.kit.edu/nsis/qspec/trunk/test/generic_param_test.h $
 *
 */
#include <cppunit/TestCase.h>
#include <cppunit/extensions/HelperMacros.h>

#include "qspec_param.h"

using namespace qspec;


/**
 * Helper macro to register test cases in child classes.
 */
#define QSPEC_PARAM_DEFAULT_TESTS() \
	CPPUNIT_TEST( testMyself ); \
	CPPUNIT_TEST( testComparison ); \
	CPPUNIT_TEST( testCopying ); \
	CPPUNIT_TEST( testSerialize ); \
	CPPUNIT_TEST( testReadWrite );


/**
 * The abstract base class for all parameter tests.
 *
 * This class uses the Strategy design pattern to run a set of tests which
 * are the same for all qspec_param child classes. Additional to those
 * tests, child classes may define more specific test cases.
 */
class GenericParamTest : public CppUnit::TestCase {

  protected:
	/**
	 * Create an instance using the default constructor.
	 *
	 * @return an instance created using the default constructor
	 */
	virtual qspec_param *createInstance1() const = 0;

	/**
	 * Create an instance that is not equal to an instance created
	 * by the default constructor or created by createInstance1().
	 *
	 * @return an instance different from that created by createInstance1()
	 */
	virtual qspec_param *createInstance2() const = 0;

	/**
	 * Modify the given parameter.
	 *
	 * An instance created by createInstance1() that is modified by
	 * this method must no longer be equal to that newly created instance.
	 *
	 * @param param the parameter to modify
	 */
	virtual void mutate1(qspec_param *param) const = 0;

  public:
	// Note: Adding a "const" modifier breaks cppunit.
	virtual void testMyself();
	virtual void testComparison();
	virtual void testCopying();
	virtual void testSerialize();
	virtual void testReadWrite();
};

// EOF
