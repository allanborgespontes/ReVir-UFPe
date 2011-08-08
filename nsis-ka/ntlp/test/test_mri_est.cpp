// ----------------------------------------*- mode: C++; -*--
// mri_est.cpp - Test the mri_est class.
// ----------------------------------------------------------
// $Id:$
// $HeadURL: https://svn.ipv6.tm.uka.de/nsis/ntlp/branches/non-public/path-directed-mrm/test/mri_pc.cpp $
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

#include "mri_est.h"
#include "sstream"

using namespace ntlp;


class mri_est_test : public CppUnit::TestCase {

  CPPUNIT_TEST_SUITE( mri_est_test );

  CPPUNIT_TEST( test_constructors );
  CPPUNIT_TEST( test_setters );
  CPPUNIT_TEST( test_equals );
  CPPUNIT_TEST( test_serialization_and_parsing );
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

		hostaddress origin_source("192.168.64.1");
		hostaddress sig_dest("192.168.10.2");
		hostaddress ingress("192.168.1.64");

		mri_explicitsigtarget mri_est(mri1,origin_source,sig_dest,ingress);
		
		CPPUNIT_ASSERT ( mri_est.get_origin_sig_address() == origin_source );
		CPPUNIT_ASSERT ( mri_est.get_dest_sig_address() == sig_dest );
		CPPUNIT_ASSERT ( mri_est.get_ingress_address() == ingress );		
	}

	void test_setters() {
	  
		mri_explicitsigtarget mri_est;

		mri_pathcoupled mri1(hostaddress("2001:638:204:6::1"), 128,
				     hostaddress("2001:638:204:42::2"), 128, true);

		hostaddress origin_source("2001:638:204:1::55");
		hostaddress sig_dest("2001:638:204:a::dead:beef");
		hostaddress ingress("2001:638:204:b::f");
		
		mri_est.set_mri_pc(mri1);
		mri_est.set_origin_sig_address(origin_source);
		mri_est.set_dest_sig_address(sig_dest);
		mri_est.set_ingress_address(ingress);

		CPPUNIT_ASSERT( mri_est.get_origin_sig_address() ==  origin_source );
		CPPUNIT_ASSERT( mri_est.get_dest_sig_address() == sig_dest );
		CPPUNIT_ASSERT( mri_est.get_ingress_address() == ingress );
	}

	// Check if the == operator works. Required for test_basics() below.
	void test_equals() {
	  mri_pathcoupled mripc(
				hostaddress("10.0.0.1"), 32, 1234,
				hostaddress("10.0.0.2"), 32, 5678,
				"tcp", 0, 0, 0, true
				);
		hostaddress origin_source("192.168.64.1");
		hostaddress sig_dest("192.168.10.2");
		hostaddress ingress("192.168.1.64");

		mri_explicitsigtarget mriest1(mripc,origin_source,sig_dest,ingress);
		CPPUNIT_ASSERT( mriest1 == mriest1 );
		mri_explicitsigtarget mriest2(mripc,origin_source,sig_dest,ingress);
		CPPUNIT_ASSERT( mriest1 == mriest2 );
		
		sig_dest.set_ip("192.168.10.3");
		mriest2.set_dest_sig_address(sig_dest);

		CPPUNIT_ASSERT( mriest1 != mriest2 );

		// UDP != TCP
		mri_pathcoupled mripc2(
			hostaddress("10.0.0.1"), 32, 1234,
			hostaddress("10.0.0.2"), 32, 5678,
			"udp", 0, 0, 0, true
		);

		mri_explicitsigtarget mriest3(mripc2,origin_source,sig_dest,ingress);
		
		CPPUNIT_ASSERT( mriest2 != mriest3 );
		CPPUNIT_ASSERT( mriest1 != mriest3 );
	}



	void test_serialization_and_parsing() {
		hostaddress source("10.0.0.1");
		hostaddress dest("10.0.0.1");

		mri_pathcoupled mri1(source, 32, dest, 20, true);

		CPPUNIT_ASSERT( mri1.get_sourceaddress() == source );
		CPPUNIT_ASSERT( mri1.get_sourceprefix() == 32 );
		CPPUNIT_ASSERT( mri1.get_destaddress() == dest );
		CPPUNIT_ASSERT( mri1.get_destprefix() == 20 );

		hostaddress origin_source("192.168.64.1");
		hostaddress sig_dest("192.168.10.2");
		hostaddress ingress("192.168.1.64");

		mri_explicitsigtarget mri_est_src(mri1,origin_source,sig_dest,ingress);

		NetMsg bytebuffer(NetMsg::max_size);

		uint32 expected_size= mri_est_src.get_serialized_size(IE::protocol_v1);
		uint32 written_bytes= 0, lastpos= 0;

		mri_est_src.serialize(bytebuffer, IE::protocol_v1, written_bytes); 

		lastpos= bytebuffer.get_pos();
		bytebuffer.truncate();
		ostringstream outstr;
                outstr << "written:" << written_bytes << " expected:" << expected_size << " get_pos(): " << lastpos <<  " get_size():" << bytebuffer.get_size();
		CPPUNIT_ASSERT( written_bytes == expected_size );
		CPPUNIT_ASSERT( expected_size == bytebuffer.get_size() );


		mri_explicitsigtarget mri_est_dst;
		
		IEErrorList errorlist;
		uint32 read_bytes= 0;
		
		mri_explicitsigtarget* mriest_parse_result= mri_est_dst.deserialize(bytebuffer, IE::protocol_v1, errorlist, read_bytes, false);
		CPPUNIT_ASSERT( mriest_parse_result != NULL );
		CPPUNIT_ASSERT( mriest_parse_result == &mri_est_dst );
		// assert no errors occured
		CPPUNIT_ASSERT( errorlist.is_empty());
		CPPUNIT_ASSERT( read_bytes == expected_size );
		CPPUNIT_ASSERT( mri_est_dst == mri_est_src );
	}		
};

CPPUNIT_TEST_SUITE_REGISTRATION( mri_est_test );
