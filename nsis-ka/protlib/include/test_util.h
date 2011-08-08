/// ----------------------------------------*- mode: C++; -*--
/// @file test_util.h
/// Interface for test utilities
/// ----------------------------------------------------------
/// $Id: test_util.h 2872 2008-02-18 10:58:03Z bless $
/// $HeadURL: https://svn.tm.kit.edu/nsis/protlib/trunk/include/test_util.h $
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
/** @ingroup tests
 *@{
 */
 
/** @file
 * Interface for test utilities.
 */

#ifndef TEST_UTIL_H
#define TEST_UTIL_H

#include <sys/time.h>

/// Record start time.
void start_test(unsigned int nr = 0);

/// stop test and print time.
void stop_test(unsigned int nr = 0);

/// stop all tests and print times.
void stop_all_tests();

#endif

//@}
