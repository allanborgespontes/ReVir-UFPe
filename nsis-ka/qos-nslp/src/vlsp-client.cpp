/// ----------------------------------------*- mode: C++; -*--
/// @file client.cpp
/// Test application that requests QoS support from the
/// QoS NSLP Daemon via UNIX Domain Socket IPC interface
/// ----------------------------------------------------------
/// $Id: vlsp-client.cpp 6314 2011-07-18 11:28:53Z bless $
/// $HeadURL: https://svn.tm.kit.edu/nsis/qos-nslp/trunk/src/vlsp-client.cpp $
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

// get session authorization stuff
#include "session_auth_object.h"

#include "benchmark_journal.h"

using namespace qos_nslp;

using namespace nslp_auth;

#ifdef BENCHMARK
namespace qos_nslp {

benchmark_journal journal(1000000, "vlsp_client_benchmark_journal.txt");

}
#endif

const port_t SRC_PORT  = 0;
const port_t DST_PORT  = 0;
const uint16 SCTP_PORT = 30000;
const std::string sockname("/tmp/qos_nslp_socket");


// three pre-shared keys
const string psk1("12345678901234567890123456789012");
const string psk2("abcdefghijklmnopqrstuvwxyz-.,+%$");
const string psk3("xabcdefghijklmnopqrstuvwxyz.,+$x");
uint32  pskey_id1= 0x1;
uint32  pskey_id2= 0x2;
uint32  pskey_id3= 0x1a1a1b1bL;

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


void listen(QoS_NSLP_Client_API& client)
{
  int status;
  uint128 sid;
  appladdress source_address;
  appladdress dest_address;
  qspec::qspec_pdu* qspec_template_p;
  info_spec* info_spec_p;
  vlsp_object* vlspobj_p; 
  session_auth_object* session_auth_obj_hmac_signed_p;
  session_auth_object* session_auth_obj_other_p;

  client.register_client();

  do {

    client.recv_pdu(status, 500000, 
		    sid, 
		    source_address,
		    dest_address,
		    qspec_template_p, 
		    info_spec_p, 
		    vlspobj_p, 
		    session_auth_obj_hmac_signed_p, 
		    session_auth_obj_other_p);
    
    std::cout << "status: " << status << endl;
    std::cout << "sid=" << sid << " source= " << source_address << " dest=" << dest_address << endl;
    if (qspec_template_p) {
      std::cout << "qspec=" << *qspec_template_p << endl;
    }
    if(info_spec_p) {
      std::cout << "info spec=" << *info_spec_p << endl;
    }
    if(vlspobj_p) {
      std::cout << "vlsp object=" << *vlspobj_p << endl;
    }
    if (session_auth_obj_hmac_signed_p) {
      std::cout << "session auth object hmac signed=" << *session_auth_obj_hmac_signed_p << endl;
    }
    if (session_auth_obj_other_p) {
      std::cout << "session auth object=" << *session_auth_obj_other_p << endl;
    } 
  } while(status >= 0 || errno==ETIMEDOUT);
}


int main(int argc, char *argv[])
{
	if (argc < 3) {
		std::cerr << "usage: " << argv[0] << 
			"<src_address> <dst_address> <vnet-id> <vnode-src> <vnode-dst> <vif-src> <vif-dst> <vlink-id> <tunnel-type> [R]" << std::endl;
		return EXIT_FAILURE;
	}

	bool sender_initiated_reserve = true;
	if (argc > 3 && strncasecmp("r", argv[3], 1) == 0)
		sender_initiated_reserve = false;

	// Establish a handler for SIGINT signals (send on Ctrl-c)
	signal(SIGINT, catch_sigint);

	// initalize netdb
	tsdb::init(true);
	appladdress src = appladdress(argv[1], "tcp", SRC_PORT);
	appladdress dst = appladdress(argv[2], "tcp", DST_PORT);

	// register PDUs and their objects
	qos_nslp::register_ies();
	AUTH_ATTR_IEManager::register_known_ies();

	QoS_NSLP_Client_API client(sockname);

	// listen mode
	if (argc > 3 && strncasecmp("l", argv[3], 1) == 0)
	{
	  listen(client);

	  tsdb::end();
	  
	  return 0;
	}

	// sender mode

	// set appropriate values for reservation request
	bool downstream = true;
	float bandwidth = 125.0 * 1000; // bytes/s (125000 bytes/s = 1 Mbit/s)

	uint128 vnetid(1,2,3,4);
	uint128 vnodeid_a(0,0,0,0x1), vnodeid_b(0,0,0,0x1);
	vlsp_object vlspobj(vnetid, vnodeid_a, vnodeid_b, 0, 0, 0, vlsp_object::tunnel_IP_in_IP);

	// prepare session auth object for hmac signed
	session_auth_object *sauth_obj_hmac= new session_auth_object;

	// Register pre-shared Keys
	client.set_hmac_key(pskey_id1, psk1.c_str(), psk1.length());
	client.set_hmac_key(pskey_id2, psk2.c_str(), psk2.length());
	client.set_hmac_key(pskey_id3, psk3.c_str(), psk3.length());

	// who authorized the whole thing?
	auth_attr_string *auth_ent_id = new auth_attr_string();
	auth_ent_id->set_string("auth%tb4@tm.kit.edu");
	sauth_obj_hmac->insert_attr(auth_ent_id);

	auth_attr_hmac_trans_id *transid = new auth_attr_hmac_trans_id(session_auth_object::TRANS_ID_AUTH_HMAC_SHA1_96);
	sauth_obj_hmac->insert_attr(transid);

	auth_attr_addr *src_addr = new auth_attr_addr(SOURCE_ADDR,IPV4_ADDRESS);
	src_addr->set_ip(src);
	sauth_obj_hmac->insert_attr(src_addr);

	auth_attr_addr *dst_addr = new auth_attr_addr(DEST_ADDR,IPV4_ADDRESS);
	dst_addr->set_ip(dst);
	sauth_obj_hmac->insert_attr(dst_addr);

	auth_attr_time *timestamp_attr = new auth_attr_time();
	timestamp_attr->set_value(time(NULL)+6000);
	sauth_obj_hmac->insert_attr(timestamp_attr);

	auth_nslp_object_list *nslp_obj_list = new auth_nslp_object_list;
	nslp_obj_list->insert(known_nslp_object::RII);
	nslp_obj_list->insert(known_nslp_object::VLSP_OBJECT);
	sauth_obj_hmac->insert_attr(nslp_obj_list);
	

	auth_attr_data *auth_dataattr = new auth_attr_data(pskey_id1);
	sauth_obj_hmac->insert_attr(auth_dataattr);

	session_auth_object *sauth_obj_token= NULL;

#ifdef SECOND_AUTH_SESSION_OBJECT
	// prepare session auth object for other stuff (auth token)
	sauth_obj_token= new session_auth_object;
	auth_attr_string *token_auth_ent_id = new auth_attr_string(AUTH_ENT_ID, X509_V3_CERT);
	token_auth_ent_id->set_string("AAA@tm.kit.edu");
	sauth_obj_token->insert_attr(token_auth_ent_id);

	auth_attr_addr *token_src_addr = new auth_attr_addr(SOURCE_ADDR,IPV4_ADDRESS);
	token_src_addr->set_ip(src);
	sauth_obj_token->insert_attr(token_src_addr);

	auth_attr_addr *token_dst_addr = new auth_attr_addr(DEST_ADDR,IPV4_ADDRESS);
	token_dst_addr->set_ip(dst);
	sauth_obj_token->insert_attr(token_dst_addr);

	auth_attr_time *token_timestamp_attr = new auth_attr_time();
	token_timestamp_attr->set_value(time(NULL)+6000);
	sauth_obj_token->insert_attr(token_timestamp_attr);

	auth_attr_data *token_auth_dataattr = new auth_attr_data();
	string auth_token("this is a test token: X-C;.VASDG:FATRR!GASD2A4SRQWTG2DSRG5KEW4T99ZH$=O§RASGFSR%ERGAF");
	token_auth_dataattr->set_auth_data(reinterpret_cast<const uchar*>(auth_token.c_str()),auth_token.length());
	sauth_obj_token->insert_attr(token_auth_dataattr);
#endif
	// this sends a RESERVE message as reservation request for 
	// a data flow from src to dst
	ntlp::sessionid *gist_sid;


	int status;
	int timeout = 5000; // ms
	unsigned int rounds = 100;

	do
	{
		std::cout << "round " << rounds << endl;
		if (sender_initiated_reserve)
		{
			MP(benchmark_journal::PRE_VLSP_SETUP);		
			gist_sid = client.send_reserve(src, dst, downstream, bandwidth, &vlspobj, sauth_obj_hmac, sauth_obj_token);
		}
		else
			gist_sid = client.send_query(src, dst, downstream, bandwidth, &vlspobj, sauth_obj_hmac, sauth_obj_token);
	    
		// this call blocks until response was received or timeout happens
		if (sender_initiated_reserve)
		{
			client.recv_response(status, timeout);
			
			MP(benchmark_journal::POST_VLSP_SETUP);
		}
		else
			client.recv_reserve(status, timeout);

		if (status == 2) {
			//std::cout << "successful reservation" << std::endl;
			sleep(2);
			// If reservation has been successful tear down resources now
			MP(benchmark_journal::PRE_VLSP_SETDOWN);		
			client.send_tear(gist_sid, src, dst, downstream);
			// ... and wait 5000ms for confirmation
			client.recv_response(status, timeout);
			MP(benchmark_journal::POST_VLSP_SETDOWN);		
			
			keep_going=1;
		} 
		else
		{
			if (status >= 0)
			{
				std::cout << "status of reservation request " << status << std::endl;
			}
			else
				std::cout << "An error occurred: " << std::strerror(errno) << std::endl;
		}
	    
		rounds--;
		// allow to settle down
		sleep(5);
	} 
	while (keep_going && rounds > 0);
	// wait until Ctrl-c sets keep_going = 0 -- e.g., continue application processing

       
	client.del_hmac_key(pskey_id1);
	client.del_hmac_key(pskey_id2);
	client.del_hmac_key(pskey_id3);

	tsdb::end();

	return 0;
}
