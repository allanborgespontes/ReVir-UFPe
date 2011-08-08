/// ----------------------------------------*- mode: C++; -*--
/// @file TestConsole.cpp
/// Simple Telnet(able) interface for testing. Allows to control
/// testing via telnet or wrapper ui.
/// ----------------------------------------------------------
/// $Id: TestConsole.cpp 5799 2010-12-15 09:06:14Z bless $
/// $HeadURL: https://svn.tm.kit.edu/nsis/qos-nslp/trunk/src/TestConsole.cpp $
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

#include <sys/poll.h>

#include <unistd.h>
#include <iostream>
#include <string>
#include <list>
#include <map>
#include <sstream>
#include <netdb.h>
#include <errno.h>

#include "sessionid.h"
#include "messages.h"
#include "queuemanager.h"
#include "logfile.h"
#include "info_spec.h"

#include "qspec_ie.h"
#include "qspec_pdu.h"
#include "qspec_param.h"

#include "TestConsole.h"

using namespace protlib::log;
using namespace protlib;
using namespace qspec;

namespace qos_nslp {

TestConsoleParam::TestConsoleParam(uint32 sleep_time, const char *host, const char *port)
    : ThreadParam(sleep_time,"TestConsoleModule"),
    source(message::qaddr_qos_appl_signaling),
    chost(host), cport(port) {}

TestConsole::TestConsole(const TestConsoleParam& p)
    : Thread(p), param(p), prompt("qosnslp>")
{
	commands["usage"] = &TestConsole::usage;
	commands["help"] = &TestConsole::usage;
	commands["?"] = &TestConsole::usage;
	commands["send"] = &TestConsole::send_pdu;
	commands["s"] = &TestConsole::send_pdu;
	commands["Tsend"] = &TestConsole::send_pdu;
	commands["Ts"] = &TestConsole::send_pdu;
	commands["Msend"] = &TestConsole::multi_send;
	commands["Ms"] = &TestConsole::multi_send;
	commands["set"] = &TestConsole::set;
	commands["quit"] = &TestConsole::quit;
	commands["q"] = &TestConsole::quit;
	commands["exit"] = &TestConsole::quit;
	commands["bye"] = &TestConsole::quit;

	send_pdu_template = false;
	sid = NULL;

	DLog("TestConsole", "created");
}

TestConsole::~TestConsole()
{
	DLog("TestConsole", "Destroying TestConsole object");
	QueueManager::instance()->unregister_queue(param.source);
}

void TestConsole::main_loop(uint32 nr) 
{

	DLog("TestConsole", "Starting thread #" << nr);

	struct addrinfo hints = { 0 }, *res;
	int error, on;

	// Setup the control socket
	hints.ai_flags = AI_PASSIVE;
	hints.ai_family = PF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = 0;
	DLog("TestConsole", param.chost << " " << param.cport);
	error = getaddrinfo(param.chost, param.cport, &hints, &res);
	if (error) {
		ERRLog("TestConsole", "getaddrinfo: " << gai_strerror(error));
		return;
	}
	sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	if (sockfd == -1) {
		ERRLog("TestConsole", "socket: " << strerror(errno));
		return;
	}
	on = 1;
	(void) setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on));
	if (bind(sockfd, res->ai_addr, res->ai_addrlen) == -1) {
		ERRLog("TestConsole", "bind: " << strerror(errno));
		return;
	}
	if (listen(sockfd, 1) == -1) {
		ERRLog("TestConsole", "listen: " << strerror(errno));
		return;
	}

	struct pollfd poll_fd = { 0 };
	struct sockaddr clientaddr;
	socklen_t clientaddr_len = sizeof(clientaddr);
	int keep_running = 1;

	DLog("TestConsole", "waiting for client to connect");
	while (keep_running && (get_state() == STATE_RUN)) {
		poll_fd.fd = sockfd;
		poll_fd.events = POLLIN | POLLPRI; 
		poll_fd.revents = 0;

		error = poll(&poll_fd, 1, 500);
		if (error == 0)
			continue;
		if (error == -1) {
			if (errno == EINTR)
				continue;			
			ERRLog("TestConsole", "poll: " << strerror(errno));
			break;
		}
		DLog("TestConsole", "accepting client");
		clientfd = accept(sockfd, &clientaddr, &clientaddr_len);
		if (clientfd == -1) {
			if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)
				continue;
			ERRLog("TestConsole", "accept: " << strerror(errno));
			break;
		}

		out_buf = "# TestConsole on '";
		if (gethostname(buf, 1024) == -1)
			out_buf += "UNKNOWN";
		else
			out_buf += (char *)buf;
		out_buf += "'\n";
		out_buf += prompt;

		send();

		args.clear();
// INNER LOOP - no extra indent
	while ( keep_running && (clientfd != -1) && (get_state()==STATE_RUN)) {
		poll_fd.fd = clientfd;
		error = poll(&poll_fd, 1, 500);
		if (error == 0)
			continue;
		if (error == -1) {
			if (errno == EINTR)
				continue;			
			ERRLog("TestConsole", "poll: " << strerror(errno));
			break;
		}
		recv();
		string::size_type pos;
		while ((pos = in_buf.find_first_of(" \n\t")) != string::npos) {
			cmd_return ret = UNSET;
			switch (in_buf[pos]) {
			case ' ':
			case '\t':
				if (pos == 0) {
					in_buf.erase(0, 1);
					break;
				}
				args.push_back(in_buf.substr(0, pos));
				in_buf.erase(0, pos + 1);
				break;
			case '\n':
				if (pos == 1) {
					in_buf.erase(0, 2);
					break;
				}
				args.push_back(in_buf.substr(0, pos - 1));
				in_buf.erase(0, pos + 1);
				typedef map<string, cmd_func_t>::iterator CI;
				pair<CI,CI> cr = commands.equal_range(args.front());
				if (cr.first == cr.second) {
					out_buf = "# Did you mean '" + (*cr.first).first + "'?\n";
					send();
					ret = OK;
					break;
				}
				if ((*cr.first).first != args.front()) {
					out_buf = "# Unknown command '" + (*cr.first).first + "', try 'help'.";
					send();
				}
				ret = (this->*((*cr.first).second))();
				break;
			}
			if (ret == ERROR) {
				out_buf = "# ERROR\n";
				send();
				args.clear();
				in_buf = "";
				break;
			} else if (ret == OK) {
				args.clear();
			}
		}
		out_buf=prompt;
                send();
	}
// INNER LOOP - no extra indent	
	}
	DLog("TestConsole", "Thread #" << nr << " stopped");
}

int
TestConsole::send()
{
	int c1 = out_buf.length();
	int c2 = write(clientfd, out_buf.c_str(), c1);

	if (c2 == -1)
		clientfd = -1;

	return (c1 - c2);
}

int
TestConsole::recv()
{
	int c1 = read(clientfd, buf, sizeof(buf));

	if (buf[0] == EOF) {
		in_buf = "";
		close(clientfd);
		clientfd = -1;
		return (-1);
	}

	if (c1 == -1)
		clientfd = -1;

	buf[c1] = '\0';

	in_buf = buf;

	return (c1);
}

TestConsole::cmd_return
TestConsole::usage()
{
	map<string, cmd_func_t>::const_iterator ci = commands.begin();

	out_buf = "# ";

	while(ci != commands.end()) {
		out_buf += (*ci).first + " ";
		ci++;
	}
	out_buf += "\n";
	send();

	return OK;
}

#define NEXT_ARG()	do {				\
	if ((++aa) == args.end()) {			\
		while (arg_names[i] != NULL) {	\
			out_buf += arg_names[i++];	\
			out_buf += " | ";			\
		}								\
		out_buf += "\n";				\
		send();							\
		return MORE_ARGS;				\
	}									\
	i++;								\
} while (0)

#define	ARG_ERROR()	do {			\
		out_buf += arg_names[i-1];	\
		out_buf += " '";			\
		out_buf += (*aa) + "'\n";	\
		send();						\
		return ERROR;				\
} while (0)

QoS_Appl_Msg *
TestConsole::create_msg(uint128 sid)
{
	assert(send_pdu_template);

	/* create qspec pdu */
	qspec_pdu* q_pdu = new qspec_pdu(ms_sender_initiated_reservations, 1);
	qspec::qspec_object *qos_desired = new qspec::qspec_object(ot_qos_desired);
	qos_desired->set_parameter(new t_mod(band, 100000, band, 1500));
	qos_desired->set_parameter(new qspec::admission_priority(ap_high_priority_flow));
	q_pdu->set_object(qos_desired);

	QoS_Appl_Msg *res_msg = new QoS_Appl_Msg();	
	res_msg->set_pdu_type(pdu_format);
	res_msg->set_direction(downstream);
	res_msg->set_reliable(reliable);

	res_msg->set_qspec(q_pdu);

	if (flags & PDU_FLAG_REPLACE) { res_msg->set_replace_flag(true); }
	if (flags & PDU_FLAG_ACK) { res_msg->set_acknowledge_flag(true); }
	if (flags & PDU_FLAG_SCOPE) { res_msg->set_scoping_flag(true); }
	if (flags & PDU_FLAG_TEAR) { res_msg->set_tear_flag(true); }
	if (flags & PDU_FLAG_INIT) { res_msg->set_reserve_init_flag(true); }


	res_msg->set_sid(sid);
	res_msg->set_source_addr(src_a);
	res_msg->set_dest_addr(dest_a);
	if (rrii) { res_msg->set_request_response_with_rii(true); }

	return res_msg;
}

TestConsole::cmd_return
TestConsole::send_pdu()
{
	static const char *const arg_names[] = {
		"src [<ip>]",
		"dest [<ip>]",
		"direction [up/dow]",
		"reliable [c/d]",
		"reserve w/ RII [yes/no]",
		"bandwidth [<float>]",
		"pdu [query/reserve]",
		"flags [R A S T I]",
		NULL
	};
	list<string>::const_iterator aa = args.begin();
	int i = 0;

	out_buf = "# ";

	if (((*aa)[0] == 'T') && send_pdu_template) {
		flags |= PDU_FLAG_TEAR;
		goto doit;
	}

	send_pdu_template = false;

	NEXT_ARG();				// src_ip
	if (!src_a.set_ip((*aa).c_str()))
		ARG_ERROR();

	NEXT_ARG();				// dest_ip
	if (!dest_a.set_ip((*aa).c_str()))
		ARG_ERROR();

	NEXT_ARG();				// direction
	if (string("down").find((*aa)) != string::npos) {
		downstream = 1;
	} else if (string("up").find((*aa)) != string::npos) {
		downstream = 0;
	} else
		ARG_ERROR();

	NEXT_ARG();				// reliable
	if (string("c").find((*aa)) != string::npos) {
		reliable = 1; // C-Mode
	} else if (string("d").find((*aa)) != string::npos) {
		reliable = 0; // D-Mode
	} else
		ARG_ERROR();

	NEXT_ARG();				// res w/ RII
	if (string("yes").find((*aa)) != string::npos) {
		rrii = 1;
	} else if (string("no").find((*aa)) != string::npos) {
		rrii = 0;
	} else
		ARG_ERROR();

	NEXT_ARG();				// bandwidth
	band = strtod((*aa).c_str(), NULL);

	NEXT_ARG();				// pdu_format
	if (string("query").find((*aa)) != string::npos) {
		pdu_format = known_nslp_pdu::QUERY;
	} else if (string("reserve").find((*aa)) != string::npos) {
		pdu_format = known_nslp_pdu::RESERVE;
	} else
		ARG_ERROR();

	NEXT_ARG();				// flags [RASTI]
	flags = 0;
	if (strchr((*aa).c_str(), 'R') != NULL || strchr((*aa).c_str(), 'r') != NULL) {
		flags |= PDU_FLAG_REPLACE;
	}
	if (strchr((*aa).c_str(), 'A') != NULL || strchr((*aa).c_str(), 'a') != NULL) {
		flags |= PDU_FLAG_ACK;
	}
	if (strchr((*aa).c_str(), 'S') != NULL || strchr((*aa).c_str(), 's') != NULL) {
		flags |= PDU_FLAG_SCOPE;
	}
	if (strchr((*aa).c_str(), 'T') != NULL || strchr((*aa).c_str(), 't') != NULL) {
		flags |= PDU_FLAG_TEAR;
	}
	if (strchr((*aa).c_str(), 'I') != NULL || strchr((*aa).c_str(), 'i') != NULL) {
		flags |= PDU_FLAG_INIT;
	}

doit:
	DLog("TestConsole", "src=" << src_a << " dst=" << dest_a <<
		" downstream=" << downstream << " reliable=" << reliable << 
		" rrii=" << rrii << " band=" << band);

	if(!send_pdu_template) {
		if(sid) {
			delete sid;
		}
		sid = new ntlp::sessionid();
		sid->generate_random();
	}

	send_pdu_template = true;

	create_msg(*sid)->send_or_delete();

	return OK;
}

TestConsole::cmd_return
TestConsole::multi_send()
{
	static const char *const arg_names[] = {
		"count [int]",
		"delay [ms]",
		"same sid [1|0]",
		NULL
	};
	list<string>::const_iterator aa = args.begin();
	int i = 0;

	out_buf = "# ";

	if (!send_pdu_template) {
		out_buf += "Set a template first by using the `set template' command!\n";
		send();
		return ERROR;
	}

	int count, delay, samesid;

	NEXT_ARG();				// count
	count = atoi((*aa).c_str());
	if (count <= 0)
		ARG_ERROR();

	NEXT_ARG();				// delay
	delay = atoi((*aa).c_str());
	if (delay < 0)
		ARG_ERROR();

	NEXT_ARG();				// samesid
	samesid = atoi((*aa).c_str());

	out_buf = "# Sending ...\n";
	send();

	DLog("TestConsole", "src=" << src_a << " dst=" << dest_a <<
		" downstream=" << downstream << " reliable=" << reliable << 
		" rrii=" << rrii << " band=" << band);

	send_pdu_template = true;

	ntlp::sessionid *sid = new ntlp::sessionid();
	for (int i = 0; i < count; ++i) {
		if (!samesid)
			sid->generate_random();

		create_msg(*sid)->send_or_delete();

		if (delay)
			usleep(delay * 1000);
	}

	return OK;
}

TestConsole::cmd_return
TestConsole::set_template(list<string>::const_iterator& aa)
{
	static const char *arg_names[] = {
		"src [<ip>]",
                "dest [<ip>]",
                "direction [upstream/downstream]",
		"reliable [c/d]",
                "reserve w/ RII [yes/no]",
                "bandwidth [<float>]",
                "pdu [QUERY/RESERVE]",
                "flags [R A S T I]",
		NULL
	};
	int i = 0;

	out_buf = "# setting template ";

	send_pdu_template = false;

	NEXT_ARG();				// src_ip
	if (!src_a.set_ip((*aa).c_str()))
		ARG_ERROR();

	NEXT_ARG();				// dest_ip
	if (!dest_a.set_ip((*aa).c_str()))
		ARG_ERROR();

	NEXT_ARG();				// direction
	if (string("down").find((*aa)) != string::npos) {
		downstream = 1;
	} else if (string("up").find((*aa)) != string::npos) {
		downstream = 0;
	} else
		ARG_ERROR();

	NEXT_ARG();				// reliable
	if (string("c").find((*aa)) != string::npos) {
		reliable = 1; // C-Mode
	} else if (string("d").find((*aa)) != string::npos) {
		reliable = 0; // D-Mode
	} else
		ARG_ERROR();

	NEXT_ARG();				// res w/ RII
	if (string("yes").find((*aa)) != string::npos) {
		rrii = 1;
	} else if (string("no").find((*aa)) != string::npos) {
		rrii = 0;
	} else
		ARG_ERROR();

	NEXT_ARG();				// bandwidth
	band = strtod((*aa).c_str(), NULL);

	NEXT_ARG();				// pdu_format
	if (string("query").find((*aa)) != string::npos) {
		pdu_format = known_nslp_pdu::QUERY;
	} else if (string("reserve").find((*aa)) != string::npos) {
		pdu_format = known_nslp_pdu::RESERVE;
	} else
		ARG_ERROR();

	NEXT_ARG();				// flags [RASTI]
	flags = 0;
	if (strchr((*aa).c_str(), 'R') != NULL || strchr((*aa).c_str(), 'r') != NULL) {
		flags |= PDU_FLAG_REPLACE;
	}
	if (strchr((*aa).c_str(), 'A') != NULL || strchr((*aa).c_str(), 'a') != NULL) {
		flags |= PDU_FLAG_ACK;
	}
	if (strchr((*aa).c_str(), 'S') != NULL || strchr((*aa).c_str(), 's') != NULL) {
		flags |= PDU_FLAG_SCOPE;
	}
	if (strchr((*aa).c_str(), 'T') != NULL || strchr((*aa).c_str(), 't') != NULL) {
		flags |= PDU_FLAG_TEAR;
	}
	if (strchr((*aa).c_str(), 'I') != NULL || strchr((*aa).c_str(), 'i') != NULL) {
		flags |= PDU_FLAG_INIT;
	}

	out_buf+= "\n";
	send();
	send_pdu_template = true;

	return OK;
}

TestConsole::cmd_return
TestConsole::set()
{
	static const char *alt_arg_names[] = {
		"template",
		NULL
	};

	list<string>::const_iterator aa = args.begin();

	bool unknownarg = (++aa == args.end()) ? true : false;

	out_buf= "# setting ";

	while (aa != args.end()) {
		if (*aa == "template")
			return set_template(aa);
		else {
			out_buf= "# I don't know how to set this: " + *aa;
			unknownarg= true;
		}
		aa++;

		if (unknownarg) {
			out_buf +="# possible parameters to set:";
			unsigned int i= 0;
			while (alt_arg_names[i] != NULL) {
				out_buf += ' ';
				out_buf += alt_arg_names[i];
				i++;
			}
		}

		out_buf +="\n";
		send();

		return OK;
	} // end while

	return OK;
}

#undef ARG_ERROR
#undef NEXT_ARG

TestConsole::cmd_return
TestConsole::quit()
{
	out_buf = "# Goodbye!\n";
	send();

	close(clientfd);
	clientfd = -1;

	return OK;
}

}
