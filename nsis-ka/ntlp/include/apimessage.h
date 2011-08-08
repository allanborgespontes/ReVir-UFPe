/// ----------------------------------------*- mode: C++; -*--
/// @file apimessage.h
/// GIST internal API Message
/// ----------------------------------------------------------
/// $Id: apimessage.h 6282 2011-06-17 13:31:57Z bless $
/// $HeadURL: https://svn.tm.kit.edu/nsis/ntlp/trunk/include/apimessage.h $
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
/** @ingroup APIMsg
 * @file
 * APIMsg
 *
 * Class apimessage carries API payload for use with internal message queue transport system designed for NTLP.
 *
 * For use with the internal API, each API caller needs to use a qaddr_api_n (n being a number from 0 to 7) 
 * Message Queue address identifying himself.
 *
 * e.g. NTLP Statemodule listens at qaddr_coordinator_ext for API Messages, it will then register the caller in a table
 * which will enable him to correctly identify the Message Address for a given NSLPID.
 *
 * API callers get called back, when coordinator looks up NSLPID in its table and then sends the message to the appropriate 
 * qaddr_api_n Queue.
 * 
 * Currently, this supports 8 callers (could be increased by extending in protlib/messages.h), each API caller has to 
 * pick a qaddr himself (there is NO register/unregister facility, as it is given that NSLPs using the internal API 
 * have source code access to both NTLP StateModule and even other NSLP implementations they iinterfere with.)
 * 
 * 
 * qaddr_api_n assigned addresses:
 * ###############################
 *
 * qaddr_api_0:  GIST API wrapper
 * qaddr_api_1:  QoS-NSLP
 * qaddr_api_2:  NATFW-NSLP
 * qaddr_api_3:  free
 * ..
 * qaddr_api_7:  free
 *
 * So for API callers, there is simply one thing to do:
 * 
 * - listen with a thread at their message queue at their assigned address
 * - send API Messages to StateModule at address qaddr_coordinator_ext
 * 
 * Thats all, answers will come back automagically!
 *
 * API Messages construction:
 *
 * 1st create an apimessage object with the default constructor
 * 2nd call the "set_XY" function for the type of API Message this will be. This function sets the subtype 
 *     and takes the appropriate payload data as pointers or atomic types.
 * 3rd call apimessage::sendto(qaddr_coordinator_ext);
 *
 */

#include "messages.h"
#include "threads.h"
#include "tp.h"
#include "nslpdata.h"
#include "mri.h"
#include "sessionid.h"
#include "msghandle.h"

#ifndef _API_MESSAGE_H
#define _API_MESSAGE_H


namespace ntlp {
    using namespace protlib;
    using namespace protlib::log;
    
  /**
   *
   * for SendMessage
   *  Reliability:  Values 'unreliable' or 'reliable'.
   *
   *  Security:  This attribute allows the NSLP to specify what level of
   *     security protection is requested for the message (such as
   *     'integrity' or 'confidentiality'), and can also be used to
   *     specify what authenticated signalling source and destination
   *     identities should be used to send the message.  The
   *     possibilities can be learned by the signalling application from
   *     prior MessageStatus or RecvMessage notifications.  If an NSLP-
   *     Message-Handle is provided, GIST will inform the signalling
   *     application of what values it has actually chosen for this
   *     attribute via a MessageStatus callback.  This might take place
   *     either synchronously (where GIST is selecting from available
   *     messaging associations), or asynchronously (when a new
   *     messaging association needs to be created).
   *
   *    Local Processing:  This attribute contains hints from the
   *      signalling application about what local policy should be
   *      applied to the message; in particular, its transmission
   *      priority relative to other messages, or whether GIST should
   *      attempt to set up or maintain forward routing state.
   *
   * for ReceiveMessage
   *   Transfer-Attributes:  The reliability and security attributes that
   *      were associated with the reception of this particular message.  As
   *      well as the attributes associated with SendMessage, GIST may
   *      indicate the level of verification of the addresses in the MRI.
   *      Three attributes can be indicated:
   *
   *      *  Whether the signalling source address is one of the flow
   *         endpoints (i.e. whether this is the first or last GIST hop);
   *
   *      *  Whether the signalling source address has been validated by a
   *         return routability check.
   *
   *      *  Whether the message was explicitly routed (and so has not been
   *         validated by GIST as delivered consistently with local routing
   *         state).
   */
    
  /// transfer attributes
  struct tx_attr_t {
    tx_attr_t() :
      reliable(false), // rcv and snd
      secure(false),   // rcv and snd
      local_policy_keep_no_routing_state(false), // send
      local_policy_tx_priority_high(false), // send
      final_hop(false),        // rcv: first or last hop
      sigsrc_validated(false), // rcv: signalling source address has been validated by a return routability check
      explicitly_routed(false) {}; // rcv: explicitly routed

    bool reliable;
    bool secure;
    bool local_policy_keep_no_routing_state;
    bool local_policy_tx_priority_high;
    bool final_hop;
    bool sigsrc_validated;
    bool explicitly_routed;
  };




/// signaling message
/** Messages of this type are exchanged between the signaling module and
 * the coordination module.
 */
class APIMsg : public message {
public:
	enum subtype_t {
	    ignore,
	    Register,
	    SendMessage,
	    RecvMessage,
	    RecvMessageAnswer,
	    MessageStatus,
	    NetworkNotification,
	    SetStateLifetime,
	    InvalidateRoutingState
	}; // end subtype_t
	/// signaling errors
	


	enum error_t {
	    //Error Codes for API messages
	    error_ok,
	    error_msg_delivered,
	    error_bad_target,
	    
	    //Error Codes used in error payload (MessageStatus msg)
	    error_msg_node_not_found,

	    //Error Codes used in network notification payload
	    last_node,
	    route_changed_status_invalid,
	    route_changed_status_good,
	    route_changed_status_bad,
	    route_changed_status_tentative,
	    route_changed_status_home_binding,
	    route_changed_status_binding

#ifdef USE_AHO
	    ,
	    anticipated_handover_event,
	    handover_event
#endif
	}; // end error_t

	enum status_t {
	    // Status Codes for Routing State
	    good,
	    bad,
	    tentative,

	    // Status Codes used in RecvMessageAnswer call
	    directive_relay,
	    directive_establish
	}; // end status_t

	APIMsg();
	/// destructor
	virtual ~APIMsg();
	/// get subtype
	subtype_t get_subtype() const { return subtype; }
	/// set result
	void set_error(error_t r) { err = r; }
	/// get result
	error_t get_error() const { return err; }

	// @{
	/// set up message for subtypes
	void set_sendmessage(nslpdata* data, msghandle_t nslpmsghandle, uint32 nslpid, sessionid* sid, mri* mr, uint32 sii_handle, tx_attr_t tx_attr, uint32 timeout, uint16 ip_ttl, uint32 ghc);

	// without SII-Handle
        void set_sendmessage(nslpdata* data, msghandle_t nslpmsghandle, uint32 nslpid, sessionid* sid, mri* mr, tx_attr_t tx_attr, uint32 timeout, uint16 ip_ttl, uint32 ghc);

        void set_recvmessage(nslpdata* data, uint32 nslpid, sessionid* sid, mri* mr, bool adjacency_check, uint32 sii_handle, tx_attr_t tx_attr, uint16 ip_ttl, uint16 ip_distance, uint32 ghc, uint16 inbound_if_index= 0);
        void set_recvmessage(nslpdata* data, appladdress *own_addr, uint32 nslpid, sessionid* sid, mri* mr, bool adjacency_check, uint32 sii_handle, tx_attr_t tx_attr, uint16 ip_ttl, uint16 ip_distance, uint32 ghc, uint16 inbound_if_index= 0);
	void set_recvmessageanswer(uint32 nslpid, sessionid* sid, mri* mr, nslpdata* data, status_t directive);
	void set_messagestatus(uint32 nslpid, sessionid* sid, msghandle_t nslpmsghandle, tx_attr_t tx_attr, error_t error);
	void set_networknotification(uint32 nslpid, sessionid* sid, mri* mr, error_t notify, uint32 sii_handle=0);
	void set_networknotification(nslpdata* data, uint32 nslpid, sessionid* sid, mri* mr, error_t notify, uint32 sii_handle=0);
        void set_setstatelifetime(mri* mr, uint32 state_lifetime);
	void set_invalidateroutingstate(uint32 nslpid, mri* mr, status_t status, bool urgent);
	void set_register(uint32 nslpid, uint32 rao);
	// @}
	/// clear pointers
	virtual void clear_pointers();
	//const char* get_error_string() const { return Signaling::get_error_string(err); }
	//@{
	/// send message to coordination or signaling module
	bool send(bool exp = false);
	void send_or_delete(bool exp = false) { if (!send(exp)) delete this; }
	//@}
	// @{
	/// confirmation
	//bool set_confirm(bool conf);
	//bool get_confirm() const { return confirm; }
	// @}

	/// get API message subtype
	const char* get_subtype_name() const { return subtypename[subtype]; }

	/// get Error Code string
	const char* get_error_str(error_t status) const { return error_t_str[status]; }
	
        /// get NSLP Identifier
	uint32 get_nslpid() const {return payload_nslpid; }

	/// get RAO value (in register call)
	uint32 get_rao() const { return payload_rao; }

	/// get NSLP Data Payload
	nslpdata* get_data() const {return payload_data; }

	/**
	 * Get the IP address to which a received GIST message was sent.
	 *
	 * NOTE: This is a GISTka extension which is not part of the official
	 *       GIST API!
	 */
	appladdress* get_receiving_addr() const {
		return payload_receiving_addr; }

	/// get NSLP Message Handle
	msghandle_t get_nslpmsghandle() const { return payload_nslpmsghandle; }
	
	/**
	 * Get the Session Identifier.
	 *
	 * NOTE: This GIST implementation makes the session ID available for
	 *       MessageStatus notifications. This is an extension which is
	 *       not part of the official GIST API!
	 */
	sessionid* get_sessionid() const {return payload_sid; }

	/// get Message Routing Information
	mri* get_mri() const {return payload_mr; }

	/// get SII-Handle
	uint32 get_sii_handle() const {return payload_sii_handle; }

	/// get Transfer Attributes
	tx_attr_t get_tx_attr() const {return payload_tx_attr;}

	/// get Timeout
	uint32 get_timeout() const {return payload_timeout;}

	/// get IP-TTL
	uint16 get_ip_ttl() const {return payload_ip_ttl;}

	/// get GIST Hop Count
	uint32 get_ghc() const {return payload_ghc;}
	
	/// get IP Distance
	uint32 get_ip_distance() const {return payload_ip_distance;}

	/// get Adjacency Check Flag
	bool get_adjacency_check() const {return payload_adjacency_check;}
	
	/// get MessageStatus
	error_t get_msgstatus() const {return msgerr;}

	/// get State Lifetime
	uint32 get_state_lifetime() const {return payload_state_lifetime;}

	/// get Routing Status Change instruction
	status_t get_status_change() const {return payload_status;}

	/// get Action Directive from RecvMessage Answer ("relay" / "establish")
	status_t get_directive() const {return payload_status; }

	/// get Urgent Flag for InvalidateRoutingState
	bool get_urgency() const {return payload_urgent;}

	/// look if NSLPID is present
	bool get_sii_presence() const {return sii_present;}

	hostaddress *get_local_addr() const {return local_addr;}
	void set_local_addr(hostaddress *ls) {local_addr = ls;}

        /// inbound interface index
        void set_inbound_ifidx(uint16 if_index) { inbound_if_index= if_index; }

        /// returns inbound interface index (that of the incoming query)
        uint16 get_inbound_ifidx() const { return inbound_if_index; }
        /// is the inbound interface an internal i/f? (inside NAT)
        uint16 is_inbound_if_internal() const { return inbound_if_internal; }

private:
	subtype_t subtype;
	error_t err;

	/// nslp data payload + length encapsulated in nslpdata object
	nslpdata* payload_data;

	/// the IP address to which the GIST peer connected
	appladdress* payload_receiving_addr;

	/// nslp message handle
	msghandle_t payload_nslpmsghandle;
	
	/// NSLP ID identifying the calling NSLP
	uint32 payload_nslpid;


	/// RAO calue for register call
	uint32 payload_rao;

	/// Session Identification
	sessionid* payload_sid;

	/// Message Routing Information (NTLP currently handles: mri_pathcoupled, mri_looseend)
	mri* payload_mr;

	/// SII-Handle
	uint32 payload_sii_handle;

	/// Transfer-Attributes
	tx_attr_t payload_tx_attr;

	/// Timeout
	uint32 payload_timeout;

	/// IP-TTL for transfer
	uint16 payload_ip_ttl;

	/// GIST Hop Count
	uint32 payload_ghc;
	
	/// IP-Distance
	uint32 payload_ip_distance;

	/// Adjacency-Check Flag
	bool payload_adjacency_check;
	
	/// Error information for MessageStatus
	error_t msgerr;

	/// State Lifetime Payload
	uint32 payload_state_lifetime;

	/// Status Change Instruction
	status_t payload_status;

	/// Urgent Flag for InvalidateRoutingState
	bool payload_urgent;

	/// NSLPIDpresent Flag
	bool sii_present;

	/// The local source this message should be sent from
	hostaddress *local_addr;

        /// interface index on which the message was received
        uint16 inbound_if_index;
        /// inbound interface location (internal/external)
        bool inbound_if_internal;

	static const char* const error_t_str[];
	static const char* const subtypename[];
}; // end class APIMsg


///typedef
    typedef hashmap_t<uint32, message::qaddr_t>  nslptable_t;
    typedef nslptable_t::iterator api_iter;

    class NSLPtable {
	
    public:
	// return Message Queue address of fitting API caller or qaddr_unknown if there's none
	inline message::qaddr_t get_address(uint32 nslpid) { 
	    
	    api_iter cur = nslptable.find(nslpid);

	    if (cur != nslptable.end()){
		
		DLog("NSLPtable", "Successfully looked up API caller address for NSLPID " << nslpid);
		return ((*cur).second);    
	    
	    } 

	    DLog("NSLPtable", "Could not look up API caller for NSLPID " << nslpid);
	    //if there is no address found
	    return message::qaddr_unknown;
	     
	}

	inline void save_address(uint32 nslpid, message::qaddr_t address)
	{
	    DLog("NSLPtable", "Saving address for NSLP ID " << nslpid << " at " << address);
	    nslptable[nslpid] = address;
        }
	inline
	NSLPtable() {}
	inline
	~NSLPtable() {}
	
/* XXX:   private: */
	nslptable_t nslptable;
	
    };








} // end Namespace ntlp

#endif
