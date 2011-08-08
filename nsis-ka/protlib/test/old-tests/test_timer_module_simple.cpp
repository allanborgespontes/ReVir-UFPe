/// ----------------------------------------*- mode: C++; -*--
/// @file test_timer_module_simple.cpp
/// Test for timer module
/// ----------------------------------------------------------
/// $Id: test_timer_module_simple.cpp 3527 2008-10-10 13:25:24Z bless $
/// $HeadURL: https://svn.tm.kit.edu/nsis/protlib/trunk/test/old-tests/test_timer_module_simple.cpp $
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

/** 
 * This is a test for the timer module.
 */

#include <unistd.h>
#include <iostream>

#include "timer_module.h"
#include "threadsafe_db.h"
#include "queuemanager.h"

using namespace protlib;
using namespace protlib::log;

logfile commonlog;
protlib::log::logfile& protlib::log::DefaultLog(commonlog);


struct timeval now;

bool done = false;
bool running = false;

pthread_t tmthread;

void* 
timerchecker(void *p) 
{
  // get my message queue: I'm known as message::qaddr_transport instance
  FastQueue* fq = QueueManager::instance()->get_queue(message::qaddr_transport);
  int32 sec = 0;
  int32 msec = 0;
  if (!fq) {
    cout << "Timerchecker cannot find message queue" << endl;
    return NULL;
  } // end if not fq

  
  running = true;
  cout << "Timerchecker started" << endl;

  message* msg = NULL;
  TimerMsg* tmsg = NULL;
  while (!done) 
  {
    cout << "Waiting for messages another 5 seconds" << endl;
    // wait for a message arriving at my queue (at most 5 seconds)
    msg = fq->dequeue_timedwait(5000);
    if (msg) 
    {
      // message arrived
      cout << "Timer Checker Thread received a message: " << endl;
	
      if (msg->get_type()==message::type_timer) 
      {
	// this is a message from the timer module
	tmsg = dynamic_cast<TimerMsg*>(msg);
	
	// just for comparison
	gettimeofday(&now,0);

	tmsg->get_time(sec,msec);
	cout << "*Timer Fired* Now:" << now.tv_sec << "." << now.tv_usec
	     << " timer id:" << tmsg->get_id()
	     << " action:" << tmsg->get_action()
	     << " ok: " << tmsg->is_ok()
	     << " relative:" << tmsg->is_relative() 
	     << " sec:" << sec
	     << " msec:" << msec << endl;
	
	tmsg = NULL;
      } // end if type_timer
      // else if (msg->get_type()==message::signaling) { // signaling PDU arrived
      // ... }
      
      delete msg;
      msg = NULL;
    } // end if msg
  } // end while not done

  cout << "timer checker done." << endl;
  
  fq->cleanup();
  
  running = false;
  return NULL;
} // end timerchecker

int main() 
{
  tsdb::init();

  // create queue for timer checker thread
  cout<<"Creating timerchecker queue..."<<endl;
  FastQueue* timerchk_fq= new FastQueue("timerchecker",true);
  QueueManager::instance()->register_queue(timerchk_fq, message::qaddr_transport);
  // creates timer check thread
  pthread_create(&tmthread,NULL,timerchecker,NULL);


  cout << "starting timer module" << endl;
  TimerModuleParam tmpar(3,false,false,false);
  // create and start Timer module thread
  ThreadStarter<TimerModule,TimerModuleParam> tmod(1,tmpar);
  
  // start timer manager
  tmod.start_processing();
  
  sleep(1);
  
  // start new timer
  // Create message for timer manager
  TimerMsg* msg = new TimerMsg(message::qaddr_transport,true);
  msg->start_relative(1); // from now on in 1 second
  // send TimerMsg to TimerManager

  gettimeofday(&now,0);
  msg->send_to(message::qaddr_timer);
  cout << "*Timer Start* Now:" << now.tv_sec << "." << now.tv_usec
       << " Setting alarm " << msg->get_id() << " to " << 1 << " second"<< endl;
  // wait two seconds
  sleep(2);
  
  msg = new TimerMsg(message::qaddr_transport,true);
  msg->start_relative(20);

  gettimeofday(&now,0);
  msg->send_to(message::qaddr_timer);
  cout << "*Timer Start* Now:" << now.tv_sec << "." << now.tv_usec
       << " Setting alarm " << msg->get_id() << " to " << 20 << " seconds"<< endl;
  
  sleep(40);
	
  // stop TimerModule
  cout << "stopping timer module" << endl;
  tmod.stop_processing();

  // stop timerchecker
  done = true;
  // wait for timer checker to finish
  while (running) {
    sleep(1);
  } // end while not done
  
  // aborting TimerModule
  tmod.abort_processing();
  // destroy QueueManager
  QueueManager::clear();
  tsdb::end();
  return 0;
}

//@}
  
