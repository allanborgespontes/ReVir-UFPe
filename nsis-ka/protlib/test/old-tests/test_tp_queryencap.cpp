/// ----------------------------------------*- mode: C++; -*--
/// @file test_tp_over_udp.cpp
/// Test for TP over UDP.
/// ----------------------------------------------------------
/// $Id: test_tp_queryencap.cpp 6183 2011-05-20 09:11:51Z bless $
/// $HeadURL: https://svn.tm.kit.edu/nsis/protlib/trunk/test/old-tests/test_tp_queryencap.cpp $
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
#include "tp_queryencap.h"
#include "logfile.h"
#include "queuemanager.h"
#include "threadsafe_db.h"
#include "setuid.h"

using namespace protlib;
using namespace protlib::log;

logfile commonlog;
protlib::log::logfile& protlib::log::DefaultLog(commonlog);

bool done = false;
bool running = false;

bool mygetmsglength(NetMsg& m, uint32& clen_words)
{
  return true;
}


pthread_t tpreceivethread;

void* tpchecker(void *p) {
	message* msg = NULL;
	TPMsg* tpmsg = NULL;
	const appladdress* peer = NULL;
	NetMsg* datamsg = NULL;
	TPError* err = NULL;

	FastQueue* fq = QueueManager::instance()->get_queue(message::qaddr_signaling);
	if (!fq) {
	  Log(ERROR_LOG, LOG_ALERT, "test_tp_over_tcp", "TPchecker cannot find message queue");
	  return NULL;
	} // end if not fq

	running = true;
	Log(EVENT_LOG, LOG_NORMAL, "test_tp_over_udp", "TPchecker started");

	while (!done) 
	{
	  msg = fq->dequeue_timedwait(5000);
	  if (msg) 
	  {
	    Log(EVENT_LOG, LOG_NORMAL, "test_tp_over_udp", "TPchecker received a message");
	    tpmsg = dynamic_cast<TPMsg*>(msg);
	    if (tpmsg) {
	      // evaluate TP message
	      peer = static_cast<const appladdress*>(tpmsg->get_peeraddress());
	      datamsg = tpmsg->get_message();
	      err = tpmsg->get_error();
	      if (peer) cout<<"from: "<<(*peer)<<endl;
	      else cout<<"no peer address"<<endl;;
	      if (datamsg) cout<<"data ("<<datamsg->get_size()<<" bytes): "
			       <<(char*)(datamsg->get_buffer())<<endl;
	      else cout<<"no data"<<endl;
	      if (err) cout<<"error: "<<err->getstr()<<endl;
	      else cout<<"no error"<<endl;
	      cout.flush();
	      tpmsg = NULL;
	    } 
	    else 
	    {
	      Log(ERROR_LOG, LOG_ALERT, "test_tp_over_udp", "TPchecker cannot cast message of type " << msg->get_type_name() << " to TPMSG");
	    } // end if tpmsg
	    delete msg;
	    msg = NULL;
	  } // end if msg
	} // end while not done
	fq->cleanup();
	Log(EVENT_LOG, LOG_NORMAL, "test_tp_over_udp",	"TPchecker stopped");
	running = false;
	return NULL;
} // end tpchecker

void test(const char* logname) {
	char data[512] = {0};
	string datastr = "12345";
	NetMsg* datamsg = NULL;
	appladdress peer;
	hostaddress local_addr;
	// initalize netdb and setuid
	protlib::tsdb::init();
	protlib::setuid::init();
	// setup log

	/* TODO: convert set_filter() to the new logging
	cout<<"log file: "<<logname<<endl<<flush;
	DefaultLog::log->set_dest(logname);
	DefaultLog::log->set_filter(ERROR_LOG, 15);
	DefaultLog::log->set_filter(WARNING_LOG, 15);
	DefaultLog::log->set_filter(EVENT_LOG, 15);
	DefaultLog::log->set_filter(INFO_LOG, 15);
	DefaultLog::log->set_filter(DEBUG_LOG, 15);
	*/
	ofstream outfile("timedump");

	port_t port = 0;
	// set protocol
	peer.set_protocol(prot_query_encap);
	peer.set_rao(1);
	// get local port
	while (!port) {
		cout<<"Listen on port: ";
		cin>>port;
	} // while not port

	TPoverUDPParam tppar(4, mygetmsglength, port);
	Log(EVENT_LOG, LOG_NORMAL, "test_tp_over_udp", "Creating TPChecker thread");
	FastQueue* tpchecker_fq= new FastQueue("tpchecker",true);
	QueueManager::instance()->register_queue(tpchecker_fq,message::qaddr_signaling);
	pthread_create(&tpreceivethread,NULL,tpchecker,NULL);
	Log(EVENT_LOG, LOG_NORMAL, "test_tp_over_udp", "Creating and starting TPoverUDP thread");
	ThreadStarter<TPoverUDP,TPoverUDPParam> tpthread(1,tppar);

	TPoverUDP* udpproto = tpthread.get_thread_object();

	// initial fill of RAOVec. We push "0" onto it
	vector<uint32> raovec;
	raovec.push_back(0);
	raovec.push_back(1);
  
	// start TPqueryEncap
	TPqueryEncapParam qepar(4, mygetmsglength, port, raovec,
	    udpproto, 12345, 5000UL);
        ThreadStarter<TPqueryEncap,TPqueryEncapParam> qetpthread(1,qepar);

	TPqueryEncap* qeproto = qetpthread.get_thread_object();

	tpthread.start_processing();
	qetpthread.start_processing();

	// read in strings from stdin and send them
	Log(EVENT_LOG, LOG_NORMAL, "test_tp_over_udp", "Reading data to send");
	while (true) {
		// get peer adress
		cout<<"Peer (IP port): ";
		if (!((cin>>datastr) && (cin>>port))) {
			sleep(30);
			datastr = "";
			port = 0;
		} // end if input error
		if (peer.set_ip(datastr.c_str()) && port) {
			peer.set_port(port);
		} else {
		  Log(ERROR_LOG, LOG_ALERT, "test_tp_over_udp", "invalid IP or port, exiting");
		  break;
		} // end if peer data OK
		cout<<"Local Address: ";
		if (!(cin>>datastr)) {
			break;
		}
		if (!local_addr.set_ip(datastr.c_str()))
			break;
		// get data
		cout<<"Data: ";
		cin.getline(data,500);
		datastr = data;
		try {
			// prepare NetMsg and send it
			datamsg = new NetMsg((unsigned char*)(datastr.c_str()),datastr.length()+1);
			qeproto->send(datamsg, peer, &local_addr, false);
			Log(EVENT_LOG, LOG_NORMAL, "test_tp_over_udp", "message sent");
		} catch(ProtLibException& e) {
		  Log(ERROR_LOG, LOG_ALERT, "test_tp_over_udp", "Error " << e.getstr() << " while preparing and sending data");
		} // end try-catch
	} // end while true
	// stop TPoverUDP
	Log(EVENT_LOG, LOG_NORMAL, "test_tp_over_udp", "Stopping TPoverUDP");
	tpthread.stop_processing();
	// stop TPchecker
	Log(EVENT_LOG, LOG_NORMAL, "test_tp_over_udp", "Stopping TPchecker thread");
	done = true;
	while (running) {
		sleep(1);
	} // end while not done
	// aborting TPoverUDP
	Log(EVENT_LOG, LOG_NORMAL, "test_tp_over_udp", "Aborting TPoverUDP");
	tpthread.abort_processing();
	// destroy QueueManager
	QueueManager::clear();
	protlib::setuid::end();
	protlib::tsdb::end();
} // end test

int main(int argc, char** argv) {
	ostringstream logname; 
	// construct log file name
	logname<<argv[0]<<"."<<getpid()<<".log";
	// start test and catch exceptions
	try {
		test(logname.str().c_str());
	} catch(ProtLibException& e) {
		cerr<<"Fatal: "<<e.getstr()<<endl;
	} catch(...) {
		cerr<<"Fatal: an error occured, no info, sorry."<<endl;
	} // end try-catch
	return 0;
} // end main

//@}
  
