/*
 * generic_attr_test.cpp - A generic test case for auth_session attributes
 *
 * $Id:$
 * $HeadURL:$
 *
 */
#include "generic_attr_test.h"
#include <stdio.h>

using namespace nslp_auth;


// Test if the child classes implemented the abstract methods correctly.
void GenericAttributeTest::testMyself() {
	auth_attr *p1 = createInstance1();
	auth_attr *p2 = createInstance2();

	CPPUNIT_ASSERT( *p1 != *p2 );

	auth_attr *p3 = createInstance1();

	CPPUNIT_ASSERT( *p1 == *p3 );

	mutate1(p3);

	CPPUNIT_ASSERT( *p1 != *p3 );

	delete p1;
	delete p2;
	delete p3;
}


void GenericAttributeTest::testComparison() {
	auth_attr *p1 = createInstance1();
	auth_attr *p2 = createInstance1();

	CPPUNIT_ASSERT( p1 != p2 );
	CPPUNIT_ASSERT( *p1 == *p2 && *p2 == *p1 );

	mutate1(p1);

	CPPUNIT_ASSERT( *p1 != *p2 && *p2 != *p1 );

	delete p1;
	delete p2;
}


void GenericAttributeTest::testCopying() {
	auth_attr *p1 = createInstance1();

	auth_attr *p2 = p1->copy();

	CPPUNIT_ASSERT( *p1 == *p2 );
	CPPUNIT_ASSERT( typeid(p1) == typeid(p2) );

	mutate1(p1);

	CPPUNIT_ASSERT( *p1 != *p2 );

	delete p1;
	delete p2;
}


void GenericAttributeTest::testSerialize() {


	// Error: Not enough space in NetMsg.

	auth_attr *p2 = createInstance2();

	NetMsg msg2(5);
	uint32 bytes_written2 = 42;

	CPPUNIT_ASSERT_THROW(
		p2->serialize(msg2, IE::protocol_v1, bytes_written2), IEError);

	CPPUNIT_ASSERT( bytes_written2 == 4 ); // just enough space for header

	delete p2;
}


void GenericAttributeTest::testReadWrite() {
	auth_attr *p1 = createInstance2();

	NetMsg msg( p1->get_serialized_size(IE::protocol_v1) );
	uint32 bytes_written= 0;
	p1->serialize(msg, IE::protocol_v1, bytes_written);

	CPPUNIT_ASSERT( bytes_written == p1->get_serialized_size(IE::protocol_v1) );
	
	//std::cout << msg.hexdump(cout) << '\n';

	// Now read the serialized object.

	msg.set_pos(0);
	IEErrorList errlist;

	auth_attr *p2 = createInstance1();
	CPPUNIT_ASSERT( *p1 != *p2 );

	uint32 num_read= 0;
	bool read_success = false;
	try {
		read_success = p2->deserialize(msg, IE::protocol_v1, errlist, num_read, false);
	} catch (...) {
		while(!errlist.is_empty()) std::cout<<"Error : "<<errlist.get()->getstr()<<std::endl;	
	}

	CPPUNIT_ASSERT( read_success );
	CPPUNIT_ASSERT( num_read == bytes_written );
	CPPUNIT_ASSERT( num_read == p2->get_serialized_size(IE::protocol_v1) );

	CPPUNIT_ASSERT( *p1 == *p2 );

	delete p1;
	delete p2;
}

// EOF
