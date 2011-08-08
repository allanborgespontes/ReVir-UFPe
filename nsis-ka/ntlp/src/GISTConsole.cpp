/// ----------------------------------------*- mode: C++; -*--
/// @file GISTConsole.cpp
/// Simple Telnet(able) interface for testing and diagnosis
/// This GISTConsole allows to control testing via telnet or wrapper ui.
/// ----------------------------------------------------------
/// $Id: GISTConsole.cpp 6296 2011-06-21 14:10:21Z bless $
/// $HeadURL: https://svn.tm.kit.edu/nsis/ntlp/trunk/src/GISTConsole.cpp $
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
#include "apimessage.h"
#include "configfile.h"

#include "GISTConsole.h"
#include "ntlp_starter.h"
#include "ntlp_statemodule.h"
#include "gist_conf.h"

#include "mri_est.h"

using namespace protlib::log;
using namespace protlib;
using namespace ntlp;

extern Thread* global_ntlpthread_p;

namespace ntlp {
const char *GISTConsoleDefaultPort= "40023";
}

struct apiargs_t {
  hostaddress sourceaddress;
  hostaddress destaddress;
  
  mri_pathcoupled pc_mri;
  mri_explicitsigtarget est_mri;

  uint32 nslpmsghandle;
  uint32 nslpid;
  sessionid mysid;
  uint32 siihandle;
  tx_attr_t tx_attr;
  uint32 timeout;
  uint16 ip_ttl;
  uint32 ghc;
  bool   downstream;

  string nslpdata;

  apiargs_t() :
    sourceaddress("0.0.0.0"),
    destaddress("0.0.0.0"),
    pc_mri(sourceaddress, 32, destaddress, 32, true),
    est_mri(pc_mri,sourceaddress,destaddress),
    nslpmsghandle(1), 
    nslpid(0), 
    siihandle(0),
    timeout(180),
    ip_ttl(64),
    ghc(32),
    downstream(true),
    nslpdata("Hello World!")
  {     
    tx_attr.secure= 0;
    tx_attr.reliable= 0;
  };

  string print() const;

} apiargs;


string
apiargs_t::print() const
{
  ostringstream os;
  os << " APIargs" << endl 
     << " handle: " << apiargs.nslpmsghandle << endl 
     << " nslpid: " << apiargs.nslpid << endl
     << " sid: " << apiargs.mysid << endl
     << " flow src: " << apiargs.sourceaddress << endl
     << " flow dst: " << apiargs.destaddress << endl
     << " direction: " << (apiargs.downstream ? "downstream" : "upstream") << endl
     << " pc mri:" << apiargs.pc_mri << endl
     << " est mri:" << apiargs.est_mri << endl
     << " sii handle:" << apiargs.siihandle << endl
     << " transfer attributes: " << (apiargs.tx_attr.reliable ? " reliable" : " unreliable") << ", "
                                 << (apiargs.tx_attr.secure ? " secure" : " not secure") << endl
     << " timeout: " << dec << apiargs.timeout << endl
     << " IP ttl: " << apiargs.ip_ttl << endl
     << " Gist Hop Count:" << apiargs.ghc << endl
     << " Data:" << apiargs.nslpdata << endl;
  return os.str();
}


GISTConsoleParam::GISTConsoleParam(pid_t main_pid, message::qaddr_t source, const char *host, uint32 sleep_time, const char *port)
    : ThreadParam(sleep_time,"GISTConsoleModule"),
    main_pid(main_pid),
    source(source),
    chost(host), cport(port)
    {}

GISTConsole::GISTConsole(const GISTConsoleParam& p)
	: Thread(p), param(p), prompt("gist>")
{
	commands["usage"] = &GISTConsole::usage;
	commands["help"] = &GISTConsole::usage;
	commands["?"] = &GISTConsole::usage;
	commands["send"] = &GISTConsole::send_pdu;
	commands["s"] = &GISTConsole::send_pdu;
	commands["Tsend"] = &GISTConsole::send_pdu;
	commands["Ts"] = &GISTConsole::send_pdu;
	commands["Msend"] = &GISTConsole::multi_send;
	commands["Ms"] = &GISTConsole::multi_send;
	commands["estsend"] = &GISTConsole::send_est_pdu;
	commands["set"] = &GISTConsole::set;
	commands["show"] = &GISTConsole::show;
	commands["setpar"] = &GISTConsole::setpar;
	commands["showpar"] = &GISTConsole::showpar;
	commands["sh"] = &GISTConsole::show;
	commands["quit"] = &GISTConsole::quit;
	commands["q"] = &GISTConsole::quit;
	commands["exit"] = &GISTConsole::quit;
	commands["bye"] = &GISTConsole::quit;
	commands["shutdown"] = &GISTConsole::shutdown;
	commands["kill"] = &GISTConsole::shutdown;
	commands["saveconfig"]= &GISTConsole::saveconfig;

	send_pdu_template = false;

	QueueManager::instance()->register_queue(get_fqueue(), p.source);

	// assumes that the configpar_repository is already created and initialized
	collectParNames();

	Log(DEBUG_LOG, LOG_NORMAL, "GISTConsole", "created");
}


void
GISTConsole::collectParNames()
{
	unsigned int max_pars= configpar_repository::instance()->getRealmSize(gist_realm);
	configparBase* cfgpar= NULL;
	for (unsigned int i= 0; i < max_pars; i++) {
		try {
			// get config par with id i
			cfgpar= configpar_repository::instance()->getConfigPar(gist_realm, i);
			// store mapping from configpar name to parameter id i
			if (cfgpar) 
				parname_map[cfgpar->getName()]= i;
		} // end try
		catch (configParExceptionParNotRegistered) {
			cfgpar= NULL;
			continue;
		} // end catch
		catch (...) {
		  throw;
		}
	} // end for
}


GISTConsole::~GISTConsole()
{
	Log(DEBUG_LOG, LOG_NORMAL, "GISTConsole", "Destroying GISTConsole object");
	QueueManager::instance()->unregister_queue(param.source);
}


void
GISTConsole::main_loop(uint32 nr) {

	Log(DEBUG_LOG, LOG_NORMAL, "GISTConsole", "Starting thread #" << nr);

	struct addrinfo hints = { 0 }, *res;
	int error, on;

	// Setup the control socket
	hints.ai_flags = AI_PASSIVE;
	hints.ai_family = PF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = 0;
	Log(DEBUG_LOG, LOG_NORMAL, "GISTConsole", param.chost << " " << param.cport);
	error = getaddrinfo(param.chost, param.cport, &hints, &res);
	if (error) {
		Log(ERROR_LOG, LOG_ALERT, "GISTConsole", "getaddrinfo: " << gai_strerror(error));
		return;
	}
	sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	if (sockfd == -1) {
		Log(ERROR_LOG, LOG_ALERT, "GISTConsole", "socket: " << strerror(errno));
		return;
	}
	on = 1;
	(void) setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on));
	if (bind(sockfd, res->ai_addr, res->ai_addrlen) == -1) {
		ERRLog("GISTConsole", "bind: " << strerror(errno));
		return;
	}
	if (listen(sockfd, 1) == -1) {
		ERRLog("GISTConsole", "listen: " << strerror(errno));
		return;
	}

	struct pollfd poll_fd = { 0 };
	struct sockaddr clientaddr;
	socklen_t clientaddr_len = sizeof(clientaddr);

	DLog("GISTConsole", "waiting for client to connect");
	while (get_state()==STATE_RUN) {
		poll_fd.fd = sockfd;
		poll_fd.events = POLLIN | POLLPRI; 
		poll_fd.revents = 0;

		error = poll(&poll_fd, 1, 500);
		if (error == 0)
			continue;
		if (error == -1) {
			if (errno == EINTR)
				continue;			
			Log(ERROR_LOG, LOG_ALERT, "GISTConsole", "poll: " << strerror(errno));
			break;
		}
		Log(DEBUG_LOG, LOG_NORMAL, "GISTConsole", "accepting client");
		clientfd = accept(sockfd, &clientaddr, &clientaddr_len);
		if (clientfd == -1) {
			if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)
				continue;

			Log(ERROR_LOG, LOG_ALERT, "GISTConsole", "accept: " << strerror(errno));
			break;
		}

		out_buf = "# GISTConsole for GIST on '";
		if (gethostname(buf, 1024) == -1)
			out_buf += "UNKNOWN";
		else
			out_buf += (char *)buf;
		out_buf += "'\n";
		out_buf += "# type ?<Return> for help.\n";
		out_buf += prompt;

		send();

		args.clear();
// INNER LOOP - no extra indent
	while (get_state()==STATE_RUN && clientfd != -1) {
		poll_fd.fd = clientfd;
		error = poll(&poll_fd, 1, 500);
		if (error == 0)
			continue;
		if (error == -1) {
			if (errno == EINTR)
				continue;			
			Log(ERROR_LOG, LOG_ALERT, "GISTConsole", "poll: " << strerror(errno));
			break;
		}

		recv();
		string::size_type pos;
		while ((pos = in_buf.find_first_of(" \n\t")) != string::npos) {
			cmd_return ret = UNSET;
			switch (in_buf[pos]) {
			  // whitespace is a separator
			case ' ':
			case '\t':
				if (pos == 0) {
					in_buf.erase(0, 1);
					break;
				}
				args.push_back(in_buf.substr(0, pos));
				in_buf.erase(0, pos + 1);
				break;
			  // newline is enter
			case '\n':
				if (pos == 1) {
					in_buf.erase(0, 2);
					break;
				}
				args.push_back(in_buf.substr(0, pos - 1));
				in_buf.erase(0, pos + 1);
				typedef map<string, cmd_func_t>::iterator CI;
				pair<CI,CI> cr = commands.equal_range(args.front());
				if (cr.first == commands.end())
				{ // nothing found
					out_buf = "# Unknown command '" + args.front() + "', try 'help'.\n";
					send();
					ret = ERROR;
					break;
										
				}
				if (cr.first == cr.second) {
					out_buf = "# Did you mean '" + (cr.first)->first + "'?\n";
					send();
					ret = OK;
					break;
				}
				if ((cr.first)->first != args.front()) {
					out_buf = "# Unknown command '" + (cr.first)->first + "', try 'help'.\n";
					send();
					ret = ERROR;
					break;
				}
				// exact match
				// call the assigned function for the given command name
				ret = (this->*((cr.first)->second))();
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
	Log(DEBUG_LOG, LOG_NORMAL, "GISTConsole", "Thread #" << nr << " stopped");
}

int
GISTConsole::send()
{
	int c1 = out_buf.length();
	int c2 = write(clientfd, out_buf.c_str(), c1);

	if (c2 == -1)
		clientfd = -1;

	return (c1 - c2);
}

int
GISTConsole::recv()
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

GISTConsole::cmd_return
GISTConsole::usage()
{
  list<string>::const_iterator aa = args.begin();

  out_buf="# ";
  bool emptyarg= (++aa == args.end()) ? true : false;
  if (emptyarg)
  {
	map<string, cmd_func_t>::const_iterator ci = commands.begin();

	out_buf = "# available commands\n# ";

	while(ci != commands.end()) {
		out_buf += (*ci).first + " ";
		ci++;
	}
  }
  else
  {
	  string parhelp= helppar(*aa);
	  //  check for command or parameter name
	  if (parhelp.empty() == false)
	  {
		  out_buf += parhelp;
	  }
	  else
	  {
	        out_buf += "nothing special known about " + *aa + ", sorry can't help you any further";
	  }
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


APIMsg*
GISTConsole::create_api_msg(uint128 sid, bool use_est_MRI)
{
  assert(send_pdu_template);


  APIMsg* apimsg= new APIMsg();

  apiargs.mysid= sid;
  bool downstream= apiargs.pc_mri.get_downstream();
  if (downstream ^ apiargs.downstream)
    apiargs.pc_mri.invertDirection();

  out_buf = "# sending API call SendMessage:\n";
  out_buf += apiargs.print();
  send();

  apimsg->set_sendmessage(new nslpdata(reinterpret_cast<const uchar *>(apiargs.nslpdata.c_str()), apiargs.nslpdata.length()),
			  apiargs.nslpmsghandle, 
			  apiargs.nslpid, 
			  new sessionid(apiargs.mysid),
			  use_est_MRI ? static_cast<mri *>(new mri_explicitsigtarget(apiargs.est_mri)) : static_cast<mri *>(new mri_pathcoupled(apiargs.pc_mri)), 
			  apiargs.siihandle, 
			  apiargs.tx_attr, 
			  apiargs.timeout, 
			  apiargs.ip_ttl, 
			  apiargs.ghc);
  apimsg->set_source(message::qaddr_api_1);

  return apimsg;
}


GISTConsole::cmd_return
GISTConsole::set_template(list<string>::const_iterator& aa)
{
	static const char *arg_names[] = {
	  "nslpid",
	  "src [ip]",
	  "dest [ip]",
	  "reliable [yes/no]",
	  "secure [yes/no]",
		NULL
	};
	int i = 0;

	out_buf = "# setting template ";

	send_pdu_template = false;

	NEXT_ARG();				// nslpid
	apiargs.nslpid = atoi((*aa).c_str());


	NEXT_ARG();				// src_ip
	if (!src_a.set_ip((*aa).c_str()))
		ARG_ERROR();
	else 
	  apiargs.sourceaddress=src_a;

	NEXT_ARG();				// dest_ip
	if (!dest_a.set_ip((*aa).c_str()))
		ARG_ERROR();
	else
	  apiargs.destaddress=dest_a;
	
	NEXT_ARG();				// reliable
	if (string("yes").find((*aa)) != string::npos) {
		apiargs.tx_attr.reliable = true;
	} else if (string("no").find((*aa)) != string::npos) {
		apiargs.tx_attr.reliable = false ;
	} else
		ARG_ERROR();

	NEXT_ARG();				// secure
	if (string("yes").find((*aa)) != string::npos) {
		apiargs.tx_attr.secure = true;
	} else if (string("no").find((*aa)) != string::npos) {
		apiargs.tx_attr.secure = false ;
	} else
		ARG_ERROR();

	out_buf+= "\n";
	send();
	send_pdu_template = true;

	return OK;
}


GISTConsole::cmd_return
GISTConsole::send_pdu()
{
	static const char *arg_names[] = {
	  "nslpid",
	  "src [ip]",
	  "dest [ip]",
	  "reliable [yes/no]",
	  "secure [yes/no]",
		NULL
	};
	list<string>::const_iterator aa = args.begin();
	int i = 0;

	out_buf = "# ";

	if (((*aa)[0] == 'T') && send_pdu_template)
		goto doit;

	send_pdu_template = false;

	NEXT_ARG();				// nslpid
	apiargs.nslpid = atoi((*aa).c_str());


	NEXT_ARG();				// src_ip
	if (!src_a.set_ip((*aa).c_str()))
		ARG_ERROR();
	else 
	  apiargs.sourceaddress=src_a;

	NEXT_ARG();				// dest_ip
	if (!dest_a.set_ip((*aa).c_str()))
		ARG_ERROR();
	else
	  apiargs.destaddress=dest_a;
	
	NEXT_ARG();				// reliable
	if (string("yes").find((*aa)) != string::npos) {
		apiargs.tx_attr.reliable = true;
	} else if (string("no").find((*aa)) != string::npos) {
		apiargs.tx_attr.reliable = false ;
	} else
		ARG_ERROR();

	NEXT_ARG();				// secure
	if (string("yes").find((*aa)) != string::npos) {
		apiargs.tx_attr.secure = true;
	} else if (string("no").find((*aa)) != string::npos) {
		apiargs.tx_attr.secure = false ;
	} else
		ARG_ERROR();

	send_pdu_template = true;

doit:
	apiargs.pc_mri.set_sourceaddress(apiargs.sourceaddress);
	apiargs.pc_mri.set_destaddress(apiargs.destaddress);

	uint128 sid_uint128= apiargs.mysid;
	
	create_api_msg(sid_uint128)->send_to(message::qaddr_coordination);

	return OK;
}

/// send to explicit signaling target
GISTConsole::cmd_return
GISTConsole::send_est_pdu()
{
	static const char *const arg_names[] = {
	  "nslpid",
	  "sigsrc [ip]",
	  "sigdest [ip]",
	  "flowsrc [ip]",
	  "flowdest [ip]",
	  "reliable [yes/no]",
	  "secure [yes/no]",
		NULL
	};
	list<string>::const_iterator aa = args.begin();
	int i = 0;

	out_buf = "# ";

	if (((*aa)[0] == 'T') && send_pdu_template)
		goto doit;

	send_pdu_template = false;

	NEXT_ARG();				// nslpid
	apiargs.nslpid = atoi((*aa).c_str());


	NEXT_ARG();				// src_ip
	if (!src_a.set_ip((*aa).c_str()))
		ARG_ERROR();
	else 
	  apiargs.est_mri.set_origin_sig_address(src_a);

	NEXT_ARG();				// dest_ip
	if (!dest_a.set_ip((*aa).c_str()))
		ARG_ERROR();
	else
	  apiargs.est_mri.set_dest_sig_address(dest_a);

	NEXT_ARG();				// src_ip
	if (!src_a.set_ip((*aa).c_str()))
		ARG_ERROR();
	else 
	  apiargs.sourceaddress=src_a;

	NEXT_ARG();				// dest_ip
	if (!dest_a.set_ip((*aa).c_str()))
		ARG_ERROR();
	else
	  apiargs.destaddress=dest_a;
	
	NEXT_ARG();				// reliable
	if (string("yes").find((*aa)) != string::npos) {
		apiargs.tx_attr.reliable = true;
	} else if (string("no").find((*aa)) != string::npos) {
		apiargs.tx_attr.reliable = false ;
	} else
		ARG_ERROR();

	NEXT_ARG();				// secure
	if (string("yes").find((*aa)) != string::npos) {
		apiargs.tx_attr.secure = true;
	} else if (string("no").find((*aa)) != string::npos) {
		apiargs.tx_attr.secure = false ;
	} else
		ARG_ERROR();

	send_pdu_template = true;

doit:
	apiargs.pc_mri.set_sourceaddress(apiargs.sourceaddress);
	apiargs.pc_mri.set_destaddress(apiargs.destaddress);
	apiargs.est_mri.set_mri_pc(apiargs.pc_mri);

	uint128 sid_uint128= apiargs.mysid;
	
	create_api_msg(sid_uint128, true)->send_to(message::qaddr_coordination);

	return OK;
}


GISTConsole::cmd_return
GISTConsole::show()
{

  static const char *alt_arg_names[] = {
    "template",
    "config",
    "rtable",
    NULL
  };
  
  list<string>::const_iterator aa = args.begin();
  
  bool unknownarg= (++aa == args.end()) ? true : false;

  while (aa != args.end())
  {
    if (*aa == "template")
    {
      out_buf = "# showing template";
      out_buf += apiargs.print();
    }
    else
    if (*aa == "config")
    {
      out_buf = "# showing global config parameters:\n";
      out_buf += gconf.to_string();
    }
    else
    if (*aa == "rtable")
    {
      out_buf = "# showing message routing state table:\n";
      if (global_ntlpstarterthread_p->getStatemodule())
      {
	out_buf += global_ntlpstarterthread_p->getStatemodule()->getParam().rt.print_routingtable();
      }
      else
      {
	out_buf += "Statemodule not available yet!\n";
      }
    }
    else
    if (*aa == "addresses")
    {
      out_buf = "# showing configured addresses:\n";
      AddressList& addresses= global_ntlpstarterthread_p->getStatemodule()->getParam().addresses;
      AddressList::addrlist_t *alist = addresses.get_addrs(AddressList::ConfiguredAddr_P);
      if (alist != 0) {
	      AddressList::addrlist_t::iterator it;
	      for (it = alist->begin(); it != alist->end(); it++) {
		      out_buf += it->get_ip_str();
	              out_buf += "\n";
	      } // end for
      }
      delete alist;
    }

    else
    {
      unknownarg= true;
      break;
    }
    aa++;
  } // end while
  
  if (unknownarg)
  {
    out_buf= "# please specify what to show!\n# show ";
    unsigned int i= 0;
    while (alt_arg_names[i] != NULL) {
      out_buf += alt_arg_names[i++];
      out_buf += " | ";
    }
  }

  out_buf += "\n";
  send();
  
  return OK;
}


GISTConsole::cmd_return
GISTConsole::setsid(list<string>::const_iterator& aa)
{
  if (aa != args.end())
  {
    if (*aa == "new")
    {
      apiargs.mysid.generate_random();
    }
    else
    {
      stringstream s;
      s << *aa;
      s >> apiargs.mysid;
    }
  }
  else
  {
	  out_buf="# missing argument: either specify 'new' or a session id value";
	  return MORE_ARGS;
  }

  return OK;
}


GISTConsole::cmd_return
GISTConsole::set()
{
  static const char *alt_arg_names[] = {
    "sid",
    "flowsrc",
    "flowdst",
    "sigsrc",
    "sigdst",
    "nslpid",
    "timeout",
    "ipttl",
    "ghc",
    "data",
    "upstream",
    "downstream",
    "reliable",
    "unreliable",
    "secure",
    "unsecure",
    "template",
    NULL
  };

  list<string>::const_iterator aa = args.begin();
  
  bool unknownarg= (++aa == args.end()) ? true : false;

  out_buf= "# setting ";

  while (aa != args.end())
  {
    if (*aa == "sid")
    {
      aa++;
      if (aa != args.end())
      {
	      setsid(aa);
	      out_buf += "sid to " + apiargs.mysid.to_string();
      }
      else
      {
	      out_buf += "sid failed, missing value ";
	      break;
      }
    }
    else
    if (*aa == "nslpid")
    {
      aa++;				// nslpid
      if (aa != args.end())
      {
	apiargs.nslpid = atoi((*aa).c_str());
	out_buf += "nslpid to "+ apiargs.nslpid;
      }
      else
      {
	      out_buf += " nslpid failed, missing value ";
	      break;
      }
    }
    else
    if (*aa == "flowsrc")
    {	
      aa++;				// src_ip
      if (!src_a.set_ip((*aa).c_str()))
      {
	break;
      }
      else 
      {
	apiargs.sourceaddress=src_a;
	out_buf += "flowsrc to ";
	out_buf	+= apiargs.sourceaddress.get_ip_str();
      }
    }
    else
    if (*aa == "flowdst")
    {	
      aa++;
      if (!dest_a.set_ip((*aa).c_str()))
      {
	break;
      }
      else
      {
	apiargs.destaddress=dest_a;
	out_buf += "flowdst to ";
	out_buf+= apiargs.destaddress.get_ip_str();
      }
    }
    else
    if (*aa == "sigsrc")
    {	
      aa++;				// sig src_ip
      hostaddress sigorigin;

      if (!sigorigin.set_ip((*aa).c_str()))
      {
	break;
      }
      else
      {
	apiargs.est_mri.set_origin_sig_address(sigorigin);
	out_buf += "sigsrc to ";
	out_buf += sigorigin.get_ip_str();
      }
    }
    else
    if (*aa == "sigdst")
    {
      aa++;
      hostaddress sigdest;
      
      if (!sigdest.set_ip((*aa).c_str())) // sig dest_ip
      {
	break;
      }
      else
      {
	apiargs.est_mri.set_dest_sig_address(sigdest);
	out_buf += "sigdst to ";
	out_buf += sigdest.get_ip_str();
      }
    }
    else
    if (*aa == "upstream")
    {	
      apiargs.downstream= false;
      out_buf += "MRI direction to upstream";
    }
    else
    if (*aa == "downstream")
    {	
      apiargs.downstream= true;
      out_buf += "MRI direction to downstream";
    }
    else
    if (*aa == "reliable")
    {	
	    apiargs.tx_attr.reliable= true;
	    out_buf += "transfer mode to reliable";
    }
    else
    if (*aa == "unreliable")
    {	
	    apiargs.tx_attr.reliable= false;
	    out_buf += "transfer mode to unreliable";
    }
    else
    if (*aa == "secure")
    {	
	    apiargs.tx_attr.secure= true;
	    out_buf += "transfer mode to secure";
    }
    else
    if (*aa == "unsecure")
    {	
	    apiargs.tx_attr.secure= false;
	    out_buf += "transfer mode to unsecure";
    }
    else
    if (*aa == "template")
    {	
	    return set_template(aa);
    }
    else
    {
	    out_buf= "# I don't know how to set this: " + *aa;
	    unknownarg= true;
    }
    aa++;
  }

  if (unknownarg)
  {
    out_buf +="# possible parameters to set:";
    unsigned int i= 0;
    while (alt_arg_names[i] != NULL)
    {
      out_buf += ' ';
      out_buf += alt_arg_names[i];
      i++;
    }
  }
    
  out_buf +="\n";
  send();

  return OK;
}


std::string
GISTConsole::parnamelist() const
{
	string parnames;
	for (parname_map_t::const_iterator cit= parname_map.begin(); cit !=  parname_map.end(); cit++)
	{
		if (parnames.length() > 0)
			parnames+= ", ";
		parnames+= cit->first; 
	} // end for
	return parnames;
}


std::string
GISTConsole::helppar(string parname)
{
	string helppartext;
	typedef parname_map_t::const_iterator PNI;
	pair<PNI,PNI> pn_pair_cit = parname_map.equal_range(parname);

	configpar_id_t configparid= 0;
	if ( pn_pair_cit.first != parname_map.end() ) // found entry for parameter
	{
		if (pn_pair_cit.first->first != parname)
		{
			helppartext+= "nearest match, ";
		}
		configparid= (pn_pair_cit.first)->second;
		configparBase* cfgpar= configpar_repository::instance()->getConfigPar(gist_realm, configparid);
		helppartext+= "help for parameter `";
		helppartext+= cfgpar->getName();
		helppartext+= "': " + cfgpar->getDescription();
	}
	return helppartext;
}


GISTConsole::cmd_return
GISTConsole::setpar()
{
  list<string>::const_iterator aa = args.begin();
  
  out_buf="# ";
  bool emptyarg= (++aa == args.end()) ? true : false;
  if (emptyarg)
  {
	  out_buf+= "parameter name missing\n";
	  out_buf+= "# available parameters: " + GISTConsole::parnamelist() + '\n';
	  send();
	  return ERROR;
  }

  // aa now points to the token following setpar
  string key= *aa;

  // lookup parameter name
  parname_map_t::const_iterator cit= parname_map.find(key);
  configpar_id_t configparid= 0;
  if ( cit == parname_map.end() )
  {
	  out_buf+= "invalid/unknown parameter name `" + key + "'\n";
	  out_buf+= "# available parameters: " + GISTConsole::parnamelist();
  }
  else
  {
	  // parameter name was found
	  configparid= cit->second;
	  
	  // treat rest of the input as parameter value(s)
	  aa++;
	  string parvaluestring;
	  // next argument(s) should be the parameter value(s), so collect them
	  while (aa != args.end())
	  {
		  parvaluestring += *aa;
		  aa++;
		  if (aa != args.end())
			  parvaluestring += ' ';
	  } // end while any more arguments

	  istringstream valinput(parvaluestring);
	  // read in the value
	  try {
		  // get parameter from config repository
		  configparBase* cfgpar= configpar_repository::instance()->getConfigPar(gist_realm, configparid);
		  if (cfgpar->isChangeableWhileRunning())
		  {
			  cfgpar->readVal(valinput);
			  out_buf+= "setting value for ";
			  out_buf+= cfgpar->getName();
			  out_buf+= ": ";
			  ostringstream outstr;
			  cfgpar->writeVal(outstr);
			  out_buf+= outstr.str();
		  }
		  else
		  {
			  out_buf+= "refused to set parameter " + cfgpar->getName() + " while running! Value unchanged.";
		  }
	  }
	  catch ( configParExceptionParseError &e ) {
		  configparBase* cfgpar= configpar_repository::instance()->getConfigPar(gist_realm, configparid);
		  
		  out_buf+= "invalid value, parse error.\n Hint: `" + cfgpar->getName() + "' ";
		  out_buf+= cfgpar->getDescription();
	  } // end catch parse exception
  } // end else


  out_buf +="\n";
  send();

  return OK;
}

GISTConsole::cmd_return
GISTConsole::showpar()
{
  list<string>::const_iterator aa = args.begin();
  
  out_buf="# ";
  bool emptyarg= (++aa == args.end()) ? true : false;
  if (emptyarg)
  {
	  out_buf+= "parameter name missing\n";
	  out_buf+= "# available parameters: " + GISTConsole::parnamelist() + '\n';
	  send();
	  return ERROR;
  }

  string key= *aa;

  while (aa != args.end())
  {
	  key= *aa;
	  parname_map_t::const_iterator cit= parname_map.find(key);
	  configpar_id_t configparid= 0;
	  if ( cit == parname_map.end() )
	  {
		  out_buf+= "invalid/unknown parameter name `" + key + "'\n";
		  out_buf+= "# available parameters: " + GISTConsole::parnamelist() + '\n';
		  send();
		  break;
	  }
	  else
	  {
		  configparid= cit->second;

		  // get parameter from config repository
		  configparBase* cfgpar= configpar_repository::instance()->getConfigPar(gist_realm, configparid);

		  out_buf+= "value of `";
		  out_buf+= cfgpar->getName();
		  out_buf+= "': ";
		  ostringstream outstr;
		  cfgpar->writeVal(outstr);
		  out_buf+= outstr.str();
		  if (cfgpar->getUnitInfo())
		  { // if parameter has got a unit, print it
			  out_buf+= " ";
			  out_buf+= cfgpar->getUnitInfo();
		  }
		  out_buf+= "\n";
		  send();
		  return OK;
	  }
  }
  return ERROR;
}


GISTConsole::cmd_return
GISTConsole::shutdown()
{
  out_buf += "# Sending QUIT to GIST process and will exit.\n";
  send();
  

  //global_ntlpthread_p->stop_processing(); // works but this will not stop threads running in ntlp_main

  // we signal a QUIT
  kill(param.main_pid, SIGQUIT);
  return quit();
}

GISTConsole::cmd_return
GISTConsole::multi_send()
{
	static const char *arg_names[] = {
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

	ostringstream outstr;

	for (int i = 0; i < count; i++) {
		if (!samesid) {
		  apiargs.mysid.generate_random();
		}

		outstr.clear();
		outstr << "# Sending message #" << i << apiargs.mysid.to_string() << '\n';
		out_buf = outstr.str();
		send();

		apiargs.pc_mri.set_sourceaddress(apiargs.sourceaddress);
		apiargs.pc_mri.set_destaddress(apiargs.destaddress);

		create_api_msg(apiargs.mysid)->send_to(message::qaddr_coordination);

		outstr.clear();
		outstr << "# sleeping " << delay << " ms ...\n";
		out_buf = outstr.str(); 
		send();
				
		if (delay)
			usleep(delay * 1000);
	}
	out_buf = "# Multi send done.\n";
	send();

	return OK;
}

#undef ARG_ERROR
#undef NEXT_ARG

GISTConsole::cmd_return
GISTConsole::quit()
{
	out_buf = "# Goodbye!\n";
	send();

	close(clientfd);
	clientfd = -1;

	return OK;
}


GISTConsole::cmd_return
GISTConsole::saveconfig()
{
  list<string>::const_iterator aa= args.begin();

  if (++aa != args.end())
  {
	    configfile save_cfgfile(configpar_repository::instance());
	    try {
		    save_cfgfile.save(*aa);
		    out_buf="# saved config to ";
		    out_buf+= *aa + "\n";    
	    }
	    catch ( configfile_error &e ) {
		    out_buf="# an error occured during saving the file:";
		    out_buf+= e.what();
		    out_buf+="\n";
	    }
	    catch ( ... ) {
		    out_buf="# an error occured during saving the file.";
	    }
	    send();

  }
  else
  {
	  out_buf="# missing argument: must specify a file name for saving the config\n";
	  send();
	  return ERROR;
  }

  return OK;
	
}

