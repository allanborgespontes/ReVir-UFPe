/// ----------------------------------------*- mode: C++; -*--
/// @file signalingmodule_ntlp.h
/// Signaling Module - sending and receiving PDUs incl. parsing
/// ----------------------------------------------------------
/// $Id: signalingmodule_ntlp.h 6282 2011-06-17 13:31:57Z bless $
/// $HeadURL: https://svn.tm.kit.edu/nsis/ntlp/trunk/src/signalingmodule_ntlp.h $
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
#ifndef _NTLP__SIGNALING_MODULE_H
#define _NTLP__SIGNALING_MODULE_H

#include "messages.h"
#include "apimessage.h"
#include "threads.h"
#include "tp.h"
#include "ntlp_pdu.h"
#include "ntlp_proto.h"
#include "ntlp_error.h"
#include "ntlp_errorobject.h"
#include "flowinfo.h"


#include "hashmap"


namespace ntlp 
{

  using namespace protlib;

//forward declaration

 class SignalingMsgNTLP;

/// signaling module parameters
struct SignalingNTLPParam : public ThreadParam 
{
  typedef hashmap_t<protocol_t,TP*> protocol_map_t;
  typedef protocol_map_t::const_iterator protocol_map_const_it_t;
  const  protocol_map_t protomap;

  nli &nli_tmpl;
  AddressList &addresses;
  const  message::qaddr_t source;
  const  bool send_error_expedited;
  const  bool send_reply_expedited;
  TP*    get_tp(protocol_t id) const;
  NSLPtable& nslptable;
  uint16 well_known_port;
  Flowinfo *fi_service;

  SignalingNTLPParam(protocol_map_t pm,
		     nli &nli_tmpl,
		     AddressList &addresses,
		     NSLPtable& nslptable,
		     uint16 well_known_port,
		     Flowinfo *fi_service= NULL,
		     uint32 sleep_time = ThreadParam::default_sleep_time,
		     bool see = true, 
		     bool sre = true
		     );
}; // end SignalingParam


/// signaling module class
/// @class Signaling
class SignalingNTLP : public Thread {
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
		error_bad_sigmsg,
		error_MAX_msg
	}; // end error_t
	static const char* get_error_string(error_t e);
	/// constructor
	SignalingNTLP(const SignalingNTLPParam& p);
	/// destructor
	virtual ~SignalingNTLP();
	/// module main loop
	virtual void main_loop(uint32 nr);

        // returns whether a given address belongs to one of the DRM addresses
        // XXXMOB: might be okay for the moment
        bool is_own_address(const hostaddress& ha) const { return gistproto.isown_address(&ha); };
        // return default originator address
        // XXXMOB: Not sure we want this anymore.
        // const appladdress* default_originator() const { return &param.default_originator; };

protected:
	void handleInternalMessage(message *msg);
	void handleTimeout();

private:
	/// module parameters
	const SignalingNTLPParam& param;
	/// GIST protocol instance
	NTLP gistproto;
	static const char* const errstr[];
	void process_queue();
	void process_tp_msg(TPMsg* msg);
	void process_sig_msg(SignalingMsgNTLP* msg);

	void send_back_sig_error(error_t err, SignalingMsgNTLP* msg);
	void send_back_confirmation(SignalingMsgNTLP* msg);
	// XXXMOB: Needs originator address arg
	error_t send_tp(NetMsg* msg, const appladdress& addr, const hostaddress *laddr = 0, bool use_exist_conn= false);

	/// Send Error Message to peer, if the error occured in SignalingModule processing
	void senderror(const known_ntlp_pdu* pdu, const appladdress* peer, GIST_Error::gist_error_code_t error);

 public:
	///test, if a net address is of the local host (route lookup and compare to 'lo' for device)
	static bool is_local_addr(const hostaddress* addr);


}; // end class Signaling


/// signaling message
/** Messages of this type are exchanged between the signaling module and
 * the coordination module.
 */
class SignalingMsgNTLP : public message {
public:
	enum subtype_t {
		ignore,
		SigReq,
		SigInd,
		SigCnf,
		SigRsp,
		SigTrm,
		SigCancel
	}; // end subtype_t
	SignalingMsgNTLP(bool conf = false);
	/// destructor
	virtual ~SignalingMsgNTLP();
	/// get subtype
	subtype_t get_subtype() const { return subtype; }
	/// set result
	void set_error(SignalingNTLP::error_t r) { err = r; }
	/// get result
	SignalingNTLP::error_t get_error() const { return err; }
	/// get PDU
	known_ntlp_pdu* get_pdu() const { return pdu; }
	/// get peer address
	const appladdress* get_peer() const { return peer; }
	/// get own/local address
	const appladdress* get_ownaddr() const { return ownaddr; }
	known_ntlp_pdu* get_encap_pdu() const { return encap_pdu; }
	gp_id_t get_ref_id() const { return ref_id; }
	// @{
	/// set up message for subtypes
	void set_req(const appladdress* peer, known_ntlp_pdu* p, gp_id_t refid = 0, uint16 oif = 0);
	
        void set_ind(gp_id_t id, const appladdress* peer, const appladdress* ownaddr, known_ntlp_pdu* p,
		     gp_id_t rid = 0, known_ntlp_pdu* epdu = NULL);
	void set_cnf(gp_id_t id, known_ntlp_pdu* p, known_ntlp_pdu* epdu = NULL);
	void set_rsp(gp_id_t id, known_ntlp_pdu* p, bool setencap = true);
        void set_trm(const appladdress* peeraddr);
	void set_cancel(gp_id_t id, bool reply = false);
	// @}
	/// clear pointers
	virtual void clear_pointers();
	const char* get_error_string() const { return SignalingNTLP::get_error_string(err); }
	//@{
	/// send message to coordination or signaling module
	bool send(bool exp = false);
	void send_or_delete(bool exp = false) { if (!send(exp)) delete this; }
	//@}
	// @{
	/// confirmation
	bool set_confirm(bool conf);
	bool get_confirm() const { return confirm; }
	// @}
	const char* get_subtype_name() const { return subtypename[subtype]; }

        /// allow to establsh a new connection or force reuse of existing one
        void set_useexistconn(bool do_not_establish= true) { use_existing_connection= do_not_establish; };
        bool get_useexistconn() const { return use_existing_connection; };

        void set_local_addr(hostaddress *ls) { local_src = ls; };
        hostaddress *get_local_addr() { return local_src; }

private:
	subtype_t subtype;
	SignalingNTLP::error_t err;
	known_ntlp_pdu* pdu;
	const appladdress* peer;
	// XXXMOB: this needs to be used for all messages?
	const appladdress* ownaddr;
	known_ntlp_pdu* encap_pdu;
	/// ID of a PDU reference or encapsulated PDU
	gp_id_t ref_id;
	bool confirm;
	static const char* const subtypename[];
	uint16 oif;

        bool use_existing_connection;

        hostaddress *local_src;
}; // end class SignalingMsg

//@}


} // end namespace protlib

#endif


