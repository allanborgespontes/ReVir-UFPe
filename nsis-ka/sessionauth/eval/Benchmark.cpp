/*
 * Benchmark.cpp - a class for performing benchmarks
 *
 *
 */
#include <iostream>
#include <sys/time.h> // gettimeofday
#include <ctime>      // ctime

#include "logfile.h"
#include "Benchmark.h"
#include "benchmark_journal.h"

/*
 * Protlib Logging, needed for linking.
 */
protlib::log::logfile commonlog;
protlib::log::logfile &protlib::log::DefaultLog(commonlog);


using namespace nslp_auth;

#ifdef BENCHMARK
extern benchmark_journal journal;
#endif


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

	struct timeval tp;
	double sec, usec, start, end;
	
	cout << "Running benchmark '" << name << "' ("
		<< num_times << " iterations) ...\n";

	// used to decide when to display a progress indicator
	unsigned tasks_per_dot = num_times / 70;

	if ( tasks_per_dot == 0 )
		tasks_per_dot = 1;

	// Time stamp before the computations
	gettimeofday(&tp, NULL);
	sec = static_cast<double>(tp.tv_sec);
	usec = static_cast<double>(tp.tv_usec) / 1E6;
	start = sec + usec;

	cout << "Starting time: " << ctime(&tp.tv_sec);

	for ( unsigned i = 1; i <= num_times; i++ ) {

		performTask();

		if ( (i % tasks_per_dot) == 0 )
			cout << '.' << std::flush;
	}

	// Time stamp after the computations
	gettimeofday(&tp, NULL);
	sec = static_cast<double>(tp.tv_sec);
	usec = static_cast<double>(tp.tv_usec) / 1E6;
	end = sec + usec;

	// Time calculation (in seconds)
	double time = end - start;

	cout << "\n";
	cout << "Ending time: " << ctime(&tp.tv_sec);
	cout << "Time used in seconds: " << time << "\n";
	cout << "\n";
}

// EOF
