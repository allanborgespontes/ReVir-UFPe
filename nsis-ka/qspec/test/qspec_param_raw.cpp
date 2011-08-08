/*
 * qspec_param_raw.cpp - Test the fallback parameter.
 *
 * $Id: qspec_param_raw.cpp 3236 2008-07-28 10:05:05Z bless $
 * $HeadURL: https://svn.tm.kit.edu/nsis/qspec/trunk/test/qspec_param_raw.cpp $
 *
 */
#include "generic_param_test.h"

#include "qspec_ie.h"

using namespace qspec;


class QSpecParamRawTest : public GenericParamTest {

	CPPUNIT_TEST_SUITE( QSpecParamRawTest );

	QSPEC_PARAM_DEFAULT_TESTS();

	CPPUNIT_TEST_SUITE_END();

  public:
	virtual qspec_param *createInstance1() const {
		return new qspec_param_raw();
	}

	virtual qspec_param *createInstance2() const {
		qspec_param *param = new qspec_param_raw();
		param->set_mandatory(false);

		return param;
	}

	virtual void mutate1(qspec_param *p) const {
		qspec_param_raw *raw = dynamic_cast<qspec_param_raw *>(p);
		raw->set_error(true);
	}

	// override parent's testSerialize() which doesn't apply here
	virtual void testSerialize() {

	        t_mod bw(2048000.0, 576.0, 100000.0, 40);
		uint32 expectedlen= bw.get_serialized_size(IE::protocol_v1);
		NetMsg msg(expectedlen);
		uint32 bytes_written;

		bw.serialize(msg, IE::protocol_v1, bytes_written);

		/*
		 * Deserialize.
		 */
		QSPEC_IEManager::clear();
		QSPEC_IEManager *mgr = QSPEC_IEManager::instance();
		mgr->register_ie(new qspec_param_raw());

		msg.set_pos(0);
		IEErrorList errlist;
		uint32 bytes_read;

		IE *ie1 = mgr->deserialize(msg, cat_qspec_param,
				IE::protocol_v1, errlist, bytes_read, false);

		qspec_param_raw *r1 = dynamic_cast<qspec_param_raw *>(ie1);

		CPPUNIT_ASSERT( r1 != NULL );
		CPPUNIT_ASSERT( bytes_read == expectedlen );

		NetMsg msg2(expectedlen);
		uint32 bytes_written2;

		ie1->serialize(msg2, IE::protocol_v1, bytes_written2);

		CPPUNIT_ASSERT( bytes_written2 == expectedlen );
		CPPUNIT_ASSERT( msg == msg2 );

		msg2.set_pos(0);
		IEErrorList errlist2;
		uint32 bytes_read2;

		IE *ie2 = mgr->deserialize(msg2, cat_qspec_param,
				IE::protocol_v1, errlist2, bytes_read2, false);

		CPPUNIT_ASSERT( ie2 != NULL );
	}
};

CPPUNIT_TEST_SUITE_REGISTRATION( QSpecParamRawTest );

// EOF
