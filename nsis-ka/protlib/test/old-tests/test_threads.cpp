/// ----------------------------------------*- mode: C++; -*--
/// @file test_threads.cpp
/// put short description here
/// ----------------------------------------------------------
/// $Id: test_threads.cpp 3527 2008-10-10 13:25:24Z bless $
/// $HeadURL: https://svn.tm.kit.edu/nsis/protlib/trunk/test/old-tests/test_threads.cpp $
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
#include <unistd.h>
#include <iostream>

#include "threads.h"

using namespace protlib;
using namespace std;

logfile commonlog;
protlib::log::logfile& protlib::log::DefaultLog(commonlog);

class TestThread : public Thread {
private:
	const ThreadParam tp;
public:
	TestThread(const ThreadParam& p) : Thread(p), tp(p) {
		cout<<"Creating TestThread object."<<endl;
	}
	virtual ~TestThread() {
		cout<<"Destroying TestThread object."<<endl;
	}
	virtual void main_loop(uint32 nr) {
		char* str = "hallo";
		state_t s;
		s=get_state();
		while(s!=STATE_ABORT) {
			if (s==STATE_RUN) str="running";
			if (s==STATE_STOP) str="stopping";
			cout<<"I am "<<tp.name<<" "<<nr<<" "<<str<<"."<<endl;
			sleep(nr);
			s=get_state();
		} // end while
	}
};

int main() {
	try {
		cout<<"test_thread is starting..."<<endl;
		ThreadParam p(5,"TestThread");
		ThreadStarter<TestThread,ThreadParam> threadcontrol(3,p);
		threadcontrol.start_processing();
		cout<<"test_thread is sleeping 60 seconds..."<<endl;
		sleep(60);
		cout<<"test_thread is stopping threads..."<<endl;
		threadcontrol.stop_processing();
		cout<<"test_thread is sleeping 20 seconds..."<<endl;
		sleep(20);
		cout<<"test_thread is aborting threads..."<<endl;
		threadcontrol.abort_processing();
		sleep(10);
		cout<<"test_threads is exiting..."<<endl;
	} catch(ThreadError& e) {
		cout<<"ThreadError: "<<e.getstr()<<endl;
	} // end try-catch
	return 0;
}
