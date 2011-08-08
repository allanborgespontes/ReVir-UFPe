// ----------------------------------------*- mode: C++; -*--
// test_nattraversal.cpp - Test the NAT traversal object class.
// ----------------------------------------------------------
// $Id:$
// $HeadURL: https://svn.ipv6.tm.uka.de/nsis/ntlp/branches/non-public/path-directed-mrm/test/mri_pc.cpp $
// ==========================================================
//                      
// Copyright (C) 2005-2009, all rights reserved by
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

#include "nattraversal.h"
#include "mri_pc.h"
#include "nli.h"

#include "sstream"

using namespace ntlp;



class nattraversal_test : public CppUnit::TestCase {

  CPPUNIT_TEST_SUITE( nattraversal_test );

  CPPUNIT_TEST( test_constructors );
  //CPPUNIT_TEST( test_equals );
  CPPUNIT_TEST( test_serialization_and_parsing );
  // Add more tests here.

  CPPUNIT_TEST_SUITE_END();


  public:
	void test_constructors() {
	  // PI
	  peer_identity* pi1=new peer_identity();
	  peer_identity* pi2=new peer_identity();
	  netaddress loc_if4("10.0.0.1");
	  
	  nli localnliv4(200, 30000, pi1, loc_if4);

          nattraversal* nto = NULL;
	  vector <natinfo> nat_information;
	  vector <uint16> translated_object_types;

	  hostaddress source("10.0.0.2");
	  hostaddress dest("10.0.0.3");
	  
	  mri_pathcoupled mri1(source, 32, dest, 20, true);
	  
	  nto = new nattraversal(&mri1,nat_information,translated_object_types);

	  nto->add_translation(known_ntlp_object::NLI);
	  nto->add_translation(known_ntlp_object::StackConfiguration);

          natinfo info;
	  string opaque_natinfo="Hello World!"; // 12 bytes
	  // only ref
	  NetMsg opaque_natinfo_buf(opaque_natinfo.length());
	  opaque_natinfo_buf.copy_from(reinterpret_cast<const uchar*>(opaque_natinfo.c_str()), opaque_natinfo.length());

          info.set(opaque_natinfo_buf);

	  // this push will actually copy the NetMsg buffer
          nto->push_natinfo(info);

	  ASSERT_BASICS_WORK(*nto);

	  // test copy constructor
	  nattraversal nto_copy(*nto);
	  
	  // should be a perfect copy now
	  CPPUNIT_ASSERT(*nto == nto_copy);

	  // modify copy now
	  nto_copy.add_translation(known_ntlp_object::StackConfiguration);
          nto_copy.push_natinfo(info);

	  // must not be equal
	  CPPUNIT_ASSERT(*nto != nto_copy);

	  // test assignment operator
	  *nto= nto_copy;

	  // should be equal again now
	  CPPUNIT_ASSERT(*nto == nto_copy);

	  delete nto;

	  delete pi1;
	  delete pi2;
	}



	void test_serialization_and_parsing() {
	  // PI
	  peer_identity* pi1=new peer_identity();
	  peer_identity* pi2=new peer_identity();
	  netaddress loc_if4("10.0.0.1");
	  
	  nli localnliv4(200, 30000, pi1, loc_if4);
	  
	  NTLP_IE *tmp;
	  tmp = new nattraversal;
	  NTLP_IEManager::instance()->register_ie(tmp);
	  tmp = new nli;
	  NTLP_IEManager::instance()->register_ie(tmp);


	  nattraversal* nto = NULL;
	  vector <natinfo> nat_information;
	  vector <uint16> translated_object_types;
		
	  hostaddress source("10.0.0.2");
	  hostaddress dest("10.0.0.3");
		
	  mri_pathcoupled mri1(source, 32, dest, 20, true);
		
	  nto = new nattraversal(&mri1,nat_information,translated_object_types);
		
	  CPPUNIT_ASSERT( nto != NULL);
	  nto->add_translation(known_ntlp_object::NLI);
	  nto->add_translation(known_ntlp_object::StackConfiguration);
		
	  natinfo info;
	  string opaque_natinfo="Hello World!"; // 12 bytes will require padding of 2 bytes
	  // only ref
	  NetMsg opaque_natinfo_buf(opaque_natinfo.length());
	  opaque_natinfo_buf.copy_from(reinterpret_cast<const uchar*>(opaque_natinfo.c_str()), opaque_natinfo.length());
          info.set(opaque_natinfo_buf);
	  // append a copy of the natinfo object to the list
	  nto->push_natinfo(info);

	  string opaque_natinfo2="Second Hello World!!"; // 21 bytes, will require padding of 1 byte
	  NetMsg opaque_natinfo_buf2(opaque_natinfo2.length());
	  opaque_natinfo_buf2.copy_from(reinterpret_cast<const uchar*>(opaque_natinfo2.c_str()), opaque_natinfo2.length());
          info.set(opaque_natinfo_buf2);
	  // append a copy of the natinfo object to the list
	  nto->push_natinfo(info);

	  NetMsg bytebuffer(NetMsg::max_size);

	  uint32 expected_size= nto->get_serialized_size(IE::protocol_v1);
	  uint32 written_bytes= 0, lastpos= 0;

	  nto->serialize(bytebuffer, IE::protocol_v1, written_bytes); 

	  lastpos= bytebuffer.get_pos();
	  bytebuffer.truncate();
	  ostringstream outstr;
	  outstr << "written:" << written_bytes << " expected:" << expected_size << " get_pos(): " << lastpos <<  " get_size():" << bytebuffer.get_size();
	  CPPUNIT_ASSERT( written_bytes == expected_size );
	  CPPUNIT_ASSERT( expected_size == bytebuffer.get_size() );


	  IEErrorList errorlist;
	  uint32 read_bytes= 0;
		
	  nattraversal nto_parsed;
	  nattraversal* nto_parse_result= nto_parsed.deserialize(bytebuffer, IE::protocol_v1, errorlist, read_bytes, false);
	  CPPUNIT_ASSERT( nto_parse_result != NULL );
	  CPPUNIT_ASSERT( nto_parse_result == &nto_parsed );

	  // assert no errors occured
	  CPPUNIT_ASSERT( errorlist.is_empty());
	  CPPUNIT_ASSERT( read_bytes == expected_size );

	  // serializing the parsed object should yield the same byte stream
	  NetMsg cmpbytebuffer(expected_size);
	  written_bytes= 0;
	  nto_parse_result->serialize(cmpbytebuffer, IE::protocol_v1, written_bytes);

	  CPPUNIT_ASSERT(written_bytes == read_bytes);
	  CPPUNIT_ASSERT(memcmp(cmpbytebuffer.get_buffer(), bytebuffer.get_buffer(), written_bytes) == 0);

	  CPPUNIT_ASSERT( *nto == nto_parsed );

	  delete nto;
	  delete pi1;
	  delete pi2;
	}		
};

CPPUNIT_TEST_SUITE_REGISTRATION( nattraversal_test );
