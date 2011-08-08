/// ----------------------------------------*- mode: C++; -*--
/// @file test_timer_module.cpp
/// put short description here
/// ----------------------------------------------------------
/// $Id: test_timer_module.cpp 3527 2008-10-10 13:25:24Z bless $
/// $HeadURL: https://svn.tm.kit.edu/nsis/protlib/trunk/test/old-tests/test_timer_module.cpp $
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
/** @addtogroup testtimermodule Timer Module
 * @ingroup tests
 * @{
 */

/** @file
 * This is a test for the timer module.
 * It does the same things as the software timer test in test_timer.cpp.
 * Have a look at the program output to see if everything is allright.
 */

#include <unistd.h>
#include <iostream>

#include "timer_module.h"
#include "threadsafe_db.h"
#include "logfile.h"
#include "queuemanager.h"

using namespace protlib;
using namespace protlib::log;

logfile commonlog;
protlib::log::logfile& protlib::log::DefaultLog(commonlog);

bool done = false;
bool running = false;

pthread_t tmthread;

void* timerchecker(void *p) {
	message* msg = NULL;
	TimerMsg* tmsg = NULL;
	FastQueue* fq = QueueManager::instance()->get_queue(message::qaddr_transport);
	int32 sec = 0;
	int32 msec = 0;
	if (!fq) {
	  Log(ERROR_LOG, LOG_ALERT, "test_timer_module", "Timerchecker cannot find message queue");
	  return NULL;
	} // end if not fq
	running = true;
	Log(EVENT_LOG, LOG_NORMAL, "test_timer_module", "Timerchecker started");
	while (!done) {
	  Log(EVENT_LOG, LOG_NORMAL, "test_timer_module", "Waiting for messages another 5 seconds");
	  msg = fq->dequeue_timedwait(5000);
	  if (msg) {
	    Log(EVENT_LOG, LOG_NORMAL, "test_timer_module", "Received a message");
	    if (msg->get_type()==message::type_timer) {
	      tmsg = dynamic_cast<TimerMsg*>(msg);
	      tmsg->get_time(sec,msec);
	      Log(EVENT_LOG, LOG_NORMAL, "test_timer_module", "Message: messageid " 
		  << tmsg->get_id() << ", action: " << color[blue] << tmsg->get_action_str() << color[off] << ", "
		  << (tmsg->is_ok() ? "ok" : "not ok") << ", relative " 
		  << tmsg->is_relative() << ", sec " << sec << ", msec " << msec);
	      if (tmsg->get_action()==TimerMsg::ac_elapsed)
	      {
		Log(EVENT_LOG, LOG_NORMAL, "test_timer_module", 
		    color[green] << "Alarm " <<  tmsg->get_id() << " went off" << color[off]);
	      }
	      tmsg = NULL;
	    } // end if type_timer
	    delete msg;
	    msg = NULL;
	  } // end if msg
	} // end while not done
	fq->cleanup();
	Log(EVENT_LOG, LOG_NORMAL, "test_timer_module", "Timerchecker stopped");
	running = false;
	return NULL;
} // end timerchecker

int main() {
	tsdb::init();
	TimerMsg* msg = NULL;
	message::id_t saved_timer = 0, first_timer = 0;
	TimerModuleParam tmpar(3,false,false,false);
	cout<<"Trying to create one TimerModule thread, but nevertheless"<<endl
		<<"two must be started."<<endl;
	ThreadStarter<TimerModule,TimerModuleParam>* tmod= new ThreadStarter<TimerModule,TimerModuleParam>(1,tmpar);
	/*
	DefaultLog::log->set_filter(ERROR_LOG, 15);
	DefaultLog::log->set_filter(WARNING_LOG, 15);
	DefaultLog::log->set_filter(EVENT_LOG, 15);
	DefaultLog::log->set_filter(INFO_LOG, 15);
	DefaultLog::log->set_filter(DEBUG_LOG, 15);
	*/
	cout<<"Creating timerchecker queue..."<<endl;
	FastQueue* timerchk_fq= new FastQueue("timerchecker",true);
	QueueManager::instance()->register_queue(timerchk_fq, message::qaddr_transport);
	pthread_create(&tmthread,NULL,timerchecker,NULL);
	tmod->start_processing();
	sleep(1);
	msg = new TimerMsg(message::qaddr_transport,true);
	Log(EVENT_LOG, LOG_NORMAL, "test_timer_module", color[red] << "Setting alarm " << msg->get_id() << " to 5 seconds" << color [off]);
	msg->start_relative(5);
	first_timer= msg->get_id();
	msg->send_to(message::qaddr_timer);

	msg = new TimerMsg(message::qaddr_transport,true);
	Log(EVENT_LOG, LOG_NORMAL, "test_timer_module", color[red] << "Setting alarm " << msg->get_id() << " to 10 seconds" << color[off]);
	msg->start_relative(10);
       	msg->send_to(message::qaddr_timer);

	msg = new TimerMsg(message::qaddr_transport,true);
	Log(EVENT_LOG, LOG_NORMAL, "test_timer_module", color[red] << "Setting alarm " << msg->get_id() << " to 15 seconds" << color[off]);
	msg->start_relative(15);
	msg->send_to(message::qaddr_timer);
	sleep(2);

	msg = new TimerMsg(message::qaddr_transport,true);
	Log(EVENT_LOG, LOG_NORMAL, "test_timer_module", color[red] << "Stopping alarm " << first_timer << color[off]);
	msg->stop(first_timer);
	first_timer= msg->get_id();
	msg->send_to(message::qaddr_timer);

	msg = new TimerMsg(message::qaddr_transport,true);
	Log(EVENT_LOG, LOG_NORMAL, "test_timer_module", color[red] << "Setting alarm " << msg->get_id() << " to 5 seconds" << color[off]);
	msg->start_relative(5);
	first_timer= msg->get_id();
	msg->send_to(message::qaddr_timer);
	sleep(25);

	msg = new TimerMsg(message::qaddr_transport,true);
	Log(EVENT_LOG, LOG_NORMAL, "test_timer_module", color[red] << "Setting alarm " << msg->get_id() << " to 1 second");
	msg->start_relative(1);
	msg->send_to(message::qaddr_timer);
	msg = new TimerMsg(message::qaddr_transport,true);
	Log(EVENT_LOG, LOG_NORMAL, "test_timer_module", color[red] << "Setting alarm " << msg->get_id() << " to 1 second");
	msg->start_relative(1);
	msg->send_to(message::qaddr_timer);

	sleep(2);
	msg = new TimerMsg(message::qaddr_transport,true);
	Log(EVENT_LOG, 	LOG_NORMAL, "test_timer_module", color[red] << "Setting alarm " << msg->get_id() << " to 80 seconds" << color[off]);
	msg->start_relative(80);
	msg->send_to(message::qaddr_timer);
	sleep(6);
	msg = new TimerMsg(message::qaddr_transport,true);
	Log(EVENT_LOG, LOG_NORMAL, "test_timer_module", color[red] << "Setting alarm " << msg->get_id() <<  " to 13 seconds" << color[off]);
	msg->start_relative(13);
	saved_timer = msg->get_id();
	msg->send_to(message::qaddr_timer);
	sleep(12);
	msg = new TimerMsg(message::qaddr_transport,true);
	Log(EVENT_LOG, LOG_NORMAL, "test_timer_module" ,color[red] << "Restarting alarm " << saved_timer << " to 3 seconds" << color[off]);
	msg->restart_relative(saved_timer,3);
	msg->send_to(message::qaddr_timer);
	sleep(15);
	msg = new TimerMsg(message::qaddr_transport,true);
	Log(EVENT_LOG, LOG_NORMAL, "test_timer_module", color[red] << "Setting alarm " << msg->get_id() << " to 17 seconds" << color[off]);
	msg->start_relative(17);
	saved_timer = msg->get_id();
	msg->send_to(message::qaddr_timer);
	sleep(15);
	msg = new TimerMsg(message::qaddr_transport,true);
	Log(EVENT_LOG, LOG_NORMAL, "test_timer_module", color[red] << "Stopping alarm " << saved_timer << color[off]);
	msg->stop(saved_timer);
	msg->send_to(message::qaddr_timer);
	sleep(1);
	msg = new TimerMsg(message::qaddr_transport,true);
	Log(EVENT_LOG, LOG_NORMAL, "test_timer_module", color[red] << "Stopping non-existing alarm 1" << color[off]);
	msg->stop(1);
	msg->send_to(message::qaddr_timer);
	sleep(40);
	// stop TimerModule
	Log(EVENT_LOG, LOG_NORMAL, "test_timer_module", "Stopping TimerModule");
	tmod->stop_processing();
	// stop timerchecker
	Log(EVENT_LOG, LOG_NORMAL, "test_timer_module" , "Stopping timerchecker thread");
	done = true;
	while (running) {
		sleep(1);
	} // end while not done
	// aborting TimerModule
	Log(EVENT_LOG, LOG_NORMAL, "test_timer_module", "Aborting TimerModule");
	tmod->abort_processing();
	delete tmod;
	// destroy QueueManager
	Log(EVENT_LOG, LOG_NORMAL, "test_timer_module", "Calling QueueManager::clear()");
	QueueManager::clear();
	Log(EVENT_LOG, LOG_NORMAL, "test_timer_module", "Calling tsdb::end()");
	tsdb::end();
	return 0;
}

//@}
  
