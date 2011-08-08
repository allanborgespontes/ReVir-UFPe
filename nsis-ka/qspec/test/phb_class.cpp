/*
 * phb_class.cpp - Test the QSPEC PHB Class Parameter
 *
 * $Id: phb_class.cpp 3236 2008-07-28 10:05:05Z bless $
 * $HeadURL: https://svn.tm.kit.edu/nsis/qspec/trunk/test/phb_class.cpp $
 *
 */
#include "generic_param_test.h"

using namespace qspec;


class PhbClassTest : public GenericParamTest {

	CPPUNIT_TEST_SUITE( PhbClassTest );

	QSPEC_PARAM_DEFAULT_TESTS();
	CPPUNIT_TEST( testDscpSetandGet );
	CPPUNIT_TEST( testDscpSet );
	CPPUNIT_TEST( testPHBSetandGet );
	CPPUNIT_TEST( testPHBSet );
	CPPUNIT_TEST( testInvalidDscp );

	CPPUNIT_TEST_SUITE_END();

  public:
	virtual qspec_param *createInstance1() const {
	  uint8 dscp_val= 17;
	  return new phb_class(dscp_val);
	}

	virtual qspec_param *createInstance2() const {
	  uint16 phb_id_val= 511;
	  return new phb_class(phb_id_val);
	}

	virtual void mutate1(qspec_param *p) const {
		phb_class *phb = dynamic_cast<phb_class *>(p);
		phb->set_phb_id_set(9);
	}


        virtual void testDscpSetandGet() {
	  uint8 dscp= 42;
	  phb_class p(dscp);

	  CPPUNIT_ASSERT( p.get_dscp_value() == dscp );
	  CPPUNIT_ASSERT( p.is_dscp_set() == false );
	  CPPUNIT_ASSERT( p.is_phb_id_set() == false );

	  dscp= 55;
	  p.set_dscp_value(dscp);
	  CPPUNIT_ASSERT( p.get_dscp_value() == dscp );
	  CPPUNIT_ASSERT( p.is_dscp_set() == false );
	  CPPUNIT_ASSERT( p.is_phb_id_set() == false );
	}

        virtual void testDscpSet() {
	  phb_class p;
	  uint8 dscp= 42;

	  p.set_dscp_set(dscp);

	  CPPUNIT_ASSERT( p.get_dscp_value() == dscp );
	  CPPUNIT_ASSERT( p.is_dscp_set() == true );
	  CPPUNIT_ASSERT( p.is_phb_id_set() == false );

	}


        virtual void testPHBSetandGet() {
	  uint16 phb_id= 96;
	  phb_class p(phb_id);

	  CPPUNIT_ASSERT( p.get_phb_id() == phb_id );
	  CPPUNIT_ASSERT( p.is_dscp_set() == false );
	  CPPUNIT_ASSERT( p.is_phb_id_set() == false );

	  phb_id= 122;
	  p.set_phb_id(phb_id);
	  CPPUNIT_ASSERT( p.get_phb_id() == phb_id );
	  CPPUNIT_ASSERT( p.is_dscp_set() == false );
	  CPPUNIT_ASSERT( p.is_phb_id_set() == false );
	}

        virtual void testPHBSet() {
	  uint16 phb_id= 122;
	  phb_class p(phb_id);

	  p.set_phb_id_set(phb_id);
	  CPPUNIT_ASSERT( p.get_phb_id() == phb_id );
	  CPPUNIT_ASSERT( p.is_dscp_set() == false );
	  CPPUNIT_ASSERT( p.is_phb_id_set() == true );

	}

	virtual void testInvalidDscp() {
		// Error: Only 6 bit for DSCP allowed.
	  uint8 invalid_dscp= 0xFF;
	  phb_class *p = new phb_class(invalid_dscp);
	  CPPUNIT_ASSERT( p->get_dscp_value() !=invalid_dscp );
	  delete p;
	}
};

CPPUNIT_TEST_SUITE_REGISTRATION( PhbClassTest );

// EOF
