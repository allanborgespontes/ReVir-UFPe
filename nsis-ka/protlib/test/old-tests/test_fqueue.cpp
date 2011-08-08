/// ----------------------------------------*- mode: C++; -*--
/// @file test_fqueue.cpp
/// This is a test for class FastQueue. 
/// ----------------------------------------------------------
/// $Id: test_fqueue.cpp 3527 2008-10-10 13:25:24Z bless $
/// $HeadURL: https://svn.tm.kit.edu/nsis/protlib/trunk/test/old-tests/test_fqueue.cpp $
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
/** @addtogroup testfqueue
 * @ingroup tests
 * @{
 */

#include <iostream>

#include "fqueue.h"
#include "test_util.h"
#include "logfile.h"

using namespace protlib;
using namespace protlib::log;

logfile commonlog;
protlib::log::logfile& protlib::log::DefaultLog(commonlog);

int main() {
	bool result = true;
	bool res = true;
	uint32 i = 0;
	uint32 j = 0;
	start_test();
	cout<<"Creating FastQueue..."<<endl;
	FastQueue* fq = new FastQueue("TestQueue",true);
	if (fq) {
		// insert
		cout<<"Inserting 100 integers, odd numbers as expedited data..."<<endl;
		for (i=1;i<=100;i++) {
			res = fq->enqueue((message*)i,((i%2)?true:false));
			if (!res) result = false;
		} // end for
		// cleanup
		cout<<"Shutting down FastQueue..."<<endl;
		fq->shutdown();
		// dequeue expedited data
		if (result) {
			cout<<"Dequeueing expedited data (odd integers)..."<<endl;
			for (i=1;i<=100;i+=2) {
				j = (int)(fq->dequeue(false));
				if (j!=i) {
					cout<<"Expected "<<i<<" but got "<<j<<"."<<endl;
					result = false;
				} // end if j!=i
			} // end for
		} // end if result
		// dequeue normal data
		if (result) {
			cout<<"Dequeueing normal data (even integers)..."<<endl;
			for (i=2;i<=100;i+=2) {
				j = (int)(fq->dequeue(false));
				if (j!=i) {
					cout<<"Expected "<<i<<" but got "<<j<<"."<<endl;
					result = false;
				} // end if j!=i
			} // end for
		} // end if result
		// enqueue after shutdown
		if (result) {
			if (fq->enqueue((message*)9999)) {
				cout<<"FastQueue accepts messages after shutdown."<<endl;
				result = false;
			} else {
				cout<<"FastQueue doesn't accept messages after shutdown, OK."<<endl;
			} // end if enqueue
		} // end if result
		// timed wait
		if (result) {
			cout<<"Timed wait..."<<endl;
			if (fq->dequeue_timedwait(5000)) {
				cout<<"Dequeued something although queue should be empty."<<endl;
				result = false;
			} else {
				cout<<"Timed wait OK."<<endl;
			} // end if dequeue
		} // end if result
		// cannot use fq->cleanup() because this would try to delete
		// the stored messages, but we store only casted ints.
		i=fq->size();
		if (i) {
			cout<<"FastQueue contains "<<i<<" entries although it should be empty."<<endl;
			cout<<"Cannot delete FastQueue in this test,"<<endl
				<<"because we are storing uint32s, not messages."<<endl;
			result=false;
		} else {
			cout<<"FastQueue is empty."<<endl;
			cout<<"Deleting FastQueue..."<<endl;
			delete fq;
		} // end if empty
	} else {
		cout<<"Cannot create FastQueue."<<endl;
		result = false;
	} // end if fq
	stop_test();
	if (result) cout<<"Test OK"<<endl;
	else cout<<"Test failed"<<endl;
   return 0;
}

//@}
