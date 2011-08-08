/*
 * Benchmark.h - a class for performing benchmarks
 *
 */
#ifndef NSLP_AUTH__BENCHMARK_H
#define NSLP_AUTH__BENCHMARK_H

#include <iostream>


namespace nslp_auth {

class Benchmark {

  public:
	Benchmark();
	virtual ~Benchmark() { }

	virtual void run(const std::string &name, unsigned long num_times);

  protected:
	virtual void performTask() = 0;
};

} // namespace nslp_auth

#endif // NSLP_AUTH__BENCHMARK_H
