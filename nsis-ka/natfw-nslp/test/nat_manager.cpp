/*
 * Test the nat_manager class.
 *
 * $Id: nat_manager.cpp 2033 2006-08-07 07:42:37Z stud-matfried $
 * $HeadURL: https://svn.tm.kit.edu/nsis/natfw-nslp/trunk/test/nat_manager.cpp $
 */
#include <cppunit/TestCase.h>
#include <cppunit/extensions/HelperMacros.h>

#include "nat_manager.h"
#include "dispatcher.h"

#include "utils.h"


using namespace natfw;


class NatManagerTest : public CppUnit::TestCase {

	CPPUNIT_TEST_SUITE( NatManagerTest );

	CPPUNIT_TEST( testReserveRelease );

	CPPUNIT_TEST_SUITE_END();

  public:
	void testReserveRelease();
};

CPPUNIT_TEST_SUITE_REGISTRATION( NatManagerTest );

void NatManagerTest::testReserveRelease() {
	using protlib::hostaddress;
	using protlib::appladdress;

	mock_natfw_config conf;
	nat_manager mgr = nat_manager(hostaddress("141.3.70.4"), 10000, 10000);

	appladdress nr_addr("10.38.2.222", 6, 1234);

	appladdress ext_addr = mgr.reserve_external_address(nr_addr);

	CPPUNIT_ASSERT( ext_addr == appladdress("141.3.70.4", 6, 10000) );

	CPPUNIT_ASSERT_THROW(
		mgr.reserve_external_address(nr_addr), nat_manager_error);

	mgr.release_external_address(ext_addr);
	CPPUNIT_ASSERT_NO_THROW( mgr.reserve_external_address(nr_addr) );
}

// EOF
