/*
 * qspec_param.cpp - Test the abstract qspec_param class.
 *
 * $Id: qspec_param.cpp 3244 2008-07-28 20:16:54Z bless $
 * $HeadURL: https://svn.tm.kit.edu/nsis/qspec/trunk/test/qspec_param.cpp $
 *
 */
#include <cppunit/TestCase.h>
#include <cppunit/extensions/HelperMacros.h>

#include "qspec_param.h"

using namespace qspec;


class QSpecParamTest : public CppUnit::TestCase {

	CPPUNIT_TEST_SUITE( QSpecParamTest );

	CPPUNIT_TEST( testInvalidLength );

	CPPUNIT_TEST_SUITE_END();

  public:
	void testInvalidLength();
};

CPPUNIT_TEST_SUITE_REGISTRATION( QSpecParamTest );


void QSpecParamTest::testInvalidLength() {
	/*
	 * Test for empty NetMsg;
	 */
	NetMsg msg1(1);
	IEErrorList errlist1;
	uint32 bytes_read1;

	qspec_param *p1 = new phb_class((uint8) 42);

	IE *ie1 = p1->deserialize(msg1, IE::protocol_v1, errlist1,
			bytes_read1, false);

	CPPUNIT_ASSERT( ie1 == NULL );
	CPPUNIT_ASSERT( errlist1.is_empty() == false );

	IEError *err1 = errlist1.get();
	CPPUNIT_ASSERT( dynamic_cast<IEMsgTooShort *>(err1) != NULL );
	delete err1;

	CPPUNIT_ASSERT( errlist1.is_empty() == true );

	delete p1;


	/*
	 * Test for invalid length.
	 */
	NetMsg msg2(100);
	msg2.encode32(0x80010002);	// length == 2
	msg2.set_pos(0);

	IEErrorList errlist2;
	uint32 bytes_read2;

	qspec_param *p2 = new phb_class((uint8) 42);

	IE *ie2 = p2->deserialize(msg2, IE::protocol_v1, errlist2,
			bytes_read2, false);

	CPPUNIT_ASSERT( ie2 == NULL );
	CPPUNIT_ASSERT( errlist2.is_empty() == false );

	IEError *err2 = errlist2.get();
	CPPUNIT_ASSERT( dynamic_cast<IEWrongLength *>(err2) != NULL );
	delete err2;

	CPPUNIT_ASSERT( errlist2.is_empty() == true );

	/* TODO: skipping is still unimplemented!
	// Once again, but with skip == true.
	msg2.set_pos(0);
	IE *ie3 = p2->deserialize(msg2, IE::protocol_v1, errlist2,
			bytes_read2, true);

	CPPUNIT_ASSERT( ie3 != NULL );
	CPPUNIT_ASSERT( errlist2.is_empty() == false );

	delete p2;
	*/
}

// EOF
