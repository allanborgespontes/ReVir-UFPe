/// ----------------------------------------*- mode: C++; -*--
/// @file GISTConsole.h
/// Simple Telnet(able) interface for testing and diagnosis
/// This GISTConsole allows to control testing via telnet or wrapper ui.
/// ----------------------------------------------------------
/// $Id: GISTConsole.h 5249 2010-04-26 08:40:59Z roehricht $
/// $HeadURL: https://svn.tm.kit.edu/nsis/ntlp/trunk/src/GISTConsole.h $
// ===========================================================
//                      
// Copyright (C) 2005-2010, all rights reserved by
// - Institute of Telematics, Karlsruhe Institute of Technology
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
#ifndef GISTCONSOLE_H_
#define GISTCONSOLE_H_

#include "protlib_types.h"
#include "messages.h"
#include "threads.h"
#include "logfile.h"
#include "address.h"
#include "gist_conf.h"

using namespace protlib::log;
using namespace protlib;

namespace ntlp {

extern  const char *GISTConsoleDefaultPort;
 
struct GISTConsoleParam : public ThreadParam {
	GISTConsoleParam(pid_t main_pid,
			 const message::qaddr_t source,
			 const char *host = "localhost",
			 uint32 sleep_time = ThreadParam::default_sleep_time,
			 const char *port = GISTConsoleDefaultPort);
        pid_t main_pid;
	const message::qaddr_t source;
	const char *chost;
	const char *cport;
};

class GISTConsole : public Thread {
public:
	GISTConsole(const GISTConsoleParam& p);
	virtual ~GISTConsole();
   
	// From Thread
	virtual void main_loop(uint32 nr);

private:
	enum cmd_return {
		OK,
		MORE_ARGS,
		ERROR,
		UNSET
	};
	typedef cmd_return (ntlp::GISTConsole::*cmd_func_t)();

	map<string, cmd_func_t> commands;
	list<string> args; 

	typedef map<std::string, configpar_id_t> parname_map_t;
	parname_map_t parname_map;

	const GISTConsoleParam param;
	int sockfd, clientfd;

	string prompt;

	string out_buf, in_buf;
	char buf[1024];
	int send();
	int recv();

	cmd_return usage();

	void collectParNames();

	// returns a string containing parameter names
	string parnamelist() const;

	// returns descriptive help text for a parameter
	string helppar(string parname);

	hostaddress src_a, dest_a;
	//known_nslp_pdu::type_t pdu_format;
	bool send_pdu_template;
	APIMsg* create_api_msg(uint128 sid, bool use_est_MRI= false);
	cmd_return setsid(list<string>::const_iterator& aa);

	cmd_return set();

	cmd_return set_template(list<string>::const_iterator& aa);

	cmd_return send_pdu(); 

	cmd_return send_est_pdu(); 

	cmd_return multi_send();
	
	cmd_return saveconfig();

	cmd_return show();

	cmd_return setpar();

	cmd_return showpar();

	cmd_return shutdown();

	cmd_return quit(); 
};

}

#endif /*GISTCONSOLE_H_*/
