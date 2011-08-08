/// ----------------------------------------*- mode: C++; -*--
/// @file test_log.cpp
/// This file contains some code to test the logging API
/// ----------------------------------------------------------
/// $Id: test_flowinfo.cpp 6176 2011-05-20 07:39:55Z bless $
/// $HeadURL: https://svn.tm.kit.edu/nsis/ntlp/trunk/test/test_flowinfo.cpp $
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
/** @addtogroup testlog Logging
 * @ingroup tests
 * @{
 */

#include <iostream>
#include "logfile.h"
#include "flowinfo.h"

using namespace protlib;
using namespace protlib::log;
using namespace ntlp;

logfile commonlog;
protlib::log::logfile& protlib::log::DefaultLog(commonlog);

int main() {

  tsdb::init();

  FlowinfoParam fiparam;
  ThreadStarter<Flowinfo, FlowinfoParam> fithread(1, fiparam);
  fithread.start_processing();

  Flowinfo *finfo = fithread.get_thread_object();

  sleep(1);

  for (;;) {
    mri_pathcoupled mri;
    hostaddress ha;
    string input;
    Flowstatus *fs;

    cout << "Source: ";
    cin >> input;
    ha.set_ip(input.c_str());
    mri.set_sourceaddress(ha);

    cout << "Dest: ";
    cin >> input;
    ha.set_ip(input.c_str());
    mri.set_destaddress(ha);

//    for (int i = 0; i < 5; i++) {
    fs = finfo->get_flowinfo(mri);
    cout << fs->orig_mri << endl;
    cout << *fs->new_mri << endl;
    if (fs->type == Flowstatus::fs_tunnel) {
      cout << "Tunneled: " << endl;
      cout << *fs->tunnel_id << endl;
    }
    cout << "Overhead: " << fs->pp_overhead << endl;
    delete fs;
//    }
  }

  tsdb::end();
  return 0;
}

//@}
