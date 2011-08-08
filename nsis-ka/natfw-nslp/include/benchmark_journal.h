/// ----------------------------------------*- mode: C++; -*--
/// @file benchmark_journal.h
/// Tools for running benchmarks.
/// ----------------------------------------------------------
/// $Id: benchmark_journal.h 2558 2007-04-04 15:17:16Z bless $
/// $HeadURL: https://svn.tm.kit.edu/nsis/natfw-nslp/trunk/include/benchmark_journal.h $
// ===========================================================
//                      
// Copyright (C) 2005-2007, all rights reserved by
// - Institute of Telematics, Universitaet Karlsruhe (TH)
//
// More information and contact:
// https://projekte.tm.uka.de/trac/NSIS
//                      
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; version 2 of the License
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
//
// ===========================================================
#ifndef NATFW__BENCHMARK_JOURNAL_H
#define NATFW__BENCHMARK_JOURNAL_H

#include <iostream>
#include <string>
#include <ctime>


namespace natfw {

#ifdef BENCHMARK
  #define MP(mp_id)	journal.add(mp_id)
#else
  #define MP(mp_id)
#endif

/**
 * A class for running benchmarks on the NATFW implementation.
 *
 * Only one instance of this class is needed. All threads share a common
 * journal.
 */
class benchmark_journal {
  public:
	/**
	 * Each measuring point has an ID on its own.
	 *
	 * Note: When adding or changing measuring points, please also adjust
	 *       the mp_names array in benchmark.cpp.
	 */
	enum measuring_point_id_t {
		INVALID_ID		= 0,
		PRE_PROCESSING		= 1,
		POST_PROCESSING		= 2,
		PRE_MAPPING		= 3,
		POST_MAPPING		= 4,
		PRE_SESSION_MANAGER	= 5,
		POST_SESSION_MANAGER	= 6,
		PRE_SERIALIZE		= 7,
		POST_SERIALIZE		= 8,
		PRE_DESERIALIZE		= 9,
		POST_DESERIALIZE	= 10,
		PRE_DISPATCHER		= 11,
		POST_DISPATCHER		= 12,
		PRE_SESSION		= 13,
		POST_SESSION		= 14,
		HIGHEST_VALID_ID	= 14
	};

	benchmark_journal(int journal_size, const std::string &filename="");
	~benchmark_journal();

	void add(measuring_point_id_t mp_id);
	void restart();

	void write_journal();
	void write_journal(const std::string &filename);
	void write_journal(std::ostream &out);

  private:
	struct measuring_point_t {
		measuring_point_id_t	point;
		pthread_t		thread_id;
		struct timespec		timestamp;
	};

	/*
	 * The journal is an array of measuring_point entries. The next entry
	 * has to be written to journal[journal_pos].
	 */
	int journal_size;
	int journal_pos;
	measuring_point_t *journal;

	pthread_mutex_t mutex;

	std::string filename;
	bool disable_journal;

	static const char *mp_names[HIGHEST_VALID_ID+1];

	static void write_header(std::ostream &out);
};


inline void benchmark_journal::add(measuring_point_id_t mp_id) {
	pthread_mutex_lock(&mutex);

	if ( journal_pos < journal_size ) {
		struct timespec res;
		clock_gettime(CLOCK_REALTIME, &res);

		struct measuring_point_t mp = { mp_id, pthread_self(), res };
		journal[journal_pos++] = mp;
	}
	else if ( disable_journal == false && journal_pos == journal_size ) {
		std::cerr << "*** benchmark journal is full ***" << std::endl;

		disable_journal = true;
	}

	pthread_mutex_unlock(&mutex);
}


} // namespace natfw


#endif // NATFW__BENCHMARK_JOURNAL_H
