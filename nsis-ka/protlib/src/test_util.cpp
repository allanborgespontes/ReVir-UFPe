/// ----------------------------------------*- mode: C++; -*--
/// @file test_util.cpp
/// Test utilities.
/// ----------------------------------------------------------
/// $Id: test_util.cpp 2549 2007-04-02 22:17:37Z bless $
/// $HeadURL: https://svn.tm.kit.edu/nsis/protlib/trunk/src/test_util.cpp $
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
/** @addtogroup tests Testing
 *@{
 */

/** @file
 * 
 */

#include <ext/hash_map>
#include <iostream>
#include "test_util.h"

using namespace std;
using namespace __gnu_cxx;

/// Is a test running?
/** If set, a test is currently running and test_start contains a valid time
 * value.
 */

typedef hash_map<unsigned int, struct timeval> timehash;

static timehash starttime;

/** Set the test start time by recording the current time. */
void start_test(unsigned int nr) {
   cout<<"starting test #"<<nr<<"..."<<endl;
   gettimeofday(&(starttime[nr]),NULL);
} // end start_test

/** End the test, compute the duration and print it to stdout. */
void stop_test(unsigned int nr) {
	struct timeval test_end = {0};
	struct timeval test_duration = {0};
	gettimeofday(&test_end,NULL);
	timehash::iterator hit = starttime.find(nr);
	if (hit!=starttime.end()) {
	   cout<<"stopped test #"<<nr<<"."<<endl;
	   timersub(&test_end, &(hit->second), &test_duration);
		double d = test_duration.tv_sec+((double)test_duration.tv_usec)/1000000;
	   cout<<"duration #"<<nr<<": "<<d<<"s"<<endl;
	} else cout<<"Test #"<<nr<<": start time not found."<<endl;
} // end stop_test

//@}
