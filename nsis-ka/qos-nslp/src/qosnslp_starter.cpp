/// ----------------------------------------*- mode: C++; -*--
/// @file qosnslp_starter.cpp
/// QoS NSLP Starter - Starts all necessary QoS NSLP modules
/// ----------------------------------------------------------
/// $Id: qosnslp_starter.cpp 5800 2010-12-15 16:13:54Z bless $
/// $HeadURL: https://svn.tm.kit.edu/nsis/qos-nslp/trunk/src/qosnslp_starter.cpp $
// ===========================================================
//                      
// Copyright (C) 2005-2010, all rights reserved by
// - Institute of Telematics, Karlsruhe Institute of Technology
//
// More information and contact:
// https://projekte.tm.uka.de/trac/NSIS
//                      
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; version 2 of the License
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
// 02110-1301 USA.
//
// ===========================================================

#include "qosnslp_starter.h"
#include "ProcessingModule.h"
#include "queuemanager.h"

#include "gist_conf.h"

using namespace qos_nslp;
using namespace ntlp;

QoSNSLPStarterParam::QoSNSLPStarterParam(ntlp::NTLPStarterParam *ntlppar) 
	: ThreadParam(ThreadParam::default_sleep_time, "QoS NSLP Starter"),
	  ntlppar(ntlppar), addresses()
{
}

/** Initialize Thread base class and QoS NSLP protocol. */
QoSNSLPStarter::QoSNSLPStarter(const QoSNSLPStarterParam& p)
	: Thread(p),
	  param(p)
{
	DLog(param.name, "Creating QoSNSLPStarter object");
}


QoSNSLPStarter::~QoSNSLPStarter() 
{
	DLog(param.name, "Destroying QoSNSLPStarter object");
}

void QoSNSLPStarter::main_loop(uint32 n)
{
	try {
		// start NTLP
		// NTLPStarterParam is necessary
		ThreadStarter<ntlp::NTLPStarter, ntlp::NTLPStarterParam> ntlpthread(1, *(param.ntlppar));
		DLog("QoS NSLP Starter", color[red] << "start NTLP Thread (instance " << n << ')'  << color[off]);
		ntlpthread.start_processing();

		sleep(4);

		FastQueue* applmsgrecv_fq= new FastQueue("applmsgchecker",true);
		QueueManager::instance()->register_queue(applmsgrecv_fq, message::qaddr_appl_qos_signaling);
			
		// start ProcessingModule (will need four threads)
		ProcessingModuleParam sim_cl_par(ThreadParam::default_sleep_time,
						 *param.addresses, *param.fi_service);

		ThreadStarter<ProcessingModule, ProcessingModuleParam> processing_module(4, sim_cl_par);

		FastQueue* apimsgrecv_fq = new FastQueue("apimsgchecker", true);
		QueueManager::instance()->register_queue(apimsgrecv_fq, message::qaddr_api_1);

		processing_module.start_processing();

		ILog(param.name, color[green] << "Startup completed" << color[off]);

		while (get_state() == STATE_RUN)
			sleep(param.sleep_time);

		ILog(param.name, color[green] << "Shutting down ... " << color[off]);

		// stop threads
		EVLog(param.name, "Stopping ProcessingModule");
		processing_module.stop_processing();

		EVLog(param.name, "Stopping NTLP Thread");
		ntlpthread.stop_processing();

		// aborting ProcessingModule
		EVLog(param.name, "Aborting ProcessingModule");
		processing_module.abort_processing();

		// aborting NTLP
		EVLog(param.name, "Aborting NTLP Thread");
		ntlpthread.abort_processing();
	} catch (ProtLibException& e) {
		cerr << "Fatal: " << e.getstr() << endl;
	} catch (...) {
		cerr << "Fatal: an error occured, no info, sorry." << endl;
	} // end try-catch
}
