/*
 * test_runner.cpp - Run the test suite.
 *
 * $Id: test_runner.cpp 6148 2011-05-18 09:36:25Z bless $
 * $HeadURL: https://svn.tm.kit.edu/nsis/protlib/trunk/test/test_runner.cpp $
 */
#include <cstdlib> // for getenv()
#include <string>

#include <cppunit/TestResult.h>
#include <cppunit/TestResultCollector.h>
#include <cppunit/TestRunner.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/BriefTestProgressListener.h>
#include <cppunit/CompilerOutputter.h>


#include "threadsafe_db.h"
#include "logfile.h"
#include "protlibconf.h"

using namespace protlib;
using namespace protlib::log;
using namespace CppUnit;

namespace protlib {
        protlibconf plibconf;
}


// needed for linking all tests
logfile commonlog("", false, true); // no colours, quiet start
logfile &protlib::log::DefaultLog(commonlog);



/*
 * Run the test suite, return 0 on success, 1 on error.
 */
int main(void) {
	// register all protlib configuration parameters at the registry
	plibconf.repository_init();
	// register all protlib configuration parameters at the registry
	plibconf.setRepository();

	// Turn off logging if the TEST_LOG environment variable is not set.
	if ( getenv("TEST_LOG") == NULL ) {
		commonlog.set_filter(ERROR_LOG, LOG_EMERG + 1);
		commonlog.set_filter(WARNING_LOG, LOG_EMERG + 1);
		commonlog.set_filter(EVENT_LOG, LOG_EMERG + 1);
		commonlog.set_filter(INFO_LOG, LOG_EMERG + 1);
		commonlog.set_filter(DEBUG_LOG, LOG_EMERG + 1);
	}

	protlib::tsdb::init();

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

	protlib::tsdb::end();

	return result.wasSuccessful() ? 0 : 1;
}

// EOF
