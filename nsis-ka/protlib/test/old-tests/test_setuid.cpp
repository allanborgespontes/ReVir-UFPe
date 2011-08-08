/// ----------------------------------------*- mode: C++; -*--
/// @file test_setuid.cpp
/// Thread-safe setuid test
/// ----------------------------------------------------------
/// $Id: test_setuid.cpp 3527 2008-10-10 13:25:24Z bless $
/// $HeadURL: https://svn.tm.kit.edu/nsis/protlib/trunk/test/old-tests/test_setuid.cpp $
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
/** @addtogroup testtssetuid Thread-safe setuid program support
 * Thread-safe setuid test. 
 */

#include <iostream>

#include "threadsafe_db.h"
#include "setuid.h"
#include "logfile.h"

using namespace protlib;
using namespace protlib::log;


logfile commonlog;
protlib::log::logfile& protlib::log::DefaultLog(commonlog);

int main() {
using namespace protlib;
	cout<<"Initializing tsdb..."<<endl;
	tsdb::init();
	cout<<"Calling setuid::init()..."<<endl;
	protlib::setuid::init();
	cout<<"Calling BEGIN_SETUID_MODE..."<<endl;
	BEGIN_SETUID_MODE;
	cout<<"Calling BEGIN_SETUID_MODE..."<<endl;
	BEGIN_SETUID_MODE;
	cout<<"Calling END_SETUID_MODE..."<<endl;
	END_SETUID_MODE;
	cout<<"Calling END_SETUID_MODE..."<<endl;
	END_SETUID_MODE;
	cout<<"Calling setuid::end()..."<<endl;
	protlib::setuid::end();
	cout<<"Calling setuid::init()..."<<endl;
	protlib::setuid::init();
	cout<<"Calling setuid::end()..."<<endl;
	protlib::setuid::end();
	cout<<"Shutting down tsdb..."<<endl;
	tsdb::end();
	cout<<"Exiting..."<<endl;
	return 0;
} // end main
