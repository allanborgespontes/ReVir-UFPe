/*
 * This file is a template which can be copied to create a new test case file.
 *
 * $Id: test_template.cpp 2183 2006-10-15 21:04:03Z stud-matfried $
 * $HeadURL: https://svn.tm.kit.edu/nsis/ntlp/trunk/test/test_template.cpp $
 */
#include "test_suite.h"

#include "sessionid.h"

using namespace ntlp;


class test_case_name : public CppUnit::TestCase {

	CPPUNIT_TEST_SUITE( test_case_name );

	CPPUNIT_TEST( test_one_thing );
	CPPUNIT_TEST( test_another );
	CPPUNIT_TEST( test_exception );

	CPPUNIT_TEST_SUITE_END();

	public:
		void test_one_thing() {
			CPPUNIT_ASSERT( 1 == 1 );
			CPPUNIT_ASSERT_MESSAGE("error message", 1 == 1 );
		}
		
		void test_another() {
			CPPUNIT_ASSERT_EQUAL(1, 1);
		}

		void test_exception() {
			CPPUNIT_ASSERT( 1 == 1 ); // not to be reached
			CPPUNIT_ASSERT_THROW(
				throw std::exception(), std::exception );
		}
};

CPPUNIT_TEST_SUITE_REGISTRATION( test_case_name );