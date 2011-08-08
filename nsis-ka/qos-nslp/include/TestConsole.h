/// ----------------------------------------*- mode: C++; -*--
/// @file TestConsole.h
/// QoS NSLP Telnet Test Console
/// ----------------------------------------------------------
/// $Id: TestConsole.h 6211 2011-06-03 19:58:27Z bless $
/// $HeadURL: https://svn.tm.kit.edu/nsis/qos-nslp/trunk/include/TestConsole.h $
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
#ifndef TESTCONSOLE_H_
#define TESTCONSOLE_H_

#include "protlib_types.h"
#include "messages.h"
#include "threads.h"
#include "logfile.h"
#include "qspec.h"
#include "nslp_pdu.h"
#include "address.h"
#include "QoS_Appl_Msg.h"
#include "sessionid.h"

using namespace protlib::log;
using namespace protlib;

namespace qos_nslp {

class QoS_Appl_Msg;		// reuse
 
struct TestConsoleParam : public ThreadParam {
	TestConsoleParam(uint32 sleep_time = ThreadParam::default_sleep_time,
	    const char *host = "localhost", const char *port = "30023");
	const message::qaddr_t source;
	const char *chost;
	const char *cport;
};

class TestConsole : public Thread {
public:
	TestConsole(const TestConsoleParam& p);
	virtual ~TestConsole();
   
	// From Thread
	virtual void main_loop(uint32 nr);

private:
	enum cmd_return {
		OK,
		MORE_ARGS,
		ERROR,
		UNSET
	};
	typedef cmd_return (qos_nslp::TestConsole::*cmd_func_t)();

	map<string, cmd_func_t> commands;
	list<string> args; 

	const TestConsoleParam param;
	int sockfd, clientfd;

	string prompt;

	string out_buf, in_buf;
	char buf[1024];
	int send();
	int recv();

	cmd_return usage();

	appladdress src_a, dest_a;
#define	PDU_FLAG_REPLACE	(1<<1)
#define	PDU_FLAG_ACK		(1<<2)
#define	PDU_FLAG_SCOPE		(1<<3)
#define	PDU_FLAG_TEAR		(1<<4)
#define	PDU_FLAG_INIT		(1<<5)
	int downstream, rrii, qosmid, flags;
	bool reliable;
	ntlp::sessionid *sid;
	known_nslp_pdu::type_t pdu_format;
	float band;
	bool send_pdu_template;
	QoS_Appl_Msg *create_msg(uint128 sid);

	cmd_return set();

	cmd_return set_template(list<string>::const_iterator& aa);

	cmd_return send_pdu(); 

	cmd_return multi_send();

	cmd_return quit(); 
};

}

#endif /*TESTCONSOLE_H_*/
