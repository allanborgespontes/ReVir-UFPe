/// ----------------------------------------*- mode: C++; -*--
/// @file apiwrapper.h
/// GIST API wrapper - maps internal API to Unix Domain Socket API
/// ----------------------------------------------------------
/// $Id: apiwrapper.h 6282 2011-06-17 13:31:57Z bless $
/// $HeadURL: https://svn.tm.kit.edu/nsis/ntlp/trunk/include/apiwrapper.h $
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
#ifndef _NTLP__APIWRAPPER_MODULE_H
#define _NTLP__APIWRAPPER_MODULE_H

#include "messages.h"
#include "apimessage.h"
#include "threads.h"
#include "tp.h"
#include "ntlp_pdu.h"
//#include "ntlp_proto.h"
#include "ntlp_error.h"
#include "ntlp_errorobject.h"
#include "hashmap"
#include "goettingen_api.h"

#include "addresslist.h"

// Constants of Goettingen's GIST API

// identifier:

#define SEND_MESSAGE             0
#define RECV_MESSAGE             1
#define MESSAGE_STATUS           2
#define NW_NOTIFICATION          3
#define SET_STATE_LIFETIME       4
#define INV_ROUTING_STATE        5
#define RECEIVE_LOCAL           10
#define TIMER_TIMEOUT           11
#define REQUEST_LOCAL_IP        12
#define SEND_LOCAL_IP           13
#define TEST_RECV_MSG           20
#define TEST_MSG_STATUS         21
#define TEST_NWN                22
#define REGISTER_NSLPID         90
#define UNREGISTER_NSLPID       91

// infofield:

//????


namespace ntlp {
    using namespace protlib;




/// signaling module parameters
struct APIWrapperParam : public ThreadParam {
    APIWrapperParam(TP* proto,
		    const message::qaddr_t clientqueue,
		    AddressList &addresses,
		    bool instantestablish,
		    uint32 sleep_time = ThreadParam::default_sleep_time);
    TP* proto; 
    const message::qaddr_t clientqueue;
    bool instantestablish;
    AddressList &addresses;

}; // end SignalingParam




/// signaling module class
/// @class Signaling
class APIWrapper : public Thread {
public:
	/// signaling errors
	enum error_t {
		error_ok,
		error_msg_sent,
		error_bad_address,
		error_unreachable,
		error_tp_internal,
		error_no_tp,
		error_wrong_pdu_type,
		error_malformed_pdu,
		error_no_request_found,
		error_max_payload,
		error_bad_msgref,
		error_gist_internal,
		error_internal,
		error_bad_sigmsg


	}; // end error_t
	static const char* get_error_string(error_t e);
	/// constructor
	APIWrapper(const APIWrapperParam& p);
	/// destructor
	virtual ~APIWrapper();
	/// module main loop
	virtual void main_loop(uint32 nr);
	///header length for API (3 byte: 1byte ID, 4byte infofield)
	static const uint32 api_header_length = 5;
	/// decode a API Message (for use in TPoverUDS)
	static bool decode_api_header_clen(NetMsg& m, uint32& clen_bytes);
	
	/// encode a APIMsg using Goettingen's definitions into a NetMsg
	static NetMsg* generate_netmsg(APIMsg* apimsg);
	/// decode a NetMsg using Goettingen's definitions into a APIMsg
	static APIMsg* decode_netmsg(NetMsg* netmsg);

private:
	/// module parameters
	const APIWrapperParam param;
	static const char* const errstr[];
	void process_queue();
	void process_tp_msg(TPMsg* msg);
	void process_api_msg(APIMsg* msg);
	error_t send_tp(NetMsg* msg, const udsaddress& addr, uint16 oif);
	void send_local_ip(NetMsg* msg, const udsaddress& addr);

	hashmap_t<uint32, udsaddress> registry;
 
}; // end class APIWrapper





/** Decode common header for getting msg content length without doing any sanity checks. Resets position pointer in m to start. 
 * @return true if NTLP header seems to be ok, false if not a NTLP header
**/
inline
bool
APIWrapper::decode_api_header_clen(NetMsg& m, uint32& clen_bytes) 
{

    uint8 type = m.decode8(); //(*(reinterpret_cast<uint8 *>(m.get_buffer())));


    uint32 len = *((uint32*) (m.get_buffer() + 1));

    DLog("API decoding", "Type: " << (int) type << " Infofield: " << len);


    if ((type <= 5) || (type == 20) || (type == 12) || (type == 13) || (type == 21) || (type == 22) || (type == 90) || (type == 91)) {
	//seems like a valid API message *LOL*

	//sleep(4);

	if (type < REGISTER_NSLPID){
	    clen_bytes = len; //  - api_header_length;
	}
	else clen_bytes = 0;
 
	DLog("API decoding", "Length without header: " << clen_bytes);
	
	m.to_start();
	return true;
    }
    

    return false;

} // end decode_api_header_clen






//@}





} // end namespace protlib

#endif


