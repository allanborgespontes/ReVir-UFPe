/*
 * test_runner.cpp - Run the test suite
 *
 * $Id: test_runner.cpp 387 2005-10-26 10:22:42Z stud-matfried $
 * $HeadURL: https://svn.tm.kit.edu/nsis/qspec/trunk/test/test_runner.cpp $
 *
 */
#include <cstdlib> // for getenv()
#include <cppunit/ui/text/TextTestRunner.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include "logfile.h"

using namespace protlib::log;
using namespace CppUnit;


// needed for linking all tests
logfile commonlog;
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

	TextTestRunner runner;
	TestFactoryRegistry &registry = TestFactoryRegistry::getRegistry();

	runner.addTest( registry.makeTest() );

	return ! runner.run("", false);
}
