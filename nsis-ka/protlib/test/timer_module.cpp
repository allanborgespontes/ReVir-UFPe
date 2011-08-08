/*
 * test/timer_module.cpp - Test the timer module.
 *
 * $Id: timer_module.cpp 4107 2009-07-16 13:49:52Z bless $
 * $HeadURL: https://svn.tm.kit.edu/nsis/protlib/trunk/test/timer_module.cpp $
 *
 */
#include <cppunit/TestCase.h>
#include <cppunit/extensions/HelperMacros.h>

#include "timer_module.h"
#include "fqueue.h"
#include "queuemanager.h"
#include "threadsafe_db.h"

using namespace protlib;
using namespace protlib::log;


void *timerchecker(void *data) {
	std::cout << "timerchecker\n";

	//FastQueue *fq = QueueManager::instance()->get_queue(message::qaddr_transport);


	return NULL;
}


class TimerModuleTest : public CppUnit::TestCase {

	CPPUNIT_TEST_SUITE( TimerModuleTest );

	CPPUNIT_TEST( testTimer );

	CPPUNIT_TEST_SUITE_END();

  public:
	void testTimer() {
		tsdb::init(); // initialize the threadsafe db

		// Create and register a queue. The timer module will send
		// messages to us using this queue.
		FastQueue *fq = new FastQueue("myqueue", false);
		QueueManager::instance()->register_queue(fq,
			message::qaddr_transport);

		// Start up the timer module.
		TimerModuleParam param(1, false, false, false);
		ThreadStarter<TimerModule, TimerModuleParam> tmod(1, param);
		tmod.start_processing();

		// Create a timer and send it to the timer module. It will
		// send it back to us as soon as the timer has elapsed.
		TimerMsg *msg = new TimerMsg(message::qaddr_transport, true);
		msg->start_relative(1);
		CPPUNIT_ASSERT( msg->get_action() == TimerMsg::ac_start );
		msg->send_to(message::qaddr_timer);

		// Check if msg is sent back to us. Block until we get it.
		CPPUNIT_ASSERT( fq->dequeue(true) == msg );
		CPPUNIT_ASSERT( msg->get_action() == TimerMsg::ac_elapsed );

		tmod.stop_processing();

		delete fq;
	}
		
};

CPPUNIT_TEST_SUITE_REGISTRATION( TimerModuleTest );
