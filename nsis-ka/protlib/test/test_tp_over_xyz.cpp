/// ----------------------------------------*- mode: C++; -*--
/// @file test_tp.cpp
/// Test for TP over <any>.
/// ----------------------------------------------------------
/// $Id: test_tp_over_udp.cpp 3511 2008-10-08 11:23:08Z bless $
/// $HeadURL: https://svn.ipv6.tm.uka.de/nsis/protlib/trunk/src/test_tp_over_udp.cpp $
// ===========================================================
//                      
// Copyright (C) 2005-2007, all rights reserved by
// - Institute of Telematics, Universitaet Karlsruhe (TH)
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
// ===========================================================
/** @addtogroup testtpUDP TP over UDP
 * @ingroup tests
 * @{
 */

/** 
 * Test for TP over UDP. You can use this program to send and receive TP
 * messages.
 */

#include <unistd.h>
#include <iostream>
#include <string>
#include <sstream>

#include "network_message.h"
#include "tp_over_udp.h"
#include "tp_over_tcp.h"
#ifdef _USE_SCTP
#include "tp_over_sctp.h"
#endif
#include "tp_over_uds.h"
#include "logfile.h"
#include "queuemanager.h"
#include "threadsafe_db.h"

#include <cppunit/TestCase.h>
#include <cppunit/extensions/HelperMacros.h>

using namespace protlib;
using namespace protlib::log;

bool done = false;
bool running = false;

const uint16 magic=0xbc3a;
const uint16 testdatsize= 4096;
port_t testport = 41444;
const uint32 commonheaderlen= 4;

string testdata1("01234567890abcdef01234567890abcdef.");
uchar testdata2[testdatsize]= {0};


class test_tp_over_xyz : public CppUnit::TestCase {

  CPPUNIT_TEST_SUITE( test_tp_over_xyz );

  CPPUNIT_TEST( test_send_receive_UDPv4 );
  CPPUNIT_TEST( test_send_receive_TCPv4 );
#ifdef _USE_SCTP
  CPPUNIT_TEST( test_send_receive_SCTPv4 );
#endif

  CPPUNIT_TEST( test_send_receive_UDPv6 );
  CPPUNIT_TEST( test_send_receive_TCPv6 );
#ifdef _USE_SCTP
  CPPUNIT_TEST( test_send_receive_SCTPv6 );
#endif

  CPPUNIT_TEST_SUITE_END();

  public:
  template <class T, class TParam> void test_send_receive(const char* destinationaddrstring);
  
	void test_send_receive_UDPv4() { test_send_receive<TPoverUDP,TPoverUDPParam>("127.0.0.1"); testport++; }
  void test_send_receive_TCPv4() { test_send_receive<TPoverTCP,TPoverTCPParam>("127.0.0.1");testport++; }
#ifdef _USE_SCTP
  void test_send_receive_SCTPv4() { test_send_receive<TPoverSCTP,TPoverSCTPParam>("127.0.0.1");testport++; }
#endif

  void test_send_receive_UDPv6() { test_send_receive<TPoverUDP,TPoverUDPParam>("::1");testport++; }
  void test_send_receive_TCPv6() { test_send_receive<TPoverTCP,TPoverTCPParam>("::1");testport++; }
#ifdef _USE_SCTP
  void test_send_receive_SCTPv6() { test_send_receive<TPoverSCTP,TPoverSCTPParam>("::1");testport++; }
#endif
};

uint16 num_testmsgs= 2;

// read length from common header
bool mygetmsglength(NetMsg& m, uint32& clen_words)
{
  if (m.decode16() == magic)
  {
    clen_words= m.decode16();
    m.to_start();
    return true;
  }
  else
  {
    m.to_start();
    return false;
  }
}

// POSIX thread object for receiver thread
pthread_t tpreceivethread;

// this is a thread that receives messages from a TPoverXYZ module
void* 
tpchecker(void *p) 
{  
  const appladdress* peer = NULL;
  
  const char *const methodname="test_tp_over_XYZ";
  FastQueue* fq = QueueManager::instance()->get_queue(message::qaddr_signaling);
  if (!fq) {
    ERRLog(methodname, "TPchecker cannot find message queue");
    return NULL;
  } // end if not fq

  running = true;
  EVLog(methodname, "TPchecker started");

  
  NetMsg* datamsg = NULL;
  TPError* err = NULL;
  TPMsg* tpmsg = NULL;
  message* msg = NULL;
  uint16 num_msgs= 0;
  
  while (num_msgs < num_testmsgs) 
  { // only two messages
    // wait some time for message arrival
    msg = fq->dequeue_timedwait(1000);
    if (msg) 
    {
      EVLog(methodname, "TPchecker received a message");

      tpmsg = dynamic_cast<TPMsg*>(msg);
      if (tpmsg) {
	// evaluate TP message
	peer = static_cast<const appladdress*>(tpmsg->get_peeraddress());
	datamsg = tpmsg->get_message();
	if (peer) 
	  EVLog(methodname, "TPchecker received a message from" << *peer);
	else 
	  EVLog(methodname, "TPchecker received from an unknown peer");

	if (datamsg)
	{
	  EVLog(methodname, "data ("<< datamsg->get_size() << " bytes): " << (char*)(datamsg->get_buffer()+commonheaderlen));
	  uint16 rcvmagicnum= datamsg->decode16();
	  if (rcvmagicnum != magic)
	  {
	    ERRLog(methodname,"rcvmagicnum:" << hex << rcvmagicnum << dec);
	  }
	  CPPUNIT_ASSERT(rcvmagicnum == magic);

	  uint32 length= datamsg->decode16();
	  CPPUNIT_ASSERT( (length + commonheaderlen) == datamsg->get_size() );

	  if (num_msgs == 0)
	  {
	    // check length
	    CPPUNIT_ASSERT( datamsg->get_size() == (testdata1.length() + commonheaderlen) );
	    // compare payload
	    CPPUNIT_ASSERT( memcmp(datamsg->get_buffer()+datamsg->get_pos(), testdata1.c_str(), length) == 0 );
	  }
	  else
	  if (num_msgs == 1)
	  {
	    // check length
	    CPPUNIT_ASSERT( datamsg->get_size() == (testdatsize + commonheaderlen) );
	    // compare payload
	    CPPUNIT_ASSERT( memcmp(datamsg->get_buffer()+datamsg->get_pos(), testdata2, length) == 0 );
	  }
	  else
	  {
	    CPPUNIT_FAIL("received more messages than expected");
	  }
	}
	else 
	  EVLog(methodname,"no data provided");

	err = tpmsg->get_error();
	if (err)
	{
	  ERRLog(methodname, "error: "<<err->getstr());
	  CPPUNIT_FAIL("Unexpected error occured.");
	}
	else
	{
	  DLog(methodname, color[green] << "no error" << color[off]);
	}
	CPPUNIT_ASSERT(err == false);

	tpmsg = NULL;
      } 
      else 
      {
	ERRLog(methodname, "TPchecker cannot cast message of type " << msg->get_type_name() << " to TPMSG");
      } // end if tpmsg

      delete msg;  // this will also delete the datamsg

      msg = NULL;

      num_msgs++;
    } // end if msg

  } // end while not done
  fq->cleanup();

  EVLog( methodname, "TPchecker stopped");
  running = false;
  return NULL;
} // end tpchecker


template <class TPoverXYZ, class TPoverXYZParam>
void 
test_tp_over_xyz::test_send_receive(const char* destaddrstr) {

	// initalize netdb and setuid
        protlib::tsdb::init(true); // true means do not resolve DNS names
	// setup log

	const char *const methodname="test_sendrecv()";

	EVLog(methodname, "Creating TPChecker thread");
	FastQueue* tpchecker_fq= new FastQueue("tpchecker",true);
	QueueManager::instance()->register_queue(tpchecker_fq,message::qaddr_signaling);
	pthread_create(&tpreceivethread,NULL,tpchecker,NULL);

	// 1s sleep time only
	TPoverXYZParam tppar(commonheaderlen, mygetmsglength, testport);
	EVLog( methodname, "Creating and starting TPoverXYZ thread");
	ThreadStarter<TPoverXYZ,TPoverXYZParam> tpthread(1,tppar);
	tpthread.start_processing();
	// busy waiting is not so nice, but faster than sleeping
	while (!tpthread.is_running());
		
	// read in strings from stdin and send them
	EVLog( methodname, "Generating data to send");
	// fill testdata2 with data
	for (uint16 i= 0; i<testdatsize; i++)
	  testdata2[i]= (char) (i % 256);

	// set peer address (localhost for this test)
	appladdress destinationaddr(destaddrstr,tpthread.get_thread_object()->get_underlying_protocol(),testport);

	NetMsg* datamsg = NULL;
	uint16 num_msgs= 0;
	while (num_msgs < num_testmsgs) 
	{
	  try {
	    // prepare NetMsg and send it	    
	    if (num_msgs == 0)
	    {
	      datamsg = new NetMsg(testdata1.length() + commonheaderlen);
	      datamsg->to_start();
	      datamsg->encode16(magic);
	      datamsg->encode16(testdata1.length());
	      DLog(methodname, "Msg 1: payload length="<< testdata1.length());
	      datamsg->copy_from(reinterpret_cast<const uchar*>(testdata1.c_str()), commonheaderlen, testdata1.length());
	    }
	    else
	    if (num_msgs == 1)
	    {
	      datamsg = new NetMsg(testdatsize + commonheaderlen);
	      datamsg->to_start();
	      datamsg->encode16(magic);
	      datamsg->encode16(testdatsize);
	      DLog(methodname, "Msg 2: payload length="<<testdatsize);

	      datamsg->copy_from(reinterpret_cast<const uchar*>(testdata2), commonheaderlen, testdatsize);
	    }

	    // invoke send method on TP thread
	    tpthread.get_thread_object()->send(datamsg, destinationaddr, false, NULL);

	    EVLog(methodname, "message sent");
	  } 
	  catch(ProtLibException& e) {
	    ERRLog(methodname, "Error " << e.getstr() << " while preparing and sending data");
	    CPPUNIT_FAIL("unexpected exception occured");
	  } // end try-catch

	  num_msgs++;
	} // end while not all test msgs sent

	// wait until receiver process has terminated
	pthread_join(tpreceivethread,NULL);

	// stop TPoverXYZ
	EVLog(methodname, "Stopping TPoverXYZ");
	tpthread.stop_processing();
	tpthread.wait_until_stopped();

	// aborting TPoverXYZ
	//EVLog( methodname, "Aborting TPoverXYZ");
	//tpthread.abort_processing();

	// destroy QueueManager
	QueueManager::clear();
	protlib::tsdb::end();
} // end test

CPPUNIT_TEST_SUITE_REGISTRATION( test_tp_over_xyz );

//@}
  
