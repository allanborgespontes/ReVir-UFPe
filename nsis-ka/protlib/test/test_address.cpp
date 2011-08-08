// ----------------------------------------*- mode: C++; -*--
// test_address.cpp - Test the address class.
// ----------------------------------------------------------
// $Id: test_address.cpp 6178 2011-05-20 08:29:15Z bless $
// $HeadURL: https://svn.tm.kit.edu/nsis/protlib/trunk/test/test_address.cpp $
// ==========================================================
//                      
// Copyright (C) 2008, all rights reserved by
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
#include <cppunit/TestCase.h>
#include <cppunit/extensions/HelperMacros.h>

#include "address.h"

using namespace protlib;
using namespace protlib::log;

class test_address : public CppUnit::TestCase {

  CPPUNIT_TEST_SUITE( test_address );

  CPPUNIT_TEST( test_constructors_hostaddress );
  CPPUNIT_TEST( test_copy_operator );
  CPPUNIT_TEST( test_setters );
  CPPUNIT_TEST( test_equals );
  CPPUNIT_TEST( test_bogus_sources );
  // Add more tests here.

  CPPUNIT_TEST( test_multicast_hostaddress );
  CPPUNIT_TEST_SUITE_END();

  public:
	void test_constructors_hostaddress() {
	  
	  // test default constructor
	  hostaddress t1, t2;

	  // default constructor should not let things uinitialized
	  // this will probably break in case pointers are used as members or uninitialized padding bytes
	  // CPPUNIT_ASSERT( memcmp(&t1,&t2,sizeof(t1)) == 0 ); // this thing fails actually, probably due to padding bytes?

	  // default constructor should not let things uinitialized
	  CPPUNIT_ASSERT(t1 == t2);

	  // should create the same thing as the default constructor
	  hostaddress* t3= t1.new_instance();
	  CPPUNIT_ASSERT( t1 == *t3 );

	  delete t3;

	  // undefined IPv6 addresses
	  hostaddress ipv6anysrc(in6addr_any) , ipv6anydst(in6addr_any);
	  // CPPUNIT_ASSERT( memcmp(&ipv6anysrc,&ipv6anydst,sizeof(ipv6anysrc)) == 0 ); // may fail due to padding
	  CPPUNIT_ASSERT( ipv6anysrc == ipv6anydst);

	  // test constructor from a string
	  hostaddress source("10.0.0.1");
	  hostaddress dest("10.0.0.1");
	  CPPUNIT_ASSERT(source.is_ipv4());

	  hostaddress destdifferent("10.0.0.2");
	  CPPUNIT_ASSERT( dest != destdifferent );
	  
	  // assignment operator
	  destdifferent= dest;
	  CPPUNIT_ASSERT( dest == destdifferent );

	  // test copy constructor ipv4
	  hostaddress copydest(dest);
	  CPPUNIT_ASSERT( dest == copydest);

	  // CPPUNIT_ASSERT( memcmp(&source,&dest,sizeof(source)) == 0 ); // may fail due to padding

	  CPPUNIT_ASSERT( dest == source );

	  hostaddress testv6src("fe80::20e:cff:fe22:57e3");
	  // this should be an ipv6 address
	  CPPUNIT_ASSERT(testv6src.is_ipv6());

	  // check whether test for unspecified v6address works
	  CPPUNIT_ASSERT( ipv6anysrc.is_ip_unspec() == true );
	  CPPUNIT_ASSERT( testv6src.is_ip_unspec() == false );

	  // check whether test for unspecified v4address works
	  hostaddress unspec_ipv4("0.0.0.0");
	  CPPUNIT_ASSERT( unspec_ipv4.is_ip_unspec() == true );
	  CPPUNIT_ASSERT( source.is_ip_unspec() == false );

	  // test copy constructor ipv6
	  hostaddress testv6eq(testv6src);
	  // CPPUNIT_ASSERT( memcmp(&testv6src,&testv6eq,sizeof(testv6src)) == 0 ); // may fail due to padding

	  CPPUNIT_ASSERT( testv6src == testv6eq);
	  
	}


	void test_copy_operator() {
	  // test copy() operator for ipv4
	  hostaddress source("10.0.0.1");
	  hostaddress* dest= source.copy();
	  
	  CPPUNIT_ASSERT( *dest == source );
	  delete dest;

	  // test for v6
	  hostaddress testv6src("fe80::20e:cff:fe22:57e3");
	  
	  hostaddress* testv6dstp= testv6src.copy();

	  CPPUNIT_ASSERT( *testv6dstp == testv6src );
	  
	  delete testv6dstp;
	}

	void test_setters() {
	  hostaddress source("10.0.0.1");
	  hostaddress dest(source);

	  /// set IPv4 from string
	  source.set_ipv4("10.0.0.2");
	  CPPUNIT_ASSERT( source != dest );

	  // test const char* setter
	  dest.set_ip("10.0.0.2");
	  CPPUNIT_ASSERT( source == dest );

	  // test const string& setter
	  const string teststring("fe80::20e:cff:fe22:57e3");
	  dest.set_ip(teststring);
	  CPPUNIT_ASSERT( dest.is_ipv6() );
	  CPPUNIT_ASSERT( dest.is_mapped_ip() == false );


	  // test const hostaddress& setter
	  source.set_ip(dest);
	  CPPUNIT_ASSERT( source.is_ipv6() );
	  CPPUNIT_ASSERT( source == dest );

	  struct in_addr myinaddr= { htonl(INADDR_LOOPBACK) };
	  // set IPv4 from inaddr
	  source.set_ip(myinaddr);
	  dest.set_ipv4("127.0.0.1");

	  CPPUNIT_ASSERT( source.is_ipv4() );
	  CPPUNIT_ASSERT( dest.is_ipv4() );

	  CPPUNIT_ASSERT( source == dest );

	  CPPUNIT_ASSERT( source.is_bogus_source() );

	  const struct in6_addr insixaddr=in6addr_loopback;
	  hostaddress v6src(insixaddr);

	  dest.set_ip(insixaddr);

	  CPPUNIT_ASSERT( dest.is_ipv6() );
	  CPPUNIT_ASSERT( dest == v6src); 

	  CPPUNIT_ASSERT( v6src.is_bogus_source() );
	  	  
	  hostaddress v4mapped("::ffff:141.3.70.4");

	  CPPUNIT_ASSERT( v4mapped.is_mapped_ip() == true );
	  CPPUNIT_ASSERT( v4mapped.is_ipv4() == false );
	  v4mapped.convert_to_ipv4();
	  CPPUNIT_ASSERT( v4mapped.is_ipv4() == true );

	}


	// Check if the == operator works.
	void test_equals() {
	  hostaddress sourceipv4("10.0.0.1");
	  hostaddress destipv4("10.0.0.2");
	  hostaddress same;
	  
	  CPPUNIT_ASSERT( sourceipv4.match_against(sourceipv4) == 32 );
	  CPPUNIT_ASSERT( sourceipv4.match_against(destipv4) == -1 );

	  CPPUNIT_ASSERT( sourceipv4 == sourceipv4 );
          
	  same= sourceipv4;
	  CPPUNIT_ASSERT( sourceipv4 == same );
	  CPPUNIT_ASSERT( sourceipv4 != destipv4 );
	  
	  same= destipv4;
	  CPPUNIT_ASSERT( destipv4 == same );

	  hostaddress sourceipv6("fe80::20e:cff:fe22:57e3");
	  hostaddress destipv6("fe80::20e:bad:c0:ffee");
	  same= sourceipv6;

	  CPPUNIT_ASSERT( sourceipv6.match_against(sourceipv6) == 128 );
	  CPPUNIT_ASSERT( sourceipv6.match_against(destipv6) == -1 );

	  CPPUNIT_ASSERT( sourceipv6 == same );
	  CPPUNIT_ASSERT( sourceipv6 != destipv6 );

	  CPPUNIT_ASSERT( sourceipv4 != sourceipv6 );
	}

	// test bogus source addresses
	void test_bogus_sources() {
	  struct in_addr myinaddr= { htonl(INADDR_LOOPBACK) };
	  // set IPv4 from inaddr
	  hostaddress source;

	  source.set_ip(myinaddr);
	  CPPUNIT_ASSERT( source.is_bogus_source() );

	  myinaddr.s_addr= htonl(INADDR_ANY);
	  source.set_ip(myinaddr);
	  CPPUNIT_ASSERT( source.is_bogus_source() );

	  myinaddr.s_addr= htonl(INADDR_BROADCAST);
	  source.set_ip(myinaddr);
	  CPPUNIT_ASSERT( source.is_bogus_source() );
	  
	  source.set_ip("224.0.0.1");
	  source.set_ip(myinaddr);
	  CPPUNIT_ASSERT( source.is_bogus_source() );

	  hostaddress v6src("::1");
	  CPPUNIT_ASSERT( v6src.is_bogus_source() );

	  v6src.set_ipv6("::");
	  CPPUNIT_ASSERT( v6src.is_bogus_source() );
	  
	}

	void test_multicast_hostaddress() {
		hostaddress ta; // testaddress

		// Test IPv4 addresses
		ta.set_ipv4("127.0.0.1");
		CPPUNIT_ASSERT( !ta.is_multicast() );

		ta.set_ipv4("224.0.0.0");
		CPPUNIT_ASSERT( ta.is_multicast() );

		// Test IPv6 addresses
		ta.set_ip("2001:638:204:1:230:48ff:fec4:813a");
		CPPUNIT_ASSERT( !ta.is_multicast() );
		ta.set_ip("FF01:0:0:0:0:0:0:1");
		CPPUNIT_ASSERT( ta.is_multicast() );
	}
};

CPPUNIT_TEST_SUITE_REGISTRATION( test_address );
