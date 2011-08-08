/*
 * ie_store.cpp - Test the ie_store class
 *
 * $Id: ie_store.cpp 3236 2008-07-28 10:05:05Z bless $
 * $HeadURL: https://svn.tm.kit.edu/nsis/qspec/trunk/test/ie_store.cpp $
 *
 */
#include <cppunit/TestCase.h>
#include <cppunit/extensions/HelperMacros.h>

#include "ie.h"
#include "ie_store.h"
#include "qspec_param.h"

using namespace qspec;


class IeStoreTest : public CppUnit::TestCase {

	CPPUNIT_TEST_SUITE( IeStoreTest );

	CPPUNIT_TEST( testSetGet );

	CPPUNIT_TEST_SUITE_END();

  public:
	void testSetGet() {
		ie_store store;
		CPPUNIT_ASSERT( store.size() == 0 );

		store.get(1); // get() has no side-effects
		CPPUNIT_ASSERT( store.size() == 0 );

		store.set(7, NULL);
		CPPUNIT_ASSERT( store.size() == 0 );
		
		uint8 dscp= 42;
		phb_class *h = new phb_class(dscp);
		store.set(9, h);
		CPPUNIT_ASSERT( store.size() == 1 );
		CPPUNIT_ASSERT( store.get(9) == h );

		// try to remove non-existing entry
		CPPUNIT_ASSERT( store.remove(5) == NULL );
		CPPUNIT_ASSERT( store.size() == 1 );

		IE *ie = store.remove(9);
		CPPUNIT_ASSERT( ie == h );
		CPPUNIT_ASSERT( store.size() == 0 );
		CPPUNIT_ASSERT( store.remove(9) == NULL );
	}
};

CPPUNIT_TEST_SUITE_REGISTRATION( IeStoreTest );
