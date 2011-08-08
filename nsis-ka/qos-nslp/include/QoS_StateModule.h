/// ----------------------------------------*- mode: C++; -*--
/// @file QoS_StateModule.h
/// QoS NSLP protocol state machine, Signaling Data
/// ----------------------------------------------------------
/// $Id: QoS_StateModule.h 6268 2011-06-15 14:21:40Z bless $
/// $HeadURL: https://svn.tm.kit.edu/nsis/qos-nslp/trunk/include/QoS_StateModule.h $
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
/** @ingroup QoS_StateModule
 * @file
 * NSLP QoS protocol state machine
 */

#ifndef _NSLP__QOS_STATEMODULE_H_
#define _NSLP__QOS_STATEMODULE_H_

#include "protlib_types.h"
#include "nslp_pdu.h"
#include "network_message.h"
#include "threadsafe_db.h"
#include "logfile.h"
#include "timer_module.h"
#include "rmf.h"
#include "reservemsg.h"
#include "querymsg.h"
#include "responsemsg.h"
#include "notifymsg.h"
#include "aggregate.h"
#include "QoS_Appl_Msg.h"
#include "SignalingAppl.h"
#include "address.h"
#include "mri_pc.h"
#include "mri_est.h"
#include "flowinfo.h"
#include "apimessage.h"
#include "addresslist.h"
#include "nslp_aho_context.h"
#include "nslp_session_context.h"
#include "nslp_flow_context.h"
#include "vlsp_object.h"

using namespace protlib;
using namespace protlib::log;

namespace qos_nslp {
    //using namespace log;
/** @addtogroup QoS_StateModule NSLP protocol state machine
 * @{
 */

class NSLP_Session_Context_Map;
class AhoContextMap;

class state_manager {
public:
	/// QoS NSLP state machine errors
	enum error_t {
		error_ok,
		error_ok_forward,
		error_ok_send_response,
		error_no_bandwidth,
		error_old_pdu,
		error_nothing_to_do,
		error_rsn_missing,
		error_qspec_missing,
		error_internal,
		error_wrong_pdu_type,
		error_no_request_found,
		error_no_mem,
		error_serialize,
		error_pdu_too_big,
		error_send_error_pdu,
		error_max_val
	}; // end error_t


  /// constructor
  state_manager();
  state_manager(AddressList &addresses, Flowinfo &fi_service);
  ~state_manager();
  void* timerchecker(void *p);
  state_manager* get_nslp();

  error_t process_tp_recv_msg(NetMsg& msg, known_nslp_pdu*& respdu, bool down, 
			      const ntlp::sessionid* sid, const ntlp::mri_pathcoupled* rcvd_mri, uint32 sii);

  void process_tp_recv_est_msg(NetMsg& msg, const ntlp::sessionid* sid, const ntlp::mri_explicitsigtarget* rcvd_mri);

  error_t process_outgoing(known_nslp_pdu& rsppdu, NetMsg*& msg, bool down, 
			   const ntlp::sessionid* rcvd_sid, ntlp::mri_pathcoupled** rcvd_mri);

  error_t process_outgoing_est_msg(known_nslp_pdu& rsppdu, NetMsg*& msg, const ntlp::sessionid* rcvd_sid,
		  		ntlp::mri_explicitsigtarget** rcvd_mri);

  error_t find_or_create_new_session_context(known_nslp_pdu &pdu, const ntlp::sessionid *rcvd_sid,
		  const ntlp::mri *rcvd_mri, bool down, NSLP_Session_Context *&session_context, ntlp::Flowstatus *fs);

  error_t update_session_context(known_nslp_pdu &pdu, const ntlp::sessionid *rcvd_sid,
		  const ntlp::mri *rcvd_mri, bool down, uint32 sii_handle, NSLP_Session_Context *session_context, ntlp::Flowstatus *fs);

  error_t create_flow_context(reservereq *reservemessage, const ntlp::sessionid *rcvd_sid,  const ntlp::mri *rcvd_mri, const ntlp::mri_pathcoupled *rcvd_pc_mri, bool down, NSLP_Session_Context *session_context, NSLP_Flow_Context *flow_context, ntlp::Flowstatus *fs, bool &return_on_error);
	  
  error_t update_existing_flow_context(reservereq *reservemessage, const ntlp::sessionid *rcvd_sid,  const ntlp::mri *rcvd_mri, const ntlp::mri_pathcoupled *rcvd_pc_mri, bool down, uint32 sii_handle, NSLP_Session_Context *session_context, NSLP_Flow_Context *flow_context, ntlp::Flowstatus *fs, bool &return_on_error);

  void forward_reserve(const reservereq* reservemsg, NSLP_Session_Context* context, const ntlp::sessionid *sid, const ntlp::mri* fwd_mri, bool down=false);

  error_t  process_rii(NSLP_Session_Context* context, reservereq* reservemsg, const ntlp::sessionid* rcvd_sid, 
		       TimerMsg* rii_tmsg, bool is_qni, bool is_qnr, const ntlp::mri_pathcoupled* rcvd_pc_mri, bool down, bool is_merging_node);

  error_t  process_qspec(reservereq* reservemsg, qspec_object *&qspec_obj);

  void send_response_with_rii(const rii* rii, const ntlp::sessionid* sid, const ntlp::mri* mri, bool down,
			      info_spec::errorclass_t err_class, info_spec::errorcode_t err_code, const session_auth_object* sauth_object= NULL, const vlsp_object* vlsp_obj= NULL);

  void send_response_with_rsn(const rsn* rsn, const ntlp::sessionid* sid, const ntlp::mri* mri, bool down,
		  info_spec::errorclass_t err_class, info_spec::errorcode_t err_code);

  void send_refreshing_reserve(const ntlp::sessionid &sid, const ntlp::mri_pathcoupled &mri,
		  NSLP_Session_Context *session_context, NSLP_Flow_Context *flow_context);

  void process_response_msg(known_nslp_pdu* known_pdu, const ntlp::sessionid* rcvd_sid, const ntlp::mri* rcvd_mri, bool down);

  void process_notify_msg(known_nslp_pdu* known_pdu, const ntlp::sessionid* rcvd_sid, const ntlp::mri_pathcoupled* rcvd_mri, bool down, error_t& nslpres);

  void forward_notify_msg(notifymsg* notify, bool down, const ntlp::mri_pathcoupled* rcvd_mri, const ntlp::sessionid* sid);

  void forward_response_msg(responsemsg* response, const ntlp::sessionid* sid, const ntlp::mri* fwd_mri, bool down=false);

  void process_info_spec_for_appl(const ntlp::sessionid* rcvd_sid, const info_spec* info_spec, const vlsp_object* vlsp_obj= NULL);  

  void process_query_msg(known_nslp_pdu *known_pdu, const ntlp::sessionid *rcvd_sid, const ntlp::mri *rcvd_mri, bool down,
		  NSLP_Session_Context *session_context, ntlp::Flowstatus *fs);

  void send_response_to_query(rii* r, qspec_object* q, bool down, const ntlp::sessionid* sid, const ntlp::mri_pathcoupled* rcvd_mri);

  void create_and_send_reserve_msg_as_response_to_query(bool send_rii, qspec_object *qspec, const bound_sessionid *bsid, 
							bool down, const ntlp::sessionid *sid, const ntlp::mri_pathcoupled *rcvd_mri,
							NSLP_Session_Context *session_context, bool replace=true);

  void forward_query_msg(querymsg* q, bool down, const ntlp::sessionid* sid, const ntlp::mri_pathcoupled* rcvd_mri);

  void notify_application(QoS_Appl_Msg::qos_info_type_t info, const ntlp::sessionid* rcvd_sid);

  void retransmit_message_with_rii(const ntlp::sessionid *sid, const ntlp::mri_pathcoupled *mri, NSLP_Session_Context *session_context,
		  querymsg *q, reservereq *r, rii *t_rii);

  void retransmit_message_with_different_rii(NSLP_Session_Context *context, rii *context_rii, bool down, 
					const ntlp::sessionid *rcvd_sid, const ntlp::mri_pathcoupled *rcvd_mri);

//  void send_reserve_with_tear_on_the_old_path(NSLP_Context* context, const ntlp::sessionid* rcvd_sid, 
//                                              const ntlp::mri_pathcoupled* rcvd_mri, bool down);

  void send_notify_on_the_old_path(const ntlp::sessionid* rcvd_sid, const ntlp::mri_pathcoupled* rcvd_mri, bool down, uint32 old_sii,
		  info_spec::errorclass_t e_class, info_spec::errorcode_t e_code);

  void tear_down_other_flow(const ntlp::sessionid* rcvd_sid, const ntlp::mri_pathcoupled* rcvd_mri, bool down,
		  NSLP_Session_Context *session_context, NSLP_Flow_Context *flow_context);

  void copy_reserve_message(const reservereq* source, reservereq* dest);
  void copy_query_message(const querymsg* source, querymsg* dest);
  void copy_notify_message(const notifymsg* source, notifymsg* dest);

  void process_sii_handle(const ntlp::sessionid *rcvd_sid, const ntlp::mri_pathcoupled *rcvd_mri, bool down, uint32 sii_handle, APIMsg::error_t status);

  void delete_flow_context(const ntlp::sessionid* sid, const ntlp::mri_pathcoupled* flow,
		  NSLP_Session_Context *session_context, NSLP_Flow_Context *flow_context);

  void process_mobility(ntlp::mri_pathcoupled *rcvd_mri, ntlp::APIMsg::error_t status);

  void send_mobility_refresh(NSLP_Session_Context *session_context, NSLP_Flow_Context *flow_context, ntlp::Flowstatus *fs);

  void create_tunnel_reserve(NSLP_Session_Context *session_context, NSLP_Flow_Context *flow_context);
#ifdef USE_AHO
  void process_recv_est_msg_aho(known_nslp_pdu *known_pdu, const ntlp::sessionid* sid, const ntlp::mri_explicitsigtarget* rcvd_mri);
  void process_aho_event(const string iface, macaddress l2_ap_n, hostaddress l3_ar_n, hostaddress l3_mn);
  void process_ho_event(const string iface, macaddress l2_ap_n, hostaddress l3_ar_n, hostaddress l3_mn);
  void aho_phase_1(NSLP_Session_Context *session_context, void *param1, void *param2);
  void ho_phase_3_2a(NSLP_Session_Context *session_context, void *param1, void *param2);
  bool is_new_access_router_and_flow_matches(const ntlp::sessionid &sid, const ntlp::mri_pathcoupled &rcvd_mri, ntlp::mri_explicitsigtarget *fwd_mri=NULL);
#endif


  static const char* error_string(error_t err) { return err < error_max_val ? errstr[err] : "** invalid error code, must update error strings **"; }

private:
  static const char *const modname;
  rmf rmf_admin;
  /// coding scheme to use
  IE::coding_t coding;
  /// accept unauthenticated messages
  const bool accept_unauthenticated;
  AddressList *addresses;
  Flowinfo *fi_service;
  // reference to nslp_session_context_map
  NSLP_Session_Context_Map* session_context_map;
  // reference to aho_context_map
  AhoContextMap* aho_contextmap;

  // return whether this node is a last hop for this data flow
  bool is_last_signaling_hop(const ntlp::mri_pathcoupled* flow_mri, bool down = true) const;
  bool is_last_signaling_hop(const ntlp::mri_explicitsigtarget* mri) const;
  bool is_flow_source(const ntlp::mri_pathcoupled* mri) const;
  bool is_flow_destination(const ntlp::mri_pathcoupled* mri) const;
  static const char *const errstr[];

  error_t generate_pdu(const known_nslp_pdu& pdu, NetMsg*& msg);
}; // end class state_manager


//@}

} // end namespace qos_nslp

#endif // _NSLP__QOS_STATEMODULE_H_

