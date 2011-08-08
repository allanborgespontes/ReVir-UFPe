/// ----------------------------------------*- mode: C++; -*--
/// @file client.cpp
/// Test application that requests QoS support from the
/// QoS NSLP Daemon via UNIX Domain Socket IPC interface
/// ----------------------------------------------------------
/// $Id: client.cpp 6297 2011-06-22 11:01:53Z bless $
/// $HeadURL: https://svn.tm.kit.edu/nsis/qos-nslp/trunk/src/client.cpp $
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

/*
 * usage: client <src_address> <dst_address>
 */

#include <iostream>
#include <cstdlib>
#include <cstring>
#include <errno.h>

#include "QoS_NSLP_Client_API.h"
#include "sessionid.h"
#include "qos_nslp.h"

using namespace qos_nslp;

const port_t SRC_PORT  = 0;
const port_t DST_PORT  = 0;
const uint16 SCTP_PORT = 30000;
const std::string sockname("/tmp/qos_nslp_socket");

/* This flag controls termination of the main loop.  */
volatile sig_atomic_t keep_going = 0;

logfile commonlog("", true, true);
qos_nslp::log::logfile& qos_nslp::log::DefaultLog(commonlog);

void catch_sigint(int sig)
{
	std::cerr << "Caught a SIGINT signal. Terminating..." << std::endl;
	keep_going = 0;
	signal(sig, catch_sigint);
}


int main(int argc, char *argv[])
{
	if (argc < 3) {
		std::cerr << "usage: " << argv[0] << 
			" <src_address> <dst_address> [R]" << std::endl;
		return EXIT_FAILURE;
	}

	bool sender_initiated_reserve = true;
	if (argc > 3 && strncasecmp("r", argv[3], 1) == 0)
		sender_initiated_reserve = false;

	// Establish a handler for SIGINT signals (send on Ctrl-c)
	signal(SIGINT, catch_sigint);

	// initalize netdb
	tsdb::init(true);
	qos_nslp::register_ies();

	appladdress src = appladdress(argv[1], "tcp", SRC_PORT);
	appladdress dst = appladdress(argv[2], "tcp", DST_PORT);

	QoS_NSLP_Client_API client(sockname);

	// set appropriate values for reservation request
	bool downstream = true;
	float bandwidth = 125.0 * 1000; // bytes/s (125000 bytes/s = 1 Mbit/s)

	// this sends a RESERVE message as reservation request for 
	// a data flow from src to dst
	ntlp::sessionid *gist_sid;
	if (sender_initiated_reserve)
		gist_sid = client.send_reserve(src, dst, downstream, bandwidth);
	else
		gist_sid = client.send_query(src, dst, downstream, bandwidth);

	int status;
	int timeout = 5000; // ms

	// this call blocks until response was received or timeout happens
	if (sender_initiated_reserve)
		client.recv_response(status, timeout);
	else
		client.recv_reserve(status, timeout);

	if (status == 2) {
		std::cout << "successful reservation" << std::endl;
		keep_going = 1;
	} else if (status >= 0)
		std::cout << "status of reservation request " << status << std::endl;
	else
		std::cout << "An error occurred: " << std::strerror(errno) << std::endl;

	// wait until Ctrl-c sets keep_going = 0 -- e.g., continue application processing
	while (keep_going)
		sleep(1);

	// If reservation has been successful tear down resources now
	if (status == 2) {
		client.send_tear(gist_sid, src, dst, downstream);
		// ... and wait 5000ms for confirmation
		client.recv_response(status, timeout);
	}

	tsdb::end();

	return 0;
}
