/*
 * queue_manager.cpp - Test the QueueManager class and message passing.
 *
 * $Id: queue_manager.cpp 4107 2009-07-16 13:49:52Z bless $
 * $HeadURL: https://svn.tm.kit.edu/nsis/protlib/trunk/test/queue_manager.cpp $
 *
 */
#include <cppunit/TestCase.h>
#include <cppunit/extensions/HelperMacros.h>

#include "queuemanager.h"
#include "fqueue.h"

using namespace protlib;


class QueueManagerTest : public CppUnit::TestCase {

	CPPUNIT_TEST_SUITE( QueueManagerTest );

	CPPUNIT_TEST( testMessagePassing );

	CPPUNIT_TEST_SUITE_END();

  public:
	void testMessagePassing() {
		// create a new queue
		FastQueue *fq = new FastQueue("testqueue", true);

		// register queue
		QueueManager::clear();
		QueueManager *mgr = QueueManager::instance();
		mgr->register_queue(fq, message::qaddr_api_0);

		// Create a message and send to the queue registered for this
		// type of message. The message's source is set to unknown.
		message *msg = new message(
			message::type_API, message::qaddr_unknown, 1);
		msg->send_to(message::qaddr_api_0);

		CPPUNIT_ASSERT( fq->dequeue(true) == msg );

		QueueManager::clear();

		delete msg;
	}
		
};

CPPUNIT_TEST_SUITE_REGISTRATION( QueueManagerTest );
