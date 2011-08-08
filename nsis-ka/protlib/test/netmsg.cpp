/*
 * netmsg.cpp - Check network message (netmsg)
 *
 * Still to check for: IPv4/IPv6 en-/decoding, exception
 *
 * $Id: netmsg.cpp 5534 2010-07-22 14:20:19Z bless $
 * $HeadURL: https://svn.tm.kit.edu/nsis/protlib/trunk/test/netmsg.cpp $
 *
 */
#include <cppunit/TestCase.h>
#include <cppunit/extensions/HelperMacros.h>

#include "network_message.h"
#include "logfile.h"

using namespace protlib;
using namespace protlib::log;

const unsigned int testsize= 255;

class NetMsgTest : public CppUnit::TestCase {

	CPPUNIT_TEST_SUITE( NetMsgTest );

	CPPUNIT_TEST( testNetMsgConstructor );
	CPPUNIT_TEST( testNetMsgConstructor2 );
        CPPUNIT_TEST( testNetMsgCompare );
        CPPUNIT_TEST( testNetMsgPos );
        CPPUNIT_TEST( testNetMsgCoding );
        CPPUNIT_TEST( testNetMsgWeirdCoding );

	CPPUNIT_TEST_SUITE_END();
private:
  char buffer[testsize];
  NetMsg* newmsgp;

  public:
	void setUp() {
	  // Executed before each of the test methods.
	  newmsgp= new NetMsg(testsize);
	}
		
	void testNetMsgConstructor() {
	  CPPUNIT_ASSERT( newmsgp->get_size() == testsize );
	  CPPUNIT_ASSERT( newmsgp->get_bytes_left() == testsize );
	  CPPUNIT_ASSERT( newmsgp->get_pos() == 0 );
	}

	void testNetMsgConstructor2() {
	  strcpy(buffer,"Hello World!");

	  newmsgp->copy_from(reinterpret_cast<uchar*>(buffer),strlen(buffer)+1);

	  NetMsg newmsg2(reinterpret_cast<uchar*>(buffer), strlen(buffer)+1, true);

	  CPPUNIT_ASSERT( memcmp(newmsgp->get_buffer(),newmsg2.get_buffer(),newmsg2.get_size()) == 0 );

	}

        
	void testNetMsgCompare() {
	  NetMsg newmsgempty(testsize);
	  CPPUNIT_ASSERT( *newmsgp == newmsgempty );

	  strcpy(buffer,"Hello World!");
	  newmsgp->copy_from(reinterpret_cast<uchar*>(buffer),strlen(buffer)+1);
	  newmsgp->set_pos(strlen(buffer));
	  NetMsg newmsg2(reinterpret_cast<uchar*>(buffer), strlen(buffer)+1, true);
	  newmsg2.set_pos(strlen(buffer));
	  newmsgp->truncate();
	  newmsg2.truncate();

	  CPPUNIT_ASSERT( *newmsgp == newmsg2 );

	  uchar* bptr= newmsg2.get_buffer();
          // cout << "strlen(buffer)=" << strlen(buffer) << " bptr[strlen(buffer)/2]:" << hex << (int) bptr[strlen(buffer)/2] 
	  //      << " " << (int) (bptr[strlen(buffer)/2] ^ 0x01) << dec << endl;

	  bptr[strlen(buffer)/2]= bptr[strlen(buffer)/2] ^ 0x01;
	  CPPUNIT_ASSERT( (*newmsgp == newmsg2) == false );
	}

        void testNetMsgPos() {
	  
	  uint32 testpos= testsize/2;

	  newmsgp->set_pos( testpos );
	  CPPUNIT_ASSERT( newmsgp->get_pos() == testpos );

	  newmsgp->set_pos_r( -testpos );
	  CPPUNIT_ASSERT( newmsgp->get_pos() == 0 );

	  newmsgp->set_pos_r( testpos );
	  CPPUNIT_ASSERT( newmsgp->get_pos() == testpos );

	  newmsgp->to_start();
	  CPPUNIT_ASSERT( newmsgp->get_pos() == 0 );

	}

        void testNetMsgCoding() {

	  newmsgp->to_start();

	  newmsgp->encode8(42);
	  CPPUNIT_ASSERT( newmsgp->get_pos() == 1 );

	  newmsgp->encode16(2);
	  CPPUNIT_ASSERT( newmsgp->get_pos() == 3 );

	  newmsgp->encode32(3UL);
	  CPPUNIT_ASSERT( newmsgp->get_pos() == 7 );

	  CPPUNIT_ASSERT( sizeof(uint32) == 4 );
	  CPPUNIT_ASSERT( sizeof(uint64) == 8 );

	  newmsgp->encode64(4UL);
	  CPPUNIT_ASSERT( newmsgp->get_pos() == 15 );

	  uint128 testvalue(1, 2, 3, 4);
	  newmsgp->encode128(testvalue);
	  CPPUNIT_ASSERT( newmsgp->get_pos() == 31 );

	  string teststring="Hello World!";
	  newmsgp->encode(teststring);
	  CPPUNIT_ASSERT( newmsgp->get_pos() == 43 );

	  newmsgp->to_start();

	  CPPUNIT_ASSERT( newmsgp->decode8() == 42 );
	  CPPUNIT_ASSERT( newmsgp->decode16() == 2 );
	  CPPUNIT_ASSERT( newmsgp->decode32() == 3UL );
	  CPPUNIT_ASSERT( newmsgp->decode64() == 4UL );
	  uint128 testvalue_res= newmsgp->decode128();
	  CPPUNIT_ASSERT( testvalue_res == testvalue );
	  
	  string teststring_result;
	  newmsgp->decode(teststring_result, teststring.length());
	  CPPUNIT_ASSERT( teststring_result == teststring );

        }

        void testNetMsgWeirdCoding() {
		uchar* buf= NULL;
		newmsgp->to_start();

		newmsgp->decode(buf,0, false);
		newmsgp->encode(buf,0, false);

		// test non NULL pointer
		buf= new uchar[1];
		newmsgp->decode(buf,0, false);
		newmsgp->encode(buf,0, false);		

	}
	void tearDown() {
	  // Executed after each of the test methods.
	  delete newmsgp;
	}
};

CPPUNIT_TEST_SUITE_REGISTRATION( NetMsgTest );
