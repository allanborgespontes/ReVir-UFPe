// ----------------------------------------*- mode: C++; -*--
// mri_pc.cpp - Test the mri_pc class.
// ----------------------------------------------------------
// $Id: mri_pc.cpp 2862 2008-02-15 23:25:27Z bless $
// $HeadURL: https://svn.tm.kit.edu/nsis/ntlp/trunk/test/mri_pc.cpp $
// ==========================================================
//                      
// Copyright (C) 2005-2007, all rights reserved by
// - Institute of Telematics, University of Karlsruhe (TH)
//
// More information and contact:
// https://projekte.tm.uka.de/trac/NSIS
//                      
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; version 2 of the License
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
//
// ==========================================================
#include "test_suite.h"

#include "mri_pc.h"

using namespace ntlp;


class mri_pc_test : public CppUnit::TestCase {

  CPPUNIT_TEST_SUITE( mri_pc_test );

  CPPUNIT_TEST( test_constructors );
  CPPUNIT_TEST( test_setters );
  CPPUNIT_TEST( test_equals );
  CPPUNIT_TEST( test_basics );
  CPPUNIT_TEST( test_direction );
  // Add more tests here.

  CPPUNIT_TEST_SUITE_END();

  public:
	void test_constructors() {
		hostaddress source("10.0.0.1");
		hostaddress dest("10.0.0.1");

		mri_pathcoupled mri1(source, 32, dest, 20, true);

		CPPUNIT_ASSERT( mri1.get_sourceaddress() == source );
		CPPUNIT_ASSERT( mri1.get_sourceprefix() == 32 );
		CPPUNIT_ASSERT( mri1.get_destaddress() == dest );
		CPPUNIT_ASSERT( mri1.get_destprefix() == 20 );
	}

	void test_setters() {
		mri_pathcoupled mri1(hostaddress("10.0.0.1"), 32,
			hostaddress("10.0.0.1"), 20, true);
		mri1.set_protocol("tcp");
		mri1.set_sourceport(1234);
		mri1.set_destport(5678);

		CPPUNIT_ASSERT( mri1.get_protocol() == 6 );
		CPPUNIT_ASSERT( mri1.get_sourceport() == 1234 );
		CPPUNIT_ASSERT( mri1.get_destport() == 5678 );

		mri1.set_ds_field(42);
		mri1.set_flowlabel(2);
		mri1.set_spi(0xdeadbeaf);

		CPPUNIT_ASSERT( mri1.get_ds_field() == 42 );
		CPPUNIT_ASSERT( mri1.get_flowlabel() == 2 );
		CPPUNIT_ASSERT( mri1.get_spi() == 0xdeadbeaf );
	}

	// Check if the == operator works. Required for test_basics() below.
	void test_equals() {
		mri_pathcoupled mri1;
		CPPUNIT_ASSERT( mri1 == mri1 );

		mri_pathcoupled mri2(
			hostaddress("10.0.0.1"), 32, 1234,
			hostaddress("10.0.0.2"), 32, 5678,
			"tcp", 0, 0, 0, true
		);
		CPPUNIT_ASSERT( mri2 == mri2 );
		CPPUNIT_ASSERT( mri1 != mri2 );

		// UDP != TCP
		mri_pathcoupled mri3(
			hostaddress("10.0.0.1"), 32, 1234,
			hostaddress("10.0.0.2"), 32, 5678,
			"udp", 0, 0, 0, true
		);
		CPPUNIT_ASSERT( mri3.get_protocol() != mri2.get_protocol() );
		CPPUNIT_ASSERT( mri1 != mri3 );
		CPPUNIT_ASSERT( mri2 != mri3 );
	}

	void test_basics() {
		mri_pathcoupled mri1;
		ASSERT_BASICS_WORK( mri1 ); // should this work?

		mri_pathcoupled mri2(
			hostaddress("10.0.0.1"), 32, 1234,
			hostaddress("10.0.0.2"), 32, 5678,
			"tcp", 0, 0, 0, true
		);
		ASSERT_BASICS_WORK( mri2 );

		// UDP != TCP
		mri_pathcoupled mri3(
			hostaddress("10.0.0.1"), 32, 1234,
			hostaddress("10.0.0.2"), 32, 5678,
			"udp", 0, 0, 0, true
		);
		ASSERT_BASICS_WORK( mri3 );

		// IPv6 addresses and a flow label
		mri_pathcoupled mri4(
			hostaddress("fe80::20e:cff:fe22:57e3"), 32, 1234,
			hostaddress("fe80::20e:cff:fe22:574d"), 32, 5678,
			"tcp", 0xfab, 0, 0, true
		);
		ASSERT_BASICS_WORK( mri4 );

	}

	void test_direction() {
		mri_pathcoupled mri1(
			hostaddress("10.0.0.1"), 32, 1234,	// source
			hostaddress("10.0.0.2"), 32, 5678,	// dest
			"tcp", 0, 0, 0, true			// downstream
		);

		appladdress *final_hop1 = mri1.determine_dest_address();
		CPPUNIT_ASSERT( final_hop1 != NULL );
		// Note: Comparison only works this way round because
		//       appladdress is a subclass of hostaddress!
		CPPUNIT_ASSERT( hostaddress("10.0.0.2") == *final_hop1 );

		mri1.invertDirection();
		appladdress *final_hop2 = mri1.determine_dest_address();
		CPPUNIT_ASSERT( final_hop2 != NULL );
		CPPUNIT_ASSERT( hostaddress("10.0.0.1") == *final_hop2 );

		delete final_hop1;
		delete final_hop2;
	}
};

CPPUNIT_TEST_SUITE_REGISTRATION( mri_pc_test );
