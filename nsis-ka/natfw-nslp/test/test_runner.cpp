/*
 * test_runner.cpp - Run the test suite.
 *
 * $Id: test_runner.cpp 2246 2006-10-26 07:40:51Z stud-matfried $
 * $HeadURL: https://svn.tm.kit.edu/nsis/natfw-nslp/trunk/test/test_runner.cpp $
 */
#include <cstdlib> // for getenv()
#include <string>

#include <cppunit/TestResult.h>
#include <cppunit/TestResultCollector.h>
#include <cppunit/TestRunner.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/BriefTestProgressListener.h>
#include <cppunit/CompilerOutputter.h>

#include "logfile.h"

using namespace protlib::log;
using namespace CppUnit;


// needed for linking all tests
logfile commonlog("", false, true); // no colours, quiet start
logfile &protlib::log::DefaultLog(commonlog);



/*
 * Run the test suite, return 0 on success, 1 on error.
 */
int main(void) {

	// Turn off logging
	if ( getenv("TEST_LOG") == NULL ) {
		commonlog.set_filter(ERROR_LOG, LOG_EMERG + 1);
		commonlog.set_filter(WARNING_LOG, LOG_EMERG + 1);
		commonlog.set_filter(EVENT_LOG, LOG_EMERG + 1);
		commonlog.set_filter(INFO_LOG, LOG_EMERG + 1);
		commonlog.set_filter(DEBUG_LOG, LOG_EMERG + 1);
	}

	TestResult controller;

	TestResultCollector result;
	controller.addListener(&result);

	BriefTestProgressListener progress;
	controller.addListener(&progress);

	TestRunner runner;
	runner.addTest( TestFactoryRegistry::getRegistry().makeTest() );
	runner.run(controller);

	CompilerOutputter outputter(&result, std::cerr);
	outputter.write();

	return result.wasSuccessful() ? 0 : 1;
}

// EOF
