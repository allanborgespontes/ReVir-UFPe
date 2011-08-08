/*
 * Test the Data Terminal Information Object
 *
 * $Id: data_terminal_info.cpp 1733 2006-05-16 09:21:01Z stud-matfried $
 * $HeadURL: https://svn.tm.kit.edu/nsis/natfw-nslp/trunk/test/data_terminal_info.cpp $
 */
#include "generic_object_test.h"
#include "msg/data_terminal_info.h"

using namespace natfw::msg;


class DataTerminalInfoTest : public GenericObjectTest {

	CPPUNIT_TEST_SUITE( DataTerminalInfoTest );

	NATFW_OBJECT_DEFAULT_TESTS();
	CPPUNIT_TEST( testFlags );
	CPPUNIT_TEST( testReadWriteEmpty );
	CPPUNIT_TEST( testReadWriteWithPorts );
	CPPUNIT_TEST( testReadWriteWithSpi );

	CPPUNIT_TEST_SUITE_END();

  public:
	virtual natfw_object *createInstance1() const {
		return new data_terminal_info();
	}

	virtual natfw_object *createInstance2() const {
		return new data_terminal_info(hostaddress("141.3.70.4"), 16);
	}

	virtual void mutate1(natfw_object *o) const {
		data_terminal_info *ti = dynamic_cast<data_terminal_info *>(o);
		ti->set_protocol(7);
	}

	void testFlags() {
		data_terminal_info *ti1 = new data_terminal_info();
		CPPUNIT_ASSERT( ti1->has_protocol() == false );
		CPPUNIT_ASSERT( ti1->has_port_numbers() == false );
		CPPUNIT_ASSERT( ti1->has_ipsec_spi() == false );

		ti1->set_protocol(7);
		CPPUNIT_ASSERT( ti1->has_protocol() == true );

		data_terminal_info *ti2 = new data_terminal_info();

		CPPUNIT_ASSERT( *ti1 != *ti2 );

		delete ti2;
		delete ti1;
	}

	void testReadWriteEmpty() {
		natfw_object *o1 = new data_terminal_info();
		natfw_object *o2 = new data_terminal_info();

		tryReadWrite(o1, o2);

		delete o2;
		delete o1;
	}

	void testReadWriteWithPorts() {
		data_terminal_info *o1 = new data_terminal_info(
			hostaddress("127.0.0.1"), 24);
		o1->set_protocol(7);
		o1->set_source_port(4747);
		o1->set_destination_port(25252);
		natfw_object *o2 = new data_terminal_info();

		tryReadWrite(o1, o2);

		delete o2;
		delete o1;
	}

	void testReadWriteWithSpi() {
		data_terminal_info *o1 = new data_terminal_info(
			hostaddress("127.0.0.1"), 24);
		o1->set_protocol(9);
		o1->set_ipsec_spi(4747);
		natfw_object *o2 = new data_terminal_info();

		tryReadWrite(o1, o2);

		delete o2;
		delete o1;
	}
};

CPPUNIT_TEST_SUITE_REGISTRATION( DataTerminalInfoTest );

// EOF
