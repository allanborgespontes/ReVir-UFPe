/*
 * Test the benchmark class.
 *
 * $Id: benchmark.cpp 2276 2006-11-06 08:03:31Z stud-matfried $
 * $HeadURL: https://svn.tm.kit.edu/nsis/natfw-nslp/trunk/test/benchmark.cpp $
 */
#include <cppunit/TestCase.h>
#include <cppunit/extensions/HelperMacros.h>

#include "benchmark_journal.h"

using namespace natfw;


class BenchmarkTest : public CppUnit::TestCase {

	CPPUNIT_TEST_SUITE( BenchmarkTest );

	CPPUNIT_TEST( testBasics );

	CPPUNIT_TEST_SUITE_END();

  public:
	void testBasics() {
		benchmark_journal bm(100);
		bm.add(benchmark_journal::PRE_MAPPING);
		bm.add(benchmark_journal::POST_MAPPING);

		//bm.write_journal("/tmp/benchmark_journal.log");
	}
};

CPPUNIT_TEST_SUITE_REGISTRATION( BenchmarkTest );

// EOF
