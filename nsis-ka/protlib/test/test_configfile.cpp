#include <cppunit/TestCase.h>
#include <cppunit/extensions/HelperMacros.h>

#include "configfile.h"
#include <fstream>

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
  cfgpar_unused,
  cfgpar_bool_a,
  cfgpar_bool_b,
  cfgpar_uint16_a,
  cfgpar_uint16_b,
  cfgpar_uint32_a,
  cfgpar_uint32_b,
  cfgpar_double_a,
  cfgpar_float_a,
  cfgpar_float_b,
  cfgpar_float_c,
  cfgpar_string_a,
  cfgpar_string_b,
  cfgpar_hostaddress_a,
  cfgpar_hostaddress_b,
  cfgpar_hostaddresslist_a,
  cfgpar_max
};

string errmsg;

class test_configparfile : public CppUnit::TestCase {

  CPPUNIT_TEST_SUITE( test_configparfile );

  CPPUNIT_TEST( test_configparfile_dump_and_read );

  CPPUNIT_TEST_SUITE_END();

  public:
  void test_configparfile_dump_and_read() 
  {
    configpar_repository cfgrepo(realm_max);
    
    // register three realms
    cfgrepo.registerRealm(realm_a,"realm a", cfgpar_max);
    cfgrepo.registerRealm(realm_b,"realm b", cfgpar_max);
    cfgrepo.registerRealm(realm_c,"realm c", cfgpar_max);

    // realm a
    cfgrepo.registerPar( new configpar<bool>(realm_a, cfgpar_bool_a, "boolpar_a", "test for bool configpar a", true, false) );
    cfgrepo.registerPar( new configpar<bool>(realm_a, cfgpar_bool_b, "boolpar_b", "test for bool configpar b", true, true) );
    cfgrepo.registerPar( new configpar<uint16>(realm_a, cfgpar_uint16_a, "uint16par_a", "test for uint16 configpar a", true, 0xabcd, "s") );
    cfgrepo.registerPar( new configpar<uint16>(realm_a, cfgpar_uint16_b, "uint16par_b", "test for uint16 configpar b", true, 0xbfff, "ms") );
    cfgrepo.registerPar( new configpar<uint32>(realm_a, cfgpar_uint32_a, "uint32par_a", "test for uint32 configpar a", true, 0x1234abcdUL, "ms") );
    cfgrepo.registerPar( new configpar<uint32>(realm_a, cfgpar_uint32_b, "uint32par_b", "test for uint32 configpar b", true, 0xabcdffffUL, "bit/s") );
    cfgrepo.registerPar( new configpar<double>(realm_a, cfgpar_double_a, "doublepar_a", "test for double configpar a", true, 3.141592654) );
    cfgrepo.registerPar( new configpar<float>(realm_a, cfgpar_float_b, "floatpar_b", "test for float configpar b", true, 1.5) );
    cfgrepo.registerPar( new configpar<string>(realm_a, cfgpar_string_a, "stringpar_a", "test for string configpar a", true, "stringtest a") );
    cfgrepo.registerPar( new configpar<string>(realm_a, cfgpar_string_b, "stringpar_b", "test for string configpar b", true, "stringtest b") );
    cfgrepo.registerPar( new configpar<hostaddress>(realm_a, cfgpar_hostaddress_a, "hostaddresspar_a", "test for hostaddress configpar a", true, "2001:db8::a") );
    cfgrepo.registerPar( new configpar<hostaddress>(realm_a, cfgpar_hostaddress_b, "hostaddresspar_b", "test for hostaddress configpar b", true, "2001:db8::b") );

    // realm b (same parameter ids but not every parameter is actually used...)
    cfgrepo.registerPar( new configpar<bool>(realm_b, cfgpar_bool_b, "boolpar_b", "test for bool configpar b", true, true) );
    cfgrepo.registerPar( new configpar<uint16>(realm_b, cfgpar_uint16_b, "uint16par_b", "test for uint16 configpar b", true, 0xbfff) );
    cfgrepo.registerPar( new configpar<uint32>(realm_b, cfgpar_uint32_b, "uint32par_b", "test for uint32 configpar b", true, 0xabcdffffUL) );
    cfgrepo.registerPar( new configpar<float>(realm_b, cfgpar_float_b, "floatpar_b", "test for float configpar b", true, 3.14159) );
    cfgrepo.registerPar( new configpar<double>(realm_b, cfgpar_double_a, "doublepar_a", "test for double configpar a", true, 3.141592654) );
    cfgrepo.registerPar( new configpar<string>(realm_b, cfgpar_string_b, "stringpar_b", "test for string configpar b", true, "stringtest b") );
    cfgrepo.registerPar( new configpar<hostaddress>(realm_b, cfgpar_hostaddress_b, "hostaddresspar_b", "test for hostaddress configpar b", true, "2001:db8::b") );
    hostaddresslist_t defaultaddresslist;
    cfgrepo.registerPar( new configpar< hostaddresslist_t >(realm_b, cfgpar_hostaddresslist_a, "hostaddresslist_a", "test for hostaddress list configpar a", true, defaultaddresslist) );
    stringstream addresslistinput;
    addresslistinput << "141.3.70.4 127.0.0.1 ::1 2001:db8::abcd 2001:db8::affe";
    cfgrepo.getConfigPar(realm_b, cfgpar_hostaddresslist_a)->readVal(addresslistinput);

    // realm c is empty

    configfile cfgfile(&cfgrepo);
    
    std::ofstream outfile("testconfigfile.dump");
    if (outfile)
    {
	    cfgfile.dump(outfile);
	    outfile.close();
    }

    configpar_repository read_cfgrepo(realm_max);
    
    // register three realms
    read_cfgrepo.registerRealm(realm_a,"realm a", cfgpar_max);
    read_cfgrepo.registerRealm(realm_b,"realm b", cfgpar_max);
    read_cfgrepo.registerRealm(realm_c,"realm c", cfgpar_max);

    // realm a
    read_cfgrepo.registerPar( new configpar<bool>(realm_a, cfgpar_bool_a, "boolpar_a", "test for bool configpar a", true, true) );
    read_cfgrepo.registerPar( new configpar<bool>(realm_a, cfgpar_bool_b, "boolpar_b", "test for bool configpar b", true, false) );
    read_cfgrepo.registerPar( new configpar<uint16>(realm_a, cfgpar_uint16_a, "uint16par_a", "test for uint16 configpar a", true, 0x1234) );
    read_cfgrepo.registerPar( new configpar<uint16>(realm_a, cfgpar_uint16_b, "uint16par_b", "test for uint16 configpar b", true, 0xcccc) );
    read_cfgrepo.registerPar( new configpar<uint32>(realm_a, cfgpar_uint32_a, "uint32par_a", "test for uint32 configpar a", true, 0x5678dcbaUL) );
    read_cfgrepo.registerPar( new configpar<uint32>(realm_a, cfgpar_uint32_b, "uint32par_b", "test for uint32 configpar b", true, 0xffffabcdUL) );
    read_cfgrepo.registerPar( new configpar<float>(realm_a, cfgpar_float_b, "floatpar_b", "test for float configpar b", true, 3.2) );
    read_cfgrepo.registerPar( new configpar<double>(realm_a, cfgpar_double_a, "doublepar_a", "test for double configpar a", true, 0.1234567) );
    read_cfgrepo.registerPar( new configpar<string>(realm_a, cfgpar_string_a, "stringpar_a", "test for string configpar a", true, "empty") );
    read_cfgrepo.registerPar( new configpar<string>(realm_a, cfgpar_string_b, "stringpar_b", "test for string configpar b", true, "empty") );
    read_cfgrepo.registerPar( new configpar<hostaddress>(realm_a, cfgpar_hostaddress_a, "hostaddresspar_a", "test for hostaddress configpar a", true, "::") );
    read_cfgrepo.registerPar( new configpar<hostaddress>(realm_a, cfgpar_hostaddress_b, "hostaddresspar_b", "test for hostaddress configpar b", true, "::") );

    // realm b (same parameter ids but not every parameter is actually used...)
    read_cfgrepo.registerPar( new configpar<bool>(realm_b, cfgpar_bool_b, "boolpar_b", "test for bool configpar b", true, false) );
    read_cfgrepo.registerPar( new configpar<uint16>(realm_b, cfgpar_uint16_b, "uint16par_b", "test for uint16 configpar b", true, 0xc00c) );
    read_cfgrepo.registerPar( new configpar<uint32>(realm_b, cfgpar_uint32_b, "uint32par_b", "test for uint32 configpar b", true, 0xffUL) );
    read_cfgrepo.registerPar( new configpar<float>(realm_b, cfgpar_float_b, "floatpar_b", "test for float configpar b", true, 5.5) );
    read_cfgrepo.registerPar( new configpar<double>(realm_b, cfgpar_double_a, "doublepar_a", "test for double configpar a", true, 9.87654321) );
    read_cfgrepo.registerPar( new configpar<string>(realm_b, cfgpar_string_b, "stringpar_b", "test for string configpar b", true, "empty") );
    read_cfgrepo.registerPar( new configpar<hostaddress>(realm_b, cfgpar_hostaddress_b, "hostaddresspar_b", "test for hostaddress configpar b", true, "::") );
    read_cfgrepo.registerPar( new configpar< hostaddresslist_t >(realm_b, cfgpar_hostaddresslist_a, "hostaddresslist_a", "test for hostaddress list configpar a", true, defaultaddresslist) );
  
    configfile read_cfgfile(&read_cfgrepo);

    // read in the config file
    CPPUNIT_ASSERT_NO_THROW(read_cfgfile.load("testconfigfile.dump"));
//    catch (configfile_error cfgerr) {
//	    errmsg="Read failed in line: " + cfgerr.get_line() + ' ' + cfgerr.get_msg();
//	    CPPUNIT_FAIL( errmsg );
    //   }

    // check for equality
    CPPUNIT_ASSERT_EQUAL( cfgrepo.getPar<bool>(realm_a, cfgpar_bool_a), read_cfgrepo.getPar<bool>(realm_a, cfgpar_bool_a) );
    CPPUNIT_ASSERT_EQUAL( cfgrepo.getPar<bool>(realm_a, cfgpar_bool_b), read_cfgrepo.getPar<bool>(realm_a, cfgpar_bool_b) );
    CPPUNIT_ASSERT_EQUAL( cfgrepo.getPar<uint16>(realm_a, cfgpar_uint16_a), read_cfgrepo.getPar<uint16>(realm_a, cfgpar_uint16_a) );
    CPPUNIT_ASSERT_EQUAL( cfgrepo.getPar<uint16>(realm_a, cfgpar_uint16_b), read_cfgrepo.getPar<uint16>(realm_a, cfgpar_uint16_b) );
    CPPUNIT_ASSERT_EQUAL( cfgrepo.getPar<uint32>(realm_a, cfgpar_uint32_a), read_cfgrepo.getPar<uint32>(realm_a, cfgpar_uint32_a) );
    CPPUNIT_ASSERT_EQUAL( cfgrepo.getPar<uint32>(realm_a, cfgpar_uint32_b), read_cfgrepo.getPar<uint32>(realm_a, cfgpar_uint32_b) );
    //CPPUNIT_ASSERT_EQUAL( cfgrepo.getPar<float>(realm_a, cfgpar_float_a), read_cfgrepo.getPar<float>(realm_a, cfgpar_float_a) );
    CPPUNIT_ASSERT_EQUAL( cfgrepo.getPar<float>(realm_a, cfgpar_float_b), read_cfgrepo.getPar<float>(realm_a, cfgpar_float_b) );
    //CPPUNIT_ASSERT_EQUAL( cfgrepo.getPar<float>(realm_a, cfgpar_float_c), read_cfgrepo.getPar<float>(realm_a, cfgpar_float_c) );
    CPPUNIT_ASSERT_EQUAL( cfgrepo.getPar<string>(realm_a, cfgpar_string_a), read_cfgrepo.getPar<string>(realm_a, cfgpar_string_a) );
    CPPUNIT_ASSERT_EQUAL( cfgrepo.getPar<string>(realm_a, cfgpar_string_b), read_cfgrepo.getPar<string>(realm_a, cfgpar_string_b) );
    CPPUNIT_ASSERT_EQUAL( cfgrepo.getPar<hostaddress>(realm_a, cfgpar_hostaddress_a), read_cfgrepo.getPar<hostaddress>(realm_a, cfgpar_hostaddress_a) );
    CPPUNIT_ASSERT_EQUAL( cfgrepo.getPar<hostaddress>(realm_a, cfgpar_hostaddress_b), read_cfgrepo.getPar<hostaddress>(realm_a, cfgpar_hostaddress_b) );
    //CPPUNIT_ASSERT_EQUAL( cfgrepo.getParRef< list<hostaddress> >(realm_a, cfgpar_hostaddresslist_a), read_cfgrepo.getParRef< list<hostaddress> >(realm_a, cfgpar_hostaddresslist_a) );

    CPPUNIT_ASSERT_EQUAL( cfgrepo.getPar<bool>(realm_b, cfgpar_bool_b), read_cfgrepo.getPar<bool>(realm_b, cfgpar_bool_b) );
    CPPUNIT_ASSERT_EQUAL( cfgrepo.getPar<uint16>(realm_b, cfgpar_uint16_b), read_cfgrepo.getPar<uint16>(realm_b, cfgpar_uint16_b) );
    CPPUNIT_ASSERT_EQUAL( cfgrepo.getPar<uint32>(realm_b, cfgpar_uint32_b), read_cfgrepo.getPar<uint32>(realm_b, cfgpar_uint32_b) );
    CPPUNIT_ASSERT_EQUAL( cfgrepo.getPar<float>(realm_b, cfgpar_float_b), read_cfgrepo.getPar<float>(realm_b, cfgpar_float_b) );
    CPPUNIT_ASSERT_EQUAL( cfgrepo.getPar<string>(realm_b, cfgpar_string_b), read_cfgrepo.getPar<string>(realm_b, cfgpar_string_b) );
    CPPUNIT_ASSERT_EQUAL( cfgrepo.getPar<hostaddress>(realm_b, cfgpar_hostaddress_b), read_cfgrepo.getPar<hostaddress>(realm_b, cfgpar_hostaddress_b) );
    CPPUNIT_ASSERT_EQUAL( cfgrepo.getParRef< hostaddresslist_t >(realm_b, cfgpar_hostaddresslist_a), read_cfgrepo.getParRef< list<hostaddress> >(realm_b, cfgpar_hostaddresslist_a) );
 
    std::ofstream cmpoutfile("testconfigfile.dump.2");
    if (cmpoutfile)
    {
	    read_cfgfile.dump(cmpoutfile);
	    cmpoutfile.close();
    }
  }

};


CPPUNIT_TEST_SUITE_REGISTRATION( test_configparfile );

