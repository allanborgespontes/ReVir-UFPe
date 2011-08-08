/*
 * Test the External Address Object
 *
 * $Id: external_address.cpp 1707 2006-03-31 11:40:41Z stud-matfried $
 * $HeadURL: https://svn.tm.kit.edu/nsis/natfw-nslp/trunk/test/external_address.cpp $
 */
#include "address.h"	// from protlib

#include "generic_object_test.h"

using namespace natfw::msg;


class ExternalAddressTest : public GenericObjectTest {

	CPPUNIT_TEST_SUITE( ExternalAddressTest );

	NATFW_OBJECT_DEFAULT_TESTS();
	CPPUNIT_TEST( testHostaddress );

	CPPUNIT_TEST_SUITE_END();

  public:
	virtual natfw_object *createInstance1() const {
		return new external_address();
	}

	virtual natfw_object *createInstance2() const {
		return new external_address(hostaddress("192.168.0.1"), 10000);
	}

	virtual void mutate1(natfw_object *o) const {
		external_address *ea = dynamic_cast<external_address *>(o);
		ea->set_address(hostaddress("127.0.0.1"));
		ea->set_port(10001);
	}

	virtual void testHostaddress() {
		hostaddress ha1 = hostaddress("192.168.0.1");
		external_address ea = external_address(ha1, 10000);

		hostaddress ha2 = ea.get_address();
		CPPUNIT_ASSERT( ha1 == ha2 ); // equal
		CPPUNIT_ASSERT( &ha1 != &ha2 ); // but not the same

		// make sure copies are used
		ha1.set_ip("127.0.0.1");
		CPPUNIT_ASSERT( ha1 != ea.get_address() );
	}
};

CPPUNIT_TEST_SUITE_REGISTRATION( ExternalAddressTest );

// EOF
