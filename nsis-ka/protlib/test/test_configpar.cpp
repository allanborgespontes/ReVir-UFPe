// ----------------------------------------*- mode: C++; -*--
// test_configpar.cpp - Test the configpar class.
// ----------------------------------------------------------
// $Id: test_configpar.cpp 4107 2009-07-16 13:49:52Z bless $
// $HeadURL: https://svn.tm.kit.edu/nsis/protlib/trunk/test/test_configpar.cpp $
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

#include "configpar.h"
#include "configpar_repository.h"

#include <sstream>

using namespace protlib;
using namespace protlib::log;

enum realm_id_type {
  realm_a,
  realm_b,
  realm_c,
  realm_d,
  realm_max
};

enum configpar_id_t {
  cfgpar_bool_a,
  cfgpar_bool_b,
  cfgpar_uint16_a,
  cfgpar_uint16_b,
  cfgpar_uint32_a,
  cfgpar_uint32_b,
  cfgpar_float_a,
  cfgpar_float_b,
  cfgpar_float_c,
  cfgpar_string_a,
  cfgpar_string_b,
  cfgpar_hostaddress_a,
  cfgpar_hostaddress_b,
  cfgpar_max
};


class test_configpar : public CppUnit::TestCase {

  CPPUNIT_TEST_SUITE( test_configpar );

  CPPUNIT_TEST( test_configparbool );
  CPPUNIT_TEST( test_configparuint16 );
  CPPUNIT_TEST( test_configparuint32 );
  CPPUNIT_TEST( test_configparfloat );
  CPPUNIT_TEST( test_configparstring );
  CPPUNIT_TEST( test_configparhostaddress );
  CPPUNIT_TEST( test_configpar_repository );

  CPPUNIT_TEST_SUITE_END();

  public:

  void test_writeval_readval(const configparBase& parameter)
  {
    stringstream teststring;

    configparBase* newparameter= parameter.copy();    
    CPPUNIT_ASSERT(parameter == *newparameter);

    parameter.writeVal(teststring);
    newparameter->readVal(teststring);
    
    CPPUNIT_ASSERT(parameter == *newparameter);
    delete newparameter;
  }
  
  void test_configparbool() 
  {
    configpar<bool> a(realm_a, cfgpar_bool_a, "boolpar_a", "test for bool configpar a", true, false);
    configpar<bool> b(realm_b, cfgpar_bool_b, "boolpar_b", "test for bool configpar b", true, true);

    // is the assignment operator working?
    configpar<bool> c= a;
    CPPUNIT_ASSERT( a == c );

    CPPUNIT_ASSERT( a.getRealmId() == c.getRealmId() );
    CPPUNIT_ASSERT( a.getParId() == c.getParId() );
    CPPUNIT_ASSERT( a.getName() == c.getName() );
    CPPUNIT_ASSERT( a.getDescription() == c.getDescription() );

    c.setPar(true);

    // c has a different value now
    CPPUNIT_ASSERT( a != c );

    // these are different parameters of the same type
    CPPUNIT_ASSERT( a != b );

    b.setPar(false);

    // now it's the same value but still a different parameter
    CPPUNIT_ASSERT( a != b );
    CPPUNIT_ASSERT( a.getPar() == b.getPar() );
    CPPUNIT_ASSERT( (bool) a == (bool) b );    

    //CPPUNIT_ASSERT_THROW( );

  }

  void test_configparuint16()
  {
    configpar<uint16> a(realm_a, cfgpar_uint16_a, "uint16par_a", "test for uint16 configpar a", true, 0xabcd);
    configpar<uint16> b(realm_b, cfgpar_uint16_b, "uint16par_b", "test for uint16 configpar b", true, 0xbfff);

    // is the assignment operator working?
    configpar<uint16> c= a;
    CPPUNIT_ASSERT( a == c );

    CPPUNIT_ASSERT( a.getRealmId() == c.getRealmId() );
    CPPUNIT_ASSERT( a.getParId() == c.getParId() );
    CPPUNIT_ASSERT( a.getName() == c.getName() );
    CPPUNIT_ASSERT( a.getDescription() == c.getDescription() );

    c.setPar(0xffff);

    // c has a different value now
    CPPUNIT_ASSERT( a != c );

    // these are different parameters of the same type
    CPPUNIT_ASSERT( a != b );

    b.setPar(a.getPar());

    // now it's the same value but still a different parameter
    CPPUNIT_ASSERT( a != b );
    CPPUNIT_ASSERT( a.getPar() == b.getPar() );
    CPPUNIT_ASSERT( (uint16) a == (uint16) b );    
  }

  void test_configparuint32()
  {
    configpar<uint32> a(realm_a, cfgpar_uint32_a, "uint32par_a", "test for uint32 configpar a", true, 0x1234abcdUL);
    configpar<uint32> b(realm_b, cfgpar_uint32_b, "uint32par_b", "test for uint32 configpar b", true, 0xabcdffffUL);

    // is the assignment operator working?
    configpar<uint32> c= a;
    CPPUNIT_ASSERT( a == c );

    CPPUNIT_ASSERT( a.getRealmId() == c.getRealmId() );
    CPPUNIT_ASSERT( a.getParId() == c.getParId() );
    CPPUNIT_ASSERT( a.getName() == c.getName() );
    CPPUNIT_ASSERT( a.getDescription() == c.getDescription() );

    c.setPar(0xfffffff);

    // c has a different value now
    CPPUNIT_ASSERT( a != c );

    // these are different parameters of the same type
    CPPUNIT_ASSERT( a != b );

    b.setPar(a.getPar());

    // now it's the same value but still a different parameter
    CPPUNIT_ASSERT( a != b );
    CPPUNIT_ASSERT( a.getPar() == b.getPar() );
    CPPUNIT_ASSERT( (uint32) a == (uint32) b );    
  }

  void test_configparfloat() 
  {
    configpar<float> a(realm_a, cfgpar_float_a, "floatpar_a", "test for float configpar a", true, 0.5);
    configpar<float> b(realm_b, cfgpar_float_b, "floatpar_b", "test for float configpar b", true, 1.0);

    // is the assignment operator working?
    configpar<float> c= a;
    CPPUNIT_ASSERT( a == c );

    CPPUNIT_ASSERT( a.getRealmId() == c.getRealmId() );
    CPPUNIT_ASSERT( a.getParId() == c.getParId() );
    CPPUNIT_ASSERT( a.getName() == c.getName() );
    CPPUNIT_ASSERT( a.getDescription() == c.getDescription() );

    c.setPar(1e3);

    // c has a different value now
    CPPUNIT_ASSERT( a != c );

    // these are different parameters of the same type
    CPPUNIT_ASSERT( a != b );

    b.setPar(a.getPar());

    // now it's the same value but still a different parameter
    CPPUNIT_ASSERT( a != b );
    CPPUNIT_ASSERT( a.getPar() == b.getPar() );
    CPPUNIT_ASSERT( (float) a == (float) b );    

    //CPPUNIT_ASSERT_THROW( );

  }

  void test_configparstring() 
  {
    configpar<string> a(realm_a, cfgpar_string_a, "stringpar_a", "test for string configpar a", true, "stringtest a");
    configpar<string> b(realm_b, cfgpar_string_b, "stringpar_b", "test for string configpar b", true, "stringtest b");

    // is the assignment operator working?
    configpar<string> c= a;
    CPPUNIT_ASSERT( a == c );

    CPPUNIT_ASSERT( a.getRealmId() == c.getRealmId() );
    CPPUNIT_ASSERT( a.getParId() == c.getParId() );
    CPPUNIT_ASSERT( a.getName() == c.getName() );
    CPPUNIT_ASSERT( a.getDescription() == c.getDescription() );

    c.setPar("stringtest c");

    // c has a different value now
    CPPUNIT_ASSERT( a != c );

    // these are different parameters of the same type
    CPPUNIT_ASSERT( a != b );

    b.setPar(a.getPar());

    // now it's the same value but still a different parameter
    CPPUNIT_ASSERT( a != b );
    CPPUNIT_ASSERT( a.getPar() == b.getPar() );
    CPPUNIT_ASSERT( (string) a == (string) b );    

    //CPPUNIT_ASSERT_THROW( );

  }

  void test_configparhostaddress() 
  {
    configpar<hostaddress> a(realm_a, cfgpar_hostaddress_a, "hostaddresspar_a", "test for hostaddress configpar a", true, "2001:db8::a");
    configpar<hostaddress> b(realm_b, cfgpar_hostaddress_b, "hostaddresspar_b", "test for hostaddress configpar b", true, "2001:db8::b");

    // is the assignment operator working?
    configpar<hostaddress> c= a;
    CPPUNIT_ASSERT( a == c );

    CPPUNIT_ASSERT( a.getRealmId() == c.getRealmId() );
    CPPUNIT_ASSERT( a.getParId() == c.getParId() );
    CPPUNIT_ASSERT( a.getName() == c.getName() );
    CPPUNIT_ASSERT( a.getDescription() == c.getDescription() );

    c.setPar("2001:db8::c");

    // c has a different value now
    CPPUNIT_ASSERT( a != c );

    // these are different parameters of the same type
    CPPUNIT_ASSERT( a != b );

    b.setPar(a.getPar());

    // now it's the same value but still a different parameter
    CPPUNIT_ASSERT( a != b );
    CPPUNIT_ASSERT( a.getPar() == b.getPar() );
    CPPUNIT_ASSERT( (hostaddress) a == (hostaddress) b );    

    //CPPUNIT_ASSERT_THROW( );

  }


  void test_configpar_repository()
  {
    configpar_repository cfgrepo(realm_max);
    
    // register three realms
    cfgrepo.registerRealm(realm_a,"realm a", cfgpar_max);
    cfgrepo.registerRealm(realm_b,"realm b", cfgpar_max);
    cfgrepo.registerRealm(realm_c,"realm c", cfgpar_max);
    CPPUNIT_ASSERT_THROW( cfgrepo.registerRealm(realm_c,"invalid realm", 10), configParExceptionRealmAlreadyRegistered );

    CPPUNIT_ASSERT( cfgrepo.getRealmName(realm_a) == "realm a" );
    CPPUNIT_ASSERT( cfgrepo.getRealmName(realm_b) == "realm b" );
    CPPUNIT_ASSERT( cfgrepo.getRealmName(realm_c) == "realm c" );

    CPPUNIT_ASSERT_THROW( cfgrepo.getRealmName(realm_d), protlib::configParExceptionRealmNotRegistered );

    CPPUNIT_ASSERT_THROW( cfgrepo.getRealmName(realm_max), protlib::configParExceptionRealmInvalid );

    // parameter registration
    cfgrepo.registerPar( new configpar<bool>(realm_a, cfgpar_bool_a, "boolpar_a", "test for bool configpar a", true, false) );
    cfgrepo.registerPar( new configpar<bool>(realm_a, cfgpar_bool_b, "boolpar_b", "test for bool configpar b", true, true) );
    cfgrepo.registerPar( new configpar<uint16>(realm_a, cfgpar_uint16_a, "uint16par_a", "test for uint16 configpar a", true, 0xabcd) );
    cfgrepo.registerPar( new configpar<uint16>(realm_a, cfgpar_uint16_b, "uint16par_b", "test for uint16 configpar b", true, 0xbfff) );
    cfgrepo.registerPar( new configpar<uint32>(realm_a, cfgpar_uint32_a, "uint32par_a", "test for uint32 configpar a", true, 0x1234abcdUL) );
    cfgrepo.registerPar( new configpar<uint32>(realm_a, cfgpar_uint32_b, "uint32par_b", "test for uint32 configpar b", true, 0xabcdffffUL) );

    // this will cause a copy to be registered
    configpar<float> floatpar_a(realm_a, cfgpar_float_a, "floatpar_a", "test for float configpar a", true, 0.5);
    cfgrepo.registerPar( floatpar_a );
    CPPUNIT_ASSERT_THROW( cfgrepo.registerPar( floatpar_a ), configParExceptionParAlreadyRegistered );

    cfgrepo.registerPar( new configpar<float>(realm_a, cfgpar_float_b, "floatpar_b", "test for float configpar b", true, 1.0) );
    cfgrepo.registerPar( new configpar<string>(realm_a, cfgpar_string_a, "stringpar_a", "test for string configpar a", true, "stringtest a") );
    cfgrepo.registerPar( new configpar<string>(realm_a, cfgpar_string_b, "stringpar_b", "test for string configpar b", true, "stringtest b") );
    cfgrepo.registerPar( new configpar<hostaddress>(realm_a, cfgpar_hostaddress_a, "hostaddresspar_a", "test for hostaddress configpar a", true, "2001:db8::a") );
    cfgrepo.registerPar( new configpar<hostaddress>(realm_a, cfgpar_hostaddress_b, "hostaddresspar_b", "test for hostaddress configpar b", true, "2001:db8::b") );
    
    configpar<float> floatpar_invalid(realm_a, cfgpar_max, "floatpar_invalid", "test for invalid float configpar", true, 0.9999);
    CPPUNIT_ASSERT_THROW( cfgrepo.registerPar( floatpar_invalid ), configParExceptionParIDInvalid );
    
    CPPUNIT_ASSERT_THROW( cfgrepo.getPar<float>( realm_a, cfgpar_float_c), configParExceptionParNotRegistered );
    CPPUNIT_ASSERT_THROW( cfgrepo.getPar<bool>( realm_a, cfgpar_max), configParExceptionParIDInvalid );

    // test read from the repository
    CPPUNIT_ASSERT( cfgrepo.getPar<bool>(realm_a, cfgpar_bool_a) == false );
    CPPUNIT_ASSERT( cfgrepo.getPar<bool>(realm_a, cfgpar_bool_b) == true );
    CPPUNIT_ASSERT( cfgrepo.getPar<uint16>(realm_a, cfgpar_uint16_a) == 0xabcd );
    CPPUNIT_ASSERT( cfgrepo.getPar<uint16>(realm_a, cfgpar_uint16_b) == 0xbfff );
    CPPUNIT_ASSERT( cfgrepo.getPar<uint32>(realm_a, cfgpar_uint32_a) == 0x1234abcdUL );
    CPPUNIT_ASSERT( cfgrepo.getPar<uint32>(realm_a, cfgpar_uint32_b) == 0xabcdffffUL );
    CPPUNIT_ASSERT( cfgrepo.getPar<float>(realm_a, cfgpar_float_a) == 0.5 );
    CPPUNIT_ASSERT( cfgrepo.getPar<float>(realm_a, cfgpar_float_b) == 1.0 );
    CPPUNIT_ASSERT( cfgrepo.getPar<string>(realm_a, cfgpar_string_a) == "stringtest a" );
    CPPUNIT_ASSERT( cfgrepo.getPar<string>(realm_a, cfgpar_string_b) == "stringtest b" );

    CPPUNIT_ASSERT_THROW( cfgrepo.getPar<string>(realm_a, cfgpar_float_b), configParExceptionTypeMismatch );

    // test set from the repository
    cfgrepo.setPar<bool>(realm_a, cfgpar_bool_a, true);
    CPPUNIT_ASSERT( cfgrepo.getPar<bool>(realm_a, cfgpar_bool_a) == true  );

    cfgrepo.setPar<bool>(realm_a, cfgpar_bool_b, false);
    CPPUNIT_ASSERT( cfgrepo.getPar<bool>(realm_a, cfgpar_bool_b) == false );

    cfgrepo.setPar<uint16>(realm_a, cfgpar_uint16_a, 0xaffe);
    CPPUNIT_ASSERT( cfgrepo.getPar<uint16>(realm_a, cfgpar_uint16_a) == 0xaffe );

    cfgrepo.setPar<uint16>(realm_a, cfgpar_uint16_b, 0xbeeb);
    CPPUNIT_ASSERT( cfgrepo.getPar<uint16>(realm_a, cfgpar_uint16_b) == 0xbeeb );

    cfgrepo.setPar<uint32>(realm_a, cfgpar_uint32_a, 0xdeadbeef);
    CPPUNIT_ASSERT( cfgrepo.getPar<uint32>(realm_a, cfgpar_uint32_a) == 0xdeadbeef );

    cfgrepo.setPar<uint32>(realm_a, cfgpar_uint32_b, 0xfedcabcd);
    CPPUNIT_ASSERT( cfgrepo.getPar<uint32>(realm_a, cfgpar_uint32_b) == 0xfedcabcdUL );

    cfgrepo.setPar<float>(realm_a, cfgpar_float_a, 3.141592654);
    CPPUNIT_ASSERT( cfgrepo.getPar<float>(realm_a, cfgpar_float_a) == (float) 3.141592654 );

    cfgrepo.setPar<float>(realm_a, cfgpar_float_b, 0.11);
    CPPUNIT_ASSERT( cfgrepo.getPar<float>(realm_a, cfgpar_float_b) == (float) 0.11 );

    cfgrepo.setPar<string>(realm_a, cfgpar_string_a, "my-test-string a");
    CPPUNIT_ASSERT( cfgrepo.getPar<string>(realm_a, cfgpar_string_a) == "my-test-string a" );

    cfgrepo.setPar<string>(realm_a, cfgpar_string_b, "my-weird-test-string b");
    CPPUNIT_ASSERT( cfgrepo.getPar<string>(realm_a, cfgpar_string_b) == "my-weird-test-string b" );

    CPPUNIT_ASSERT_THROW( cfgrepo.setPar<string>(realm_a, cfgpar_float_b,"error"), configParExceptionTypeMismatch );
    CPPUNIT_ASSERT_THROW( cfgrepo.setPar<float>(realm_a, cfgpar_float_c,1.0), configParExceptionParNotRegistered );
    CPPUNIT_ASSERT_THROW( cfgrepo.setPar<bool>( realm_a, cfgpar_max, true), configParExceptionParIDInvalid );
    
    test_writeval_readval( *cfgrepo.getConfigPar(realm_a, cfgpar_bool_a) );
    test_writeval_readval( *cfgrepo.getConfigPar(realm_a, cfgpar_uint16_a) );
    test_writeval_readval( *cfgrepo.getConfigPar(realm_a, cfgpar_uint32_b) );
    test_writeval_readval( *cfgrepo.getConfigPar(realm_a, cfgpar_float_a) );
    test_writeval_readval( *cfgrepo.getConfigPar(realm_a, cfgpar_string_a) );
    test_writeval_readval( *cfgrepo.getConfigPar(realm_a, cfgpar_string_a) );
    test_writeval_readval( *cfgrepo.getConfigPar(realm_a, cfgpar_hostaddress_a) );
  } 
};



CPPUNIT_TEST_SUITE_REGISTRATION( test_configpar );
