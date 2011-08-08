/*
 * qspec_object.cpp - Test QSPEC objects
 *
 * $Id: qspec_object.cpp 3244 2008-07-28 20:16:54Z bless $
 * $HeadURL: https://svn.tm.kit.edu/nsis/qspec/trunk/test/qspec_object.cpp $
 *
 */
#include <cppunit/TestCase.h>
#include <cppunit/extensions/HelperMacros.h>

#include "qspec_ie.h"
#include "qspec_object.h"

using namespace qspec;


class QSpecObjectTest : public CppUnit::TestCase {

	CPPUNIT_TEST_SUITE( QSpecObjectTest );

	CPPUNIT_TEST( testComparison );
	CPPUNIT_TEST( testCopying );
	CPPUNIT_TEST( testSerialize );
	CPPUNIT_TEST( testDeserialize );
	CPPUNIT_TEST( testReadWrite );

	CPPUNIT_TEST_SUITE_END();

  public:
	void testComparison();
	void testCopying();
	void testSerialize();
	void testDeserialize();
	void testReadWrite();

  private:

};

CPPUNIT_TEST_SUITE_REGISTRATION( QSpecObjectTest );


void QSpecObjectTest::testComparison() {
	qspec_object *o1 = new qspec_object();
	qspec_object *o2 = new qspec_object(ot_qos_available);

	CPPUNIT_ASSERT( *o1 != *o2 );

	o2->set_object_id(ot_qos_desired);

	CPPUNIT_ASSERT( *o1 == *o1 );
	CPPUNIT_ASSERT( *o1 == *o2 );

	o1->set_parameter(new phb_class((uint8) 42));

	CPPUNIT_ASSERT( *o1 != *o2 );

	o2->set_parameter(new phb_class((uint8) 42));

	CPPUNIT_ASSERT( *o1 == *o2 );

	o2->set_error(true);

	CPPUNIT_ASSERT( *o1 != *o2 );

	delete o1;
	delete o2;
}


void QSpecObjectTest::testCopying() {
  qspec_object *o1 = new qspec_object(ot_qos_available);

  phb_class *h = new phb_class((uint8) 42);
	o1->set_parameter(h);

	qspec_object *o2 = o1->copy();

	CPPUNIT_ASSERT( *o1 == *o2 );

	// now make sure that this was a deep copy

	CPPUNIT_ASSERT( o1->get_num_parameters() == o2->get_num_parameters() );
	CPPUNIT_ASSERT( h == o1->get_parameter(phb_class::PARAM_ID) );
	CPPUNIT_ASSERT( h != o2->get_parameter(phb_class::PARAM_ID) );

	h->set_dscp_value(11);

	CPPUNIT_ASSERT( *o1 != *o2 );

	delete o1;
	delete o2;
}


void QSpecObjectTest::testSerialize() {
	/*
	 * Error: parameter ID larger than 12 bit.
	 */
  qspec_object *o1 = new qspec_object(0xFABC);
  o1->set_parameter(new phb_class((uint8) 42));

	CPPUNIT_ASSERT( o1->get_object_id() == 0xFABC );
	CPPUNIT_ASSERT( o1->check() == false );

	NetMsg msg1(8);
	uint32 bytes_written1 = 42;

	CPPUNIT_ASSERT_THROW(
		o1->serialize(msg1, IE::protocol_v1, bytes_written1), IEError);

	CPPUNIT_ASSERT( bytes_written1 == 0 );

	delete o1;


	/*
	 * Error: Not enough space in NetMsg.
	 */
	qspec_object *o2 = new qspec_object(ot_qos_desired);
	o2->set_parameter(new phb_class((uint8) 42));

	NetMsg msg2(2);
	uint32 bytes_written2 = 42;

	CPPUNIT_ASSERT_THROW(
		o2->serialize(msg2, IE::protocol_v1, bytes_written2), IEError);

	CPPUNIT_ASSERT( bytes_written2 == 0 );

	delete o2;


	/*
	 * Error: Empty object (without parameters).
	 */
	qspec_object *o3 = new qspec_object(ot_qos_desired);

	NetMsg msg3(100);
	uint32 bytes_written3 = 42;

	CPPUNIT_ASSERT_THROW(
		o3->serialize(msg3, IE::protocol_v1, bytes_written3), IEError
	);

	delete o3;

	// for a serialization without errors see testReadWrite()
}


void QSpecObjectTest::testDeserialize() {

	/*
	 * Test for empty NetMsg;
	 */
	NetMsg msg1(1);
	IEErrorList errlist1;
	uint32 bytes_read1;

	qspec_object *obj1 = new qspec_object();

	IE *ie1 = obj1->deserialize(msg1, IE::protocol_v1, errlist1,
			bytes_read1, false);

	CPPUNIT_ASSERT( ie1 == NULL );
	CPPUNIT_ASSERT( errlist1.is_empty() == false );

	IEError *err1 = errlist1.get();
	CPPUNIT_ASSERT( dynamic_cast<IEMsgTooShort *>(err1) != NULL );
	delete err1;

	CPPUNIT_ASSERT( errlist1.is_empty() == true );

	delete obj1;


	/*
	 * Test for QSPECs without objects.
	 */
	NetMsg msg2(4);
	msg2.encode32(0x00000000);
	msg2.set_pos(0);

	IEErrorList errlist2;
	uint32 bytes_read2;

	qspec_object *obj2 = new qspec_object();

	IE *ie2 = obj2->deserialize(msg2, IE::protocol_v1, errlist2,
			bytes_read2, false);

	CPPUNIT_ASSERT( ie2 == NULL );
	CPPUNIT_ASSERT( errlist2.is_empty() == false );

	IEError *err2 = errlist2.get();
	CPPUNIT_ASSERT( dynamic_cast<PDUSyntaxError *>(err2) != NULL );
	delete err2;

	CPPUNIT_ASSERT( errlist2.is_empty() == true );


	// Once again, but with skip == true.
	msg2.set_pos(0);
	IE *ie3 = obj2->deserialize(msg2, IE::protocol_v1, errlist2,
			bytes_read2, true);

	CPPUNIT_ASSERT( ie3 != NULL );
	CPPUNIT_ASSERT( errlist2.is_empty() == false );

	delete obj2;
}


void QSpecObjectTest::testReadWrite() {

	qspec_object *o1 = new qspec_object(ot_qos_desired);
	o1->set_parameter(new t_mod(2048000.0, 576.0, 100000.0, 40));
	o1->set_parameter(new path_latency(100000));

	uint32 expected_len= o1->get_serialized_size(IE::protocol_v1);
	NetMsg msg(expected_len);
	uint32 bytes_written = 42;

	o1->serialize(msg, IE::protocol_v1, bytes_written);

	CPPUNIT_ASSERT( bytes_written == expected_len );

	//std::cout << msg.hexdump(cout) << '\n';


	/*
	 * Now read the serialized object.
	 */
	QSPEC_IEManager::instance()->register_known_ies();
	msg.set_pos(0);
	IEErrorList errlist;
	uint32 bytes_read;

	qspec_object *o2 = new qspec_object();

	IE *ie = o2->deserialize(msg, IE::protocol_v1, errlist,
			bytes_read, false);

	//std::cout << errlist.get()->get_msg() << "\n";
	//std::cout << "Written: " << bytes_written << "\n";
	//std::cout << "Read: " << bytes_read << "\n";

	CPPUNIT_ASSERT ( errlist.is_empty() );
	CPPUNIT_ASSERT( ie != NULL );
	CPPUNIT_ASSERT( ie == o2 );
	CPPUNIT_ASSERT( bytes_read == bytes_written );

	if ( *o1 != *o2)
	  std::cout << "o1:" << *o1 << "o2:" << *o2;

	CPPUNIT_ASSERT( *o1 == *o2 );

	delete o1;
	delete o2;
}


// EOF
