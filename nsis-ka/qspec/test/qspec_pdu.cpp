/*
 * qspec_pdu.cpp - Test QSPEC PDUs
 *
 * $Id: qspec_pdu.cpp 6176 2011-05-20 07:39:55Z bless $
 * $HeadURL: https://svn.tm.kit.edu/nsis/qspec/trunk/test/qspec_pdu.cpp $
 *
 */
#include <cppunit/TestCase.h>
#include <cppunit/extensions/HelperMacros.h>

#include "qspec_ie.h"
#include "qspec_pdu.h"
#include <string.h>

using namespace qspec;


class QSpecPduTest : public CppUnit::TestCase {

	CPPUNIT_TEST_SUITE( QSpecPduTest );

	CPPUNIT_TEST( testComparison );
	CPPUNIT_TEST( testCopying );
	CPPUNIT_TEST( testSerialize );
	CPPUNIT_TEST( testDeserialize );
	CPPUNIT_TEST( testReadWrite );

	CPPUNIT_TEST_SUITE_END();

  public:
	void setUp();

	void testComparison();
	void testCopying();
	void testSerialize();
	void testDeserialize();
	void testReadWrite();


  private:

};

CPPUNIT_TEST_SUITE_REGISTRATION( QSpecPduTest );


// executed before each test method
void QSpecPduTest::setUp() {
	QSPEC_IEManager::clear();
}


void QSpecPduTest::testComparison() {
	qspec_pdu *p1 = new qspec_pdu();
	qspec_pdu *p2 = new qspec_pdu(ms_resource_queries, 1, 1);

	CPPUNIT_ASSERT( *p1 != *p2 );

	p1->set_qspec_type(1);
	p1->set_msg_sequence(ms_resource_queries);
	p1->set_obj_combination(1);

	CPPUNIT_ASSERT( *p1 == *p1 );
	CPPUNIT_ASSERT( *p2 == *p2 );
	CPPUNIT_ASSERT( *p1 == *p2 );

	p1->set_object(new qspec_object(ot_qos_available));

	CPPUNIT_ASSERT( *p1 != *p2 );

	p2->set_object(new qspec_object(ot_qos_available));

	CPPUNIT_ASSERT( *p1 == *p2 );

	p2->set_qspec_type(2);

	CPPUNIT_ASSERT( *p1 != *p2 );

	delete p1;
	delete p2;
}


void QSpecPduTest::testCopying() {
	phb_class *h = new phb_class((uint8) 42);
	qspec_object *o = new qspec_object(ot_qos_desired);
	o->set_parameter(h);

	qspec_pdu *p1 = new qspec_pdu(ms_resource_queries, 1, 1);
	p1->set_object(o);


	qspec_pdu *p2 = p1->copy();

	CPPUNIT_ASSERT( *p1 == *p2 );

	// now make sure that this was a deep copy

	CPPUNIT_ASSERT( p1->get_num_objects() == p2->get_num_objects() );
	CPPUNIT_ASSERT( o == p1->get_object(ot_qos_desired) );
	CPPUNIT_ASSERT( o != p2->get_object(ot_qos_desired) );

	o->set_error(true);

	CPPUNIT_ASSERT( *p1 != *p2 );

	delete p1;
	delete p2;
}


void QSpecPduTest::testSerialize() {
	/*
	 * Error: parameter ID larger than 12 bit.
	 */
        qspec_pdu *p1 = new qspec_pdu(ms_resource_queries, 0, 0xAB);
	qspec_object *o1 = new qspec_object(ot_qos_desired);
	o1->set_parameter(new phb_class((uint8) 42));
	p1->set_object(o1);

	CPPUNIT_ASSERT( p1->get_qspec_type() == 0xAB );
	// expect that check fails
	CPPUNIT_ASSERT( p1->check() == false );

	NetMsg msg1(8);
	uint32 bytes_written1 = 42;

	CPPUNIT_ASSERT_THROW(
		p1->serialize(msg1, IE::protocol_v1, bytes_written1), IEError);

	CPPUNIT_ASSERT( bytes_written1 == 0 );

	delete p1;


	/*
	 * Error: Not enough space in NetMsg.
	 */
	qspec_pdu *p2 = new qspec_pdu(ms_resource_queries, 0, 1);
	qspec_object *o2 = new qspec_object(ot_qos_desired);
	o2->set_parameter(new phb_class((uint8) 42));
	p2->set_object(o2);

	NetMsg msg2(2);
	uint32 bytes_written2 = 42;

	CPPUNIT_ASSERT_THROW(
		p2->serialize(msg2, IE::protocol_v1, bytes_written2), IEError);

	CPPUNIT_ASSERT( bytes_written2 == 0 );

	delete p2;


	/*
	 * Error: Empty QSPEC PDU (without objects).
	 */
	qspec_pdu *p3 = new qspec_pdu(ms_resource_queries, 0, 1);

	NetMsg msg3(100);
	uint32 bytes_written3 = 42;

	CPPUNIT_ASSERT_THROW(
		p3->serialize(msg3, IE::protocol_v1, bytes_written3), IEError
	);

	delete p3;

	// for a serialization without errors see testReadWrite()
}


void QSpecPduTest::testDeserialize() {
	QSPEC_IEManager::clear();
	QSPEC_IEManager::register_known_ies();

	QSPEC_IEManager *iem = QSPEC_IEManager::instance();

	/*
	 * Test for empty NetMsg;
	 */
	NetMsg msg1(1);
	IEErrorList errlist1;
	uint32 bytes_read1;

	IE *ie1 = iem->deserialize(msg1, cat_qspec_pdu, IE::protocol_v1,
			errlist1, bytes_read1, false);

	CPPUNIT_ASSERT( ie1 == NULL );
	CPPUNIT_ASSERT( errlist1.is_empty() == false );

	IEError *err1 = errlist1.get();
	CPPUNIT_ASSERT( dynamic_cast<IEMsgTooShort *>(err1) != NULL );
	delete err1;

	CPPUNIT_ASSERT( errlist1.is_empty() == true );


	/*
	 * Test for QSPECs without objects.
	 */
	NetMsg msg2(4);
	msg2.encode32(0x00011100);
	msg2.set_pos(0);

	IEErrorList errlist2;
	uint32 bytes_read2;

	IE *ie2 = iem->deserialize(msg2, cat_qspec_pdu, IE::protocol_v1,
			errlist2, bytes_read2, false);

	CPPUNIT_ASSERT( ie2 == NULL );
	CPPUNIT_ASSERT( errlist2.is_empty() == false );

	IEError *err2 = errlist2.get();
	CPPUNIT_ASSERT( dynamic_cast<PDUSyntaxError *>(err2) != NULL );
	delete err2;

	CPPUNIT_ASSERT( errlist2.is_empty() == true );


	// Once again, but with skip == true.
	msg2.set_pos(0);
	IE *ie3 = iem->deserialize(msg2, cat_qspec_pdu, IE::protocol_v1,
			errlist2, bytes_read2, true);

	CPPUNIT_ASSERT( ie3 != NULL );
	CPPUNIT_ASSERT( errlist2.is_empty() == false );
}


void QSpecPduTest::testReadWrite() {

        qspec_pdu *p1 = new qspec_pdu(ms_resource_queries, 0, 15);

	qspec_object *o1 = new qspec_object(ot_qos_desired);
	o1->set_parameter(new phb_class((uint8) 42));
	p1->set_object(o1);

	qspec_object *o2 = new qspec_object(ot_minimum_qos);
	o2->set_parameter(new t_mod(2048000.0, 576.0, 100000.0, 40));
	p1->set_object(o2);

	uint32 expected_size= p1->get_serialized_size(IE::protocol_v1);
	NetMsg msg(expected_size);
	uint32 bytes_written = 42;

	p1->serialize(msg, IE::protocol_v1, bytes_written);

	CPPUNIT_ASSERT( bytes_written == expected_size );

	//std::cout << msg.hexdump(cout) << '\n';


	/*
	 * Now read the serialized object.
	 */
	QSPEC_IEManager::clear();
	QSPEC_IEManager::instance()->register_ie(new qspec_pdu());
	QSPEC_IEManager::instance()->register_ie(new qspec_object());
	QSPEC_IEManager::instance()->register_ie(new t_mod());
	QSPEC_IEManager::instance()->register_ie(new phb_class());
	msg.set_pos(0);
	IEErrorList errlist;
	uint32 bytes_read= 0;

	qspec_pdu *p2 = dynamic_cast<qspec_pdu*>(QSPEC_IEManager::instance()->deserialize(msg,
			cat_qspec_pdu, IE::protocol_v1, errlist, bytes_read, false));

	CPPUNIT_ASSERT( errlist.is_empty() );
	CPPUNIT_ASSERT( p2 != NULL );
	CPPUNIT_ASSERT( bytes_read == bytes_written );

	//std::cout << "p1" << *p1 << endl
	//	  << "p2" << *p2 << endl;

	CPPUNIT_ASSERT( *p1 == *p2 );

	// serializing p2 again should yield an identical bytes stream
	NetMsg msg_cmp(bytes_written);
	p2->serialize(msg_cmp, IE::protocol_v1, bytes_written);

	// byte stream the same now?
	CPPUNIT_ASSERT( memcmp(msg.get_buffer(), msg_cmp.get_buffer(), bytes_written) == 0 );
	
	delete p1;
	delete p2;
}


// EOF
