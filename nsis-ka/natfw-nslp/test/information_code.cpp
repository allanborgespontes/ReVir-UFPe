/*
 * Test the Information Code Object
 *
 * $Id: information_code.cpp 1743 2006-05-22 13:37:47Z stud-matfried $
 * $HeadURL: https://svn.tm.kit.edu/nsis/natfw-nslp/trunk/test/information_code.cpp $
 */
#include "generic_object_test.h"
#include "msg/information_code.h"
#include "msg/natfw_msg.h"

using namespace natfw::msg;


class InformationCodeTest : public GenericObjectTest {

	CPPUNIT_TEST_SUITE( InformationCodeTest );

	NATFW_OBJECT_DEFAULT_TESTS();
	CPPUNIT_TEST( testBasics );

	CPPUNIT_TEST_SUITE_END();

  public:
	virtual natfw_object *createInstance1() const {
		return new information_code();
	}

	virtual natfw_object *createInstance2() const {
		return new information_code(information_code::sc_success);
	}

	virtual void mutate1(natfw_object *o) const {
		information_code *ic = dynamic_cast<information_code *>(o);
		ic->set_severity_class(information_code::sc_success);
	}

	void testBasics() {
		information_code ic(
			information_code::sc_success,
			information_code::suc_successfully_processed,
			natfw_create::MSG_TYPE);

		CPPUNIT_ASSERT( ic.get_response_object_type()
					== natfw_create::MSG_TYPE );
	}
};

CPPUNIT_TEST_SUITE_REGISTRATION( InformationCodeTest );

// EOF
