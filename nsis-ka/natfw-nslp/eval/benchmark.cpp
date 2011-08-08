/*
 * benchmark.cpp - a class for performing benchmarks
 *
 * $Id: benchmark.cpp 4118 2009-07-16 16:13:10Z bless $
 * $HeadURL: https://svn.tm.kit.edu/nsis/natfw-nslp/trunk/eval/benchmark.cpp $
 *
 */
#include <iostream>
#include <ctime>
#include <cstdlib>

#include <stdlib.h>

#include "logfile.h"
#include "benchmark.h"


/*
 * Protlib logging, needed for linking.
 */
protlib::log::logfile commonlog("natfwd.log", false);
protlib::log::logfile &protlib::log::DefaultLog(commonlog);


using namespace natfw;


benchmark::benchmark() {
	using namespace protlib::log;

	/*
	 * Turn off logging.
	 */
	if ( ::getenv("TEST_LOG") == NULL ) {
		commonlog.set_filter(ERROR_LOG, LOG_EMERG + 1);
		commonlog.set_filter(WARNING_LOG, LOG_EMERG + 1);
		commonlog.set_filter(EVENT_LOG, LOG_EMERG + 1);
		commonlog.set_filter(INFO_LOG, LOG_EMERG + 1);
		commonlog.set_filter(DEBUG_LOG, LOG_EMERG + 1);
	}
}


/**
 * Run the benchmark.
 *
 * This method runs the perform_task() method @a num_times.
 *
 * @param name the name of the benchmark
 * @param num_times the number of times to run perform_task()
 */
void benchmark::run(const std::string &name, unsigned long num_times) {
	using std::cout;

	cout << "Running benchmark '" << name << "' ("
		<< num_times << " iterations) ...\n";

	// used to decide when to display a progress indicator
	unsigned tasks_per_dot = num_times / 70;

	if ( tasks_per_dot == 0 )
		tasks_per_dot = 1;

	time_t start = time(NULL);

	cout << "Starting time: " << ctime(&start);

	for ( unsigned i = 1; i <= num_times; i++ ) {

		perform_task();

		if ( (i % tasks_per_dot) == 0 )
			cout << '.' << std::flush;
	}

	time_t stop = time(NULL);

	cout << "\n";
	cout << "Ending time: " << ctime(&stop);
	cout << "Time used in seconds: " << stop - start << "\n";
	cout << "\n";
}

// EOF
