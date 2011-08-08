/// ----------------------------------------*- mode: C++; -*--
/// @file QoS_NSLP_UDS_API.h
/// QoS NSLP UDS API - maps internal API to Unix Domain Socket API
/// ----------------------------------------------------------
/// $Id: QoS_NSLP_UDS_API.h 6258 2011-06-14 11:03:15Z bless $
/// $HeadURL: https://svn.tm.kit.edu/nsis/qos-nslp/trunk/include/QoS_NSLP_UDS_API.h $
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
/**
 * @description
 * This is the UNIX domain socket based interface on the daemon side.
 * Please see QoS_NSLP_API.h for the client side
 */

#ifndef _QOS_NSLP_UDS_API_H_
#define _QOS_NSLP_UDS_API_H_

#include "messages.h"
#include "threads.h"
#include "tp.h"
#include "QoS_Appl_Msg.h"

/// header length for UDS API -- 4 bytes magic number, 1 byte for type, 4 bytes for length
static const uint32 uds_api_header_length = 9;

// identifier
static const uint8 SEND_MESSAGE = 0;
static const uint8 RECV_MESSAGE = 1;
static const uint8 SET_HMAC_KEY = 2;
static const uint8 DEL_HMAC_KEY = 3;
static const uint8 REQUIRE_DECISION  = 4;
static const uint8 DECISION          = 5;
static const uint8 REGISTER = 6;
static const uint8 LAST_VALID_TYPE = 6;

namespace qos_nslp {

const char *const qos_nslp_unix_domain_socket = "/tmp/qos_nslp_socket";

/// UDS API module parameters
struct UDS_API_Param : public ThreadParam {
	UDS_API_Param(TP* proto,
			const message::qaddr_t clientqueue,
			const message::qaddr_t internalqueue,
			uint32 sleep_time = ThreadParam::default_sleep_time);
	TP* proto;
	const message::qaddr_t clientqueue;
	const message::qaddr_t internalqueue;
}; // end UDS_API_Param

class UDS_API : public Thread {
public:
	/// constructor
	UDS_API(const UDS_API_Param& p);
	/// destructor
	virtual ~UDS_API();
	/// module main loop
	virtual void main_loop(uint32 nr);
	/// decode a API Message (for use in TPoverUDS)
	static bool decode_api_header_clen(NetMsg& m, uint32& clen_bytes);
	/// encode a QoS_Appl_Msg into a NetMsg
	NetMsg* generate_netmsg(const QoS_Appl_Msg* applmsg);
	/// decode a NetMsg into a QoS_Appl_Msg
	QoS_Appl_Msg* decode_netmsg(NetMsg* netmsg);

private:
	/// module parameters
	const UDS_API_Param param;
	void process_queue();
	void process_tp_msg(const TPMsg* msg);
	void process_appl_msg(const QoS_Appl_Msg* msg);
	udsaddress peeraddr;

	static const uint32 version_magic;

	void set_hmac_key(NetMsg* m);
	void del_hmac_key(NetMsg* m);

}; // end class UDS_API

/*
 * Decode common header for getting msg content length without doing
 * any sanity checks. Resets position pointer to start.
 * API Header:
 * [MAGIC]  32-bit magic number
 * [type]   8-bit message type
 * [length] 32-bit message length in bytes not including this header (counting all following content bytes only)
 * @return true if header seems to be ok, false otherwise
 */
inline bool UDS_API::decode_api_header_clen(NetMsg& m, uint32& clen_bytes)
{
	uint32 this_version_magic= m.decode32();
	if (this_version_magic != UDS_API::version_magic)
	{
		ERRLog("UDS API decoding", "Magic number mismatch - different UDS API version? Seen: 0x" << hex << this_version_magic << " Supported: 0x" << UDS_API::version_magic << dec );
		return false;
	}
	uint8 type = m.decode8(); 
	clen_bytes = m.decode32();

	if (type > LAST_VALID_TYPE)
		return false;

	DLog("UDS API decoding", "type " << (uint32) type << " length without header: " << clen_bytes);

	m.to_start();
	return true;
}

} // end namespace qos_nslp

#endif
