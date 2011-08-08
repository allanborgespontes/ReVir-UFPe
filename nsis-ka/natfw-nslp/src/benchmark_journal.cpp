/// ----------------------------------------*- mode: C++; -*--
/// @file benchmark_journal.cpp
/// The benchmark class.
/// ----------------------------------------------------------
/// $Id: benchmark_journal.cpp 2558 2007-04-04 15:17:16Z bless $
/// $HeadURL: https://svn.tm.kit.edu/nsis/natfw-nslp/trunk/src/benchmark_journal.cpp $
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
#include <fstream>
#include <cstring>
#include <pthread.h>

#include "benchmark_journal.h"

using namespace natfw;


#ifdef BENCHMARK
benchmark_journal journal(1000000, "benchmark_journal.txt");
#endif

/**
 * Human readable measuring point names, used in write_header().
 */
const char *
benchmark_journal::mp_names[benchmark_journal::HIGHEST_VALID_ID+1] = {
	"INVALID_ID",
	"PRE_PROCESSING",
	"POST_PROCESSING",
	"PRE_MAPPING",
	"POST_MAPPING",
	"PRE_SESSION_MANAGER",
	"POST_SESSION_MANAGER",
	"PRE_SERIALIZE",
	"POST_SERIALIZE",
	"PRE_DESERIALIZE",
	"POST_DESERIALIZE",
	"PRE_DISPATCHER",
	"POST_DISPATCHER",
	"PRE_SESSION",
	"POST_SESSION"
};


/**
 * Constructor to create a journal of the given size.
 *
 * The journal is initialized to avoid page faults during usage.
 * If a filename is given (!= the empty string), the journal is written to
 * that file as soon as the journal is full.
 *
 * @param journal_size the size of the in-memory journal
 * @param filename the name of the file to write the journal to
 */
benchmark_journal::benchmark_journal(
		int journal_size, const std::string &filename)
		: journal_size(journal_size), journal_pos(0),
		  filename(filename), disable_journal(false) {

	journal = new measuring_point_t[journal_size];
	memset(journal, 0,
		sizeof(benchmark_journal::measuring_point_t) * journal_size);

	/*
	 * Initialize the mutex. We use a recursive mutex to make calling
	 * write_journal() from add() easier.
	 */
	pthread_mutexattr_t mutex_attr;

	pthread_mutexattr_init(&mutex_attr);
	pthread_mutexattr_settype(&mutex_attr, PTHREAD_MUTEX_RECURSIVE);
	pthread_mutex_init(&mutex, &mutex_attr);

	pthread_mutexattr_destroy(&mutex_attr);
}


benchmark_journal::~benchmark_journal() {
	std::cerr << "Exiting. Writing journal ..." << std::endl;
	write_journal();
	std::cerr << "Journal written." << std::endl;

	delete journal;
	pthread_mutex_destroy(&mutex);
}


/**
 * Reset the journal, removing all measuring points recorded so far.
 */
void benchmark_journal::restart() {
	pthread_mutex_lock(&mutex);

	journal_pos = 0;
	disable_journal = false;

	pthread_mutex_unlock(&mutex);
}


void benchmark_journal::write_journal() {
	write_journal(filename);
}


void benchmark_journal::write_journal(const std::string &filename) {
	std::ofstream out(filename.c_str());

	if ( ! out ) {
		std::cerr << "Error opening journal file `" + filename + "'\n";
		return;
	}

	try {
		this->write_journal(out);
	}
	catch ( ... ) {
		std::cerr << "Error writing journal" << std::endl;
	}

	out.close();
}


/**
 * Write the journal to the given stream.
 *
 * Note that the caller is responsible for closing the stream.
 */
void benchmark_journal::write_journal(std::ostream &out) {
	pthread_mutex_lock(&mutex);

	write_header(out);

	for ( int i = 0; i < journal_size
				&& journal[i].point != INVALID_ID; i++ )
		out << (int) journal[i].point << ' '
			<< journal[i].thread_id << ' '
			<< journal[i].timestamp.tv_sec << ' '
			<< journal[i].timestamp.tv_nsec << std::endl;

	pthread_mutex_unlock(&mutex);
}


/**
 * Write a header to the journal documenting the measuring points.
 */
void benchmark_journal::write_header(std::ostream &out) {
	time_t t;

	time(&t);

	out << "# Benchmark Journal, created " << ctime(&t);
	out << "# $Id: benchmark_journal.cpp 2558 2007-04-04 15:17:16Z bless $" << std::endl;
	out << "# Format: <measuring point ID> <Thread ID>"
		" <seconds> <nano seconds>" << std::endl;
	out << "# Measuring points:" << std::endl;

	for (int i = 0; i <= HIGHEST_VALID_ID; i++)
		out << "# \t" << i << " " << mp_names[i] << std::endl;

	out << "#" << std::endl;
}

// EOF
