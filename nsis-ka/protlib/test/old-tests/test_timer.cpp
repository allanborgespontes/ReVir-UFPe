/// ----------------------------------------*- mode: C++; -*--
/// @file test_timer.cpp
/// put short description here
/// ----------------------------------------------------------
/// $Id: test_timer.cpp 3527 2008-10-10 13:25:24Z bless $
/// $HeadURL: https://svn.tm.kit.edu/nsis/protlib/trunk/test/old-tests/test_timer.cpp $
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
/** @addtogroup testtimer Timer
 * @ingroup tests
 * @{
 */

/** @file
 * This is a simple software timer test. Have a look at the log output to
 * check that class TimerManager works correctly.
 */

#include <unistd.h>
#include <iostream>

#include "timer.h"
#include "logfile.h"


namespace protlib {

  using namespace log;


class CallBack : public TimerCallback {
public:
	virtual void timer_expired(timer_id_t id, timer_callback_param_t param) {
		int i = (int)param;
		Log(EVENT_LOG, LOG_ALERT, "test_timer",	"Timer expired id " <<  id << ", param " << i);
	} // end timer_expired
}; // end CallBack

CallBack cb;

TimerManager tm;

bool done = false;
bool running = false;

pthread_t tmthread;

void* timerchecker(void *p) {
	uint32 num = 0;
	running = true;
	Log(EVENT_LOG, LOG_NORMAL, "test_timer", "Timerchecker started");
	while (!done) {
	  Log(EVENT_LOG, LOG_NORMAL, "test_timer", "Checking timers");
	  num = tm.check_timers_wait(5000);
	  Log(EVENT_LOG, LOG_NORMAL, "test_timer" , num << " timers checked");
	} // end while not done
	Log(EVENT_LOG, LOG_NORMAL, "test_timer", "Timerchecker stopped");
	running = false;
	return NULL;
} // end timerchecker

int test() {
  /* DefaultLog::log->set_filter(ERROR_LOG, 15);
     DefaultLog::log->set_filter(WARNING_LOG, 15);
     DefaultLog::log->set_filter(EVENT_LOG, 15);
     DefaultLog::log->set_filter(INFO_LOG, 15);
     DefaultLog::log->set_filter(DEBUG_LOG, 15);
  */
	pthread_create(&tmthread,NULL,timerchecker,NULL);
	sleep(1);
	Log(EVENT_LOG, LOG_NORMAL, "test_timer", "Setting alarm param 99 to 1 seconds");
	tm.start_relative(&cb,1,0,(timer_callback_param_t)99);
	sleep(2);
	Log(EVENT_LOG, LOG_NORMAL, "test_timer", "Setting alarm param 1 to 80 seconds");
	tm.start_relative(&cb,80,0,(timer_callback_param_t)1);
	sleep(6);
	Log(EVENT_LOG, LOG_NORMAL, "test_timer", "Setting alarm param 2 to 13 seconds");
	timer_id_t id = tm.start_relative(&cb,13,0,(timer_callback_param_t)2);
	sleep(12);
	Log(EVENT_LOG, LOG_NORMAL, "test_timer", "Restarting alarm " << id << " to 3 seconds");
	tm.restart_relative(id,3);
	sleep(15);
	Log(EVENT_LOG, LOG_NORMAL, "test_timer", "Setting alarm param 35 to 17 seconds");
	id = tm.start_relative(&cb,17,0,(timer_callback_param_t)35);
	sleep(15);
	Log(EVENT_LOG, LOG_NORMAL, "test_timer", "Stopping alarm " << id);
	bool s = tm.stop(id);
	if (s) {
	  Log(EVENT_LOG, LOG_NORMAL, "test_timer", "Alarm " << id << " stopped");
	} else {
	  Log(ERROR_LOG, LOG_NORMAL, "test_timer", "Alarm " << id << " not stopped");
	} // end if s
	Log(EVENT_LOG, LOG_NORMAL, "test_timer", "Stopping non-existing alarm 9999");
	s = tm.stop(9999);
	if (s) {
	  Log(ERROR_LOG, LOG_NORMAL, "test_timer", "Alarm 9999 stopped although it doesn't exist");
	} else {
	  Log(EVENT_LOG, LOG_NORMAL, "test_timer", "Alarm 9999 not stopped, OK");
	} // end if s
	sleep(40);
	Log(EVENT_LOG, LOG_NORMAL, "test_timer", "Stopping timerchecker thread");
	done = true;
	while (running) {
		sleep(1);
	}
	return 0;
}

} // end namespace protlib;

using namespace protlib;
using namespace protlib::log;

logfile commonlog;
protlib::log::logfile& protlib::log::DefaultLog(commonlog);

int main() { return protlib::test(); }

//@}
  
