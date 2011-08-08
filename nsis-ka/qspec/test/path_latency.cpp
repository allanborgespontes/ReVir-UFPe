/*
 * path_latency.cpp - Test the QSPEC Path Latency Parameter
 *
 * $Id: path_latency.cpp 382 2005-10-26 08:09:16Z stud-matfried $
 * $HeadURL: https://svn.tm.kit.edu/nsis/qspec/trunk/test/path_latency.cpp $
 *
 */
#include "generic_param_test.h"

using namespace qspec;


class PathLatencyTest : public GenericParamTest {

	CPPUNIT_TEST_SUITE( PathLatencyTest );

	QSPEC_PARAM_DEFAULT_TESTS();

	CPPUNIT_TEST_SUITE_END();

  public:
	virtual qspec_param *createInstance1() const {
		return new path_latency();
	}

	virtual qspec_param *createInstance2() const {
		return new path_latency(200000); // 0.2 seconds
	}

	virtual void mutate1(qspec_param *p) const {
		path_latency *b = dynamic_cast<path_latency *>(p);
		b->set_value(10000);  // 0.01 seconds
	}
};

CPPUNIT_TEST_SUITE_REGISTRATION( PathLatencyTest );

// EOF
