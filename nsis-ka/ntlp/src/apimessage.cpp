/// ----------------------------------------*- mode: C++; -*--
/// @file apimessage.cpp
/// GIST internal API Message
/// ----------------------------------------------------------
/// $Id: apimessage.cpp 5249 2010-04-26 08:40:59Z roehricht $
/// $HeadURL: https://svn.tm.kit.edu/nsis/ntlp/trunk/src/apimessage.cpp $
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
 *
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


#include "apimessage.h"


namespace ntlp {
    using namespace protlib;
    using namespace protlib::log;



    APIMsg::APIMsg() :
	message(type_API), 
	subtype(ignore), 
	err(error_ok),
	payload_data(NULL),
	payload_receiving_addr(NULL),
	payload_nslpmsghandle(0),
	payload_nslpid(0),
	payload_rao(0),
	payload_sid(NULL),
	payload_mr(NULL),
	payload_sii_handle(0),
	payload_timeout(0),
	payload_ip_ttl(0),
	payload_ghc(0),
	payload_ip_distance(0),
	payload_adjacency_check(false),
	msgerr(error_ok),
	payload_state_lifetime(0),
	payload_status(good),
	payload_urgent(false),
	local_addr(0),
	inbound_if_index(0),
	inbound_if_internal(false)
{
	payload_tx_attr.reliable=false;
	payload_tx_attr.secure=false;
}
    
    APIMsg::~APIMsg() {


	if (payload_data) delete payload_data;
	if (payload_receiving_addr) delete payload_receiving_addr;
	if (payload_sid) delete payload_sid;
	if (payload_mr) delete payload_mr;
	if (local_addr) delete local_addr;

    } // end destructor





    const char* const APIMsg::subtypename[] = {
	"ignore",
	"Register",
	"SendMessage",
	"RecvMessage",
	"RecvMessageAnswer",
	"MessageStatus",
	"NetworkNotification",
	"SetStateLifetime",
	"InvalidateRoutingState"
    }; // end subtypename
	

    const char* const APIMsg::error_t_str[] = {
    	    //Error Codes for API messages
	    "error_ok",
	    "error_msg_delivered",
	    "error_bad_target",
	    
	    //Error Codes used in error payload (MessageStatus msg)
	    "error_msg_node_not_found",

	    //Error Codes used in network notification payload
	    "last_node",
	    "route_changed_status_invalid",
	    "route_changed_status_good",
	    "route_changed_status_bad",
	    "route_changed_status_tentative",
	    "route_changed_status_home_binding",
	    "route_changed_status_binding"
#ifdef USE_AHO
	    ,
	    "anticipated_handover_event",
	    "handover_event"
#endif
    };



  /// Methods for setting API Message type and filling in payload objects


    void APIMsg::set_sendmessage(nslpdata* data, msghandle_t nslpmsghandle, uint32 nslpid, sessionid* sid, mri* mr, uint32 sii_handle, tx_attr_t tx_attr, uint32 timeout, uint16 ip_ttl, uint32 ghc)
    {
	//set fitting subtype
	subtype=SendMessage;

       
	//take over given objects/pointers
	payload_data=data;
	payload_nslpmsghandle=nslpmsghandle;
	payload_nslpid=nslpid;
	payload_sid=sid;
	payload_mr=mr;
	payload_sii_handle=sii_handle;
	payload_tx_attr.secure=tx_attr.secure;
	payload_tx_attr.reliable=tx_attr.reliable;
	payload_timeout=timeout;
	payload_ip_ttl=ip_ttl;
	payload_ghc=ghc;

    }


    void APIMsg::set_sendmessage(nslpdata* data, msghandle_t nslpmsghandle, uint32 nslpid, sessionid* sid, mri* mr, tx_attr_t tx_attr, uint32 timeout, uint16 ip_ttl, uint32 ghc)
    {
	// set fitting subtype
	subtype=SendMessage;

       
	payload_sii_handle= 0;
	// take over given objects/pointers
	payload_data= data;
	payload_nslpmsghandle= nslpmsghandle;
	payload_nslpid= nslpid;
	payload_sid= sid;
	payload_mr= mr;

	payload_tx_attr.secure= tx_attr.secure;
	payload_tx_attr.reliable= tx_attr.reliable;
	payload_timeout= timeout;
	payload_ip_ttl= ip_ttl;
	payload_ghc= ghc;

    }

    

    // this is the old set_recvmessage, without the own_addr parameter
    void APIMsg::set_recvmessage(nslpdata* data, uint32 nslpid, sessionid* sid,
		mri* mr, bool adjacency_check, uint32 sii_handle,
		tx_attr_t tx_attr, uint16 ip_ttl, uint16 ip_distance,
				 uint32 ghc, uint16 inbound_if_idx) {

	    set_recvmessage(data, NULL, nslpid, sid, mr, adjacency_check,
			    sii_handle, tx_attr, ip_ttl, ip_distance, ghc, inbound_if_idx);
    }

    void APIMsg::set_recvmessage(nslpdata* data, appladdress *own_addr, uint32 nslpid, sessionid* sid, mri* mr, bool adjacency_check, uint32 sii_handle, tx_attr_t tx_attr, uint16 ip_ttl, uint16 ip_distance, uint32 ghc, uint16 inbound_if_idx){


	// set fitting subtype
	subtype=RecvMessage;

	// take over given objects/pointers
	payload_data=data;
	payload_receiving_addr=own_addr;
	payload_nslpid=nslpid;
	payload_sid=sid;
	payload_mr=mr;
	payload_adjacency_check=adjacency_check;
	payload_sii_handle=sii_handle;
	payload_tx_attr.secure=tx_attr.secure;
	payload_tx_attr.reliable=tx_attr.reliable;
	payload_tx_attr.final_hop=tx_attr.final_hop;
	payload_ip_ttl=ip_ttl;
	payload_ip_distance=ip_distance;
	payload_ghc=ghc;

	inbound_if_index= inbound_if_idx;
    }

    void APIMsg::set_messagestatus(uint32 nslpid, sessionid* sid, msghandle_t nslpmsghandle, tx_attr_t tx_attr, error_t error){

	// set fitting subtype
	subtype=MessageStatus;

	// set NSLPID
	payload_nslpid = nslpid;
       
	// set Session-ID
	payload_sid = sid;

	// take over given objects/pointers
	payload_tx_attr.secure=tx_attr.secure;
	payload_tx_attr.reliable=tx_attr.reliable;

	payload_nslpmsghandle=nslpmsghandle;
	msgerr=error;

    }

    void APIMsg::set_networknotification(uint32 nslpid, sessionid* sid, mri* mr, error_t notice, uint32 sii_handle){

	// set fitting subtype
	subtype=NetworkNotification;

	//take over given objects/pointers
	payload_nslpid=nslpid;
	payload_sid=sid;
	payload_mr=mr;
	msgerr=notice;
	//sii_handle
	payload_sii_handle=sii_handle;

    }

    void APIMsg::set_networknotification(nslpdata* data, uint32 nslpid, sessionid* sid, mri* mr, error_t notice, uint32 sii_handle) {

	set_networknotification(nslpid, sid, mr, notice, sii_handle);

	//take over given objects/pointers
	payload_data=data;

    }

    void APIMsg::set_setstatelifetime(mri* mr, uint32 state_lifetime){

	// set fitting subtype
	subtype=SetStateLifetime;

	// take over given objects/pointers

	payload_mr=mr;
	payload_state_lifetime=state_lifetime;

    }

    void APIMsg::set_recvmessageanswer(uint32 nslpid, sessionid* sid, mri* mr, nslpdata* data, status_t directive){

	// set fitting subtype
	subtype=RecvMessageAnswer;

	// take over given objects/pointers

	payload_data=data;
	payload_status=directive;
	payload_nslpid=nslpid;
	payload_sid = sid;
	payload_mr = mr;

    }

    void APIMsg::set_invalidateroutingstate(uint32 nslpid, mri* mr, status_t status, bool urgent){

	// set fitting subtype
	subtype=InvalidateRoutingState;

	// take over given objects/pointers
	payload_nslpid=nslpid;
	payload_mr=mr;
	payload_status=status;
	payload_urgent=urgent;
       
    }


    void APIMsg::set_register(uint32 nslpid, uint32 rao) {
	
	subtype=Register;

	payload_nslpid=nslpid;
	payload_rao = rao;

    }


    void APIMsg::clear_pointers() {
	delete payload_data;
	delete payload_mr;
	delete payload_sid;
	delete local_addr;

    }








} // end namespace protlib
