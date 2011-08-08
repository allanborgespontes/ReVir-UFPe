/*
 * Boilerplate test code and custom assertions to make testing easier.
 *
 * This file should be included from all test case .cpp files.
 *
 * $Id: test_suite.h 4885 2010-01-17 12:41:18Z bless $
 * $HeadURL: https://svn.tm.kit.edu/nsis/qos-nslp/trunk/unittests/test_suite.h $
 */
#ifndef TESTSUITE_UTILS_H
#define TESTSUITE_UTILS_H

#include <cppunit/TestCase.h>
#include <cppunit/extensions/HelperMacros.h>

#include "ie.h"


/*
 * Custom assertions.
 */

// Test if instantiation using IE::new_instance() works.
#define ASSERT_INSTANTIABLE(ie) \
	test_is_instantiable(ie, CPPUNIT_SOURCELINE())

// Test if copying works.
#define ASSERT_COPYABLE(ie) \
	test_is_copyable(ie, CPPUNIT_SOURCELINE())

// Test if an IE is serialized correctly.
#define ASSERT_SERIALIZABLE(ie) \
	test_is_serializable(ie, CPPUNIT_SOURCELINE())

// Test a complete serialize()/deserialize() cycle.
#define ASSERT_DESERIALIZABLE(ie) \
	test_is_deserializable(ie, CPPUNIT_SOURCELINE())

// Test all the above.
#define ASSERT_BASICS_WORK(ie) \
	test_basics_work(ie, CPPUNIT_SOURCELINE())


// Helper functions for the macros. Don't use directly.
void test_is_instantiable(const protlib::IE &ie, CppUnit::SourceLine line);
void test_is_copyable(const protlib::IE &ie, CppUnit::SourceLine line);
void test_is_serializable(const protlib::IE &ie, CppUnit::SourceLine line);
void test_is_deserializable(const protlib::IE &ie, CppUnit::SourceLine line);
void test_basics_work(const protlib::IE &ie, CppUnit::SourceLine line);

#endif // TESTSUITE_UTILS_H
