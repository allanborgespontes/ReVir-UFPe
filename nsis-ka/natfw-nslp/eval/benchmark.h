/*
 * benchmark.h - a class for performing benchmarks
 *
 * $Id: benchmark.h 2274 2006-11-04 09:14:24Z stud-matfried $
 * $HeadURL: https://svn.tm.kit.edu/nsis/natfw-nslp/trunk/eval/benchmark.h $
 *
 */
#ifndef NATFW__BENCHMARK_H
#define NATFW__BENCHMARK_H

#include <iostream>


namespace natfw {

class benchmark {

  public:
	benchmark();
	virtual ~benchmark() { }

	virtual void run(const std::string &name, unsigned long num_times);

  protected:
	virtual void perform_task() = 0;
};

} // namespace natfw

#endif // NATFW__BENCHMARK_H
