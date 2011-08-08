/// ----------------------------------------*- mode: C++; -*--
/// @file ProcessingModule.h
/// QoS NSLP Processing Module
/// ----------------------------------------------------------
/// $Id: ProcessingModule.h 6282 2011-06-17 13:31:57Z bless $
/// $HeadURL: https://svn.tm.kit.edu/nsis/qos-nslp/trunk/include/ProcessingModule.h $
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
/** @ingroup processing_module
 * @file
 * NSLP Processing Module
 */

#ifndef _PROCESSING_MODULE_H_
#define _PROCESSING_MODULE_H_

#include "messages.h"
#include "threads.h"
#include "tp.h"
#include "QoS_StateModule.h"
#include "rsn.h"
#include "rii.h"
#include "refresh_period.h"
#include "sessionid.h"
#include "logfile.h"
#include "mri.h"
#include "mri_est.h"
#include "SignalingAppl.h"
#include "apimessage.h"
#include "flowinfo.h"

#include "hashmap"

namespace qos_nslp {

// forward declaration
class SignalingMsg;
class ExplicitSignalingMsg;
class NSLP_Session_Context_Map;

/// ProcessingModule module parameters
struct ProcessingModuleParam : public ThreadParam {
	ProcessingModuleParam(
			uint32 sleep_time,
			AddressList &addresses,
			Flowinfo &fi_service,
			bool see = true,
			bool sre = true
	);
	const  message::qaddr_t source;
	AddressList &addresses;
	Flowinfo &fi_service;
	const  bool send_error_expedited;
	const  bool send_reply_expedited;
}; // end ProcessingModuleParam

/// @class ProcessingModule
class ProcessingModule : public Thread {
	public:
		/// signaling errors
		enum error_t {
			error_ok,
			error_msg_sent,
			error_bad_address
		}; // end error_t
		static const char* get_error_string(error_t e);
		/// constructor
		ProcessingModule(const ProcessingModuleParam& p);
		const ProcessingModuleParam& getParam() const { return param; }
		/// destructor
		virtual ~ProcessingModule();
		/// module main loop
		virtual void main_loop(uint32 nr);
		state_manager::error_t SendMessage(uchar* nslp_data,
				uint32 nslp_data_size,
				sessionid* sid,
				mri* mr,
				uint32 sii_handle=0,
				msghandle_t nslpmsghandle=0,
				tx_attr_t tx_attr=tx_attr_t(),
				uint32 timeout=15,
				uint16 ip_ttl=103,
				uint32 ghc=100);	// GIST hop count
	protected:
		void process_timer_message(message* msg);
		void process_gist_api_msg(ntlp::APIMsg* apimsg);
		void process_appl_msg(QoS_Appl_Msg* applmsg);
        void process_sig_msg(SignalingMsg* msg);
        void process_est_sig_msg(ExplicitSignalingMsg* msg);

	private:
		/// module parameters
		const ProcessingModuleParam param;
		/// StateModule instance
		state_manager *statemodule;
		// reference to nslp_session_context_map
		NSLP_Session_Context_Map* session_context_map;
		static const char* const errstr[];
		void process_queue();
		void rcv_appl_messages();
		void rcv_gist_messages();
		void process_elapsed_timers();
}; // end class ProcessingModule


/// signaling message
/** Messages of this type are exchanged between the signaling module and
 * the coordination module.
 */
class SignalingMsg : public message {
public:
	SignalingMsg();
	/// destructor
	virtual ~SignalingMsg();
	/// get PDU
	known_nslp_pdu* get_pdu() const { return pdu; }
	const ntlp::mri_pathcoupled*  get_sig_mri() const { return sig_mri; }
	void set_sig_mri(const ntlp::mri_pathcoupled*  set_mri);
	/// set sid
	void set_sid(const uint128& r) { 
	  sig_sid.w1 = r.w1;
	  sig_sid.w2 = r.w2;
	  sig_sid.w3 = r.w3;
	  sig_sid.w4 = r.w4;
	}
	/// get sid
	void get_sid(uint128& r) const { 
	  r.w1 = sig_sid.w1;
	  r.w2 = sig_sid.w2;
	  r.w3 = sig_sid.w3;
	  r.w4 = sig_sid.w4;
	}

	const uint128 get_sid() const { return sig_sid; }

	// @{
	void set_msg(known_nslp_pdu* p);
	// @}
	/// clear pointers
	virtual void clear_pointers();
	//@{
	/// send message to coordination or signaling module
	bool send(bool exp = false);
	void send_or_delete(bool exp = false) { if (!send(exp)) delete this; }
	//@}
	// @{
	/// get downstream
	void set_downstream(bool d);
	bool get_downstream() const { return down; }
	// @}
	void set_allow_translate(bool d) { translate = d; }
	bool get_allow_translate() const { return translate; }
	void set_invalidate(bool d) { invalidate = d; }
	bool get_invalidate() const { return invalidate; }
	void set_sii_handle(uint32 sii) { sii_handle = sii; }
	uint32 get_sii_handle() const { return sii_handle; }
	void set_reliable(bool rel) { reliable = rel; }
	bool get_reliable() { return reliable; }
	void set_secure(bool sec) { secure = sec; }
	bool get_secure() { return secure; }

private:
	known_nslp_pdu* pdu;
	uint128 sig_sid;
	ntlp::mri_pathcoupled*  sig_mri;
	uint32 sii_handle;
	bool down;
	bool translate;
	bool invalidate;
	bool reliable;
	bool secure;
}; // end class SignalingMsg


/// explicit signaling message
/** Messages of this type are exchanged between the signaling module and
 * the coordination module.
 */
class ExplicitSignalingMsg : public message {
public:
	/// constructor
	ExplicitSignalingMsg();
	/// destructor
	virtual ~ExplicitSignalingMsg();

	/// mri
	const ntlp::mri_explicitsigtarget* get_sig_mri() const { return sig_mri; }
	void set_sig_mri(const ntlp::mri_explicitsigtarget* set_mri);

	/// set sid
	void set_sid(const uint128& r) { 
	  sig_sid.w1 = r.w1;
	  sig_sid.w2 = r.w2;
	  sig_sid.w3 = r.w3;
	  sig_sid.w4 = r.w4;
	}
	/// get sid
	void get_sid(uint128& r) const { 
	  r.w1 = sig_sid.w1;
	  r.w2 = sig_sid.w2;
	  r.w3 = sig_sid.w3;
	  r.w4 = sig_sid.w4;
	}

	const uint128 get_sid() const { return sig_sid; }

	// @{
	void set_pdu(known_nslp_pdu* p);
	known_nslp_pdu* get_pdu() const { return pdu; }
	// @}

	void set_sii_handle(uint32 sii) { sii_handle = sii; }
	uint32 get_sii_handle() const { return sii_handle; }

	/// clear pointers
	virtual void clear_pointers();
	//@{
	/// send message to coordination or signaling module
	bool send(bool exp = false);
	void send_or_delete(bool exp = false) { if (!send(exp)) delete this; }
	//@}

private:
	known_nslp_pdu* pdu;
	uint128 sig_sid;
	ntlp::mri_explicitsigtarget* sig_mri;
	uint32 sii_handle;
}; // end class ExplicitSignalingMsg

} // end namespace qos_nslp

#endif // _PROCESSING_MODULE_H_
