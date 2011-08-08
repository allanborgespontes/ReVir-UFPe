/*
 * generic_param_test.cpp - A generic test case for QSPEC parameters
 *
 * $Id: generic_param_test.cpp 571 2005-11-03 09:44:38Z stud-matfried $
 * $HeadURL: https://svn.tm.kit.edu/nsis/qspec/trunk/test/generic_param_test.cpp $
 *
 */
#include "generic_param_test.h"

using namespace qspec;


// Test if the child classes implemented the abstract methods correctly.
void GenericParamTest::testMyself() {
	qspec_param *p1 = createInstance1();
	qspec_param *p2 = createInstance2();

	CPPUNIT_ASSERT( *p1 != *p2 );

	qspec_param *p3 = createInstance1();

	CPPUNIT_ASSERT( *p1 == *p3 );

	mutate1(p3);

	CPPUNIT_ASSERT( *p1 != *p3 );

	delete p1;
	delete p2;
	delete p3;
}


void GenericParamTest::testComparison() {
	qspec_param *p1 = createInstance1();
	qspec_param *p2 = createInstance1();

	CPPUNIT_ASSERT( p1 != p2 );
	CPPUNIT_ASSERT( *p1 == *p2 && *p2 == *p1 );

	mutate1(p1);

	CPPUNIT_ASSERT( *p1 != *p2 && *p2 != *p1 );

	delete p1;
	delete p2;
}


void GenericParamTest::testCopying() {
	qspec_param *p1 = createInstance1();
	p1->set_error(true);

	qspec_param *p2 = p1->copy();

	CPPUNIT_ASSERT( *p1 == *p2 );
	CPPUNIT_ASSERT( typeid(p1) == typeid(p2) );

	p1->set_error(false);

	CPPUNIT_ASSERT( *p1 != *p2 );

	delete p1;
	delete p2;
}


void GenericParamTest::testSerialize() {
	/*
	 * Error: parameter ID larger than 12 bit.
	 */
	qspec_param *p1 = createInstance1();
	p1->set_parameter_id(0xFABC);

	CPPUNIT_ASSERT( p1->check() == false );

	NetMsg msg1( p1->get_serialized_size(IE::protocol_v1) );
	uint32 bytes_written1 = 42;

	CPPUNIT_ASSERT_THROW(
		p1->serialize(msg1, IE::protocol_v1, bytes_written1), IEError);

	CPPUNIT_ASSERT( bytes_written1 == 0 );

	delete p1;


	/*
	 * Error: Not enough space in NetMsg.
	 */
	qspec_param *p2 = createInstance2();

	NetMsg msg2(5);
	uint32 bytes_written2 = 42;

	CPPUNIT_ASSERT_THROW(
		p2->serialize(msg2, IE::protocol_v1, bytes_written2), IEError);

	CPPUNIT_ASSERT( bytes_written2 == 4 ); // just enough space for header

	delete p2;
}


void GenericParamTest::testReadWrite() {
	qspec_param *p1 = createInstance2();
	p1->set_mandatory(false);
	p1->set_error(true);

	NetMsg msg( p1->get_serialized_size(IE::protocol_v1) );
	uint32 bytes_written;
	p1->serialize(msg, IE::protocol_v1, bytes_written);

	
	//std::cout << msg.hexdump(cout) << '\n';

	/*
	 * Now read the serialized object.
	 */
	msg.set_pos(0);
	IEErrorList errlist;
	uint32 num_read;

	qspec_param *p2 = createInstance1();
	IE *ie = p2->deserialize(msg, IE::protocol_v1, errlist,
			num_read, false);

	CPPUNIT_ASSERT( num_read == p2->get_serialized_size(IE::protocol_v1) );
	CPPUNIT_ASSERT( ie != NULL );

	CPPUNIT_ASSERT( *p1 == *p2 );

	delete p1;
	delete p2;
}

// EOF
