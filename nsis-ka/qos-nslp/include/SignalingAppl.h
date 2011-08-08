/// ----------------------------------------*- mode: C++; -*--
/// @file SignalingAppl.h
/// QoS NSLP Signaling Application
/// ----------------------------------------------------------
/// $Id: SignalingAppl.h 5253 2010-04-26 13:41:09Z roehricht $
/// $HeadURL: https://svn.tm.kit.edu/nsis/qos-nslp/trunk/include/SignalingAppl.h $
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
/** @ingroup signaling_appl
 * @file
 * NSLP Signaling Application
 */

#ifndef _SIGNALING_APPL_H_
#define _SIGNALING_APPL_H_

#include "QoS_Appl_Msg.h"
#include "protlib_types.h"
#include "messages.h"
#include "threads.h"
#include "logfile.h"
#include "qspec.h"
#include "nslp_pdu.h"
#include "address.h"

using namespace protlib::log;
using namespace protlib;

namespace qos_nslp {

/// SignalingApplModule module parameters
struct SignalingApplParam : public ThreadParam {
	SignalingApplParam(uint32 sleep_time = ThreadParam::default_sleep_time,
			bool see = true,
			bool sre = true
			);
	const  message::qaddr_t source;
	const  bool send_error_expedited;
	const  bool send_reply_expedited;
}; // end SignalingParam

/// signaling application class
/// @class SignalingAppl
class SignalingAppl : public Thread {
public:
	/// constructor
	SignalingAppl(const SignalingApplParam& p);

	/// destructor
	virtual ~SignalingAppl();

	/// module main loop
	virtual void main_loop(uint32 nr);
	void get_and_send_pdu();

	/// get info type string
	static const char* get_info_type_string(QoS_Appl_Msg::qos_info_type_t t) { return info_type_string[t]; }

private:
	/// module parameters
	const SignalingApplParam param;
	/// printable info types
	static const char* const info_type_string[];
	void process_queue();
	void process_qos_msg(QoS_Appl_Msg* msg);
}; // end class SignalingAppl

} // end namespace qos_nslp

#endif // _SIGNALING_APPL_H_
