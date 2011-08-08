/*
 * Benchmark.cpp - a class for performing benchmarks
 *
 * $Id: Benchmark.cpp 896 2005-11-17 11:54:18Z stud-matfried $
 * $HeadURL: https://svn.tm.kit.edu/nsis/qspec/trunk/eval/Benchmark.cpp $
 *
 */
#include <iostream>
#include <ctime>

#include "logfile.h"
#include "Benchmark.h"

/*
 * Protlib Logging, needed for linking.
 */
protlib::log::logfile commonlog;
protlib::log::logfile &protlib::log::DefaultLog(commonlog);


using namespace qspec;


Benchmark::Benchmark() {
	using namespace protlib::log;

	/*
	 * Turn off logging.
	 */
	commonlog.set_filter(ERROR_LOG, LOG_EMERG + 1);
	commonlog.set_filter(WARNING_LOG, LOG_EMERG + 1);
	commonlog.set_filter(EVENT_LOG, LOG_EMERG + 1);
	commonlog.set_filter(INFO_LOG, LOG_EMERG + 1);
	commonlog.set_filter(DEBUG_LOG, LOG_EMERG + 1);
}


/**
 * Run the benchmark.
 *
 * This method runs the performTask() method @a num_times.
 *
 * @param name the name of the benchmark
 * @param num_times the number of times to run performTask()
 */
void Benchmark::run(const std::string &name, unsigned long num_times) {
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

		performTask();

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
