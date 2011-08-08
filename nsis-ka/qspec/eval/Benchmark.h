/*
 * Benchmark.h - a class for performing benchmarks
 *
 * $Id: Benchmark.h 896 2005-11-17 11:54:18Z stud-matfried $
 * $HeadURL: https://svn.tm.kit.edu/nsis/qspec/trunk/eval/Benchmark.h $
 *
 */
#ifndef QSPEC__BENCHMARK_H
#define QSPEC__BENCHMARK_H

#include <iostream>


namespace qspec {

class Benchmark {

  public:
	Benchmark();
	virtual ~Benchmark() { }

	virtual void run(const std::string &name, unsigned long num_times);

  protected:
	virtual void performTask() = 0;
};

} // namespace qspec

#endif // QSPEC__BENCHMARK_H
