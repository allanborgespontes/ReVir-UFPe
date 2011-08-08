/*
 * natfw_msg.cpp - Test the natfw_msg class.
 *
 * $Id: natfw_msg.cpp 2438 2006-12-08 15:07:34Z stud-matfried $
 * $HeadURL: https://svn.tm.kit.edu/nsis/natfw-nslp/trunk/test/natfw_msg.cpp $
 *
 */
#include <cppunit/TestCase.h>
#include <cppunit/extensions/HelperMacros.h>

#include "network_message.h"

#include "msg/natfw_ie.h"
#include "msg/natfw_msg.h"
#include "msg/natfw_object.h"

using namespace natfw::msg;


class NatFwMsgTest : public CppUnit::TestCase {

	CPPUNIT_TEST_SUITE( NatFwMsgTest );

	CPPUNIT_TEST( testComparison );
	CPPUNIT_TEST( testCopying );
	CPPUNIT_TEST( testManager );

	CPPUNIT_TEST_SUITE_END();

  public:
	void testComparison();
	void testCopying();
	void testManager();
};

CPPUNIT_TEST_SUITE_REGISTRATION( NatFwMsgTest );


void NatFwMsgTest::testComparison() {
	natfw_ext *m1 = new natfw_ext();
	natfw_ext *m2 = new natfw_ext(true);

	CPPUNIT_ASSERT( *m1 == *m1 );
	CPPUNIT_ASSERT( *m2 == *m2 );
	CPPUNIT_ASSERT( *m1 != *m2 );
	CPPUNIT_ASSERT( *m2 != *m1 );

	m1->set_proxy_mode(true);
	CPPUNIT_ASSERT( *m1 == *m2 );
	CPPUNIT_ASSERT( *m2 == *m1 );

	m1->set_session_lifetime(10);
	CPPUNIT_ASSERT( *m1 != *m2 );

	m2->set_session_lifetime(10);
	CPPUNIT_ASSERT( *m1 == *m2 );

	m2->set_proxy_mode(false);
	CPPUNIT_ASSERT( *m1 != *m2 );

	delete m2;
	delete m1;
}


void NatFwMsgTest::testCopying() {
	natfw_create *m1 = new natfw_create(true);
	m1->set_nonce(42);

	natfw_create *m2 = m1->copy();
	CPPUNIT_ASSERT( m1 != m2 );
	CPPUNIT_ASSERT( *m1 == *m2 );

	CPPUNIT_ASSERT( m1->get_nonce() == m2->get_nonce() );

	delete m2;
	delete m1;
}


void NatFwMsgTest::testManager() {
	NATFW_IEManager::clear();
	NATFW_IEManager::register_known_ies();
	NATFW_IEManager *mgr = NATFW_IEManager::instance();

	natfw_create *m1 = new natfw_create();
	m1->set_session_lifetime(30);
	m1->set_msg_sequence_number(47);

	NetMsg msg( m1->get_serialized_size(IE::protocol_v1) );
	uint32 bytes_written;
	m1->serialize(msg, IE::protocol_v1, bytes_written);

	/*
	 * Now read the serialized object.
	 */
	msg.set_pos(0);
	IEErrorList errlist;
	uint32 num_read;

	IE *ie = mgr->deserialize(msg, cat_natfw_msg, IE::protocol_v1, errlist,
			num_read, false);
	CPPUNIT_ASSERT( ie != NULL );
	CPPUNIT_ASSERT( errlist.is_empty() );

	CPPUNIT_ASSERT( num_read == ie->get_serialized_size(IE::protocol_v1) );
	CPPUNIT_ASSERT( ie != NULL );

	CPPUNIT_ASSERT( *m1 == *ie );
}

// EOF
