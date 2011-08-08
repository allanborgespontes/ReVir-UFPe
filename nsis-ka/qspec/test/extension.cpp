/*
 * extension.cpp - Extending the implementation
 *
 * $Id: extension.cpp 3236 2008-07-28 10:05:05Z bless $
 * $HeadURL: https://svn.tm.kit.edu/nsis/qspec/trunk/test/extension.cpp $
 *
 */
#include <cppunit/TestCase.h>
#include <cppunit/extensions/HelperMacros.h>

#include "qspec_ie.h"
#include "qspec_pdu.h"
#include "qspec_object.h"
#include "qspec_param.h"

using namespace qspec;


/**
 * A Mapping error has occured.
 */
class MappingError {

};

class my_qspec {

  public:
	static const uint16 QSPEC_TYPE = 12; // For private/experimental use

  my_qspec(float bandwidth) : qspec(ms_two_way_transactions, 0, QSPEC_TYPE) {
	  qspec_object *qos_desired = new qspec_object(ot_qos_desired);

	  qos_desired->set_parameter(new t_mod(bandwidth, 576.0, 200000.0, 40));
	  
	  qspec.set_object(qos_desired);
  }


  my_qspec(const qspec_pdu &pdu) : qspec(pdu) {

    // Check for errors.
    // There is much more to check in the real world.
    
    if ( pdu.get_qspec_type() != QSPEC_TYPE )
		  throw MappingError();
		

    qspec_object *obj = pdu.get_object(ot_qos_desired);
    
    if ( obj == NULL )
      throw MappingError();
    
    qspec_param *param = obj->get_parameter(t_mod::PARAM_ID1);
    
    if ( param == NULL )
      throw MappingError();
    
    qspec::t_mod *bw = dynamic_cast<qspec::t_mod *>(param);

    bandwidth = ( bw != NULL ) ? bw->get_rate() : -1;
  }
  
  const qspec_pdu &get_qspec_pdu() const {
    return qspec;
  }

  float get_bandwidth() const {
    return bandwidth;
  }

  private:
  qspec_pdu qspec;

  float bandwidth;
};

class ExtensionTest : public CppUnit::TestCase {

	CPPUNIT_TEST_SUITE( ExtensionTest );

	CPPUNIT_TEST( testExtension );

	CPPUNIT_TEST_SUITE_END();

  public:

	void testExtension() {
		
		my_qspec qspec_test(1000.0);

		const qspec_pdu &pdu = qspec_test.get_qspec_pdu();

		/*
		 * Serialize the qspec_pdu.
		 */
		NetMsg msg(1024);
		uint32 bytes_written;

		pdu.serialize(msg, IE::protocol_v1, bytes_written);

		msg.truncate(bytes_written); // IMPORTANT!

		/* 
		 * Deserialize the QSPEC.
		 */
		QSPEC_IEManager::clear();
		QSPEC_IEManager::instance()->register_ie(new qspec_pdu());
		QSPEC_IEManager::instance()->register_ie(new qspec_object());
		QSPEC_IEManager::instance()->register_ie(new t_mod());


		msg.set_pos(0);
		IEErrorList errlist;
		uint32 bytes_read;

		IE *ie = QSPEC_IEManager::instance()->deserialize(msg,
				cat_qspec_pdu, IE::protocol_v1, errlist,
				bytes_read, false);

		CPPUNIT_ASSERT( ie != NULL );
		CPPUNIT_ASSERT( errlist.is_empty() );

		qspec_pdu *pdu2 = dynamic_cast<qspec_pdu *>(ie);
		CPPUNIT_ASSERT( pdu2 != NULL );


		/*
		 * Now create a my_qspec object.
		 */
		my_qspec qspec_test2(*pdu2);

		CPPUNIT_ASSERT( qspec_test2.get_bandwidth() == 1000.0 );

		delete ie;
	}
};

CPPUNIT_TEST_SUITE_REGISTRATION( ExtensionTest );
