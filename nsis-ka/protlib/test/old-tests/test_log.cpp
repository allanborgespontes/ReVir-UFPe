/// ----------------------------------------*- mode: C++; -*--
/// @file test_log.cpp
/// This file contains some code to test the logging API
/// ----------------------------------------------------------
/// $Id: test_log.cpp 3527 2008-10-10 13:25:24Z bless $
/// $HeadURL: https://svn.tm.kit.edu/nsis/protlib/trunk/test/old-tests/test_log.cpp $
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
/** @addtogroup testlog Logging
 * @ingroup tests
 * @{
 */

#include <iostream>
#include "logfile.h"

using namespace protlib;
using namespace protlib::log;

logfile commonlog;
protlib::log::logfile& protlib::log::DefaultLog(commonlog);

int main() {
  Log(INFO_LOG, LOG_CRIT, "log_test", "nur zur info." );
  Log(ERROR_LOG, LOG_EMERG, "log_test", "program terminating - but it is a test.");

  cout<<"This should never be executed."<<endl;
  return 0;
}

//@}
