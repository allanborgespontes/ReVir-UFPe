/*
 * Test the ICMP Types Object
 *
 * $Id: icmp_types.cpp 1726 2006-05-11 09:38:02Z stud-matfried $
 * $HeadURL: https://svn.tm.kit.edu/nsis/natfw-nslp/trunk/test/icmp_types.cpp $
 */
#include "generic_object_test.h"

using namespace natfw::msg;


class IcmpTypesTest : public GenericObjectTest {

	CPPUNIT_TEST_SUITE( IcmpTypesTest );

	CPPUNIT_TEST( testSize );
	NATFW_OBJECT_DEFAULT_TESTS();

	CPPUNIT_TEST_SUITE_END();

  public:
	virtual natfw_object *createInstance1() const {
		return new icmp_types();
	}

	virtual natfw_object *createInstance2() const {
		std::list<uint8> types; // empty
		return new icmp_types(types, natfw_object::tr_mandatory);
	}

	virtual void mutate1(natfw_object *o) const {
		icmp_types *it = dynamic_cast<icmp_types *>(o);
		it->set_treatment(natfw_object::tr_mandatory);
	}

	void testSize() {
		icmp_types t1 = icmp_types();
		CPPUNIT_ASSERT( t1.get_serialized_size(IE::protocol_v1) == 8 );

		NetMsg msg1( t1.get_serialized_size(IE::protocol_v1) );
		uint32 bytes_written;
		t1.serialize(msg1, IE::protocol_v1, bytes_written);


		std::list<uint8> types;
		types.push_back(47);
		types.push_back(48);
		types.push_back(49);
		types.push_back(50);

		icmp_types t2 = icmp_types(types);
		CPPUNIT_ASSERT( t2.get_serialized_size(IE::protocol_v1) == 12 );

		// serialize the object
		NetMsg msg2( t2.get_serialized_size(IE::protocol_v1) );
		t2.serialize(msg2, IE::protocol_v1, bytes_written);

		// now deserialize again
		icmp_types t3 = icmp_types();
		msg2.set_pos(0);
		IEErrorList errlist;
		uint32 num_read;

		IE *ie = t3.deserialize(msg2, IE::protocol_v1, errlist,
			num_read, false);

		CPPUNIT_ASSERT( ie != NULL );
		CPPUNIT_ASSERT( t3 == t2 );
	}
};

CPPUNIT_TEST_SUITE_REGISTRATION( IcmpTypesTest );

// EOF
