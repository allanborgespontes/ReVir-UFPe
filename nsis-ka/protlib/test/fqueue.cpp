/*
 * test/fqueue.cpp - Test the FastQueue class.
 *
 * $Id: fqueue.cpp 4107 2009-07-16 13:49:52Z bless $
 * $HeadURL: https://svn.tm.kit.edu/nsis/protlib/trunk/test/fqueue.cpp $
 *
 */
#include <cppunit/TestCase.h>
#include <cppunit/extensions/HelperMacros.h>

#include "fqueue.h"
#include "threadsafe_db.h"

using namespace protlib;


class FastQueueTest : public CppUnit::TestCase {

	CPPUNIT_TEST_SUITE( FastQueueTest );

	CPPUNIT_TEST( testQueue );

	CPPUNIT_TEST_SUITE_END();

  public:
	void testQueue() {
		// Necessary because of threadsafe message ID generation.
		tsdb::init();

		FastQueue *fq = new FastQueue("testqueue", true);

		// test blocking
		CPPUNIT_ASSERT( fq->dequeue(false) == NULL );
		CPPUNIT_ASSERT( fq->is_empty() );

		// basic add+remove test
		message *msg1 = new message(message::type_API);
		CPPUNIT_ASSERT( fq->enqueue(msg1, false) );
		CPPUNIT_ASSERT( fq->dequeue(true) == msg1 );
		CPPUNIT_ASSERT( fq->is_empty() );

		// test ordering without expedited messages
		message *msg2 = new message(message::type_API);
		CPPUNIT_ASSERT( fq->enqueue(msg1, false) );
		CPPUNIT_ASSERT( fq->enqueue(msg2, false) );
		CPPUNIT_ASSERT( fq->size() == 2 );
		CPPUNIT_ASSERT( fq->dequeue(true) == msg1 );
		CPPUNIT_ASSERT( fq->dequeue(true) == msg2 );
		CPPUNIT_ASSERT( fq->is_empty() );

		// test with expedited messages
		message *msg3 = new message(message::type_API);
		CPPUNIT_ASSERT( fq->enqueue(msg1, false) );
		CPPUNIT_ASSERT( fq->enqueue(msg2, true) ); // expedited
		CPPUNIT_ASSERT( fq->enqueue(msg3, true) ); // expedited
		CPPUNIT_ASSERT( fq->dequeue(true) == msg2 );
		CPPUNIT_ASSERT( fq->dequeue(true) == msg3 );
		CPPUNIT_ASSERT( fq->dequeue(true) == msg1 );
		CPPUNIT_ASSERT( fq->is_empty() );

		// test shutdown flag
		fq->shutdown();
		CPPUNIT_ASSERT( fq->enqueue(msg1, false) == false );
		CPPUNIT_ASSERT( fq->is_empty() );

		delete msg3;
		delete msg2;
		delete msg1;
		delete fq;
	}
		
};

CPPUNIT_TEST_SUITE_REGISTRATION( FastQueueTest );
