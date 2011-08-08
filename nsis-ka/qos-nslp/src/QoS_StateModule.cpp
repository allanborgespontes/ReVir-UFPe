/// ----------------------------------------*- mode: C++; -*--
/// @file QoS_StateModule.cpp
/// QoS NSLP protocol state machine
/// ----------------------------------------------------------
/// $Id: QoS_StateModule.cpp 6328 2011-07-26 01:45:14Z bless $
/// $HeadURL: https://svn.tm.kit.edu/nsis/qos-nslp/trunk/src/QoS_StateModule.cpp $
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

#include "protlib_types.h"
#include "QoS_StateModule.h"
#include "reservemsg.h"
#include "responsemsg.h"
#include "notifymsg.h"
#include "querymsg.h"
#include "logfile.h"
#include "cleanuphandler.h"
#include "ProcessingModule.h"
#include "timer_module.h"
#include "ie.h"
#include "qos_nslp_conf.h"
#include "qos_nslp_aho_contextmap.h"
#include "nslp_session_context_map.h"
#include "context_manager.h"

#include "benchmark_journal.h"

#include <netdb.h>
#include <unistd.h>
#include <cmath>

#ifdef NSIS_OMNETPP_SIM
#include <omnetpp.h>
#endif

using namespace protlib;
using namespace protlib::log;



namespace qos_nslp {

extern benchmark_journal journal;

  /** @addtogroup QoS_StateModule QoS NSLP protocol state machine
   * @{
   */

  const char *const state_manager::modname="QoSStateModule";
  const char *const state_manager::errstr[] = {
    "ok",
    "ok_to_forward",
    "ok_send_response",
    "not_enough_bandwidth",
    "pdu_out_of_order",
    "everything_has_been_done",
    "rsn_missing_by_reserve",
    "no_qspec_by_new_rsn",
    "internal",
    "wrong_pdu_type",
    "no_request_found",
    "no_mem",
    "serialize",
    "pdu_too_big",
    "send_error_pdu",
    "END OF NSLPERROR STRINGS"
  };


/***** class state_manager *****/
state_manager::state_manager(AddressList &addresses, Flowinfo &fi_service)
	: coding(IE::nslp_v1), accept_unauthenticated(true),
	  addresses(&addresses), fi_service(&fi_service)  {
        session_context_map = Context_Manager::instance()->get_scm();
#ifdef USE_AHO
        aho_contextmap = Context_Manager::instance()->get_acm();
#endif
}
/** Default constructor for SESSIONID without any params. The value of SESSIONID will be set to the default.
 */
state_manager::state_manager()
	: coding(IE::nslp_v1), accept_unauthenticated(true)  {
        session_context_map = Context_Manager::instance()->get_scm();
    	abort();
}

/** Destructor of the state_manager class.  */
state_manager::~state_manager() {
} // end destructor


/** returns whether this QNE is the last hop for QoS NSLP signaling
 * this is true if flow destination is one of my addresses or if
 * I'm acting as proxy for the flow destination address
* @param recv_mri path-coupled MRI
*/
bool
state_manager::is_last_signaling_hop(const ntlp::mri_pathcoupled* flow_mri, bool down) const
{
  // choose own address for comparison according to protocol type
  const hostaddress& flow_dest = down ? flow_mri->get_destaddress() : flow_mri->get_sourceaddress();

  // if flow source is my address, I am a QNI
  if (addresses->addr_is(flow_dest, AddressList::ConfiguredAddr_P) ||
      addresses->addr_is(flow_dest, AddressList::LocalAddr_P))
        return true;
  else // if flow dest is my address, I am a QNR
        return false;
}


/** returns whether this Entity is the last hop for QoS NSLP signaling
 * this is true if the signaling destination is one of my addresses
 * or if I'm acting as proxy for the signaling destination address
 * @param recv_mri EST-MRI
 */
bool
state_manager::is_last_signaling_hop(const ntlp::mri_explicitsigtarget* mri) const
{
  // choose own address for comparison
  const hostaddress& sig_dest = mri->get_dest_sig_address();

  if (addresses->addr_is(sig_dest, AddressList::ConfiguredAddr_P) ||
      addresses->addr_is(sig_dest, AddressList::LocalAddr_P)) {
		return true;
	}
	else {
		return false;
	}
}


/** returns whether this entity is the source of the flow
 * described by the given PC-MRI.
 * @param pc_mri PC-MRI
 */
bool
state_manager::is_flow_source(const ntlp::mri_pathcoupled* mri) const
{
  if(mri == NULL) {
    return false;
  }

  // choose own address for comparison
  const hostaddress& flow_src = mri->get_sourceaddress();

  if (addresses->addr_is(flow_src, AddressList::ConfiguredAddr_P) ||
      addresses->addr_is(flow_src, AddressList::LocalAddr_P)) {
        return true;
  }
  else {
        return false;
  }
}


/** returns whether this entity is the destination of the flow
 * described by the given PC-MRI.
 * @param pc_mri PC-MRI
 */
bool
state_manager::is_flow_destination(const ntlp::mri_pathcoupled* mri) const
{
  if(mri == NULL) {
    return false;
  }

  // choose own address for comparison
  const hostaddress& flow_dest = mri->get_destaddress();

  if (addresses->addr_is(flow_dest, AddressList::ConfiguredAddr_P) ||
      addresses->addr_is(flow_dest, AddressList::LocalAddr_P)) {
        return true;
  }
  else {
        return false;
  }
}


/** process/parse received incoming message from network.
* This method parses the netmsg and returns the parsed PDU
* in respdu.
* @param netmsg Incoming Network message (byte buffer).
* @retval resultpdu Result PDU (or errormsg case of error).
* @param down set to TRUE, if the direction is downstream, otherwise set to FALSE.
* @param sid the session id the current message belongs to.
* @param rcvd_mri the MRI of the current session.
* @return the normal return code is error_nothing_to_do
*/
state_manager::error_t 
state_manager::process_tp_recv_msg(NetMsg& netmsg, known_nslp_pdu*& resultpdu, bool down, 
		  const ntlp::sessionid* sid, const ntlp::mri_pathcoupled* rcvd_mri, uint32 sii)
{
	// initialize result variables
	resultpdu = NULL;
	error_t nslpres;
	nslpres = error_ok;

	// parse NetMsg
	IEErrorList errorlist;
	uint32 nbytesread = 0;
	protlib::IE* tmpie = NSLP_IEManager::instance()->deserialize(netmsg, NSLP_IE::cat_known_nslp_pdu, coding, errorlist, nbytesread, true);
	DLog(state_manager::modname, "process_tp_recv_msg(): deserialization completed (" << nbytesread << " bytes).");

	nslp_pdu* pdu = (tmpie != NULL) ? dynamic_cast<nslp_pdu*>(tmpie) : NULL;

	// known_pdu contains parsed objects
	known_nslp_pdu* known_pdu = pdu ? dynamic_cast<known_nslp_pdu*>(pdu) : NULL;

	assert(pdu != NULL);
	assert(known_pdu != NULL);

	DLog(state_manager::modname, "process_tp_recv_msg() - " << color[blue] << "received " << known_pdu->get_name() << color[off]);

	// check for incoming reserve message
	if (known_pdu->is_reserve()) {

		// Notify connected applications about current state
		QoS_Appl_Msg* applmsg = new QoS_Appl_Msg();
		applmsg->set_pdu_type(known_nslp_pdu::RESERVE);
		applmsg->set_sid(*sid);
		const appladdress src_addr(rcvd_mri->get_sourceaddress(), rcvd_mri->get_protocol(), rcvd_mri->get_sourceport());
		const appladdress dst_addr(rcvd_mri->get_destaddress(), rcvd_mri->get_protocol(), rcvd_mri->get_destport());
		applmsg->set_source_addr(src_addr);
		applmsg->set_dest_addr(dst_addr);
		applmsg->set_info_spec(known_pdu->get_errorobject()); // must be NULL
		if (known_pdu->get_qspec()) applmsg->set_qspec(known_pdu->get_qspec()->get_qspec_data());
		applmsg->set_vlsp_object(known_pdu->get_vlsp_object());
		applmsg->set_session_auth_object(known_pdu->get_sessionauth());
		applmsg->set_session_auth_object_hmac_signed(known_pdu->get_sessionauth_hmac_signed());
		applmsg->set_source(message::qaddr_appl_qos_signaling);
		applmsg->send_or_delete();


		NSLP_Session_Context *session_context=NULL;
		nslpres = find_or_create_new_session_context(*known_pdu, sid, rcvd_mri, down, session_context, NULL);

		DLog(state_manager::modname, "find_or_create_new_session_context() - returned: " << error_string(nslpres));

		if (nslpres == error_ok) {
			nslpres = update_session_context(*known_pdu, sid, rcvd_mri, down, sii, session_context, NULL);

			DLog(state_manager::modname, "update_session_context() - returned: " << error_string(nslpres));
		}

		if ((nslpres != error_ok) && 
		    (nslpres != error_ok_send_response) && 
		    (nslpres != error_ok_forward))
			return nslpres;

		resultpdu = known_pdu;
		// reset to start and set message length correctly, 
		// because we will just forward this message
		netmsg.truncate();
		// NOTE: returning error_nothing_to_do will delete known_pdu in processing
		// error_ok, error_ok_send_response, error_ok_forward will be mapped to error_nothing_to_do
		nslpres = error_nothing_to_do; 
		return nslpres;
	}

	if (known_pdu->is_response()) {
		process_response_msg(known_pdu, sid, rcvd_mri, down);
		resultpdu = known_pdu;
		// reset to start and set message length correctly, 
		// because we will just forward this message
		netmsg.truncate();
		return state_manager::error_nothing_to_do;
	}

	if (known_pdu->is_query()) {
		querymsg *query = dynamic_cast<querymsg *>(known_pdu);
		assert(query != NULL);

		NSLP_Session_Context *session_context=NULL;
		if (down && query->is_reserve_init_flag()) {
			error_t err = find_or_create_new_session_context(*known_pdu, sid, rcvd_mri, down, session_context, NULL);

			DLog(state_manager::modname, "find_or_create_new_session_context() - returned: " << error_string(err));
		}
		else {
			session_context = session_context_map->find_session_context(*sid);
		}

	
		process_query_msg(known_pdu, sid, rcvd_mri, down, session_context, NULL);
		resultpdu = known_pdu;
		// reset to start and set message length correctly, 
		// because we will just forward this message
		netmsg.truncate();
		return state_manager::error_nothing_to_do;
	}

	if (known_pdu->is_notify()) {
		process_notify_msg(known_pdu, sid, rcvd_mri, down, nslpres);
		resultpdu = known_pdu;
		// reset to start and set message length correctly, 
		// because we will just forward this message
		netmsg.truncate();
		return nslpres;
	}

	return state_manager::error_wrong_pdu_type;
}


/** process / parse received incoming EST-MRM message from network.
* @param netmsg Incoming Network message (byte buffer).
* @param sid the session id the current message belongs to.
* @param rcvd_mri the EST-MRI of the current session.
*/
void state_manager::process_tp_recv_est_msg(NetMsg& netmsg, const ntlp::sessionid* sid, const ntlp::mri_explicitsigtarget* rcvd_mri) {
	// parse NetMsg
	IEErrorList errorlist;
	uint32 nbytesread = 0;
	protlib::IE *tmpie = NSLP_IEManager::instance()->deserialize(netmsg, NSLP_IE::cat_known_nslp_pdu, coding, errorlist, nbytesread, true);
	DLog(state_manager::modname, "process_tp_recv_est_msg(): deserialization completed (" << nbytesread << " bytes).");

	nslp_pdu *pdu = (tmpie != NULL) ? dynamic_cast<nslp_pdu*>(tmpie) : NULL;

	// known_pdu contains parsed objects
	known_nslp_pdu *known_pdu = pdu ? dynamic_cast<known_nslp_pdu*>(pdu) : NULL;

	assert(pdu != NULL);
	assert(known_pdu != NULL);

	assert(rcvd_mri != NULL);

	DLog(state_manager::modname, "process_tp_recv_est_msg() - " << color[blue] << "received " << known_pdu->get_name() << color[off]);

        // RB-REMARK: this should also handle normal processing not only AHO related stuff
#ifdef USE_AHO
	process_recv_est_msg_aho(known_pdu,  sid, rcvd_mri);
#endif
}


/** prepares sending of a NSLP PDUs via the network.
* @param pdu NSLP PDU to be sent.
* @param resultid Internal Id of signaling table entry (is returned)
* @param netbuffer NetMsg buffer for serialization of the PDU into a bytes stream (is returned).
* @return internal NSLP_error_t errorcode, usually error_ok for success.
* @param rcvd_sid the session id the current message belongs to.
* @param rcvd_mri the MRI of the current session.
*
* @note this method is called by the ProcessingModule
*/
state_manager::error_t state_manager::process_outgoing(known_nslp_pdu& pdu, NetMsg*& netbuffer, bool down, 
		const ntlp::sessionid* rcvd_sid, ntlp::mri_pathcoupled** rcvd_mri)
{
	error_t nslpres = error_ok;
	error_t ret_nslpres = error_ok;
	netbuffer = NULL;
//	bool updated = false;
	Flowstatus *fs = NULL;


#ifdef USE_FLOWINFO
	fs = fi_service->get_flowinfo(**rcvd_mri);
	switch (fs->type) {
	case ntlp::Flowstatus::fs_normal:
		*rcvd_mri = fs->new_mri;
		break;
	case ntlp::Flowstatus::fs_tunnel:
	case ntlp::Flowstatus::fs_nothing:
	default:
		break;
	}

	ILog(state_manager::modname, "Flowstatus: " << fs->type << fs->orig_mri << *fs->new_mri);
#endif

	if (rcvd_sid) {
		ILog(state_manager::modname, "process_outgoing(). SID [" <<  rcvd_sid->to_string() << "]");
	}
	else {
		ILog(state_manager::modname, "process_outgoing(). SID [undefined]");
	}
	
	hostaddress orig_a, source_a, dest_a;
	source_a = (*rcvd_mri)->get_sourceaddress();
	dest_a = (*rcvd_mri)->get_destaddress();


	if (pdu.is_notify()) {
		ILog(state_manager::modname, color[magenta] << "Outgoing NOTIFY message!" << color[off]);
	}


	if (pdu.is_response()) {
		ILog(state_manager::modname, color[magenta] << "Outgoing RESPONSE message!" << color[off]);
	}


	if (pdu.is_reserve()) {
		ILog(state_manager::modname, color[magenta] << "Outgoing RESERVE message!" << color[off]);

		reservereq *reserve = dynamic_cast<reservereq *>(&pdu);
		bool processing = reserve->get_originator();

		// QSPEC and RSN processing
		NSLP_Session_Context *session_context = session_context_map->find_session_context(*rcvd_sid);
		if (session_context != NULL) {

			rsn *rcvd_rsn_obj = reserve->get_rsn();
			if (rcvd_rsn_obj != NULL) {

				bool add_qspec = false;
				session_context->lock();
				if (session_context->is_own_rsn_last_sent_set_for_flow(**rcvd_mri)) {
					const rsn& own_rsn_last_sent= session_context->get_own_rsn_last_sent_for_flow(**rcvd_mri);

					// add a qspec in case that we have NEW state according to RSNs
					if ( (rcvd_rsn_obj->get_epoch_id() != own_rsn_last_sent.get_epoch_id() )
					     || 
					     (*rcvd_rsn_obj > own_rsn_last_sent) ) {
						add_qspec = true;
					}
				}
				else {
					add_qspec = true;
				}

				// add local QSPEC
				if (add_qspec && (reserve->get_qspec() == NULL)) {
					NSLP_Flow_Context *flow_context = session_context->find_flow_context(**rcvd_mri);
					if (flow_context != NULL) {
						flow_context->lock();
						qspec_object qspec_obj = flow_context->get_qspec();
						flow_context->unlock();

						reserve->set_qspec(qspec_obj.copy());
					}
				}
				
				// update last sent own RSN
				session_context->set_own_rsn_last_sent_for_flow(**rcvd_mri, *rcvd_rsn_obj);
				session_context->unlock();
			} // end if rcvd_rsn_obj

			session_context->lock();
			
			reserve->set_rsn((session_context->get_own_rsn()).copy());

		       	session_context->unlock();

			ILog(state_manager::modname, "update_session_context() - Set own RSN to: " << reserve->get_rsn());


		} // end if session context exists


		if (pdu.is_tear_flag()) {
			ILog(state_manager::modname, "TEAR FLAG is set to TRUE!");

			// do not process session state afterwards
			processing = false;

			// get current RSN
			rsn* current_rsn = reserve->get_rsn();

			NSLP_Session_Context *session_context = session_context_map->find_session_context(*rcvd_sid);
			if (session_context != NULL) {
				session_context->lock();

				// since state is manipulated, we must increase the RSN
				session_context->increment_own_rsn();
				*current_rsn= session_context->get_own_rsn();

				NSLP_Flow_Context *flow_context = session_context->find_flow_context(**rcvd_mri);
				if (flow_context != NULL) {
					delete_flow_context(rcvd_sid, *rcvd_mri, session_context, flow_context);
				}

				session_context->unlock();
			}

			if (current_rsn)
				ILog(state_manager::modname, "Own RSN: " << *current_rsn);
			else
				ERRLog(state_manager::modname, "Own RSN missing - this should not happen.");

		} // end if is TEARing RESERVE

		// perform session state context manipulation/update if necessary
		// this is only necessary for nodes that are QNI
		if (processing) {
			if (pdu.is_reserve()) {
				NSLP_Session_Context *session_context=NULL;
				ret_nslpres = find_or_create_new_session_context(pdu, rcvd_sid, *rcvd_mri, down, session_context, fs);
				
				DLog(state_manager::modname, "find_or_create_new_session_context() - returned: " << error_string(ret_nslpres));
				
				if (ret_nslpres == error_ok) {
					ret_nslpres = update_session_context(pdu, rcvd_sid, *rcvd_mri, down, 0, session_context, fs);
				
					DLog(state_manager::modname, "update_session_context() - returned: " << error_string(ret_nslpres));
				}
			}
			
			if ((ret_nslpres!=error_ok) && (ret_nslpres != error_ok_send_response) && (ret_nslpres != error_ok_forward)) {
				if (netbuffer) {
					delete netbuffer;
				}
				
				netbuffer = NULL;
				return ret_nslpres;
			}
		} // end if processing 
	}


	if (pdu.is_query()) {
		ILog(state_manager::modname, color[magenta] << "Outgoing QUERY message!" << color[off]);

		ret_nslpres = error_ok;
		querymsg *query = dynamic_cast<querymsg *>(&pdu);
		    	
		bool continue_it = true;
		bool i_am_originator = query->get_originator();
		if (i_am_originator) {
			rii* check_rii = query->get_rii();
			if (check_rii) {
				if ( ( down && is_last_signaling_hop(*rcvd_mri) )  || ( down == false ) ) {
					// this situation should not happen, QNR MAY send QUERY msgs ONLY w/o RII
					ret_nslpres = error_nothing_to_do;
					ERRLog(state_manager::modname, "QNR attempting to send a QUERY msg with RII or QUERY upstream w/ RII, deleting it!");
					continue_it = false;
				}
			}
			else {
				// no RII object present
				if ( ( down || ((!down) && is_last_signaling_hop(*rcvd_mri) ) ) && query->is_reserve_init_flag() == false ) {
					// this situation should not happen, QNI MAY send QUERY msgs ONLY with RII
					ret_nslpres = error_nothing_to_do;
					ERRLog(state_manager::modname, "QNI attempting to send a QUERY msg w/o RII or R-Flag, delete it!");
					continue_it = false;
				}
			} // end else
		} // end if I am an originator of this QUERY
		    	
		if (i_am_originator && continue_it) {
			// don't know what's the purpose of this conditions (conjunctive form of the conditions above)
			assert( ((query->get_rii() != NULL) && down && !is_last_signaling_hop(*rcvd_mri)) ||
				((query->get_rii() == NULL) && !down && !is_last_signaling_hop(*rcvd_mri)) ||
				((query->get_rii() == NULL) && query->is_reserve_init_flag()) );


			NSLP_Session_Context *session_context=NULL;
			if (down && query->is_reserve_init_flag()) {
				ret_nslpres = find_or_create_new_session_context(pdu, rcvd_sid, *rcvd_mri, down, session_context, fs);
			
				DLog(state_manager::modname, "find_or_create_new_session_context() - returned: " << error_string(ret_nslpres));
			}
			else {
				session_context = session_context_map->find_session_context(*rcvd_sid);
			}


			process_query_msg(&pdu, rcvd_sid, *rcvd_mri, down, session_context, fs);
		}
		
		if (ret_nslpres != error_ok) {
			if (netbuffer) {
				delete netbuffer;
			}

			netbuffer = NULL;
			return ret_nslpres;
		}
	}


	if (is_last_signaling_hop(*rcvd_mri, down)) {
		ERRLog(state_manager::modname, "Trying to send message to myself - I'll stop right here!");
		if (netbuffer) {
			delete netbuffer;
		}

		netbuffer = NULL;
		return error_nothing_to_do;
	}
	

	// now serialize
	nslpres = generate_pdu(pdu,netbuffer);
	if (nslpres != error_ok) {
		if (netbuffer) {
			delete netbuffer;
		}

		netbuffer = NULL;
	}


	return nslpres;
} // end process_outgoing


/** prepares sending of a NSLP PDUs via the network.
* @param pdu NSLP PDU to be sent.
* @param netbuffer NetMsg buffer for serialization of the PDU into a bytes stream (is returned).
* @return internal NSLP_error_t errorcode, usually error_ok for success.
* @param rcvd_sid the session id the current message belongs to.
* @param rcvd_mri the MRI of the current session.
*/
state_manager::error_t state_manager::process_outgoing_est_msg(known_nslp_pdu& pdu, NetMsg*& netbuffer,
		const ntlp::sessionid* rcvd_sid, ntlp::mri_explicitsigtarget** rcvd_mri)
{
	error_t nslpres = error_ok;


	if(rcvd_sid) {
		ILog(state_manager::modname, "process_outgoing_est_msg(). SID [" <<  rcvd_sid->to_string() << "]");
	}
	else {
		ILog(state_manager::modname, "process_outgoing_est_msg(). SID [undefined]");
	}

	// check if MRI state is valid
	if(!(*rcvd_mri)->check()) {
		ERRLog(state_manager::modname, "Invalid EST-MRI state!");
		
		return error_nothing_to_do;
	}

	const ntlp::mri_pathcoupled rcvd_pc_mri = (*rcvd_mri)->get_mri_pc();
	bool down = rcvd_pc_mri.get_downstream();


	if(pdu.is_notify()) {
		ILog(state_manager::modname, color[magenta] << "Outgoing NOTIFY message!" << color[off]);
	}
	
	if(pdu.is_response()) {
		ILog(state_manager::modname, color[magenta] << "Outgoing RESPONSE message!" << color[off]);
	}
	
	if(pdu.is_reserve()) {
		ILog(state_manager::modname, color[magenta] << "Outgoing RESERVE message!" << color[off]);

		reservereq *reserve = dynamic_cast<reservereq *>(&pdu);
		bool processing = reserve->get_originator();


		// prevent from looping endless:
		// if there is no flow context corresponding to the current RESERVE message
		// we either have already processed the message or it is unnecessary to
		// process the message at all
		if (pdu.is_tear_flag()) {
			ILog(state_manager::modname, "TEAR FLAG is set to TRUE!");

			NSLP_Session_Context *session_context = session_context_map->find_session_context(*rcvd_sid);
			if (session_context != NULL) {
				session_context->lock();
				NSLP_Flow_Context *flow_context = session_context->find_flow_context(rcvd_pc_mri);
				if (flow_context == NULL) {
					processing = false;
				}
				session_context->unlock();
			}
			else {
				processing = false;
			}
		}

		if (processing) {
			error_t ret_nslpres = error_ok;

			if (pdu.is_reserve()) {
				NSLP_Session_Context *session_context=NULL;
				ret_nslpres = find_or_create_new_session_context(pdu, rcvd_sid, *rcvd_mri, down, session_context, NULL);
				
				DLog(state_manager::modname, "find_or_create_new_session_context() - returned: " << error_string(ret_nslpres));
				
				if (ret_nslpres == error_ok) {
					ret_nslpres = update_session_context(pdu, rcvd_sid, *rcvd_mri, down, 0, session_context, NULL);
				
					DLog(state_manager::modname, "update_session_context() - returned: " << error_string(ret_nslpres));
				}
			}
			
			if ((ret_nslpres!=error_ok) && (ret_nslpres != error_ok_send_response) && (ret_nslpres != error_ok_forward)) {
				if (netbuffer) {
					delete netbuffer;
				}
				
				netbuffer = NULL;
				return ret_nslpres;
			}
		}
	}
	
	if(pdu.is_query()) {
		ILog(state_manager::modname, color[magenta] << "Outgoing QUERY message!" << color[off]);
	}

	if(is_last_signaling_hop(*rcvd_mri)) {
		ERRLog(state_manager::modname, "Trying to send EST-Message to myself - I'll stop right here!");
//		if (netbuffer) 
//			delete netbuffer;
//		netbuffer = NULL;
		return error_nothing_to_do;
	}

	// now serialize
	nslpres = generate_pdu(pdu, netbuffer);

	if(nslpres!=error_ok) {
		if (netbuffer) {
			delete netbuffer;
		}
		netbuffer = NULL;
	} // end if error


	return nslpres;
} // end process_outgoing_est_msg


/** processes a QUERY message.
 * @param known_pdu NSLP PDU to be processed.
 * @param rcvd_sid the session id the current message belongs to.
 * @param rcvd_mri the MRI of the current session.
 * @param down set to TRUE, if the direction is downstream, otherwise set to FALSE. Only evaluated if MRI is path-coupled.
 */
void state_manager::process_query_msg(known_nslp_pdu* known_pdu, const ntlp::sessionid* rcvd_sid, const ntlp::mri* rcvd_mri, bool down,
		                  NSLP_Session_Context *session_context, ntlp::Flowstatus *fs)
{
	ILog(state_manager::modname, "process_query_msg()");

	querymsg *query = dynamic_cast<querymsg *>(known_pdu);
	assert(query != NULL);

	// (down && query->is_reserve_init_flag()) --> (session_context != NULL)
	assert(!(down && query->is_reserve_init_flag()) || session_context != NULL);
	

	if(rcvd_mri == NULL) {
	        ERRLog(state_manager::modname, "process_query_msg() - rcvd_mri == NULL");
	        return;
	}
	
	const ntlp::mri_pathcoupled *rcvd_pc_mri=NULL;
	if(rcvd_mri->get_mrm() == mri::mri_t_pathcoupled) {
	        rcvd_pc_mri = dynamic_cast<const ntlp::mri_pathcoupled *>(rcvd_mri);
	        assert(rcvd_pc_mri != NULL);
	}
	else if(rcvd_mri->get_mrm() == mri::mri_t_explicitsigtarget) {
	        const ntlp::mri_explicitsigtarget *rcvd_est_mri = dynamic_cast<const ntlp::mri_explicitsigtarget *>(rcvd_mri);
	        assert(rcvd_est_mri != NULL);
	
	        rcvd_pc_mri = &rcvd_est_mri->get_mri_pc();
	        down = rcvd_pc_mri->get_downstream();
	}
	else {
	        ERRLog(state_manager::modname, "process_query_msg() - Unknown MRI with MRI-ID: " << rcvd_mri->get_mrm());
	        return;
	}

	assert((rcvd_mri->get_mrm() == mri::mri_t_pathcoupled) || (rcvd_mri->get_mrm() == mri::mri_t_explicitsigtarget));
	assert(rcvd_pc_mri != NULL);


	// The anticipated handover doesn't need QUERY messages with an included RII object!
	if((rcvd_mri->get_mrm() == mri::mri_t_explicitsigtarget) && (query->get_rii() != NULL)) {
		ERRLog(state_manager::modname, "QUERY message received via EST-MRM including RII. This should not happen!");
		return;
	}

	// (query->get_rii() != NULL) --> (rcvd_mri->get_mrm() == mri::mri_t_pathcoupled)
	assert(!(query->get_rii() != NULL) || (rcvd_mri->get_mrm() == mri::mri_t_pathcoupled));



	qspec_object *q = dynamic_cast<qspec_object *>(query->get_qspec());
	bool is_x_flag_set = query->is_x_flag();
	const bound_sessionid *rcvd_b_sid = query->get_bound_sid();

	if (query->get_originator()) {
		// here comes the code which was formerly in process_outgoing_query()
		// (outgoing QUERY messages)

		//=========================== process RII ==================================
/*
		NSLP_Session_Context::qn_type_t type;
		session_context->lock();
		type = session_context->get_qn_type();
		session_context->unlock();

		DLog(state_manager::modname, "process_query_msg(): I am a "
				<< (((type == NSLP_Session_Context::QNI) ? "QNI" : ((type == NSLP_Session_Context::QNR) ? "QNR" : "QNE"))));
*/


		rii *r = query->get_rii();
		if (r) {
/*
			if (type == NSLP_Session_Context::QNR) {
				// this situation should not happen, because QNR can send QUERY msg ONLY w/0 RII
				nslpres = error_nothing_to_do;
				ERRLog(state_manager::modname, "QNR attempting to send QUERY with RII, delete it!");

				return nslpres;
			}
*/

			// check if the RII already in the list saved
			uint32 rii_number;
			r->get(rii_number);

			ILog(state_manager::modname, "Check for duplicates for RII " << rii_number);

			NSLP_Session_Context_Map::rii_hashmap_it_t rii_hit;
			NSLP_Session_Context_Map::riimap_t &rii_hashmap = session_context_map->get_rii_hashmap(*rcvd_sid);
			bool check_it_again = true;
			while (check_it_again) {
				r->get(rii_number);

				if ((rii_hit = rii_hashmap.find(rii_number)) != rii_hashmap.end()) {
					ILog(state_manager::modname, "FOUND RII " << rii_number << " in HASHMAP!");
					ILog(state_manager::modname, "I am trying to send a message with the duplicated RII. Generate new one.");
					r->generate_new();
				}
				else {
					check_it_again = false;
				}
			}
			
			rii* timer_rii = new rii();
			rii* context_rii = new rii();
			timer_rii->set(rii_number);
			context_rii->set(rii_number);
			
#ifndef NSIS_OMNETPP_SIM
			timer_t rii_t = time(NULL) + qosnslpconf.getpar<uint32>(qosnslpconf_refresh_period);
#else
			timer_t rii_t = (int32)(simTime().dbl()+1) + qosnslpconf.getpar<uint32>(qosnslpconf_refresh_period);
#endif

			TimerMsg* tmsg = new TimerMsg(message::qaddr_qos_nslp_timerprocessing);
			id_t check_rii_id = tmsg->get_id();
			timer_rii->set_rii_timer_id(check_rii_id);
			context_rii->set_rii_timer_id(check_rii_id);
			
			timer_rii->set_own(true);
			context_rii->set_own(true);
			
			uint32 retry_counter;
			r->get_retry_counter(retry_counter);
			timer_rii->set_retry_counter(retry_counter);
			context_rii->set_retry_counter(retry_counter);
			
			qspec_object* rcvd_qspec = query->get_qspec();
			context_rii->set_qspec(rcvd_qspec);
			
			context_rii->set_is_reserve(false);
			
			rii_hashmap[rii_number] = context_rii;
				
			// outgoing query doesn't refresh context!!
			//timer_t t = time(NULL) + qosnslpconf.getpar<uint32>(qosnslpconf_refresh_period);
			//context->lock();
			//context->set_time_to_live(t);
			//context->unlock();
			
			querymsg* timer_query = new querymsg();
			copy_query_message(query, timer_query);
			timer_query->set_rii(timer_rii);
			// I think this was forgotten:
			timer_query->set_bool_rii(true);
			
			// start timer with three parameters: (SID, MRI, PDU)
			const int32 msec = 0;
			tmsg->start_absolute(rii_t, msec, rcvd_sid->copy(), rcvd_mri->copy(), timer_query);
			if (tmsg->send_to(message::qaddr_timer)) {
				DLog(state_manager::modname, "Starting retransmit timer with ID " << tmsg->get_id() <<  " for QUERY of flow ("
						<< rcvd_pc_mri->get_sourceaddress() << ", " << rcvd_pc_mri->get_destaddress()
						<< "). Retransmit time set to " << rii_t << " s");
			}
			tmsg = NULL;
		}  // end if RII found
		else {
			if (down && query->is_reserve_init_flag()) {
				assert(session_context != NULL);

				NSLP_Flow_Context *flow_context = session_context->find_flow_context(*rcvd_pc_mri);
				if (flow_context == NULL) {
					TimerMsg *tmsg = new TimerMsg(message::qaddr_qos_nslp_timerprocessing);
					
#ifndef NSIS_OMNETPP_SIM
					timer_t r_t = time(NULL) + qosnslpconf.getpar<uint32>(qosnslpconf_refresh_period);
#else
					timer_t r_t = (int32)(simTime().dbl()+1) + qosnslpconf.getpar<uint32>(qosnslpconf_refresh_period);
#endif
					
					// outgoing query doesn't refresh context!!
					//timer_t t = time(NULL) + qosnslpconf.getpar<uint32>(qosnslpconf_refresh_period);
					//context->lock();
					//context->set_time_to_live(t);
					//context->unlock();
					
					// start timer for RESERVE, must not wait forever
					id_t check_id = tmsg->get_id();
					
					//context->lock();
					//context->set_timer_id_for_reserve_or_query(check_id);
					//context->unlock();
					
					querymsg* t_query = new querymsg();
					copy_query_message(query, t_query);
					
					const int32 msec = 0;
					tmsg->start_absolute(r_t, msec, rcvd_sid->copy(), rcvd_mri->copy(), t_query);
					if (tmsg->send_to(message::qaddr_timer)) {
						ILog(state_manager::modname, "process_query(), Timer for QUERY " << check_id << " set to " << r_t << " s");
					}
					tmsg = NULL;
				}
			}
			else {
				ERRLog(state_manager::modname, "QUERY message includes neither RII object nor Reserve-Init-Flag is set!");
				return;
			}
		}


		if (session_context != NULL) {
			//====================== process BOUND_SESSIONID in CONTEXT =====================
			bound_sessionid *rcvd_b_sid = query->get_bound_sid();
			if (rcvd_b_sid) {
				uint128 recv_b_sid;
				rcvd_b_sid->get(recv_b_sid);
				session_context->lock();
				session_context->set_bound_sid(recv_b_sid);
				session_context->unlock();
			} // end if BOUND_SESSIONID is found in the QUERY msg
			else {
				// check, if there is a BOUND_SESSIONID saved for this SID
				uint128 saved_b_sid;
				session_context->lock();
				bool found = session_context->get_bound_sid(saved_b_sid);
				session_context->unlock();
				if (found) {
					bound_sessionid *send_bs = new bound_sessionid();
					send_bs->set(saved_b_sid);
					query->set_bound_sid(send_bs);
				}
			} // end if BOUND_SESSIONID is not in the QUERY msg
		}
	}
	else {
		// here comes the code which was formerly in process_query_msg()
		// (incoming QUERY messages)

#ifdef USE_AHO
		// if QUERY was received by the PC-MRM and has the X-bit set, delete NLSP_AHO_Context for this session
		if((rcvd_mri->get_mrm() == mri::mri_t_pathcoupled) && (query->is_x_flag())) {
			NSLP_AHO_Context *aho_context = aho_contextmap->find(*rcvd_sid);
			if (aho_context != NULL) {
				aho_context->lock();
				ntlp::mri_pathcoupled flow = *rcvd_pc_mri;
				aho_context->remove_flow_from_context(flow);
				flow.invertDirection();
				aho_context->remove_flow_from_context(flow);
				bool erase_context = aho_context->is_set_of_flows_empty();
				aho_context->unlock();

				if (erase_context) {
					DLog(state_manager::modname, "Erasing NSLP_AHO_Context.");
					aho_contextmap->erase(*rcvd_sid);
				}
			}
		}
#endif

		if (session_context != NULL) {
			ILog(state_manager::modname, "process_query_msg(), Found session_context!");

			NSLP_Session_Context::qn_type_t type;
			session_context->lock();
			type = session_context->get_qn_type();
			session_context->unlock();
			
			//================================= process RII ======================================
			rii* r = query->get_rii();
			if (r) {
				uint32 rii_number;
				r->get(rii_number);
				
				ILog(state_manager::modname, "RII in QUERY existing " << rii_number);
				
				if (type != NSLP_Session_Context::QNR) {
					// check if the RII number already existing
					NSLP_Session_Context_Map::rii_hashmap_it_t rii_hit;
					NSLP_Session_Context_Map::riimap_t &rii_hashmap = session_context_map->get_rii_hashmap(*rcvd_sid);
					
					if ((rii_hit = rii_hashmap.find(rii_number)) != rii_hashmap.end()) { 
						ILog(state_manager::modname, "FOUND RII " << rii_number << " in HASHMAP! Is it my entry?");

						rii* found_context_rii = rii_hit->second;
						
						// I think this should likely be:
						//if (found_context_rii->get_own()) {
						if (!found_context_rii->get_own()) {
							ERRLog(state_manager::modname, "Another node has sent a message with the duplicated RII!!!");
							send_response_with_rii(r, rcvd_sid, rcvd_mri, !down, info_spec::transient, info_spec::RIIconflict);
							return;
						}
					}


					if (query->is_scoping_flag()) {
						send_response_to_query(r, q, !down, rcvd_sid, rcvd_pc_mri);
					}
					else {
						forward_query_msg(query, down, rcvd_sid, rcvd_pc_mri);
					}
				}
				else {
					// type == NSLP_Session_Context::QNR

					send_response_to_query(r, q, !down, rcvd_sid, rcvd_pc_mri);
				}
			}  // end if RII existing
			else {
#ifdef USE_AHO
				// In Anticipated Handover, Case 3
				// we have to send the RESERVE message upstream on behalf of the mobile node.
				if ((rcvd_mri->get_mrm() == mri::mri_t_pathcoupled) &&
						(query->is_reserve_init_flag()) &&
						(is_new_access_router_and_flow_matches(*rcvd_sid, *rcvd_pc_mri)))
				{
				
					NSLP_AHO_Context *aho_context = aho_contextmap->find(*rcvd_sid);
					if (aho_context != NULL) {
						aho_context->lock();
						const hostaddress ar_n = aho_context->get_addr_new_access_router();
						const hostaddress mn = aho_context->get_addr_mobile_node();
					
						const rii *rii_obj = aho_context->get_rii();
						const rp *rp_obj = aho_context->get_refresh_period();
						const bound_sessionid *bs_obj = aho_context->get_bound_sessionid();
						qspec_object* send_qspec = aho_context->get_qspec();
						aho_context->unlock();
					
					
						// create an EST-MRI which causes correct delivery of RESPONSE
						// messages during processing of the created RESERVE message.
						ntlp::mri_pathcoupled new_pc_mri = *rcvd_pc_mri;
						new_pc_mri.invertDirection();
						ntlp::mri_explicitsigtarget est_mri(new_pc_mri, mn, ar_n);
						
						// create RESERVE message
						reservereq *reserve = new reservereq();
					
						// RSN
						uint32 own_rsn;
						session_context->lock();
						own_rsn= session_context->get_own_rsn();
						session_context->unlock();
						rsn *rsn_obj = new rsn(own_rsn);
						reserve->set_rsn(rsn_obj);
						
						// RII
						if(rii_obj != NULL) {
							reserve->set_rii(rii_obj->copy());
						}
						
						// Refresh Period
						if(rp_obj != NULL) {
							reserve->set_rp(rp_obj->copy());
						}
						
						// BOUND SID
						if(bs_obj != NULL) {
							reserve->set_bound_sid(bs_obj->copy());
						}
						
						// QSPEC
						reserve->set_qspec(send_qspec);
						
						
						// process created RESERVE message
						error_t nslpres = error_ok;
						nslpres = update_session_context(*reserve, rcvd_sid, &est_mri, !down, 0, session_context, NULL);
						
						DLog(state_manager::modname, "update_session_context() - returned: " << error_string(nslpres));
						
						switch (nslpres) {
						case state_manager::error_old_pdu:
						case state_manager::error_rsn_missing:
						case state_manager::error_no_bandwidth:
							DLog(state_manager::modname,
									"process_query_msg() - processing of RESERVE msg returned an error: "
									<< error_string(nslpres));
							break;
						case state_manager::error_nothing_to_do:
						case state_manager::error_ok_forward:
						case state_manager::error_ok_send_response:
						case state_manager::error_ok:
							break;
						default:
							ERRLog(state_manager::modname, "Received an NSLP error (#" << nslpres << ':' <<
									state_manager::error_string(nslpres) << ") that cannot be handled.");
							break;
						}
						
						if(reserve != NULL) {
							delete reserve;
						}
					} // end if aho_context != NULL
				} // end if (is_reserve_init_flag && is_new_access_router_and_flow_matches)
				else 
#endif // USE_AHO
				{
					// if I'm the QNR I should respond to the Query
					if (query->is_reserve_init_flag() && is_last_signaling_hop(rcvd_pc_mri, down)) { 
						// create RESERVE msg, send to QUERY initiator
						create_and_send_reserve_msg_as_response_to_query(true, q, rcvd_b_sid, !down, rcvd_sid, rcvd_pc_mri,
								session_context, is_x_flag_set);
					}
					else if (query->is_reserve_init_flag()) {
						forward_query_msg(query, down, rcvd_sid, rcvd_pc_mri);
					}
					else { // I'm a QNE, so simply forward this QUERY
						// !query->is_reserve_init_flag()
						ERRLog(state_manager::modname, "Reservation existing! Wrong message requesting new reservation!!!");
					}
				}
			}


			//========================== process BOUND_SESSIONID ================================
			if (rcvd_b_sid) {
				DLog(state_manager::modname, "process_query_msg() - saving BOUND_SID [" << rcvd_b_sid->to_string() << "]");

				session_context->lock();
				session_context->set_bound_sid(*rcvd_b_sid);
				session_context->unlock();
			}
			//XXX: I think this is useless since send_bs is never used.
			//Note the scope of the previously declared variable send_bs
			//and the later declared with the same name!
			//XXX: Other problem: if a BOUND SID would be found in the
			//context, it had obviously no effect to include it in the
			//QUERY PDU since here the QUERY message is already forwarded!
			//No idea what this code is / was for.
			//else {
			//  if (type != NSLP_Context::QNR) {
			//    // check, if there is a BOUND_SESSIONID saved for this SID
			//    uint128 saved_b_sid;
			//    session_context->lock();
			//    found = session_context->get_bound_sid(saved_b_sid);
			//    session_context->unlock();
			//    if (found) {
			//      ILog(state_manager::modname, "Found saved BOUND_SID. Setting BOUND_SESSIONID in QUERY message to [" << saved_b_sid << "]");
			//      send_bs = new bound_sessionid();
			//      send_bs->set(saved_b_sid);
			//    }  
			//  }
			//}
		}
		else {
			// session_context == NULL
			ILog(state_manager::modname, "process_query_msg(), No NSLP_Session_Context found!");


			//================================ process RII ======================================
			rii* r = query->get_rii();
			if (r) {
				// if it RII is present, it cannot have the R flag set
				ILog(state_manager::modname, "RII existing!");

				if (is_last_signaling_hop(rcvd_pc_mri, down)) {
					send_response_to_query(r, q, !down, rcvd_sid, rcvd_pc_mri);
				}
				else {
					if (query->is_scoping_flag()) {
						send_response_to_query(r, q, !down, rcvd_sid, rcvd_pc_mri);
					}
					else {
						forward_query_msg(query, down, rcvd_sid, rcvd_pc_mri);
					}
				}
			}  // end if RII existing
			else {
				// no RII present

				// In Anticipated Handover, Case 3
				// we have to send a RESERVE message upstream on behalf of the mobile node
				// if we receive a QUERY message with the Reserve-Init flag set and if we
				// are the new access router ARN. Now we are in the case where no
				// NSLP_Session_Context exists. If we were in case 3 of the Anticipated
				// Handover the foregoing QUERY message would have established a
				// NSLP_Session_Context. Thus we are not in case 3 of the Anticipated
				// Handover!

				if (rcvd_mri->get_mrm() == mri::mri_t_pathcoupled) {
					// if I'm the QNR I should respond to the Query
					if (query->is_reserve_init_flag() && is_last_signaling_hop(rcvd_pc_mri)) { 
						// create RESERVE msg, send to QUERY initiator
						create_and_send_reserve_msg_as_response_to_query(true, q, rcvd_b_sid, !down, rcvd_sid, rcvd_pc_mri,
								session_context, is_x_flag_set);
					}
					else { // I'm a QNE, so simply forward this QUERY
						// HA TEnter
						if (down && addresses->addr_is_in(rcvd_pc_mri->get_destaddress(), AddressList::HomeNet_P)) {
							ntlp::mri_pathcoupled *logical_tmri = rcvd_pc_mri->copy();
							hostaddress *my_src = dynamic_cast<hostaddress *>(addresses->get_first(AddressList::HAAddr_P,
							    		    rcvd_pc_mri->get_destaddress().is_ipv4()));
							logical_tmri->set_sourceaddress(*my_src);
							
							uint128 tsid;
							
							querymsg* tunnel_query = new querymsg();
							copy_query_message(query, tunnel_query);
							
							tunnel_query->set_originator(true);
							known_nslp_pdu *pdu = dynamic_cast<known_nslp_pdu*>(tunnel_query);
							SignalingMsg* sigmsg = new SignalingMsg();
							sigmsg->set_msg(pdu);
							
							sigmsg->set_sig_mri(logical_tmri);
							
							sigmsg->set_downstream(down);
							
							ntlp::sessionid* sid = new sessionid();
							sid->generate_random();
							sid->generate_random();
							tsid = *sid;
							
							bound_sessionid* send_bs = new bound_sessionid();
							send_bs->set(tsid);
							
							query->set_bound_sid(send_bs);
							
							sigmsg->set_sid(tsid);
							sigmsg->send_or_delete();
						}
											
						forward_query_msg(query, down, rcvd_sid, rcvd_pc_mri);
					}
				}

			} // end else - no RII present
		} // end else - session_context != NULL
	} // end else - query->get_originator()


	ILog(state_manager::modname,"END process_query_msg()");
}


/** This method searches for an existing NSLP_Session_Context with the given session id. On success,
 * the found context will be returned. If no context is found, a new NSLP_Session_Context will be
 * created and subsequently returned. This method HAS TO be called for incoming RESERVE and QUERY
 * messages (with Reserve-Init-Bit set), but especially for the first message of a new GIST route (MA),
 * since this allows process_sii to update the predecessor SII and the successor SII respectively.
 *
 * @param pdu RESERVE or QUERY message used to find or create a new NSLP_Session_Context.
 * @param rcvd_sid the session id the message in pdu belongs to.
 * @param rcvd_mri the MRI to which the message in pdu corresponds.
 * @param down set to TRUE, if the direction is downstream, otherwise set to FALSE. Only useful with PC-MRM.
 * @return an error code of type error_t which determines the success of the foregoing execution of this method.
 */
state_manager::error_t state_manager::find_or_create_new_session_context(known_nslp_pdu &pdu,
	const ntlp::sessionid *rcvd_sid, const ntlp::mri *rcvd_mri, bool down, NSLP_Session_Context *&session_context, ntlp::Flowstatus *fs)
{
	DLog(state_manager::modname, "find_or_create_new_session_context() - SID [" << rcvd_sid->to_string() << "]");


	// initialize output parameter
	session_context = NULL;


	//========================================================================================================================
	// Search NSLP_Session_Context in the NSLP_Session_Context_Map
	//========================================================================================================================
	session_context = session_context_map->find_session_context(*rcvd_sid);
	if (session_context != NULL) {
		return error_ok;
	}


	//========================================================================================================================
	// Determine whether we are called for a RESERVE message or for a QUERY message
	//========================================================================================================================
	reservereq *reservemessage = NULL;
	querymsg *querymessage = NULL;

	if (pdu.is_reserve()) {
		reservemessage = dynamic_cast<reservereq *>(&pdu);
		assert(reservemessage != NULL);
	}
	else if (pdu.is_query()) {
		querymessage = dynamic_cast<querymsg *>(&pdu);
		assert(querymessage != NULL);

		// if this method was called with a QUERY message as PDU,
		// assert that the QUERY message has the R-Flag set
		assert(querymessage->is_reserve_init_flag());
	}
	else {
		DLog(state_manager::modname, "find_or_create_new_session_context() - pdu is neither a RESERVE nor a QUERY message!");

		return error_wrong_pdu_type;
	}

	assert(pdu.is_reserve() || pdu.is_query());


	//========================================================================================================================
	// Get the pathcoupled MRI from the given MRI
	//========================================================================================================================
	if (rcvd_mri == NULL) {
		ERRLog(state_manager::modname, "find_or_create_new_session_context() - rcvd_mri == NULL");
		
		return error_wrong_pdu_type;
	}
	
	const ntlp::mri_pathcoupled *rcvd_pc_mri=NULL;
	if (rcvd_mri->get_mrm() == mri::mri_t_pathcoupled) {
		rcvd_pc_mri = dynamic_cast<const ntlp::mri_pathcoupled *>(rcvd_mri);
		assert(rcvd_pc_mri != NULL);
	}
	else if (rcvd_mri->get_mrm() == mri::mri_t_explicitsigtarget) {
		const ntlp::mri_explicitsigtarget *rcvd_est_mri = dynamic_cast<const ntlp::mri_explicitsigtarget *>(rcvd_mri);
		assert(rcvd_est_mri != NULL);
		
		rcvd_pc_mri  = &rcvd_est_mri->get_mri_pc();
		down = rcvd_pc_mri->get_downstream();
	}
	else {
		ERRLog(state_manager::modname, "find_or_create_new_session_context() - Unknown MRI with MRI-ID: " << rcvd_mri->get_mrm());
		
		return error_wrong_pdu_type;
	}

	assert((rcvd_mri->get_mrm() == mri::mri_t_pathcoupled) || (rcvd_mri->get_mrm() == mri::mri_t_explicitsigtarget));
	assert(rcvd_pc_mri != NULL);


	//========================================================================================================================
	// Determine role of the QoS NSLP node (either QNI or QNE or QNR)
	//========================================================================================================================
	bool i_am_originator = pdu.get_originator();
	NSLP_Session_Context::qn_type_t type = NSLP_Session_Context::QNE;

	if (pdu.is_reserve()) {
		if (i_am_originator) {
			type = NSLP_Session_Context::QNI;
			ILog(state_manager::modname, "find_or_create_new_session_context() - I am a QNI (new context due to RESERVE)");
		}
		else if (is_last_signaling_hop(rcvd_pc_mri, down)) {
			type = NSLP_Session_Context::QNR;
			ILog(state_manager::modname, "find_or_create_new_session_context() - I am a QNR (last signaling hop for RESERVE)");
		}
		else {
			ILog(state_manager::modname, "find_or_create_new_session_context() - I am a QNE");
		}
	}
	else {
		// pdu.is_query()

		if (i_am_originator) {
			type = NSLP_Session_Context::QNR;
			ILog(state_manager::modname, "find_or_create_new_session_context() - I am a QNR (new context due to QUERY)");
		}
		else if (is_last_signaling_hop(rcvd_pc_mri, down)) {
			type = NSLP_Session_Context::QNI;
			ILog(state_manager::modname, "find_or_create_new_session_context() - I am a QNI (last signaling hop for QUERY)");
		}
		else {
			ILog(state_manager::modname, "find_or_create_new_session_context() - I am a QNE");
		}
	}


	//========================================================================================================================
	// Determine type of reservation (either sender initiated or receiver initiated)
	//========================================================================================================================
	assert(!(pdu.is_query() && !down));

	bool is_sender_init = pdu.is_reserve() && down;
	if (is_sender_init) {
		ILog(state_manager::modname, "find_or_create_new_session_context() - sender initiated reservation");
	}
	else {
		ILog(state_manager::modname, "find_or_create_new_session_context() - receiver initiated reservation");
	}
	
	
	//========================================================================================================================
	// Create new NSLP_Session_Context
	//========================================================================================================================
	session_context = new NSLP_Session_Context(*rcvd_sid, type, is_sender_init);


	//========================================================================================================================
	// Prepare later tear down of this NSLP_Session_Context
	//========================================================================================================================
	uint32 new_lifetime;

	// calculate the value for new_lifetime
	rp refr_period(qosnslpconf.getpar<uint32>(qosnslpconf_refresh_period));
	refr_period.get_rand_lifetime(qosnslpconf.getpar<uint32>(qosnslpconf_refresh_period), new_lifetime);

	// now calculate the lifetime for our NSLP_Session_Context
#ifndef NSIS_OMNETPP_SIM
	timer_t lifetime = time(NULL) + new_lifetime;
#else
	timer_t lifetime = (int32)(simTime().dbl()+1) + new_lifetime;
#endif

	TimerMsg* tmsg = new TimerMsg(message::qaddr_qos_nslp_timerprocessing);
	id_t lifetime_timer_id = tmsg->get_id();

	// save time to live and timer id in NSLP_Session_Context
	session_context->lock();
	session_context->set_time_to_live(lifetime);
	session_context->set_timer_id_for_lifetime(lifetime_timer_id);
	session_context->unlock();

	// start timer with one parameter: (SID)
	const int32 msec = 0;
	tmsg->start_absolute(lifetime, msec, rcvd_sid->copy());
	if (tmsg->send_to(message::qaddr_timer)) {
		ILog(state_manager::modname, "Starting lifetime timer with ID " << lifetime_timer_id <<  " for SID ["
				<< rcvd_sid->to_string() << "]. Lifetime set to " << lifetime << " s."); 
	}


	//========================================================================================================================
	// Insert the created NSLP_Session_Context into the NSLP_Session_Context_Map
	//========================================================================================================================
	session_context_map->insert_session_context(*rcvd_sid, session_context);


	//========================================================================================================================
	// Mobility processing
	//========================================================================================================================
	if (pdu.is_reserve()) {
		if (fs && fs->type == ntlp::Flowstatus::fs_tunnel && type != NSLP_Session_Context::QNR) {
			uint128 tsid;
	
			reservereq* tunnel_res = new reservereq();
			copy_reserve_message(reservemessage, tunnel_res);
	
			rsn* new_rsn = new rsn();
			tunnel_res->set_rsn(new_rsn);
	
			if (reservemessage->get_bool_rii()) {
				rii* new_rii = new rii();
				new_rii->generate_new();
				new_rii->set_own(true);
				tunnel_res->set_rii(new_rii);
			}
			
			tunnel_res->set_originator(true);
			
			known_nslp_pdu *pdu = dynamic_cast<known_nslp_pdu*>(tunnel_res);
			SignalingMsg* sigmsg = new SignalingMsg();
			sigmsg->set_msg(pdu);
			
			ntlp::mri_pathcoupled *logical_tmri = fs->new_mri->copy();
			if (type == NSLP_Session_Context::QNI) {
				logical_tmri->set_sourceaddress(fs->orig_mri.get_sourceaddress());
			}
			else {
				logical_tmri->set_destaddress(fs->orig_mri.get_destaddress());
			}
			sigmsg->set_sig_mri(logical_tmri);
			
			sigmsg->set_downstream(true);
			
			ntlp::sessionid* sid = new sessionid();
			sid->generate_random();
			tsid = *sid;
			
			bound_sessionid* send_bs = new bound_sessionid();
			send_bs->set(tsid);
			reservemessage->set_bound_sid(send_bs);
			
			sigmsg->set_sid(tsid);
			sigmsg->send_or_delete();
		}
	
		// HA as TEnter ... a bit of guess work
		if (down && type == NSLP_Session_Context::QNE && addresses->addr_is_in(rcvd_pc_mri->get_destaddress(), AddressList::HomeNet_P)) {
			ILog(state_manager::modname, "HA TEnter for: " << *rcvd_pc_mri);
			
			ntlp::mri_pathcoupled *logical_tmri = rcvd_pc_mri->copy();
			hostaddress *my_src = dynamic_cast<hostaddress *>(addresses->get_first(AddressList::HAAddr_P, rcvd_pc_mri->get_destaddress().is_ipv4()));
			logical_tmri->set_sourceaddress(*my_src);
			
			ILog(state_manager::modname, "Tunnel MRI: " << *logical_tmri);
			
			uint128 tsid;
			
			reservereq* tunnel_res = new reservereq();
			copy_reserve_message(reservemessage, tunnel_res);
			
			rsn* new_rsn = new rsn();
			tunnel_res->set_rsn(new_rsn);
			
			if (reservemessage->get_bool_rii()) {
				rii* new_rii = new rii();
				new_rii->generate_new();
				new_rii->set_own(true);
				tunnel_res->set_rii(new_rii);
			}
			
			tunnel_res->set_originator(true);
			
			known_nslp_pdu *pdu = dynamic_cast<known_nslp_pdu*>(tunnel_res);
			SignalingMsg* sigmsg = new SignalingMsg();
			sigmsg->set_msg(pdu);
			
			sigmsg->set_sig_mri(logical_tmri);
			
			sigmsg->set_downstream(true);
			
			ntlp::sessionid* sid = new sessionid();
			sid->generate_random();
			tsid = *sid;
			
			bound_sessionid* send_bs = new bound_sessionid();
			send_bs->set(tsid);
			reservemessage->set_bound_sid(send_bs);
			
			sigmsg->set_sid(tsid);
			sigmsg->send_or_delete();
		}
	}
	else if (pdu.is_query()) {
		// In tunnel mode send a QUERY-T
		if (fs && fs->type == ntlp::Flowstatus::fs_tunnel) {
			uint128 tsid;
			
			querymsg* tunnel_query = new querymsg();
			copy_query_message(querymessage, tunnel_query);

			//if (query->get_bool_rii()) {
			//	rii* new_rii = new rii();
			//	new_rii->generate_new();
			//	new_rii->set_own(true);
			//	tunnel_query->set_rii(new_rii);
			//}
		
			tunnel_query->set_originator(true);
			known_nslp_pdu *pdu = dynamic_cast<known_nslp_pdu*>(tunnel_query);
			SignalingMsg *sigmsg = new SignalingMsg();
			sigmsg->set_msg(pdu);
			
			ntlp::mri_pathcoupled *logical_tmri = fs->new_mri->copy();
			/*
			if (type == NSLP_Context::QNI) {
				logical_tmri->set_sourceaddress(fs->orig_mri.get_sourceaddress());
			}
			else {
				logical_tmri->set_destaddress(fs->orig_mri.get_destaddress());
			}
			*/
			// adapted to the following (see old source --> process_outgoing_query)
			logical_tmri->set_sourceaddress(fs->orig_mri.get_sourceaddress());
			sigmsg->set_sig_mri(logical_tmri);
			
			sigmsg->set_downstream(down);
			
			ntlp::sessionid* sid = new sessionid();
			sid->generate_random();
			tsid = *sid;
			
			bound_sessionid* send_bs = new bound_sessionid();
			send_bs->set(tsid);
			
			querymessage->set_bound_sid(send_bs);
			
			sigmsg->set_sid(tsid);
			sigmsg->send_or_delete();
		}
	}


	DLog(state_manager::modname, "END find_or_create_new_session_context()");

	return error_ok;
}


/** This function updates an existing NSLP_Session_Context identified by the given
 * session id with the informations from a received RESERVE message.
 *
 * @param pdu RESERVE message used to update an existing NSLP_Session_Context.
 * @param rcvd_sid the session id the message in pdu belongs to.
 * @param rcvd_mri the MRI to which the message in pdu corresponds.
 * @param down set to TRUE, if the direction is downstream, otherwise set to FALSE. Only useful with PC-MRM.
 * @return an error code of type error_t which determines the success of the foregoing execution of this method.
 */
state_manager::error_t state_manager::update_session_context(known_nslp_pdu &pdu,
	const ntlp::sessionid *rcvd_sid, const ntlp::mri *rcvd_mri, bool down, uint32 sii_handle, NSLP_Session_Context *session_context, ntlp::Flowstatus *fs)
{
	DLog(state_manager::modname, "update_session_context() - SID [" << rcvd_sid->to_string() << "]");


	//========================================================================================================================
	// Assert that session_context points to a valid NSLP_Session_Context
	//========================================================================================================================
	assert(session_context != NULL);


	//========================================================================================================================
	// Assert that we are called only for RESERVE messages
	//========================================================================================================================
	reservereq *reservemessage = NULL;

	if (pdu.is_reserve()) {
		reservemessage = dynamic_cast<reservereq *>(&pdu);
		assert(reservemessage != NULL);
	}
	else {
		DLog(state_manager::modname, "update_session_context() - pdu is not a RESERVE message!");

		return error_wrong_pdu_type;
	}

	assert(pdu.is_reserve());
	assert(reservemessage != NULL);


	//========================================================================================================================
	// Get the pathcoupled MRI from the given MRI
	//========================================================================================================================
	if (rcvd_mri == NULL) {
		ERRLog(state_manager::modname, "update_session_context() - rcvd_mri == NULL");
		
		return error_wrong_pdu_type;
	}
	
	const ntlp::mri_pathcoupled *rcvd_pc_mri=NULL;
	if (rcvd_mri->get_mrm() == mri::mri_t_pathcoupled) {
		rcvd_pc_mri = dynamic_cast<const ntlp::mri_pathcoupled *>(rcvd_mri);
		assert(rcvd_pc_mri != NULL);
	}
	else if (rcvd_mri->get_mrm() == mri::mri_t_explicitsigtarget) {
		const ntlp::mri_explicitsigtarget *rcvd_est_mri = dynamic_cast<const ntlp::mri_explicitsigtarget *>(rcvd_mri);
		assert(rcvd_est_mri != NULL);
		
		rcvd_pc_mri  = &rcvd_est_mri->get_mri_pc();
		down = rcvd_pc_mri->get_downstream();
	}
	else {
		ERRLog(state_manager::modname, "update_session_context() - Unknown MRI with MRI-ID: " << rcvd_mri->get_mrm());
		
		return error_wrong_pdu_type;
	}

	assert((rcvd_mri->get_mrm() == mri::mri_t_pathcoupled) || (rcvd_mri->get_mrm() == mri::mri_t_explicitsigtarget));
	assert(rcvd_pc_mri != NULL);


	//========================================================================================================================
	// Get role of the QoS NSLP node (either QNI or QNE or QNR)
	//========================================================================================================================
	session_context->lock();
	NSLP_Session_Context::qn_type_t qn_type = session_context->get_qn_type();
	session_context->unlock();


	//========================================================================================================================
	// Setup initial SII numbers in the NSLP_Session_Context
	//========================================================================================================================
	// The sii_handle for the interface by which the current RESERVE message was received is only meaningful if the
	// RESERVE message was received by the pathcoupled MRM.
	if(rcvd_mri->get_mrm() == mri::mri_t_pathcoupled) {
		session_context->lock();

		if (qn_type != NSLP_Session_Context::QNI) {
			if (!session_context->is_predecessor_sii_set_for_flow(*rcvd_pc_mri)) {
				session_context->set_predecessor_sii(*rcvd_pc_mri, sii_handle);

				ILog(state_manager::modname, "update_session_context() - Set predecessor SII for Flow ("
						<< rcvd_pc_mri->get_sourceaddress() << ", " << rcvd_pc_mri->get_destaddress()
						<< ") to: " << sii_handle);
			}
		}

		session_context->unlock();
	}


	//========================================================================================================================
	// Check if RESERVE message was received on the correct interface
	//========================================================================================================================
	if ((rcvd_mri->get_mrm() == mri::mri_t_pathcoupled) && (qn_type != NSLP_Session_Context::QNI)) {
		session_context->lock();

		assert(session_context->is_predecessor_sii_set_for_flow(*rcvd_pc_mri));

		if (!session_context->matches_given_sii_with_saved_predecessor_sii(*rcvd_pc_mri, sii_handle)) {
			// A predecessor MRI is saved for the given MRI, but the current RESERVE message was
			// not received on the expected interface. Maybe a rerouting event has occurred.
			uint32 sii_expected;
			session_context->get_predecessor_sii(*rcvd_pc_mri, sii_expected);

			ILog(state_manager::modname, "update_session_context() - RESERVE message received with SII " << sii_handle
					<< " but expected with SII " << sii_expected << ". Assuming that a rerouting has occured.");

			// handle rerouting event
			NSLP_Flow_Context *flow_context = session_context->find_flow_context(*rcvd_pc_mri);
			bool sender_init = session_context->get_sender_initiated();

			if (flow_context != NULL) {
				if (reservemessage->is_replace_flag()) {
					flow_context->lock();
					flow_context->set_merging_node(true);
					flow_context->unlock();
				}

				if (!sender_init) {
					// TODO send QUERY with Reserve-Init-Flag set upstream

					return error_ok;
				}
			}

			// TODO
			// Maybe move this block, behind RSN processing, and thus allow RSN processing to use the
			// expected SII handle instead of the current SII handle for retrieving the predecessor RSN.
			// Therefore the RSN corresponding to the expected SII would be still OK for the current RESERVE
			// message which identifies a rerouting event. But the next RESERVE message must use the RSN
			// corresponding to the new SII handle.

			// update predecessor SII
			session_context->set_predecessor_sii(*rcvd_pc_mri, sii_handle);

			ILog(state_manager::modname, "update_session_context() - Updating predecessor SII for Flow ("
					<< rcvd_pc_mri->get_sourceaddress() << ", " << rcvd_pc_mri->get_destaddress()
					<< ") to: " << sii_handle);
			// end TODO
		}

		session_context->unlock();
	}


	//========================================================================================================================
	// Setup initial RSN numbers in the NSLP_Session_Context
	//========================================================================================================================
	rsn *rcvd_rsn_obj = reservemessage->get_rsn();
	if (rcvd_rsn_obj) {
		session_context->lock();

		if (qn_type != NSLP_Session_Context::QNI)
		{
			// update incoming RSN from a peer
			if (rcvd_mri->get_mrm() == mri::mri_t_pathcoupled) {
				// Set predecessor RSN only if not already done
				if (!session_context->is_predecessor_rsn_set_for_sii(sii_handle)) {
					session_context->set_predecessor_rsn(sii_handle, *rcvd_rsn_obj);
	
					ILog(state_manager::modname, "update_session_context() - Set predecessor RSN for SII " << sii_handle << " to: " << *rcvd_rsn_obj);
				}
			}
/*
			else if (rcvd_mri->get_mrm() == mri::mri_t_explicitsigtarget) {
				// Set predecessor RSN only if not already done
				if (!session_context->is_predecessor_rsn_set_for_est_msg()) {
					session_context->set_predecessor_rsn_for_est_msg(rcvd_rsn);
	
					ILog(state_manager::modname, "update_session_context() - Set predecessor RSN for EST-MRM to: " << rcvd_rsn);
				}
			}
*/
		}

		session_context->unlock();
	} // RSN object present
	else { // no RSN object found in RESERVE message

		// RSN may be missing if I am the originator, since the application doesn't provide one
		if (qn_type == NSLP_Session_Context::QNI) {
			session_context->lock();
			
			reservemessage->set_rsn(session_context->get_own_rsn().copy());
			
			session_context->unlock();
			
			ILog(state_manager::modname, "update_session_context() - Set own RSN to: " << reservemessage->get_rsn());
			
			rcvd_rsn_obj= reservemessage->get_rsn();
		}
		else
		{ // Mandatory RSN object missing in RESERVE message
			ERRLog(state_manager::modname, "update_session_context() - Mandatory RSN object missing in RESERVE message!");
			
			rii *rcvd_rii_obj = reservemessage->get_rii();
			if (rcvd_rii_obj) {
				send_response_with_rii(rcvd_rii_obj, rcvd_sid, rcvd_mri, !down, info_spec::protocol, info_spec::MandatoryObjectMissing);
			}
			
			return error_rsn_missing;
		}
	}
	
	assert(rcvd_rsn_obj != NULL);


	//========================================================================================================================
	// Check if RESERVE message is "new", that is rcvd_rsn >= saved_rsn
	//========================================================================================================================

	session_context->lock();

	rsn saved_rsn;
	if (qn_type == NSLP_Session_Context::QNI) {
		saved_rsn= session_context->get_own_rsn();
	}
	else {
		if (rcvd_mri->get_mrm() == mri::mri_t_pathcoupled) {
			saved_rsn= session_context->get_predecessor_rsn(sii_handle);
		}
	}

	session_context->unlock();

	if (rcvd_mri->get_mrm() == mri::mri_t_explicitsigtarget) {
		saved_rsn = *rcvd_rsn_obj;
	}

        if (rcvd_rsn_obj->get_epoch_id() != saved_rsn.get_epoch_id() || *rcvd_rsn_obj < saved_rsn) {
		// discard RESERVE message
		ILog(state_manager::modname, "update_session_context() - Received RESERVE message with old RSN "
				<< *rcvd_rsn_obj << " < " << saved_rsn << ". Discarding message!");

		return error_old_pdu;
	}

        if ( (*rcvd_rsn_obj == saved_rsn || *rcvd_rsn_obj > saved_rsn) == false )
		ERRLog(state_manager::modname, "update_session_context() - expected that the incoming RSN is equal or greater than what I have stored.");

	//========================================================================================================================
	// Process BOUND_SID
	//========================================================================================================================
	bound_sessionid* rcvd_b_sid = reservemessage->get_bound_sid();
	if (rcvd_b_sid) {
		uint128 rcvd_b_sid_num;
		rcvd_b_sid->get(rcvd_b_sid_num);

		ILog(state_manager::modname, "update_session_context() - Setting BOUND_SID to [" << rcvd_b_sid_num << "]");

		session_context->lock();
		session_context->set_bound_sid(rcvd_b_sid_num);
		session_context->unlock();
	}
	else {
		// if a RESERVE message contains no BOUND_SID object but a BOUND_SID is saved
		// in the context, add it to the RESERVE message

		uint128 b_sid_num;
		session_context->lock();
		bool found = session_context->get_bound_sid(b_sid_num);
		session_context->unlock();
		
		if (found) {
			ILog(state_manager::modname, "update_session_context() - Adding saved BOUND_SID ["
					<< b_sid_num << "] to RESERVE message.");

			bound_sessionid *b_sid = new bound_sessionid();
			b_sid->set(b_sid_num);
			reservemessage->set_bound_sid(b_sid);
		}
	}

#ifdef USE_AHO
	bool is_anticipated_reservation = false;
	//========================================================================================================================
	// Processing for Anticipated Handover
	//========================================================================================================================
	// if RESERVE was received by the PC-MRM and has the Replace-bit set, delete NLSP_AHO_Context for this session
	if((rcvd_mri->get_mrm() == mri::mri_t_pathcoupled) && (reservemessage->is_replace_flag())) {
		NSLP_AHO_Context *aho_context = aho_contextmap->find(*rcvd_sid);
		if (aho_context != NULL) {
			aho_context->lock();
			ntlp::mri_pathcoupled flow = *rcvd_pc_mri;
			aho_context->remove_flow_from_context(flow);
			flow.invertDirection();
			aho_context->remove_flow_from_context(flow);
			bool erase_context = aho_context->is_set_of_flows_empty();
			aho_context->unlock();
		
			if (erase_context) {
				DLog(state_manager::modname, "Erasing NSLP_AHO_Context.");
				aho_contextmap->erase(*rcvd_sid);
			}
		}
	}

	if (reservemessage->get_anticipated_reservation()) {
		is_anticipated_reservation = true;
	}
#endif

	//========================================================================================================================
	// Find or create new NSLP_Flow_Context
	//========================================================================================================================
	session_context->lock();
	NSLP_Flow_Context *flow_context = session_context->find_flow_context(*rcvd_pc_mri);
	session_context->unlock();

	bool return_on_error= false;
	error_t nslpres= error_ok;
	if (flow_context == NULL) {

		nslpres= create_flow_context(reservemessage, rcvd_sid, rcvd_mri, rcvd_pc_mri, down, session_context, flow_context, fs, return_on_error);      

	} // end if flow_context == NULL
	else {
		// existing flow_context found, thus update it

		nslpres= update_existing_flow_context(reservemessage, rcvd_sid, rcvd_mri, rcvd_pc_mri, down, sii_handle, session_context, flow_context, fs, return_on_error);
		
	} // update existing flow context

	if (return_on_error) { // abort now returning an error
		return nslpres;
	}

        DLog(state_manager::modname, "END update_session_context()");

        return nslpres;
}




state_manager::error_t 
state_manager::create_flow_context(reservereq *reservemessage, const ntlp::sessionid *rcvd_sid,  const ntlp::mri *rcvd_mri, const ntlp::mri_pathcoupled *rcvd_pc_mri, bool down, NSLP_Session_Context *session_context, NSLP_Flow_Context *flow_context, ntlp::Flowstatus *fs, bool &return_on_error)
{
	return_on_error= false;

	session_context->lock();
	NSLP_Session_Context::qn_type_t qn_type = session_context->get_qn_type();
	session_context->unlock();

	rsn *rcvd_rsn_obj = reservemessage->get_rsn();

	// no flow_context found, thus create one

	//================================================================================================================
	// Process TEAR-Flag
	//================================================================================================================
	if (reservemessage->is_tear_flag()) {
		ILog(state_manager::modname, "update_session_context() - RESERVE message with TEAR-Flag set");

		if ((!reservemessage->get_originator()) && (qn_type != NSLP_Session_Context::QNR)) {
#ifdef USE_AHO
			ntlp::mri_explicitsigtarget fwd_est_mri;

			if((rcvd_mri->get_mrm() == mri::mri_t_pathcoupled) &&
			   is_new_access_router_and_flow_matches(*rcvd_sid, *rcvd_pc_mri, &fwd_est_mri)) {
				// forward RESERVE message towards the mobile node by means of the EST-MRM
				forward_reserve(reservemessage, session_context, rcvd_sid, &fwd_est_mri);
			}
			else {
				// forward RESERVE message by means of the PC-MRM
				forward_reserve(reservemessage, session_context, rcvd_sid, rcvd_pc_mri, down);
			}
#else
			forward_reserve(reservemessage, session_context, rcvd_sid, rcvd_pc_mri, down);
#endif
		}

		session_context->lock();
		session_context->remove_predecessor_sii(*rcvd_pc_mri);
		session_context->remove_successor_sii(*rcvd_pc_mri);
		session_context->unlock();
		
		return_on_error= true;
		return error_ok;
	}


	//================================================================================================================
	// Process QSPEC and allocate resources
	//================================================================================================================

	qspec_object *qspec_obj=NULL;
	error_t ret;

	qspec_object* rcvd_qspec = reservemessage->get_qspec();
	if (rcvd_qspec) {
		ret = process_qspec(reservemessage, qspec_obj);
		assert((ret == error_ok) || (ret == error_no_bandwidth));

		if (ret == error_no_bandwidth) {
			ILog(state_manager::modname, "update_session_context() - Reservation for flow ("
			     << rcvd_pc_mri->get_sourceaddress() << ", "
			     << rcvd_pc_mri->get_destaddress()
			     << ") failed due to low resources.");

			if (qn_type != NSLP_Session_Context::QNI) {
				// if the node is not QNI send RESPONSE with error either with RII(if existing) or RSN

				rii *rcvd_rii_obj = reservemessage->get_rii();
				if (rcvd_rii_obj) {
					send_response_with_rii(rcvd_rii_obj, rcvd_sid, rcvd_mri, !down,
							       info_spec::transient,
							       info_spec::ReservationFailure
						);
				}
				else {
					send_response_with_rsn(rcvd_rsn_obj, rcvd_sid, rcvd_mri, !down,
							       info_spec::transient,
							       info_spec::ReservationFailure
						);
				}
			}
			else {
				notify_application(QoS_Appl_Msg::type_not_enough_resources, rcvd_sid);
			}

			return_on_error= true;
			return error_no_bandwidth;
		}
	}  // end if QSPEC found
	else {
		ILog(state_manager::modname, "update_session_context() - No QSPEC found in RESERVE message. Cannot establish a NSLP_Flow_Context without it!");
		send_response_with_rsn(rcvd_rsn_obj, rcvd_sid, rcvd_mri, !down, info_spec::transient, info_spec::FullQSPECrequired);

		return_on_error= true;
		return error_qspec_missing;
	}

	if (reservemessage->get_sessionauth())
		DLog(state_manager::modname,"Session Authorization object present");

	const vlsp_object *vlspobj = reservemessage->get_vlsp_object();
	if (vlspobj)
	{
		DLog(state_manager::modname,"VLSP object present");
		
		if (qn_type == NSLP_Session_Context::QNR)
		{
			DLog(state_manager::modname,"QNR will process VLSP - calling RMF");
			MP(benchmark_journal::PRE_VLSP_SETUP_SCRIPT_SINK);
			rmf_admin.process_vlsp_object_setup_req(vlspobj);
			MP(benchmark_journal::POST_VLSP_SETUP_SCRIPT_SINK);
		}
		else
			DLog(state_manager::modname,"QNE/QNI: VLSP object not processed");
	}

	assert(ret == error_ok);
	assert(qspec_obj != NULL);
	assert(rcvd_qspec != NULL);


	//================================================================================================================
	// Process RII
	//================================================================================================================
	error_t nslpres;
	if ((qn_type == NSLP_Session_Context::QNI) || (qn_type == NSLP_Session_Context::QNR)) {
		nslpres = error_ok;
	}
	else {
		nslpres = error_ok_forward;
	}

	bool send = true;

	rii *rcvd_rii = reservemessage->get_rii();
	if (rcvd_rii) {
		TimerMsg *tmsg = new TimerMsg(message::qaddr_qos_nslp_timerprocessing);
			
		const bool is_qni = (qn_type == NSLP_Session_Context::QNI);
		const bool is_qnr = (qn_type == NSLP_Session_Context::QNR);
		error_t ret = process_rii(session_context, reservemessage, rcvd_sid, tmsg, is_qni, is_qnr, rcvd_pc_mri, down, false);
		assert((ret == error_ok) || (ret == error_nothing_to_do) ||
		       (ret == error_ok_send_response) || (ret == error_ok_forward));

		/// send back RESPONSE with RII
		if (ret == error_ok_send_response) {
			// indicate if session authorization object was present
			session_auth_object* sauth_object_hmac_signed= reservemessage->get_sessionauth_hmac_signed();

			send_response_with_rii(rcvd_rii, rcvd_sid, rcvd_mri, !down, info_spec::success, info_spec::ReservationSuccessful, sauth_object_hmac_signed, vlspobj);
				
			if (reservemessage->is_scoping_flag()) {
				send = false;
			}
				
			nslpres = error_ok;
		}
		else if (ret == error_nothing_to_do) {
			return_on_error= true;
			return error_nothing_to_do;
		}
		else if (ret == error_ok_forward) {
			nslpres = error_ok_forward;
		}

		assert((nslpres == error_ok) || (nslpres == error_ok_forward));

	}  // end if rii found

	assert((nslpres == error_ok) || (nslpres == error_ok_forward));

	// check, if QNE or QNR and A-Flag set - send RESPONSE msg
	// if the node is QNR and the RESPONSE was already sent with RII do not send it again with RSN
	if (send) {
		if (qn_type != NSLP_Session_Context::QNI) {
			if ((reservemessage->is_acknowledge_flag()) || (reservemessage->is_scoping_flag())) {
				ILog(state_manager::modname, "update_session_context() - QNE or QNR, A-Flag set, send RESPONSE");
					
				send_response_with_rsn(rcvd_rsn_obj, rcvd_sid, rcvd_mri, !down,
						       info_spec::success,
						       info_spec::Acknowledgement
					);
			}
		}
	}


	//================================================================================================================
	// Create new NSLP_Flow_Context
	//================================================================================================================
	flow_context = new NSLP_Flow_Context(*rcvd_pc_mri, *qspec_obj);


	//================================================================================================================
	// Determine if we are sender or receiver of a flow or none of them
	//================================================================================================================
	bool is_flow_sender = is_flow_source(rcvd_pc_mri);
	bool is_flow_receiver = is_flow_destination(rcvd_pc_mri);

	flow_context->lock();
	flow_context->set_is_flow_sender(is_flow_sender);
	flow_context->set_is_flow_receiver(is_flow_receiver);
	flow_context->unlock();

#ifdef USE_AHO
	// determine node role within AHO
	NSLP_AHO_Context::node_role_t role = NSLP_AHO_Context::OTHER;
	NSLP_AHO_Context *aho_context = aho_contextmap->find(*rcvd_sid);
	if(aho_context != NULL) {
		aho_context->lock();
		role = aho_context->get_aho_node_role();
		if(role == NSLP_AHO_Context::MN) {
			ntlp::mri_pathcoupled flow_mri = *rcvd_pc_mri;
			if (qn_type == NSLP_Session_Context::QNR) {
				flow_mri.invertDirection();
			}

			if (aho_context->is_flow_in_context(flow_mri)) {
				bool is_flow_sender = aho_context->get_is_flow_sender();
				bool is_flow_receiver = aho_context->get_is_flow_receiver();

				flow_context->lock();
				flow_context->set_is_flow_sender(is_flow_sender);
				flow_context->set_is_flow_receiver(is_flow_receiver);
				flow_context->unlock();
			}
		}
		aho_context->unlock();
	}
#endif

	//================================================================================================================
	// Mobility processing
	//================================================================================================================
	ntlp::mri_pathcoupled lmri;

	flow_context->lock();
	if (fs) {
		flow_context->set_logical_mri(&fs->orig_mri);

		lmri = fs->orig_mri;
	}
	else {
		flow_context->set_logical_mri(rcvd_pc_mri);

		lmri = *rcvd_pc_mri;
	}
	flow_context->unlock();

	session_context_map->insert_mri2context_map(*rcvd_sid, lmri);

	bool is_anticipated_reservation = false;
#ifdef USE_AHO
	flow_context->lock();
	is_anticipated_reservation = flow_context->get_anticipated_reservation();
	flow_context->unlock();

	//================================================================================================================
	// Processing for Anticipated Handover
	//================================================================================================================
	if (is_anticipated_reservation) {
		flow_context->lock();
		flow_context->set_anticipated_reservation(true);
		flow_context->unlock();
	}
#endif

	//================================================================================================================
	// Set the RESERVED-Flag in the NSLP_Flow_Context
	//================================================================================================================
	if (reservemessage->get_rii() == NULL) {
		flow_context->lock();
		flow_context->set_reserved(true);
		flow_context->unlock();
	}


	//================================================================================================================
	// Process REPLACE-Flag
	//================================================================================================================
	// If REPLACE-Flag is set and no RII object is present, delete all NSLP_Flow_Contexts with MRIs different than
	// the current. If the REPLACE-Flag is set and a RII object is present, only set the REPLACE-Bit in the
	// NSLP_Flow_Context, since the later RESPONSE message initiates the tear down of the other flows.
	if (reservemessage->is_replace_flag()) {
		flow_context->lock();
		flow_context->set_replace(true);
		flow_context->unlock();
	}
		
	if (reservemessage->is_replace_flag() && (reservemessage->get_rii() == NULL)) {
		// tear down all other branches
		// if (reservemessage->get_rii() != NULL) then process_response_msg does this job

		session_context->lock();
		NSLP_Session_Context::flowcontext_const_it_t it;
		it = session_context->flowcontext.begin();
		while (it != session_context->flowcontext.end()) {
			const ntlp::mri_pathcoupled flow = it->first;
			NSLP_Flow_Context *flow_context_other_flow = it->second;

			// Since the following code can delete the current (key, value) pair,
			// we jump to the next pair in advance.
			++it;

			if ((flow.get_sourceaddress() != rcvd_pc_mri->get_sourceaddress()) ||
			    (flow.get_destaddress() != rcvd_pc_mri->get_destaddress()))
			{
				tear_down_other_flow(rcvd_sid, &flow, down, session_context, flow_context_other_flow);

				delete_flow_context(rcvd_sid, &flow, session_context, flow_context_other_flow);
			}
		}
		session_context->unlock();
	}


	//================================================================================================================
	// Process REFRESH_PERIOD
	//================================================================================================================
	rp *refr_period = NULL;
	uint32 refresh_period, new_refr_p, new_lifetime;

	rp *r_p = reservemessage->get_rp();
	if (r_p) {
		r_p->get(refresh_period);
	}
	else {
		refresh_period = qosnslpconf.getpar<uint32>(qosnslpconf_refresh_period);
	}

	refr_period = new rp(refresh_period);
	refr_period->get_rand_wait(refresh_period, new_refr_p);
	refr_period->get_rand_lifetime(refresh_period, new_lifetime);

	flow_context->lock();
	flow_context->set_refresh_period(refresh_period);
	flow_context->unlock();


	//================================================================================================================
	// Prepare later tear down of this NSLP_Flow_Context
	//================================================================================================================
	// calculate the lifetime for our NSLP_Flow_Context
#ifndef NSIS_OMNETPP_SIM
    timer_t lifetime = time(NULL) + new_lifetime;
#else
    timer_t lifetime = (int32)(simTime().dbl()+1) + new_lifetime;
#endif

	TimerMsg* tmsg = new TimerMsg(message::qaddr_qos_nslp_timerprocessing);
	id_t lifetime_timer_id = tmsg->get_id();
		
	// save time to live and timer id in NSLP_Flow_Context
	flow_context->lock();
	flow_context->set_time_to_live(lifetime);
	flow_context->set_timer_id_for_lifetime(lifetime_timer_id);
	flow_context->unlock();
		
	// start timer with two parameters: (SID, MRI)
	const int32 msec = 0;
	tmsg->start_absolute(lifetime, msec, rcvd_sid->copy(), rcvd_pc_mri->copy());
	if (tmsg->send_to(message::qaddr_timer)) {
		ILog(state_manager::modname, "Starting lifetime timer with ID " << lifetime_timer_id <<  " for flow ("
		     << rcvd_pc_mri->get_sourceaddress() << ", " << rcvd_pc_mri->get_destaddress()
		     << "). Lifetime set to " << lifetime << " s."); 
	}


	//================================================================================================================
	// Insert the created NSLP_Flow_Context into the NSLP_Session_Context_Map
	//================================================================================================================
	session_context->lock();
	session_context->add_flow_context(*rcvd_pc_mri, flow_context);
	session_context->unlock();


	// redundant, see assertion above
	assert((nslpres == error_ok) || (nslpres == error_ok_send_response) || (nslpres == error_ok_forward));


	//================================================================================================================
	// Start refresh timer
	//================================================================================================================
#ifndef NSIS_OMNETPP_SIM
	timer_t t_refresh = time(NULL) + new_refr_p;
#else
    timer_t t_refresh = (int32)(simTime().dbl()+1) + new_refr_p;
#endif
		
	if (!is_anticipated_reservation) {
		// if the node is QNI or QNE and the SCOPING flag is not set, start refresh timer for RESERVE message
		if ((qn_type == NSLP_Session_Context::QNI) || ((qn_type == NSLP_Session_Context::QNE) && (!(reservemessage->is_scoping_flag())))) {
			TimerMsg *tmsg = new TimerMsg(message::qaddr_qos_nslp_timerprocessing);
				
			reservereq *timer_reserve = new reservereq();
			copy_reserve_message(reservemessage, timer_reserve);
				
			timer_reserve->set_rii(NULL);
			timer_reserve->set_rp(refr_period);
			timer_reserve->set_bool_rii(false);
			timer_reserve->set_bool_rp(true);
				
			// start timer with three parameters: (SID, MRI, PDU)
			const int32 msec = 0;
			tmsg->start_absolute(t_refresh, msec, rcvd_sid->copy(), rcvd_pc_mri->copy(), timer_reserve);
			if (tmsg->send_to(message::qaddr_timer)) {
				DLog(state_manager::modname, "Starting refresh timer with ID " << tmsg->get_id() <<  " for flow ("
				     << rcvd_pc_mri->get_sourceaddress() << ", " << rcvd_pc_mri->get_destaddress()
				     << "). Refresh time set to " << t_refresh << " s");
			}
		}
	}


	//================================================================================================================
	// Forward RESERVE message
	//================================================================================================================
	if ((nslpres == error_ok_forward) || ((!reservemessage->get_originator()) && reservemessage->is_replace_flag())) {
		// if the node is QNI or QNE and the SCOPING flag is not set, forward the RESERVE message
		if ((qn_type == NSLP_Session_Context::QNI) ||
		    ((qn_type == NSLP_Session_Context::QNE) && (!(reservemessage->is_scoping_flag()))))
		{
#ifdef USE_AHO
			ntlp::mri_explicitsigtarget fwd_est_mri;
	
			if((rcvd_mri->get_mrm() == mri::mri_t_pathcoupled) &&
			   is_new_access_router_and_flow_matches(*rcvd_sid, *rcvd_pc_mri, &fwd_est_mri))
			{
				// forward RESERVE message towards the mobile node by means of the EST-MRM
				forward_reserve(reservemessage, session_context, rcvd_sid, &fwd_est_mri);
			}
			else {
				// forward RESERVE message by means of the PC-MRM
				forward_reserve(reservemessage, session_context, rcvd_sid, rcvd_pc_mri, down);
			}
#else
			forward_reserve(reservemessage, session_context, rcvd_sid, rcvd_pc_mri, down);
#endif
			// nslpres is still error_ok_forward
			// XXX or error_ok_forward !?!
		}
	}

	return nslpres;
} // end create flow context


/**
 * This function is called by update_session_context
 */
state_manager::error_t 
state_manager::update_existing_flow_context(reservereq *reservemessage, const ntlp::sessionid *rcvd_sid,  const ntlp::mri *rcvd_mri, const ntlp::mri_pathcoupled *rcvd_pc_mri, bool down, uint32 sii_handle, NSLP_Session_Context *session_context, NSLP_Flow_Context *flow_context, ntlp::Flowstatus *fs, bool &return_on_error)
{
	return_on_error= false;

	// figure out QoS NSLP Node type
	session_context->lock();
	NSLP_Session_Context::qn_type_t qn_type = session_context->get_qn_type();
	session_context->unlock();

	// get RSN from message and context respectively
	rsn *rcvd_rsn_obj = reservemessage->get_rsn();
	rsn saved_rsn;

	session_context->lock();

	if (qn_type == NSLP_Session_Context::QNI) {
		saved_rsn= session_context->get_own_rsn();
	}
	else {
		if (rcvd_mri->get_mrm() == mri::mri_t_pathcoupled) {
			saved_rsn= session_context->get_predecessor_rsn(sii_handle);
		}
	}

	session_context->unlock();

	//================================================================================================================
	// Process TEAR-Flag
	//================================================================================================================
	flow_context->lock();
	bool is_merging_node = flow_context->get_merging_node();
	flow_context->unlock();
		
	if (reservemessage->is_tear_flag() && is_merging_node) {
		ILog(state_manager::modname, "update_existing_flow_context() - Merging node received RESERVE message with"
		     << "TEAR-Flag set, not forwarding message.");

		flow_context->lock();
		flow_context->set_merging_node(false);
		flow_context->unlock();

		// nothing to do for a merging node. Must not forward this message.
		return error_ok;
	}

	// TODO copied from old source, but I think this isn't correct!
	/*
	  context->lock();
	  if (reservemsg->is_tear_flag() && (context->get_branchingNode()) && i_am_originator) {
	  // nothing to do for originator(branching node). Just forward this message.
	  context->unlock();
	  return true;
	  }
	  else {
	  context->unlock();
	  }
	*/

	if (reservemessage->is_tear_flag()) {
		ILog(state_manager::modname, "update_existing_flow_context() - RESERVE message with TEAR-Flag set");

		// TEARing down a reservation is state manipulation, so increase RSN
		session_context->lock();
		session_context->increment_own_rsn();
		session_context->unlock();

		if ((!reservemessage->get_originator()) && (qn_type != NSLP_Session_Context::QNR)) {
#ifdef USE_AHO
			ntlp::mri_explicitsigtarget fwd_est_mri;

			if((rcvd_mri->get_mrm() == mri::mri_t_pathcoupled) &&
			   is_new_access_router_and_flow_matches(*rcvd_sid, *rcvd_pc_mri, &fwd_est_mri)) {
				// forward RESERVE message towards the mobile node by means of the EST-MRM
				forward_reserve(reservemessage, session_context, rcvd_sid, &fwd_est_mri);
			}
			else {
				// forward RESERVE message by means of the PC-MRM
				forward_reserve(reservemessage, session_context, rcvd_sid, rcvd_pc_mri, down);
			}
#else
			forward_reserve(reservemessage, session_context, rcvd_sid, rcvd_pc_mri, down);
#endif
		} 
		
		// QNR: Check whether the sender of the TEARing RESERVE wants a RESPONSE (RII given)
		if (qn_type == NSLP_Session_Context::QNR) {
			if (reservemessage->get_rii() != NULL) {
				ILog(state_manager::modname, "The sender of the TEARing RESERVE wants a RESPONSE (RII given)");
				send_response_with_rii(reservemessage->get_rii(), rcvd_sid, rcvd_mri, !down,
							info_spec::success, info_spec::TeardownSuccessful);
			} else if (reservemessage->is_acknowledge_flag()) {
				ILog(state_manager::modname, "The sender of the TEARing RESERVE wants a RESPONSE (ACK-REQ set)");
				send_response_with_rsn(reservemessage->get_rsn(), rcvd_sid, rcvd_mri, !down,
							info_spec::success, info_spec::TeardownSuccessful);
			}
		}

		// RB-REMARK: deleting the flow context may be too early here in case an RII 
		// was contained, i.e., as a QNI/QNE we have to wait until the RESPONSE comes back
		session_context->lock();
		delete_flow_context(rcvd_sid, rcvd_pc_mri, session_context, flow_context);
		session_context->unlock();

		return error_ok;
	}

	
	//================================================================================================================
	// TODO Is this needed anymore ???
	//================================================================================================================
	/*
	  context->lock();
	  bool reserve = context->get_reserved();
	  context->unlock();

	  if (!reserve) {
	  id_t query_timer_id;
	  context->lock();
	  context->set_reserved(true);
	  context->get_timer_id_for_reserve_or_query(query_timer_id);
	  context->unlock();
			
	  TimerMsg* tmsg = new TimerMsg(message::qaddr_qos_nslp_timerprocessing);
	  bool stop;
	  stop = tmsg->stop(query_timer_id);
	  tmsg->send_to(message::qaddr_timer);
	  if (stop) { Log(INFO_LOG,LOG_NORMAL,state_manager::modname,"RESERVE timer " << query_timer_id << " stopped!"); }
	  }
	*/

	// updateQSPEC will enforce QSPEC processing in case the incoming RSN has a new epoch ID
	bool updateQSPEC= false;

	// check for different epoch_id, if this is the case we need to update the saved RSN
	if (rcvd_rsn_obj->get_epoch_id() != saved_rsn.get_epoch_id())
	{
		ILog(state_manager::modname, "update_session_context() - found different EpochID in RSN, peer may have restarted");

		saved_rsn= *rcvd_rsn_obj;
		session_context->lock();
		
		// update RSN object
		if (rcvd_mri->get_mrm() == mri::mri_t_pathcoupled) {
			session_context->set_predecessor_rsn(sii_handle, saved_rsn);
		}
		
		session_context->unlock();
		
		updateQSPEC= true;
	}

	//================================================================================================================
	// Process RSN and QSPEC
	//================================================================================================================
        if ( (*rcvd_rsn_obj == saved_rsn || *rcvd_rsn_obj > saved_rsn) == false )
		ERRLog(state_manager::modname, "update_session_context() - expected that the incoming RSN is equal or greater than what I have stored.");

	bool forward = false;

	if (*rcvd_rsn_obj > saved_rsn || updateQSPEC) {
		// new RSN received, thus process QSPEC
		qspec_object* rcvd_qspec = reservemessage->get_qspec();
		if (rcvd_qspec) {
			forward = true;
				
			const qspec_pdu *q_pdu = rcvd_qspec->get_qspec_data();
			if ( rmf_admin.reserve_resources(q_pdu) == false ) {
				// resource reservation failed

				ILog(state_manager::modname, "update_existing_flow_context() - Changing the Reservation for flow ("
				     << rcvd_pc_mri->get_sourceaddress() << ", "
				     << rcvd_pc_mri->get_destaddress()
				     << ") failed due to low resources.");

				if (qn_type != NSLP_Session_Context::QNI) {
					rii* rcvd_rii = reservemessage->get_rii();
					if (rcvd_rii) {
						send_response_with_rii(rcvd_rii, rcvd_sid, rcvd_mri, !down,
								       info_spec::transient,
								       info_spec::ReservationFailure
							);
					}
					else {
						// why is down == false ??
						send_response_with_rsn(rcvd_rsn_obj, rcvd_sid, rcvd_mri, false,
								       info_spec::transient,
								       info_spec::ReservationFailure
							);
					}
				}
				else {
					// if the node is QNI then notify application
					notify_application(QoS_Appl_Msg::type_not_enough_resources, rcvd_sid);
				}

				session_context->lock();
				delete_flow_context(rcvd_sid, rcvd_pc_mri, session_context, flow_context);
				session_context->unlock();

				return error_no_bandwidth;
			}
			else {
				flow_context->lock();
				flow_context->set_qspec(*rcvd_qspec);
				flow_context->unlock();
			}
			
			if (reservemessage->get_sessionauth())
				DLog(state_manager::modname,"Session Authorization object present");


			const vlsp_object *vlspobj = reservemessage->get_vlsp_object();
			if (vlspobj)
			{
			  DLog(state_manager::modname,"VLSP object present");

			  if (qn_type == NSLP_Session_Context::QNR)
			  {
				  DLog(state_manager::modname,"QNR will process VLSP - calling RMF");
				  MP(benchmark_journal::PRE_VLSP_SETUP_SCRIPT_SINK);
				  rmf_admin.process_vlsp_object_setup_req(vlspobj);
				  MP(benchmark_journal::POST_VLSP_SETUP_SCRIPT_SINK);
			  }
			}


		} // end if QSPEC found
		else
		{
			ILog(state_manager::modname, "update_existing_flow_context() - No QSPEC found in RESERVE message. Cannot update NSLP_Flow_Context without it!");

			// why is down == false ???
			send_response_with_rsn(rcvd_rsn_obj, rcvd_sid, rcvd_mri, false, info_spec::transient, info_spec::FullQSPECrequired);

			return error_qspec_missing;
		}


		// new RSN received, update it
		session_context->lock();
			
		if (qn_type == NSLP_Session_Context::QNI) {
			// RB-REMARK: I don't think that this is correct, should be: session_context->increment_own_rsn();
			session_context->set_own_rsn(*rcvd_rsn_obj); 
			
			ILog(state_manager::modname, "update_existing_flow_context() - Set own RSN to: " << session_context->get_own_rsn());
		}
		else {
			if (rcvd_mri->get_mrm() == mri::mri_t_pathcoupled) {
				// Since the RESERVE message was received with the expected SII handle, we now can safely update
				// the RSN. Note that updating the RSN with a RSN of a RESERVE message received with an
				// unexpected SII handle can lead into trouble!
				assert(session_context->matches_given_sii_with_saved_predecessor_sii(*rcvd_pc_mri, sii_handle));

				session_context->set_predecessor_rsn(sii_handle, *rcvd_rsn_obj);

				ILog(state_manager::modname, "update_existing_flow_context() - Set predecessor RSN for SII "
							<< sii_handle << " to: " << *rcvd_rsn_obj);
			}
			else if (rcvd_mri->get_mrm() == mri::mri_t_explicitsigtarget) {
				session_context->set_predecessor_rsn_for_est_msg(*rcvd_rsn_obj);

				ILog(state_manager::modname, "update_existing_flow_context() - Set predecessor RSN for EST-MRM to: " << *rcvd_rsn_obj);
			}

			session_context->increment_own_rsn();

			ILog(state_manager::modname, "update_existing_flow_context() - Incrementing own RSN.");
		}
			
		session_context->unlock();
	} // end if rcvd_rsn > saved_rsn or updateQSPEC is set


	//================================================================================================================
	// Process RII
	//================================================================================================================
	error_t nslpres;
	if ((qn_type == NSLP_Session_Context::QNI) || (qn_type == NSLP_Session_Context::QNR)) {
		nslpres = error_ok;
	}
	else {
		nslpres = error_ok_forward;
	}

	flow_context->lock();
	is_merging_node = flow_context->get_merging_node();
	flow_context->unlock();

	bool send = true;

	rii *rcvd_rii = reservemessage->get_rii();
	if (rcvd_rii) {
		TimerMsg *tmsg = new TimerMsg(message::qaddr_qos_nslp_timerprocessing);
			
		const bool is_qni = (qn_type == NSLP_Session_Context::QNI);
		const bool is_qnr = (qn_type == NSLP_Session_Context::QNR);
		error_t ret = process_rii(session_context, reservemessage, rcvd_sid, tmsg, is_qni, is_qnr, rcvd_pc_mri, down, is_merging_node);
		assert((ret == error_ok) || (ret == error_nothing_to_do) ||
		       (ret == error_ok_send_response) || (ret == error_ok_forward));

		if (ret == error_ok_send_response) {
			if (qn_type == NSLP_Session_Context::QNR) {
				send_response_with_rii(rcvd_rii, rcvd_sid, rcvd_mri, !down,
						       info_spec::success,
						       info_spec::ReservationSuccessful
					);
			}
			else {
				// if the node is merging node or if the SCOPING flag is set then send RESPONSE

				if (!forward || (reservemessage->is_scoping_flag())) {
					send_response_with_rii(rcvd_rii, rcvd_sid, rcvd_mri, !down,
							       info_spec::success, info_spec::ReservationSuccessful);
				}
			}
				
			if (reservemessage->is_scoping_flag()) {
				send = false;
			}
				
			nslpres = error_ok;
		}
		else if (ret == error_nothing_to_do) {
			nslpres = error_nothing_to_do;
			//return error_nothing_to_do;
		}
		else if (ret == error_ok_forward) {
			nslpres = error_ok_forward;
		}

		assert((nslpres == error_ok) || (nslpres == error_ok_forward) || (nslpres == error_nothing_to_do));

	}  // end if rii found

	assert((nslpres == error_ok) || (nslpres == error_ok_forward) || (nslpres == error_nothing_to_do));

	// check, if QNE or QNR and A-Flag set - send RESPONSE msg
	// if the node is QNR and the RESPONSE was already sent with RII do not send it again with RSN
	if (send) {
		if (qn_type != NSLP_Session_Context::QNI) {
			// TODO why is the following code different from those above (new flow) ???
			if (reservemessage->is_acknowledge_flag()) {
				ILog(state_manager::modname, "update_existing_flow_context() - QNE or QNR, A-Flag set, send RESPONSE");

				send_response_with_rsn(rcvd_rsn_obj, rcvd_sid, rcvd_mri, false, info_spec::success,
						       info_spec::Acknowledgement
					);
			}
		}
	}


	//================================================================================================================
	// Set the RESERVED-Flag in the NSLP_Flow_Context
	//================================================================================================================
	if (reservemessage->get_rii() == NULL) {
		flow_context->lock();
		flow_context->set_reserved(true);
		flow_context->unlock();
	}


	//================================================================================================================
	// Process REPLACE-Flag
	//================================================================================================================
	// If REPLACE-Flag is set and no RII object is present, delete all NSLP_Flow_Contexts with MRIs different than
	// the current. If the REPLACE-Flag is set and a RII object is present, only set the REPLACE-Bit in the
	// NSLP_Flow_Context, since the later RESPONSE message initiates the tear down of the other flows.
	if (reservemessage->is_replace_flag()) {
		flow_context->lock();
		flow_context->set_replace(true);
		flow_context->unlock();
	}
		
	// handle rerouting
	flow_context->lock();
	is_merging_node = flow_context->get_merging_node();
	flow_context->unlock();

	if (is_merging_node && reservemessage->is_replace_flag() && (reservemessage->get_rii() == NULL)) {
		uint32 old_sii;
		session_context->lock();
		session_context->get_predecessor_sii_old(*rcvd_pc_mri, old_sii);
		session_context->unlock();

		send_notify_on_the_old_path(rcvd_sid, rcvd_pc_mri, !down, old_sii, info_spec::information, info_spec::RouteChange);
	}


	if (reservemessage->is_replace_flag() && (reservemessage->get_rii() == NULL)) {
		// tear down all other branches
		// if (reservemessage->get_rii() != NULL) then process_response_msg does this job

		session_context->lock();
		NSLP_Session_Context::flowcontext_const_it_t it;
		it = session_context->flowcontext.begin();
		while (it != session_context->flowcontext.end()) {
			const ntlp::mri_pathcoupled flow = it->first;
			NSLP_Flow_Context *flow_context_other_flow = it->second;

			++it;

			if ((flow.get_sourceaddress() != rcvd_pc_mri->get_sourceaddress()) ||
			    (flow.get_destaddress() != rcvd_pc_mri->get_destaddress()))
			{
				tear_down_other_flow(rcvd_sid, &flow, down, session_context, flow_context_other_flow);

				delete_flow_context(rcvd_sid, &flow, session_context, flow_context_other_flow);
			}
		}
		session_context->unlock();
	}


	//================================================================================================================
	// Process REFRESH_PERIOD
	//================================================================================================================
	rp *refr_period = NULL;
	uint32 refresh_period, new_refr_p, new_lifetime;

	rp *r_p = reservemessage->get_rp();
	if (r_p) {
		r_p->get(refresh_period);
	}
	else {
		refresh_period = qosnslpconf.getpar<uint32>(qosnslpconf_refresh_period);
	}

	refr_period = new rp(refresh_period);
	refr_period->get_rand_wait(refresh_period, new_refr_p);
	refr_period->get_rand_lifetime(refresh_period, new_lifetime);

	flow_context->lock();
	flow_context->set_refresh_period(refresh_period);
	flow_context->unlock();


	//================================================================================================================
	// Update lifetime of our NSLP_Flow_Context
	//================================================================================================================
	// calculate the new lifetime for our NSLP_Flow_Context
#ifndef NSIS_OMNETPP_SIM
    timer_t lifetime = time(NULL) + new_lifetime;
#else
    timer_t lifetime = (int32)(simTime().dbl()+1) + new_lifetime;
#endif
		
	// update time to live in NSLP_Flow_Context
	flow_context->lock();
	flow_context->set_time_to_live(lifetime);
	flow_context->unlock();

	ILog(state_manager::modname, "Updating lifetime for flow ("
	     << rcvd_pc_mri->get_sourceaddress() << ", " << rcvd_pc_mri->get_destaddress()
	     << "). Lifetime set to " << lifetime << " s.");

#ifdef USE_AHO
	//================================================================================================================
	// Start refresh timer for an anticipated reservation
	//================================================================================================================
#ifndef NSIS_OMNETPP_SIM
    timer_t t_refresh = time(NULL) + new_refr_p;
#else
    timer_t t_refresh = (int32)(simTime().dbl()+1) + new_refr_p;
#endif

	flow_context->lock();
	bool is_anticipated_reservation = flow_context->get_anticipated_reservation();
	flow_context->unlock();

	if (is_anticipated_reservation &&
	    (rcvd_mri->get_mrm() == mri::mri_t_pathcoupled) &&
	    reservemessage->is_replace_flag())
	{
		flow_context->lock();
		flow_context->set_anticipated_reservation(false);
		flow_context->unlock();

		// if the node is QNI or QNE and the SCOPING flag is not set, start refresh timer for RESERVE message
		if ((qn_type == NSLP_Session_Context::QNI) || ((qn_type == NSLP_Session_Context::QNE) && (!(reservemessage->is_scoping_flag())))) {
			TimerMsg *tmsg = new TimerMsg(message::qaddr_qos_nslp_timerprocessing);
				
			reservereq *timer_reserve = new reservereq();
			copy_reserve_message(reservemessage, timer_reserve);
				
			timer_reserve->set_rii(NULL);
			timer_reserve->set_rp(refr_period);
			timer_reserve->set_bool_rii(false);
			timer_reserve->set_bool_rp(true);
				
			// start timer with three parameters: (SID, MRI, PDU)
			const int32 msec = 0;
			tmsg->start_absolute(t_refresh, msec, rcvd_sid->copy(), rcvd_pc_mri->copy(), timer_reserve);
			if (tmsg->send_to(message::qaddr_timer)) {
				DLog(state_manager::modname, "Starting refresh timer with ID " << tmsg->get_id() <<  " for flow ("
				     << rcvd_pc_mri->get_sourceaddress() << ", " << rcvd_pc_mri->get_destaddress()
				     << "). Refresh time set to " << t_refresh << " s");
			}
		}
	}

#endif		

	//================================================================================================================
	// Forward RESERVE message
	//================================================================================================================
	if (((nslpres == error_ok_forward) && forward) || ((!reservemessage->get_originator()) && reservemessage->is_replace_flag())) {
		if ((qn_type == NSLP_Session_Context::QNI) ||
		    ((qn_type == NSLP_Session_Context::QNE) && (!(reservemessage->is_scoping_flag()))))
		{
#ifdef USE_AHO
			ntlp::mri_explicitsigtarget fwd_est_mri;
	
			if((rcvd_mri->get_mrm() == mri::mri_t_pathcoupled) &&
			   is_new_access_router_and_flow_matches(*rcvd_sid, *rcvd_pc_mri, &fwd_est_mri))
			{
				// forward RESERVE message towards the mobile node by means of the EST-MRM
				forward_reserve(reservemessage, session_context, rcvd_sid, &fwd_est_mri);
			}
			else {
				// forward RESERVE message by means of the PC-MRM
				forward_reserve(reservemessage, session_context, rcvd_sid, rcvd_pc_mri, down);
			}
#else
			forward_reserve(reservemessage, session_context, rcvd_sid, rcvd_pc_mri, down);
#endif
			// nslpres is still error_ok_forward
		}
	}

	return nslpres;
} // end update_existing_flow_context()




/** This function sends a refreshing RESERVE message.
 * @param context RESERVE message will be constructed from this NSLP_Session_Context.
 */
void state_manager::send_refreshing_reserve(const ntlp::sessionid &sid, const ntlp::mri_pathcoupled &mri,
		NSLP_Session_Context *session_context, NSLP_Flow_Context *flow_context)
{
	ILog(state_manager::modname, "send_refreshing_reserve()");
	

	reservereq *res = new reservereq();

	// RSN
	session_context->lock();
	res->set_rsn(session_context->get_own_rsn().copy());
	session_context->unlock();	

	// originator flag
	session_context->lock();
	NSLP_Session_Context::qn_type_t qn_type = session_context->get_qn_type();
	session_context->unlock();
	if (qn_type == NSLP_Session_Context::QNI) {
	        res->set_originator(true);
	}
	

	// QSPEC
	flow_context->lock();
//	if ((!flow_context->get_reduced_refresh())) {
	        qspec_object *qspec_obj = flow_context->get_qspec().copy();
	        res->set_qspec(qspec_obj);
//	}
	flow_context->unlock();


	// BOUND_SID
	uint128 saved_b_sid;
	session_context->lock();
	bool found = session_context->get_bound_sid(saved_b_sid);
	session_context->unlock();
	if (found) {
	        bound_sessionid *send_bs = new bound_sessionid();
	        send_bs->set(saved_b_sid);
	        res->set_bound_sid(send_bs);
	}
	

	known_nslp_pdu *pdu = dynamic_cast<known_nslp_pdu*>(res);


	// D-Bit
	session_context->lock();
	bool down = session_context->get_sender_initiated();
	session_context->unlock();


#ifdef USE_AHO
	ntlp::mri_explicitsigtarget fwd_est_mri;
	
	if (is_new_access_router_and_flow_matches(sid, mri, &fwd_est_mri)) {
		ExplicitSignalingMsg *sigmsg = new ExplicitSignalingMsg();
		sigmsg->set_pdu(pdu);
		sigmsg->set_sig_mri(&fwd_est_mri);
		sigmsg->set_sid(sid);
		sigmsg->send_or_delete();
	}
	else {
		// send message
		SignalingMsg* sigmsg = new SignalingMsg();
		sigmsg->set_msg(pdu);
		sigmsg->set_sig_mri(mri.copy());
		sigmsg->set_downstream(down);
		sigmsg->set_sid(sid);
		sigmsg->send_or_delete();
	}
#else
	// send message
	SignalingMsg* sigmsg = new SignalingMsg();
	sigmsg->set_msg(pdu);
	sigmsg->set_sig_mri(mri.copy());
	sigmsg->set_downstream(down);
	sigmsg->set_sid(sid);
	sigmsg->send_or_delete();
#endif
	

	// start timer for next refreshing RESERVE
	uint32 refresh_p, new_refr_p;
	flow_context->lock();
	flow_context->get_refresh_period(refresh_p);
	flow_context->unlock();

	rp *refr_period = new rp(refresh_p);
	refr_period->get_rand_wait(refresh_p, new_refr_p);
#ifndef NSIS_OMNETPP_SIM
    timer_t t_refresh = time(NULL) + new_refr_p;
#else
    timer_t t_refresh = (int32)(simTime().dbl()+1) + new_refr_p;
#endif

	reservereq* timer_reserve = new reservereq();
	copy_reserve_message(res, timer_reserve);
	timer_reserve->set_rp(refr_period);
	timer_reserve->set_bool_rii(false);
	timer_reserve->set_bool_rp(true);

	// start timer with three parameters: (SID, MRI, PDU)
	TimerMsg* tmsg_refresh = new TimerMsg(message::qaddr_qos_nslp_timerprocessing);
	tmsg_refresh->start_absolute(t_refresh, 0, sid.copy(), mri.copy(), timer_reserve);
	if (tmsg_refresh->send_to(message::qaddr_timer)) {
		ILog(state_manager::modname, "Timer for refreshing RESERVE msg with ID " << tmsg_refresh->get_id() << " set to " << t_refresh << " s");
	}

	ILog(state_manager::modname, "END send_refreshing_reserve()");
}


/** This function sends RESPONSE message to a QUERY message with RII object.
 * @param r the RII object from the QUERY message to be copied into RESPONSE message.
 * @param q the QSPEC from the QUERY message after it has been processed by RMF.
 * @param down set to TRUE, if the direction is downstream, otherwise set to FALSE.
 * @param local_sid the session id the current message belongs to.
 * @param rcvd_mri the MRI of the current session.
 */
void state_manager::send_response_to_query(rii* r, qspec_object* q, bool down, const ntlp::sessionid* sid, const ntlp::mri_pathcoupled* rcvd_mri)
{
	uint128 local_sid;
	uint32 number_rii;
	
	ILog(state_manager::modname, "send_response_to_query()");
	
	known_nslp_pdu* pdu = NULL;
	responsemsg* res = new responsemsg();
	res->set_rsn(NULL);
	r->get(number_rii);
	rii* send_rii = new rii();
	send_rii->set(number_rii);
	res->set_rii(send_rii);
	
	// For simplicity, extract v4-mapped address for v4 case
	in6_addr addr6;
	netaddress *na = addresses->get_first(AddressList::ConfiguredAddr_P, rcvd_mri->get_destaddress().is_ipv4());
	na->get_ip(addr6);
	delete(na);
	
	info_spec* send_error = new info_spec(&addr6, info_spec::success,
			info_spec::ReservationSuccessful,
			0);
	res->set_errorobject(send_error);

	if (q) {
		res->set_qspec(q->copy());
	}
	
	pdu = dynamic_cast<known_nslp_pdu*>(res);
	local_sid= *sid;
	SignalingMsg* sigmsg = new SignalingMsg;
	sigmsg->set_msg(pdu);
	sigmsg->set_sig_mri(rcvd_mri);
	sigmsg->set_downstream(down);
	sigmsg->set_sid(local_sid);
	sigmsg->send_or_delete();
	
	ILog(state_manager::modname, "END send_response_to_query()");
}


/**
 * This function sends a RESPONSE message triggered by RII.
 * @param rii the RII object from the received message that will be copied into the RESPONSE message.
 * @param sid the session id the current message belongs to.
 * @param mri the MRI used to send the RESPONSE message.
 * @param down set to TRUE, if the direction is downstream, otherwise set to FALSE.
 * @param err_class this is the error class for the INFO_SPEC object in the RESPONSE message.
 * @param err_code this is the error code for the INFO_SPEC object in the RESPONSE message.
 * @param sauth_object a session authorization object from the RESERVE (optional, may be NULL)
 * @param vlsp_object VLSP object to send back (optional, may be NULL)
 */
void state_manager::send_response_with_rii(const rii* rii, const ntlp::sessionid* sid, const ntlp::mri* mri, bool down,
					   info_spec::errorclass_t err_class, info_spec::errorcode_t err_code, const session_auth_object* sauth_object, const vlsp_object* vlsp_obj)
{
	DEBUG_ENTER(state_manager::modname, "");

	known_nslp_pdu *pdu = NULL;
	responsemsg *resp = new responsemsg();
	resp->set_rii(rii->copy());
	
	// check for session authorization object
	if (sauth_object && sauth_object->is_hmac_signed())
	{
		// also provide integrity protection for the response
		// MUST contain at least 
		// SOURCE_ADDR: the source address of the entity that created the HMAC
                // START_TIME: the timestamp when the HMAC signature was calculated
	        // NSLP_OBJECT_LIST: this attribute lists all NSLP objects that are included in HMAC calculation.
                // AUTHENTICATION_DATA

		session_auth_object* sauth_obj_hmac = new session_auth_object;

		auth_attr_hmac_trans_id *transid = new auth_attr_hmac_trans_id(session_auth_object::TRANS_ID_AUTH_HMAC_SHA1_96);
		sauth_obj_hmac->insert_attr(transid);

		const ntlp::mri_pathcoupled* pc_mri = dynamic_cast<const ntlp::mri_pathcoupled *>(mri);		
		auth_attr_addr *src_addr = new auth_attr_addr(SOURCE_ADDR,pc_mri->get_destaddress().is_ipv4() ? IPV4_ADDRESS : IPV6_ADDRESS);
		src_addr->set_ip(pc_mri->get_destaddress());
		sauth_obj_hmac->insert_attr(src_addr);

		auth_attr_time *timestamp_attr = new auth_attr_time();
		timestamp_attr->set_value(time(NULL));
		sauth_obj_hmac->insert_attr(timestamp_attr);

		auth_nslp_object_list *nslp_obj_list = new auth_nslp_object_list;
		nslp_obj_list->insert(known_nslp_object::RII);
		if (vlsp_obj)
			nslp_obj_list->insert(known_nslp_object::VLSP_OBJECT);
		sauth_obj_hmac->insert_attr(nslp_obj_list);

		auth_attr_data *auth_givendataattr= static_cast<auth_attr_data *>(sauth_object->get_attr(AUTHENTICATION_DATA,0));
		if (auth_givendataattr)
		{
			auth_attr_data *auth_dataattr = new auth_attr_data(auth_givendataattr->get_key_id());
			sauth_obj_hmac->insert_attr(auth_dataattr);

			DLog(state_manager::modname, "Session auth object present, adding new one to RESPONSE");

			resp->set_sessionauth_hmac_signed(sauth_obj_hmac);
		}
		else
		{
			// delete everything not needed
			delete transid;
			delete src_addr;
			delete timestamp_attr;
			delete nslp_obj_list;
			delete sauth_obj_hmac;
		}
	}

	// copy optionally present vlsp object
	if (vlsp_obj)
        {
	  DLog(state_manager::modname, "VLSP object present, copying back to RESPONSE");
	  
	  resp->set_vlsp_object(vlsp_obj->copy());
	}

	if(mri == NULL) {
		DEBUG_LEAVE(state_manager::modname, "- mri == NULL");
		return;
	}

	if(mri->get_mrm() == mri::mri_t_pathcoupled) {
		const ntlp::mri_pathcoupled* pc_mri = dynamic_cast<const ntlp::mri_pathcoupled *>(mri);
		assert(pc_mri != NULL);

		// For simplicity, extract v4-mapped address for v4 case
		in6_addr addr6;
		netaddress *na = addresses->get_first(AddressList::ConfiguredAddr_P, pc_mri->get_destaddress().is_ipv4());
		na->get_ip(addr6);
		delete(na);

		info_spec* info_spec_obj = new info_spec(&addr6, err_class, err_code, 0);
		resp->set_errorobject(info_spec_obj);

		pdu = dynamic_cast<known_nslp_pdu*>(resp);
		assert(pdu != NULL);

		SignalingMsg* sigmsg = new SignalingMsg;
		sigmsg->set_msg(pdu);
		sigmsg->set_sig_mri(pc_mri);
		sigmsg->set_downstream(down);
		sigmsg->set_sid(*sid);
		sigmsg->send_or_delete();
	}
	else if(mri->get_mrm() == mri::mri_t_explicitsigtarget) {
		const ntlp::mri_explicitsigtarget *est_mri = dynamic_cast<const ntlp::mri_explicitsigtarget *>(mri);
		assert(est_mri != NULL);

		// prepare EST-MRI for sending RESPONSE message
		mri_pathcoupled pc_mri_send = est_mri->get_mri_pc();
		pc_mri_send.invertDirection();
		const hostaddress orig_sig_addr = est_mri->get_dest_sig_address();
		const hostaddress dest_sig_addr = est_mri->get_origin_sig_address();
		mri_explicitsigtarget *est_mri_send = new mri_explicitsigtarget(pc_mri_send, orig_sig_addr, dest_sig_addr);

		// For simplicity, extract v4-mapped address for v4 case
		in6_addr addr6;
		netaddress *na = addresses->get_first(AddressList::ConfiguredAddr_P, est_mri->get_dest_sig_address().is_ipv4());
		na->get_ip(addr6);
		delete(na);

		info_spec* info_spec_obj = new info_spec(&addr6, err_class, err_code, 0);
		resp->set_errorobject(info_spec_obj);
		
		pdu = dynamic_cast<known_nslp_pdu*>(resp);
		assert(pdu != NULL);
		
		ExplicitSignalingMsg* sigmsg = new ExplicitSignalingMsg();
		sigmsg->set_pdu(pdu);
		sigmsg->set_sig_mri(est_mri_send);
		sigmsg->set_sid(*sid);
		sigmsg->send_or_delete();
	}
	else {
		ERRLog(state_manager::modname, "send_response_with_rii() - Unknown MRI with MRI-ID: " << mri->get_mrm());
	}

	DEBUG_LEAVE(state_manager::modname, "(end of function)");
}


/** This function sends a RESPONSE message with RSN.
 * @param rsn the RSN object from the received message that will be copied into the RESPONSE message.
 * @param sid the session id the current message belongs to.
 * @param mri the MRI used to send the RESPONSE message.
 * @param down set to TRUE, if the direction is downstream, otherwise set to FALSE.
 * @param err_class this is the error class for the INFO_SPEC object in the RESPONSE message.
 * @param err_code this is the error code for the INFO_SPEC object in the RESPONSE message.
 */
void state_manager::send_response_with_rsn(const rsn* rsn, const ntlp::sessionid* sid, const ntlp::mri* mri, bool down,
		info_spec::errorclass_t err_class, info_spec::errorcode_t err_code)
{
	DEBUG_ENTER(state_manager::modname, "send_response_with_rsn()");

	known_nslp_pdu *pdu = NULL;
	responsemsg *resp = new responsemsg();
	resp->set_rsn(rsn->copy());

	if(mri == NULL) {
		DEBUG_LEAVE(state_manager::modname, "- mri == NULL");
		return;
	}

	if(mri->get_mrm() == mri::mri_t_pathcoupled) {
		const ntlp::mri_pathcoupled* pc_mri = dynamic_cast<const ntlp::mri_pathcoupled *>(mri);
		assert(pc_mri != NULL);

		// For simplicity, extract v4-mapped address for v4 case
		in6_addr addr6;
		netaddress *na = addresses->get_first(AddressList::ConfiguredAddr_P, pc_mri->get_destaddress().is_ipv4());
		na->get_ip(addr6);
		delete(na);

		info_spec* info_spec_obj = new info_spec(&addr6, err_class, err_code, 0);
		resp->set_errorobject(info_spec_obj);

		pdu = dynamic_cast<known_nslp_pdu*>(resp);
		assert(pdu != NULL);

		SignalingMsg* sigmsg = new SignalingMsg;
		sigmsg->set_msg(pdu);
		sigmsg->set_sig_mri(pc_mri);
		sigmsg->set_downstream(down);
		sigmsg->set_sid(*sid);
		sigmsg->send_or_delete();
	}
	else if(mri->get_mrm() == mri::mri_t_explicitsigtarget) {
		const ntlp::mri_explicitsigtarget *est_mri = dynamic_cast<const ntlp::mri_explicitsigtarget *>(mri);
		assert(est_mri != NULL);

		// prepare EST-MRI for sending RESPONSE message
		mri_pathcoupled pc_mri_send = est_mri->get_mri_pc();
		pc_mri_send.invertDirection();
		const hostaddress orig_sig_addr = est_mri->get_dest_sig_address();
		const hostaddress dest_sig_addr = est_mri->get_origin_sig_address();
		mri_explicitsigtarget *est_mri_send = new mri_explicitsigtarget(pc_mri_send, orig_sig_addr, dest_sig_addr);

		// For simplicity, extract v4-mapped address for v4 case
		in6_addr addr6;
		netaddress *na = addresses->get_first(AddressList::ConfiguredAddr_P, est_mri->get_dest_sig_address().is_ipv4());
		na->get_ip(addr6);
		delete(na);

		info_spec* info_spec_obj = new info_spec(&addr6, err_class, err_code, 0);
		resp->set_errorobject(info_spec_obj);
		
		pdu = dynamic_cast<known_nslp_pdu*>(resp);
		assert(pdu != NULL);
		
		ExplicitSignalingMsg* sigmsg = new ExplicitSignalingMsg();
		sigmsg->set_pdu(pdu);
		sigmsg->set_sig_mri(est_mri_send);
		sigmsg->set_sid(*sid);
		sigmsg->send_or_delete();
	}
	else {
		ERRLog(state_manager::modname, "send_response_with_rsn() - Unknown MRI with MRI-ID: " << mri->get_mrm());
	}

	DEBUG_LEAVE(state_manager::modname, "(end of function)");
}


/** This function forwards the received QUERY message.
 * @param q received QUERY message.
 * @param down set to TRUE, if the direction is downstream, otherwise set to FALSE.
 * @param sid the session id the current message belongs to.
 * @param rcvd_mri the MRI of the current session.
 */
void state_manager::forward_query_msg(querymsg *q, bool down, const ntlp::sessionid *sid, const ntlp::mri_pathcoupled *rcvd_mri) {
	ILog(state_manager::modname, "forward_query_msg()");
	
	querymsg *query = new querymsg();
	copy_query_message(q, query);

	known_nslp_pdu *pdu = dynamic_cast<known_nslp_pdu*>(query);
	
	SignalingMsg* sigmsg = new SignalingMsg();
	sigmsg->set_msg(pdu);
	sigmsg->set_sig_mri(rcvd_mri);
	sigmsg->set_downstream(down);
	sigmsg->set_sid(*sid);
	sigmsg->send_or_delete();
	
	ILog(state_manager::modname, "END forward_query_msg()");
}


/** This function creates and sends a RESERVE message as response to an empty QUERY message.
 *  In this case I am a QNI and send the RESERVE message upstream towards the flow sender
 *  (receiver-initiated reservation).
 *
 * @param send_rii this variable is set to TRUE if the RESERVE message must include a RII object.
 * @param q QSPEC from the received QUERY message.
 * @param b BOUND_SESSIONID if the new session must be bound to another session.
 * @param down set to TRUE, if the direction is downstream, otherwise set to FALSE.
 * @param sid the session id the current message belongs to.
 * @param rcvd_mri the MRI of the current session.
 */
void state_manager::create_and_send_reserve_msg_as_response_to_query(bool send_rii, qspec_object *qspec, const bound_sessionid *bsid, 
							    bool down, const ntlp::sessionid *sid, const ntlp::mri_pathcoupled *rcvd_mri,
							    NSLP_Session_Context *session_context, bool x_flag)
{
	DLog(state_manager::modname, "create_and_send_reserve_msg_as_response_to_query()");

	uint32 n;

	if (qspec) {
		bool success_reserve = rmf_admin.reserve_resources(qspec->get_qspec_data());
		
		if (!success_reserve) {
			return;
		}
	}

	reservereq *res = new reservereq();
	res->set_originator(true); // we are originator of the RESERVE

	if (!x_flag) {
		res->set_anticipated_reservation(true);
	}


	// RSN
	if (session_context != NULL) {
		session_context->lock();

		// TODO should we increment the RSN?
		session_context->increment_own_rsn();
		session_context->increment_own_rsn();
		res->set_rsn(session_context->get_own_rsn().copy());

		session_context->unlock();
	} 


	// RII
	if (send_rii) {
		rii* send_new_rii = new rii();
		send_new_rii->get(n);
		res->set_rii(send_new_rii);
	}

	// QSPEC
	// TODO: QSPEC is mandatory in a QUERY message!
	// But in a Receiver Initiated Reservation it
	// is in question if sensible
	if (qspec) {
		res->set_qspec(qspec->copy());
	}


	// BOUND SID
	if (bsid) {
		uint128 s;
		bsid->get(s);
		bound_sessionid* send_bs = new bound_sessionid();
		send_bs->set(s);
		res->set_bound_sid(send_bs);
	}


	// REPLACE-Flag
	if(x_flag) {
	    res->set_replace_flag();
	}

	known_nslp_pdu *pdu = dynamic_cast<known_nslp_pdu *>(res);

	SignalingMsg* sigmsg = new SignalingMsg();
	sigmsg->set_msg(pdu);
	sigmsg->set_sig_mri(rcvd_mri);
	sigmsg->set_downstream(down);
	sigmsg->set_sid(*sid);
	sigmsg->send_or_delete();

	DLog(state_manager::modname, "END create_and_send_reserve_msg_as_response_to_query()");
}


/** This function forwards RESERVE message.
 * @note This functions copies the RESERVE message and forwards it
 *
 * @param reservemsg received RESERVE message. Will be copied for forwarding.
 * @param session_context NSLP_Session_Context
 * @param fwd_mri the MRI used to forward this message.
 * @param down set to TRUE, if the direction is downstream, otherwise set to FALSE. Only used by the PC-MRM.
 */
void state_manager::forward_reserve(const reservereq* reservemsg, NSLP_Session_Context* session_context,
		const ntlp::sessionid *sid, const ntlp::mri* fwd_mri, bool down)
{
	DLog(state_manager::modname, "forward_reserve()");


	reservereq* res; 
	known_nslp_pdu* pdu = NULL;
	
	if(fwd_mri == NULL) {
		ERRLog(state_manager::modname, "forward_reserve() - fwd_mri == NULL");
		return;
	}
	
	res = new reservereq();
	copy_reserve_message(reservemsg, res);
	
	session_context->lock();

	if (res->get_sessionauth()) 
	  DLog(state_manager::modname, "forward_reserve() - copied Session Authorization Object");

	if (res->get_vlsp_object()) 
	  DLog(state_manager::modname, "forward_reserve() - copied VLSP Object");

	DLog(state_manager::modname, "forward_reserve() - Set RSN to " << session_context->get_own_rsn());
	res->set_rsn(session_context->get_own_rsn().copy());
	session_context->unlock();
	
	bound_sessionid* bs = reservemsg->get_bound_sid();
	if (bs == NULL) {
		uint128 s;
		session_context->lock();
		if (session_context->get_bound_sid(s)) {
			bound_sessionid* send_bs = new bound_sessionid(s);
			res->set_bound_sid(send_bs);
		}
		session_context->unlock();
	}
	
	// how long should be the reservation kept on the old path???
	/*
	if (context->get_branchingNode()) {
	  SignalingMsg* sigmsg_br;
	  uint32 sii_handle;
	  context->get_old_downstream_sii(sii_handle);
	  reservereq* branch_res = new reservereq();
	  copy_reserve_message(res, branch_res);
	  pdu = dynamic_cast<known_nslp_pdu*>(branch_res);
	
	  sigmsg_br = new SignalingMsg();
	  sigmsg_br->set_msg(pdu);
	  sigmsg_br->set_sig_mri(rcvd_mri);
	  sigmsg_br->set_downstream(down);
	  sigmsg_br->set_sid(*sid);
	  sigmsg_br->set_sii_handle(sii_handle);
	  sigmsg_br->send_or_delete();
	}
	*/
	
	pdu = dynamic_cast<known_nslp_pdu*>(res);
	
	if(fwd_mri->get_mrm() == mri::mri_t_pathcoupled) {
	        const ntlp::mri_pathcoupled* pc_mri = dynamic_cast<const ntlp::mri_pathcoupled *>(fwd_mri);
	        assert(pc_mri != NULL);
	
	        SignalingMsg *sigmsg = new SignalingMsg();
	        sigmsg->set_msg(pdu);
	        sigmsg->set_sig_mri(pc_mri);
	        sigmsg->set_downstream(down);
	        sigmsg->set_sid(*sid);
	        sigmsg->set_sii_handle(0);
	        sigmsg->send_or_delete();
	}
	else if(fwd_mri->get_mrm() == mri::mri_t_explicitsigtarget) {
	        const ntlp::mri_explicitsigtarget *est_mri = dynamic_cast<const ntlp::mri_explicitsigtarget *>(fwd_mri);
	        assert(est_mri != NULL);
	
	        ExplicitSignalingMsg *sigmsg = new ExplicitSignalingMsg();
	        sigmsg->set_pdu(pdu);
	        sigmsg->set_sig_mri(est_mri);
	        sigmsg->set_sid(*sid);
	        sigmsg->send_or_delete();
	}
	else {
	        ERRLog(state_manager::modname, "forward_reserve() - Unknown MRI with MRI-ID: " << fwd_mri->get_mrm());
	        return;
	}
	
	DLog(state_manager::modname,"END forward_reserve()");
}


/** This function processes the RII object from a received message or for the message to be sent.
 * @param context the NSLP context of the current session.
 * @param reservemsg received RESERVE message.
 * @param rii_tmsg the timer for the RII if the RII will be saved in the own list with RIIs.
 * @param is_qni set to TRUE, if the node is QNI, otherwise set to FALSE.
 * @param is_qnr set to TRUE, if the node is QNR, otherwise set to FALSE.
 * @param local_sid the session id the current message belongs to.
 * @param down set to TRUE, if the direction is downstream, otherwise set to FALSE.
 * @param rcvd_mri the MRI of the current session.
 */
state_manager::error_t  state_manager::process_rii(NSLP_Session_Context* session_context, reservereq* reservemsg, const ntlp::sessionid* local_sid, 
		TimerMsg* rii_tmsg, bool is_qni, bool is_qnr, const ntlp::mri_pathcoupled* rcvd_pc_mri, bool down, bool is_merging_node)
{
    error_t nslpres = error_ok;
    rii* saved_rii;
    bool is_qne = false;
    uint32 rii_number= 0, n= 0;
    int32 msec = 0;

    ILog(state_manager::modname, "process_rii()");

    rii_tmsg->set_source(message::qaddr_qos_nslp_timerprocessing);

    if ((!is_qni) && (!is_qnr)) {
	is_qne = true;
    }

    rii *rcvd_rii = reservemsg->get_rii();

    if (!is_qnr) {
	// check if the RII number already existing
	NSLP_Session_Context_Map::rii_hashmap_it_t rii_hit;
//	NSLP_Context::rii_hashmap_it_t rii_hit;
	bool check_it_again = true;

	while (check_it_again) {
	  rcvd_rii->get(rii_number);
	  ILog(state_manager::modname, "Check for duplicates for RII " << rii_number);

	  NSLP_Session_Context_Map::riimap_t &rii_hashmap = session_context_map->get_rii_hashmap(*local_sid);
//	  session_context->lock();
//	  if ((rii_hit=session_context->rii_hashmap.find(rii_number))!=session_context->rii_hashmap.end())
	  if ((rii_hit = rii_hashmap.find(rii_number)) != rii_hashmap.end())
	  { 
	    ILog(state_manager::modname, "FOUND RII " << rii_number << " in HASHMAP!");
	    saved_rii = rii_hit->second;
	  }
	  else {
	    saved_rii = NULL;
	    check_it_again = false;
	  }
//	  session_context->unlock();

	  // in case that a received RII is equal to the own saved RII send response with error
	  if (saved_rii) {
//XXX: retransmissions are broken!
//TODO:	    rcvd_rii->get_retry_counter(n);
	    if (!rcvd_rii->get_own()) {
	      ERRLog(state_manager::modname, "Another node has sent a message with the duplicated RII!!!");
	      send_response_with_rii(rcvd_rii, local_sid, rcvd_pc_mri, !down, info_spec::transient, info_spec::RIIconflict);
	      check_it_again = false;
	      nslpres = error_nothing_to_do;
	      return nslpres;
	    }
	    else /* TODO: if (n == 0) */ {
	      ERRLog(state_manager::modname, "I am trying to send a message with the duplicated RII!!!");
	      rcvd_rii->generate_new();
	    }
// TODO:    else
//	      check_it_again = false;
	  }
	} // end while
    }

    // if Originator, save rii to match response, forward the message
    
    if (rcvd_rii->get_own()) {
      ILog(state_manager::modname, "Setting RII in hashmap to " << rii_number);

      rii* timer_rii = new rii();
      rii* context_rii = new rii();
      timer_rii->set(rii_number);
      context_rii->set(rii_number);
      timer_rii->set_own(true);
      context_rii->set_own(true);
      
      // start timer for RII, must not wait forever
      id_t check_id = rii_tmsg->get_id();
      timer_rii->set_rii_timer_id(check_id);
      context_rii->set_rii_timer_id(check_id);
      rcvd_rii->get_retry_counter(n);
      timer_rii->set_retry_counter(n);
      context_rii->set_retry_counter(n);
      qspec_object* rcvd_qspec = reservemsg->get_qspec();
      context_rii->set_qspec(rcvd_qspec);
      context_rii->set_is_reserve(true);

//      session_context->lock();
//      session_context->rii_hashmap[rii_number] = context_rii;
//      session_context->unlock();
      NSLP_Session_Context_Map::riimap_t &rii_hashmap = session_context_map->get_rii_hashmap(*local_sid);
      rii_hashmap[rii_number] = context_rii;
      
      reservereq* timer_reserve = new reservereq();
      copy_reserve_message(reservemsg, timer_reserve);
      timer_reserve->set_rii(timer_rii);
      timer_reserve->set_bool_rii(true);
      timer_reserve->set_bool_rp(false);
      
      double repeat = pow((double)qosnslpconf.getpar<uint32>(qosnslpconf_request_retry), (double)(n + 1));

#ifndef NSIS_OMNETPP_SIM
      timer_t rii_t = time(NULL) + (int)repeat;
#else
      timer_t rii_t = (int32)(simTime().dbl()+1) + (int)repeat;
#endif

      uint128 sid= *local_sid;
      ntlp::sessionid* timer_sid = new sessionid(sid);
      rii_tmsg->start_absolute(rii_t, msec, timer_sid, rcvd_pc_mri->copy(), timer_reserve);
      if (rii_tmsg->send_to(message::qaddr_timer)) 
      {
	ILog(state_manager::modname, "Timer for RII " << check_id << " set to " << rii_t);
      }
      rii_tmsg = NULL;
    }
    else {
      // if QNR, generate RESPONSE message, send RESPONSE to RII-Initiator
      if ((is_qnr) || is_merging_node) {
	ILog(state_manager::modname, "RII existing, Setting nslpres = (error_ok_send_response) ");
	nslpres = error_ok_send_response;
      }

      // if QNE, but S-Flag set, send RESPONSE to RII-Initiator
      if (is_qne) {
	if (reservemsg->is_scoping_flag()) {
	  ILog(state_manager::modname, "QNE: RII is existing and S-Flag is set, sending RESPONSE!");
	  nslpres = error_ok_send_response;
	}
	else {
	  ILog(state_manager::modname, "QNE: set nslpres = error_ok_forward!");
	  nslpres = error_ok_forward;
	}
	}
    }

    ILog(state_manager::modname,"end process_rii()");
    return nslpres;
}


/** This function copies RESERVE message.
 * @param source the source RESERVE message.
 * @param dest the destination RESERVE message.
 */
void state_manager::copy_reserve_message(const reservereq* source, reservereq* dest)
{
    uint32 n;
    bool orig = source->get_originator();
    dest->set_originator(orig);

    rsn* rcvd_rsn = source->get_rsn();
    if (rcvd_rsn) {
	n= rcvd_rsn->get();
	rsn* new_rsn = new rsn(n);
	dest->set_rsn(new_rsn);
	rcvd_rsn = NULL;
    }
    packet_classifier* rcvd_pc = source->get_packet_classifier();
    if (rcvd_pc) {
	packet_classifier* new_pc = new packet_classifier();
	new_pc = rcvd_pc->copy();
	dest->set_packet_classifier(new_pc);
	rcvd_pc = NULL;
    }
    rii* rcvd_rii = source->get_rii();
    if (rcvd_rii) {
	rii* new_rii = new rii();
	rcvd_rii->get(n);
	new_rii->set(n);
	bool own = rcvd_rii->get_own();
	new_rii->set_own(own);
	bool is_res = rcvd_rii->get_is_reserve();
	new_rii->set_is_reserve(is_res);
	dest->set_rii(new_rii);
	rcvd_rii = NULL;
    }
    rp* rcvd_rp = source->get_rp();
    if (rcvd_rp) {
	rcvd_rp->get(n);
	rp* new_rp = new rp(n);
	dest->set_rp(new_rp);
	rcvd_rp = NULL;
    }
    bound_sessionid* bs = source->get_bound_sid();
    if (bs) {
	uint128 s;
	bs->get(s);
	bound_sessionid* send_bs = new bound_sessionid(s);
	dest->set_bound_sid(send_bs);
	bs = NULL;
    }

    const qspec_object* qs = source->get_qspec();
    if (qs) {
	dest->set_qspec(qs->copy());
    }

    const vlsp_object* vlspobj = source->get_vlsp_object();
    if (vlspobj) {
	dest->set_vlsp_object(vlspobj->copy());
    }

    const session_auth_object* sauthobj= source->get_sessionauth();
    if (sauthobj)
    {
      dest->set_sessionauth(sauthobj->copy());
    }

    const session_auth_object* sauthobj_hmac_signed= source->get_sessionauth_hmac_signed();
    if (sauthobj_hmac_signed)
    {
      dest->set_sessionauth_hmac_signed(sauthobj_hmac_signed->copy());
    }

    if (source->is_replace_flag()) {
	dest->set_replace_flag();
    }
    if (source->is_acknowledge_flag()) {
	dest->set_acknowledge_flag();
    }
    if (source->is_scoping_flag()) {
	dest->set_scoping_flag();
    }
    if (source->is_tear_flag()) {
	dest->set_tear_flag();
    }
}

/** This function copies QUERY message.
 * @param source the source QUERY message.
 * @param dest the destination QUERY message.
 */
void state_manager::copy_query_message(const querymsg* source, querymsg* dest)
{
    ILog(state_manager::modname, "copy_query_message()");

    uint32 n;
    bool orig = source->get_originator();
    dest->set_originator(orig);

    packet_classifier* rcvd_pc = source->get_packet_classifier();
    if (rcvd_pc) {
        Log(INFO_LOG,LOG_NORMAL,state_manager::modname,"Found PC. Copy it.");
	packet_classifier* new_pc = new packet_classifier();
	new_pc = rcvd_pc->copy();
	dest->set_packet_classifier(new_pc);
	rcvd_pc = NULL;
    }
    rii* rcvd_rii = source->get_rii();
    if (rcvd_rii) {
        Log(INFO_LOG,LOG_NORMAL,state_manager::modname,"Found RII. Copy it.");
	rii* new_rii = new rii();
	rcvd_rii->get(n);
	new_rii->set(n);
	bool own = rcvd_rii->get_own();
	new_rii->set_own(own);
	bool is_res = rcvd_rii->get_is_reserve();
	new_rii->set_is_reserve(is_res);
	const qspec_object* qs = rcvd_rii->get_qspec();
	if (qs) {
	  new_rii->set_qspec(qs);
	}
	dest->set_rii(new_rii);
	rcvd_rii = NULL;
    }
    bound_sessionid* bs = source->get_bound_sid();
    if (bs) {
	uint128 s;
	bs->get(s);
	bound_sessionid* send_bs = new bound_sessionid(s);
	dest->set_bound_sid(send_bs);
	bs = NULL;
    }
    qspec_object* qs = source->get_qspec();
    if (qs) {
	dest->set_qspec(qs->copy());
    }
    if (source->is_scoping_flag()) {
	dest->set_scoping_flag();
    }
    if (source->is_reserve_init_flag()) {
	dest->set_reserve_init_flag();
    }
    if (source->is_x_flag()) {
	dest->set_x_flag();
    }

    const vlsp_object* vlspobj = source->get_vlsp_object();
    if (vlspobj) {
	dest->set_vlsp_object(vlspobj->copy());
    }

    const session_auth_object* sauthobj= source->get_sessionauth();
    if (sauthobj)
    {
      dest->set_sessionauth(sauthobj->copy());
    }

    const session_auth_object* sauthobj_hmac_signed= source->get_sessionauth_hmac_signed();
    if (sauthobj_hmac_signed)
    {
      dest->set_sessionauth_hmac_signed(sauthobj_hmac_signed->copy());
    }


    ILog(state_manager::modname, "END copy_query_message()");
}

/** This function copies NOTIFY message.
 * @param source the source NOTIFY message.
 * @param dest the destination NOTIFY message.
 */
void
state_manager::copy_notify_message(const notifymsg* source, notifymsg* dest)
{
    Log(INFO_LOG,LOG_NORMAL,state_manager::modname,"copy_notify_message()");
    bool orig = source->get_originator();
    dest->set_originator(orig);

    bound_sessionid* bs = source->get_bound_sid();
    if (bs) {
	uint128 s;
	bs->get(s);
	bound_sessionid* send_bs = new bound_sessionid(s);
	dest->set_bound_sid(send_bs);
	bs = NULL;
    }
    qspec_object* qs = source->get_qspec();
    if (qs) {
	dest->set_qspec(qs->copy());
    }
    info_spec* err_obj = source->get_errorobject();
    if (err_obj) {
        info_spec *send_err_obj = new info_spec(*err_obj);
        dest->set_errorobject(send_err_obj);
    }

    if (source->is_scoping_flag()) {
	dest->set_scoping_flag();
    }
    if (source->is_reserve_init_flag()) {
	dest->set_reserve_init_flag();
    }
    if (source->is_x_flag()) {
	dest->set_x_flag();
    }

    ILog(state_manager::modname, "END copy_notify_message()");
}


/** This function processes the QSPEC object from a received message or for the message to be sent.
 * @param reservemsg received RESERVE message.
 * @param qspec_obj points to a copy of the QSPEC object contained in the reserve message.
 * @return error_ok | error_no_bandwidth
 */
state_manager::error_t  state_manager::process_qspec(reservereq* reservemsg, qspec_object *&qspec_obj)
{
	error_t nslpres = error_ok;
	qspec_object* rcvd_qspec = reservemsg->get_qspec();
	ILog(state_manager::modname, "process_qspec()");

	qspec_obj = rcvd_qspec->copy();

	if (!(rmf_admin.reserve_resources(rcvd_qspec->get_qspec_data()))) {
		nslpres = error_no_bandwidth;

		Log(ERROR_LOG, LOG_NORMAL, state_manager::modname, "not enough bandwidth!");
	}
	else {
		ILog(state_manager::modname, "Reservation OK!");
	}

	ILog(state_manager::modname, "end process_qspec()");
	return nslpres;
}


/** This function deletes a NSLP_Flow_Context.
 */
void state_manager::delete_flow_context(const ntlp::sessionid *sid, const ntlp::mri_pathcoupled *flow,
		NSLP_Session_Context *session_context, NSLP_Flow_Context *flow_context)
{
	flow_context->lock();
	qspec_object qspec_obj = flow_context->get_qspec();
	flow_context->unlock();

	// release resources
	const qspec_pdu *q_pdu = qspec_obj.get_qspec_data();
	if (q_pdu) {
		MP(benchmark_journal::PRE_VLSP_SETDOWN_SCRIPT_SOURCE);
		rmf_admin.release_resources(q_pdu);
		MP(benchmark_journal::POST_VLSP_SETDOWN_SCRIPT_SOURCE);
	}


	id_t lifetime_timer_id;
	bool stop;

	flow_context->lock();
	flow_context->get_timer_id_for_lifetime(lifetime_timer_id);
	flow_context->unlock();

	TimerMsg* tmsg = new TimerMsg(message::qaddr_qos_nslp_timerprocessing);
	stop = tmsg->stop(lifetime_timer_id);
	tmsg->send_to(message::qaddr_timer);
	if (stop) {
		ILog(state_manager::modname, "Stopped lifetime timer with id " << lifetime_timer_id << " for flow ("
				<< flow->get_sourceaddress() << ", " << flow->get_destaddress() << ").");
	}


	// session_context is already locked!
	session_context->remove_flow_context(*flow);
	delete flow_context;

	// session_context is already locked!
	session_context->remove_predecessor_sii(*flow);
	session_context->remove_successor_sii(*flow);

	ILog(state_manager::modname, "Deleted NSLP_Flow_Context for flow ("
			<< flow->get_sourceaddress() << ", " << flow->get_destaddress() << ").");
	

	// Inhibits sending of messages yet in the outgoing queue,
	// thus commenting it out.
	/*
	SignalingMsg *sigmsg = new SignalingMsg();
	sigmsg->set_sig_mri(flow);
	sigmsg->set_msg(0);
	sigmsg->set_downstream(true);
	sigmsg->set_invalidate(true);
	sigmsg->send_or_delete();
	
	sigmsg = new SignalingMsg();
	sigmsg->set_sig_mri(flow);
	sigmsg->set_msg(0);
	sigmsg->set_downstream(false);
	sigmsg->set_invalidate(true);
	sigmsg->send_or_delete();
	*/
}


/** This function processes a received RESPONSE message.
 * @param known_pdu NSLP PDU to be processed.
 * @param rcvd_sid the session id the current message belongs to.
 * @param rcvd_mri the MRI of the current session.
 * @param down set to TRUE, if the direction is downstream, otherwise set to FALSE. Only evaluated with PC-MRM.
 */
void state_manager::process_response_msg(known_nslp_pdu* known_pdu, const ntlp::sessionid* rcvd_sid, const ntlp::mri* rcvd_mri, bool down)
{
	responsemsg* response = dynamic_cast<responsemsg*>(known_pdu);

	uint32 rii_number;

	NSLP_Session_Context_Map::rii_hashmap_it_t rii_hit;
	rii* context_rii = NULL;
	
	const ntlp::mri_pathcoupled *rcvd_pc_mri=NULL;
	ntlp::mri_explicitsigtarget fwd_est_mri;

	ILog(state_manager::modname, "process_response_msg()");


	if(rcvd_mri == NULL) {
		ERRLog(state_manager::modname, "process_response_msg() - rcvd_mri == NULL");
		return;
	}
  
	if(rcvd_mri->get_mrm() == mri::mri_t_pathcoupled) {
		rcvd_pc_mri = dynamic_cast<const ntlp::mri_pathcoupled *>(rcvd_mri);
		assert(rcvd_pc_mri != NULL);
	}
	else if(rcvd_mri->get_mrm() == mri::mri_t_explicitsigtarget) {
		const ntlp::mri_explicitsigtarget *rcvd_est_mri = dynamic_cast<const ntlp::mri_explicitsigtarget *>(rcvd_mri);
		assert(rcvd_est_mri != NULL);

		rcvd_pc_mri  = &rcvd_est_mri->get_mri_pc();
		down = rcvd_pc_mri->get_downstream();
	}
	else {
		ERRLog(state_manager::modname, "process_response_msg() - Unknown MRI with MRI-ID: " << rcvd_mri->get_mrm());
		return;
	}


	NSLP_Session_Context *session_context = rcvd_sid ? session_context_map->find_session_context(*rcvd_sid) : NULL;

	// RESPONSE msg is not for me, forward it
	if (session_context == NULL) {
#ifdef USE_AHO
		if((rcvd_mri->get_mrm() == mri::mri_t_pathcoupled) &&
				is_new_access_router_and_flow_matches(*rcvd_sid, *rcvd_pc_mri, &fwd_est_mri)) {
			// forward RESPONSE message towards the mobile node by means of the EST-MRM
			forward_response_msg(response, rcvd_sid, &fwd_est_mri);
		}
		else {
			// forward RESPONSE message by means of the PC-MRM
			forward_response_msg(response, rcvd_sid, rcvd_pc_mri, down);
		}
#else
		forward_response_msg(response, rcvd_sid, rcvd_pc_mri, down);
#endif
		ILog(state_manager::modname, "END process_response_msg()");

		return;
	}

	assert(session_context != NULL);

	session_context->lock();
	NSLP_Session_Context::qn_type_t qn_type = session_context->get_qn_type();
	session_context->unlock();

	rii* check_rii = response->get_rii();

	// Request Identification Information set
	if (check_rii) {
		bool stop;
		check_rii->get(rii_number);

		ILog(state_manager::modname, "RESPONSE msg WITH RII " << rii_number);


		// Check whether RII is in hashmap
		bool is_rii_in_hashmap = false;
//		session_context->lock();
//		if ((rii_hit = session_context->rii_hashmap.find(rii_number)) != session_context->rii_hashmap.end()) { 
		NSLP_Session_Context_Map::riimap_t &rii_hashmap = session_context_map->get_rii_hashmap(*rcvd_sid);
		if ((rii_hit = rii_hashmap.find(rii_number)) != rii_hashmap.end()) { 
			ILog(state_manager::modname, "FOUND RII in HASHMAP!");

			is_rii_in_hashmap = true;
		}
		else {
			ILog(state_manager::modname, "Processing RESPONSE: RII " << rii_number << " NOT FOUND!");
		}
//		session_context->unlock();


		// stop timer and erase RII in hashmap
		if (is_rii_in_hashmap) { 
			id_t rii_timer_id;

			context_rii = rii_hit->second;
			context_rii->get_rii_timer_id(rii_timer_id);

			TimerMsg* tmsg = new TimerMsg(message::qaddr_qos_nslp_timerprocessing);
			stop = tmsg->stop(rii_timer_id);
			tmsg->send_to(message::qaddr_timer);
			if (stop) 
				ILog(state_manager::modname, "RII timer " << rii_timer_id << " stopped!");

			ILog(state_manager::modname, "Processing RESPONSE: DELETING RII " << rii_number);

//			session_context->lock();
//			session_context->rii_hashmap.erase(rii_number);
//			session_context->unlock();
			rii_hashmap.erase(rii_number);



			// I think this isn't useful anymore with NSLP_Session_Context and NSLP_Flow_Context,
			// since a NSLP_Flow_Context will be installed only if the corresponding reservation
			// was successful.
/*
			if (!(context->get_reserved())) {
				id_t lifetime_timer_id;
				context->get_timer_id_for_lifetime(lifetime_timer_id);
				context->unlock();

				tmsg = new TimerMsg(message::qaddr_qos_nslp_timerprocessing);
				tmsg->set_source(message::qaddr_qos_nslp_timerprocessing);
				stop = tmsg->stop(lifetime_timer_id);
				tmsg->send_to(message::qaddr_timer);
				if (stop) 
					ILog(state_manager::modname, "LIFETIME " << lifetime_timer_id << "  timer stopped!");

				contextmap.erase(*rcvd_sid);
			}
			else {
				context->unlock();
			}

			tmsg = NULL;
*/
		}

		//-----------------
		// TODO Rerouting
		//-----------------
		/*
			if (context_rii->get_own()) {
				//if branching node check whether to tear down the reservation on the old path
				context->lock();
				if ((context->get_branchingNode()) && (context->get_replace()) && qn_type != NSLP_Context::QNI) {
					context->set_branchingNode(false);
					context->unlock();
					send_reserve_with_tear_on_the_old_path(context, rcvd_sid, rcvd_pc_mri, !down);
					return;
				}
				else {
					ILog(state_manager::modname, "Checking to see if we should send a TEAR");
					if (context->get_branchingNode()) {
		                               	ILog(state_manager::modname, "Looks like it, looking up old MRI");
						ntlp::mri_pathcoupled *old_mri;
						context->set_branchingNode(false);
						old_mri = context->get_old_mri();
						if (old_mri) {
							context->unlock();
							ILog(state_manager::modname, "Branching node - sending TEAR");
							send_reserve_with_tear_on_the_old_path(context, rcvd_sid, old_mri, !down);
							context->lock();
						}
					}
					context->unlock();
                                }
			} else {
				ILog(state_manager::modname, "Checking to see if we should send a TEAR");
				context->lock();
				if ((context->get_branchingNode()) && qn_type != NSLP_Context::QNI) {
                                	ILog(state_manager::modname, "Looks like it, looking up old MRI");
					ntlp::mri_pathcoupled *old_mri;
					context->set_branchingNode(false);
					old_mri = context->get_old_mri();
					context->unlock();
					if (old_mri) {
						ILog(state_manager::modname, "Branching node - sending TEAR");
						send_reserve_with_tear_on_the_old_path(context, rcvd_sid, old_mri, !down);
					}
				}
			}

			// -----------------------------------------------

			ILog(state_manager::modname, "Checking to see if we should send a TEAR");
			if ((context->get_branchingNode()) && qn_type != NSLP_Context::QNI) {
                               	ILog(state_manager::modname, "Looks like it, looking up old MRI");
				ntlp::mri_pathcoupled *old_mri;
				context->set_branchingNode(false);
				old_mri = context->get_old_mri();
				if (old_mri) {
					context->unlock();
					ILog(state_manager::modname, "Branching node - sending TEAR");
					send_reserve_with_tear_on_the_old_path(context, rcvd_sid, old_mri, !down);
					context->lock();
				}
			}
		*/


		//================================================================================================================
		// Forward RESPONSE message
		//================================================================================================================
		if (is_rii_in_hashmap) {
			if (!context_rii->get_own()) {
				if (qn_type == NSLP_Session_Context::QNE) {
					ILog(state_manager::modname, "I am not the originator of RII: Forwarding it!");
#ifdef USE_AHO
					if((rcvd_mri->get_mrm() == mri::mri_t_pathcoupled) &&
							is_new_access_router_and_flow_matches(*rcvd_sid, *rcvd_pc_mri, &fwd_est_mri)) {
						// forward RESPONSE message towards the mobile node by means of the EST-MRM
						forward_response_msg(response, rcvd_sid, &fwd_est_mri);
					}
					else {
						// forward RESPONSE message by means of the PC-MRM
						forward_response_msg(response, rcvd_sid, rcvd_pc_mri, down);
					}
#else
					forward_response_msg(response, rcvd_sid, rcvd_pc_mri, down);
#endif
				}
			}

		}
		else {
			// RESPONSE msg is not for me, forward it
			if (qn_type == NSLP_Session_Context::QNE) {
#ifdef USE_AHO
				if((rcvd_mri->get_mrm() == mri::mri_t_pathcoupled) &&
						is_new_access_router_and_flow_matches(*rcvd_sid, *rcvd_pc_mri, &fwd_est_mri)) {
					// forward RESPONSE message towards the mobile node by means of the EST-MRM
					forward_response_msg(response, rcvd_sid, &fwd_est_mri);
				}
				else {
					// forward RESPONSE message by means of the PC-MRM
					forward_response_msg(response, rcvd_sid, rcvd_pc_mri, down);
				}
#else
				forward_response_msg(response, rcvd_sid, rcvd_pc_mri, down);
#endif
			}
		}


		// check for error class: if transient or permanent error then delete context
		bool erase_state = false;
		info_spec* er_obj = response->get_errorobject();
		if (er_obj) {
			info_spec::errorclass_t er_class = (info_spec::errorclass_t) er_obj->get_errorclass();
			info_spec::errorcode_t er_code = (info_spec::errorcode_t) er_obj->get_errorcode();

			if (er_class == info_spec::success) {
				session_context->lock();
				NSLP_Flow_Context *flow_context = session_context->find_flow_context(*rcvd_pc_mri);

				if (flow_context != NULL) {
					flow_context->lock();
					// set RESERVED-Flag
					flow_context->set_reserved(true);
					// reduced refresh
					flow_context->set_reduced_refresh(true);

					if (flow_context->get_replace()) {
						// tear down all other branches

						NSLP_Session_Context::flowcontext_const_it_t it;
						it = session_context->flowcontext.begin();
						while (it != session_context->flowcontext.end()) {
							const ntlp::mri_pathcoupled flow = it->first;
							NSLP_Flow_Context *flow_context_other_flow = it->second;
						
							// Since the following code can delete the current (key, value) pair,
							// we jump to the next pair in advance.
							++it;
						
							if ((flow.get_sourceaddress() != rcvd_pc_mri->get_sourceaddress()) ||
									(flow.get_destaddress() != rcvd_pc_mri->get_destaddress()))
							{
								tear_down_other_flow(rcvd_sid, &flow, !down, session_context, flow_context_other_flow);
						
								delete_flow_context(rcvd_sid, &flow, session_context, flow_context_other_flow);
							}
						}
					}
					flow_context->unlock();
				}
				session_context->unlock();
			} // if success

			const vlsp_object *vlspobj = response->get_vlsp_object();
			if (vlspobj)
			{
			    DLog(state_manager::modname,"VLSP object present");

			    if (qn_type == NSLP_Session_Context::QNI)
			      {
				
				DLog(state_manager::modname,"I am QNI - will process VLSP - calling RMF");
				MP(benchmark_journal::PRE_VLSP_SETUP_SCRIPT_SOURCE);
				rmf_admin.process_vlsp_object_setup_resp(vlspobj);
				MP(benchmark_journal::POST_VLSP_SETUP_SCRIPT_SOURCE);
			      }
			    MP(benchmark_journal::POST_VLSP_SETUP);

			} // endif vlsp object

			// provide feedback to the application
			process_info_spec_for_appl(rcvd_sid, er_obj, response->get_vlsp_object());

			if ((er_class == info_spec::transient) || (er_class == info_spec::permanent)) {
				erase_state = true;
			}

			if (is_rii_in_hashmap && context_rii->get_own())  {
				if ((er_class == info_spec::transient) && (er_code == info_spec::RIIconflict)) {
					erase_state = false;

					//retransmit_message_with_different_rii(session_context, context_rii, false, rcvd_sid, rcvd_pc_mri);
					retransmit_message_with_different_rii(session_context, context_rii, !down, rcvd_sid, rcvd_pc_mri);
				}
			}
		}

		if (erase_state == true) {
			session_context->lock();

			NSLP_Flow_Context *flow_context = session_context->find_flow_context(*rcvd_pc_mri);
			if (flow_context != NULL) {
				ILog(state_manager::modname, "Transient or permanent Error: Deleting context!");

				delete_flow_context(rcvd_sid, rcvd_pc_mri, session_context, flow_context);
			}

			session_context->unlock();
		}
	}  // end if RII in RESPONSE available
	else {
		rsn* check_rsn = response->get_rsn();
		if (check_rsn) {

			ILog(state_manager::modname, "RESPONSE msg WITH RSN " << check_rsn); 

			session_context->lock();
			rsn local_rsn= session_context->get_own_rsn();
			session_context->unlock();

			ILog(state_manager::modname, "LOCAL RSN is " << local_rsn);

			if (*check_rsn == local_rsn) {
				info_spec* er_obj = response->get_errorobject();
				if (er_obj) {
					info_spec::errorclass_t er_class = (info_spec::errorclass_t) er_obj->get_errorclass();
					info_spec::errorcode_t er_code = (info_spec::errorcode_t) er_obj->get_errorcode();

					if ((er_class == info_spec::transient) && (er_code == info_spec::FullQSPECrequired)) {
						// nothing to do because resend of the complete RESERVE 
						// message with QSPEC will be done by process_sii_handle() 
						// function. The SII_HANDLE will then also be processed.
						return;
					}

					if (er_class == info_spec::success) {
						session_context->lock();
						NSLP_Flow_Context *flow_context = session_context->find_flow_context(*rcvd_pc_mri);
						session_context->unlock();

						if (flow_context != NULL) {
							flow_context->lock();
							flow_context->set_reserved(true);
							flow_context->set_reduced_refresh(true);
							flow_context->unlock();
						}
					}

					process_info_spec_for_appl(rcvd_sid, er_obj);

					if ((er_class == info_spec::transient) || (er_class == info_spec::permanent)) {
						//====================================================
						// ++++++++++++++This is from version 09++++++++++++++

						if (qn_type == NSLP_Session_Context::QNE) {
							rsn forward_rsn;

							session_context->lock();
							bool is_sender_init = session_context->get_sender_initiated();
							if((is_sender_init && down) || ((!is_sender_init) && (!down))) {
								// use ownRSN to forward RESPONSE message
								forward_rsn= session_context->get_own_rsn();
							}
							else if ((is_sender_init && (!down)) || ((!is_sender_init) && down)) {
								// use predRSN to forward RESPONSE message

								// get predecessor SII corresponding to the given flow
								uint32 predSII;
								session_context->get_predecessor_sii(*rcvd_pc_mri, predSII);

								// get predRSN corresponding to predSII
								forward_rsn= session_context->get_predecessor_rsn(predSII);
							}
							session_context->unlock();

							response->set_rsn(forward_rsn.copy());

#ifdef USE_AHO
							if((rcvd_mri->get_mrm() == mri::mri_t_pathcoupled) &&
									is_new_access_router_and_flow_matches(*rcvd_sid, *rcvd_pc_mri, &fwd_est_mri)) {
								// forward RESPONSE message towards the mobile node by means of the EST-MRM
								forward_response_msg(response, rcvd_sid, &fwd_est_mri);
							}
							else {
								// forward RESPONSE message by means of the PC-MRM
								forward_response_msg(response, rcvd_sid, rcvd_pc_mri, down);
							}
#else
							forward_response_msg(response, rcvd_sid, rcvd_pc_mri, down);
#endif
						}
						//====================================================

						// delete context
						session_context->lock();
			
						NSLP_Flow_Context *flow_context = session_context->find_flow_context(*rcvd_pc_mri);
						if (flow_context != NULL) {
							ILog(state_manager::modname, "Transient or permanent Error: Deleting context!");
			
							delete_flow_context(rcvd_sid, rcvd_pc_mri, session_context, flow_context);
						}
			
						session_context->unlock();
					}
				}
			} //  END if received RSN == local RSN

			// I think this is dead code since a QNR never receives a RESPONSE message!
			/*
			else {
				context->lock();
				if ((!(context->get_reserved())) && qn_type == NSLP_Context::QNR) {
					context->unlock();

					info_spec* er_obj = response->get_errorobject();
					if (er_obj) {
						info_spec::errorclass_t er_class = (info_spec::errorclass_t) er_obj->get_errorclass();

						if (er_class == info_spec::success) {
							session_context->lock();
							session_context->set_ReducedRefresh(true);
							session_context->unlock();
						}

						process_info_spec_for_appl(rcvd_sid, er_obj);

						if ((er_class == info_spec::transient) || (er_class == info_spec::permanent)) {
							id_t rii_timer_id;
							context->lock();
							context->get_timer_id_for_reserve_or_query(rii_timer_id);
							context->unlock();

							tmsg = new TimerMsg(message::qaddr_qos_nslp_timerprocessing);
							bool stop = tmsg->stop(rii_timer_id);
							tmsg->send_to(message::qaddr_timer);
							if (stop) {
								ILog(state_manager::modname, "Reserve timer " << rii_timer_id << " stopped!");
							}

							id_t lifetime_timer_id;
							context->lock();
							context->get_timer_id_for_lifetime(lifetime_timer_id);
							context->unlock();

							tmsg = new TimerMsg(message::qaddr_qos_nslp_timerprocessing);
							stop = tmsg->stop(lifetime_timer_id);
							tmsg->send_to(message::qaddr_timer);
							if (stop) 
								ILog(state_manager::modname, "LIFETIME " << lifetime_timer_id << "  timer stopped!");
						}
					}
				}
				else
					context->unlock();
			}
			*/

		}  // end if RSN found
	}

	ILog(state_manager::modname, "END process_response_msg()");
}


/** This function processes a received NOTIFY message.
 * @param known_pdu NSLP PDU to be processed.
 * @param rcvd_sid the session id the current message belongs to.
 * @param down set to TRUE, if the direction is downstream, otherwise set to FALSE.
 */
void state_manager::process_notify_msg(known_nslp_pdu* known_pdu, const ntlp::sessionid* rcvd_sid,
		const ntlp::mri_pathcoupled* rcvd_mri, bool down, error_t& nslpres)
{
	DLog(state_manager::modname, "process_notify_msg()");

	nslpres = error_nothing_to_do;

	notifymsg *notify = dynamic_cast<notifymsg *>(known_pdu);


	NSLP_Session_Context *session_context = rcvd_sid ? session_context_map->find_session_context(*rcvd_sid) : NULL;
	if (session_context == NULL) {
		return;
	}


	assert(session_context != NULL);

	bool erase_state = false;

	info_spec* er_obj = notify->get_errorobject();
	if (er_obj) {
		info_spec::errorclass_t er_class = (info_spec::errorclass_t) er_obj->get_errorclass();
		info_spec::errorcode_t er_code = (info_spec::errorcode_t) er_obj->get_errorcode();
	
		// Will this ever be used??
		/*
		if (er_class == info_spec::success) {
			if (context) {
				context->lock();
				context->set_ReducedRefresh(true);
				context->unlock();
			}
		}
		*/

		// forward ERROR_SPEC to application
		process_info_spec_for_appl(rcvd_sid, er_obj);
		
		// check for error class: if transient or permanent error then delete context
		if ((er_class == info_spec::transient) || (er_class == info_spec::permanent)) {
			erase_state = true;
			ILog(state_manager::modname, "Processing NOTIFY: SET erase_state to TRUE!");
		}
		
		if (er_class == info_spec::information && er_code == info_spec::RouteChange) {
			session_context->lock();
			session_context->set_is_maybe_dead_end(true);
			session_context->unlock();
		}

		// The following code isn't standard behavior as of QoS NSLP Draft 16, but needed for
		// an Anticipated Handover!
		if (er_class == info_spec::information && er_code == info_spec::TearDownBranch) {
			reservereq *res = new reservereq();

			// send tearing RESERVE upstream
			session_context->lock();
			session_context->increment_own_rsn();
			res->set_rsn(session_context->get_own_rsn().copy());
			session_context->unlock();
			
			// the originator flag causes processing of the outgoing message
			res->set_originator(true);
			res->set_tear_flag();
			
			known_nslp_pdu* pdu = dynamic_cast<known_nslp_pdu*>(res);
			assert(pdu != NULL);
			
			SignalingMsg* sigmsg = new SignalingMsg();
			sigmsg->set_msg(pdu);
			sigmsg->set_sig_mri(rcvd_mri);
			sigmsg->set_downstream(!down);
			sigmsg->set_sid(*rcvd_sid);
			sigmsg->send_or_delete();
		}
	}

	if (!is_last_signaling_hop(rcvd_mri, down)) {
		forward_notify_msg(notify, down, rcvd_mri, rcvd_sid);
	}
	else {
		if (er_obj) {
			info_spec::errorclass_t er_class = (info_spec::errorclass_t) er_obj->get_errorclass();
			info_spec::errorcode_t er_code = (info_spec::errorcode_t) er_obj->get_errorcode();
			
			if (er_class == info_spec::information && er_code == info_spec::RouteChange) {

				reservereq *res = new reservereq();
				// send tearing RESERVE upstream
				session_context->lock();
				session_context->increment_own_rsn();
				res->set_rsn(session_context->get_own_rsn().copy());
				session_context->unlock();
				
				// the originator flag causes processing of the outgoing message
				res->set_originator(true);
				res->set_tear_flag();
				
				known_nslp_pdu* pdu = dynamic_cast<known_nslp_pdu*>(res);
				assert(pdu != NULL);
				
				SignalingMsg* sigmsg = new SignalingMsg();
				sigmsg->set_msg(pdu);
				sigmsg->set_sig_mri(rcvd_mri);
				sigmsg->set_downstream(!down);
				sigmsg->set_sid(*rcvd_sid);
				sigmsg->send_or_delete();
			}
		}
	}
	
	if (erase_state == true) {
		session_context->lock();

		NSLP_Flow_Context *flow_context = session_context->find_flow_context(*rcvd_mri);
		if (flow_context != NULL) {
			ILog(state_manager::modname, "Transient or permanent Error: Deleting context!");

			delete_flow_context(rcvd_sid, rcvd_mri, session_context, flow_context);
		}

		session_context->unlock();
	}
	
	DLog(state_manager::modname, "END process_notify_msg()");
}


void state_manager::forward_notify_msg(notifymsg* notify, bool down, const ntlp::mri_pathcoupled* rcvd_mri, const ntlp::sessionid* sid) {
	ILog(state_manager::modname, "forward_notify_msg()");
	
	notifymsg *s_notify = new notifymsg();
	copy_notify_message(notify, s_notify);

	known_nslp_pdu *pdu = dynamic_cast<known_nslp_pdu *>(s_notify);

	SignalingMsg* sigmsg = new SignalingMsg();
	sigmsg->set_msg(pdu);
	sigmsg->set_sig_mri(rcvd_mri);
	sigmsg->set_downstream(down);
	sigmsg->set_sid(*sid);
	sigmsg->send_or_delete();
	
	ILog(state_manager::modname, "END forward_notify_msg()");
}


/** This function retransmits a message with RII because no RESPONSE message was received.
 * @param context NSLP context of the current session.
 * @param q QUERY message to be retransmitted, set to NULL if the message is RESERVE.
 * @param r RESERVE mesasge to be retransmitted, set to NULL if the message is QUERY.
 * @param t_rii the RII object for the current message to be retransmitted.
 * @param sid the session id the current message belongs to.
 */
void state_manager::retransmit_message_with_rii(const ntlp::sessionid *sid, const ntlp::mri_pathcoupled *mri, NSLP_Session_Context *session_context,
		querymsg *q, reservereq *r, rii *t_rii)
{
	DLog(state_manager::modname, "retransmit_message_with_rii()");
	
	uint32 rii_number, retry_counter;
	known_nslp_pdu* pdu = NULL;
#ifndef NSIS_OMNETPP_SIM
      timer_t t = time(NULL) + qosnslpconf.getpar<uint32>(qosnslpconf_refresh_period);
#else
      timer_t t = (int32)(simTime().dbl()+1) + qosnslpconf.getpar<uint32>(qosnslpconf_refresh_period);
#endif
	
	session_context->lock();
	session_context->set_time_to_live(t);
	session_context->unlock();
	
	id_t check_rii_id= 0;
	
	// get a reference to the rii_hashmap
	NSLP_Session_Context_Map::riimap_t &rii_hashmap = session_context_map->get_rii_hashmap(*sid);
	
	if (q) {
		DLog(state_manager::modname,"Retransmit with QUERY!");
		querymsg* send_q = new querymsg();
		copy_query_message(q, send_q);
		
		if (t_rii) {
			rii* timer_rii = new rii();
			rii* new_rii = new rii();
			t_rii->get(rii_number);
			timer_rii->set(rii_number);
			new_rii->set(rii_number);
			bool o = t_rii->get_own();
			timer_rii->set_own(o);
			new_rii->set_own(o);
			t_rii->get_retry_counter(retry_counter);
			timer_rii->set_retry_counter(retry_counter+1);
			new_rii->set_retry_counter(retry_counter+1);
			t_rii->get_rii_timer_id(check_rii_id);
			new_rii->set_rii_timer_id(check_rii_id);
			rii_hashmap[rii_number] = new_rii;
			timer_rii->set_rii_timer_id(check_rii_id);
			send_q->set_rii(timer_rii);
		}
		else {
			// increment retry counter
			uint32 r_counter;
			q->get_retry_counter(r_counter);
			send_q->set_retry_counter(r_counter+1);
		}
		
		pdu = dynamic_cast<known_nslp_pdu*>(send_q);
	}
	
	if (r) {
		DLog(state_manager::modname,"Retransmit with RESERVE!");
		reservereq* send_r = new reservereq();
		copy_reserve_message(r, send_r);
		
		rii* timer_rii = new rii();
		rii* new_rii = new rii();
		t_rii->get(rii_number);
		timer_rii->set(rii_number);
		new_rii->set(rii_number);
		bool o = t_rii->get_own();
		timer_rii->set_own(o);
		new_rii->set_own(o);
		t_rii->get_retry_counter(retry_counter);
		timer_rii->set_retry_counter(retry_counter+1);
		new_rii->set_retry_counter(retry_counter+1);
		new_rii->set_rii_timer_id(check_rii_id);
		rii_hashmap[rii_number] = new_rii;
		timer_rii->set_rii_timer_id(check_rii_id);
		send_r->set_rii(timer_rii);
		pdu = dynamic_cast<known_nslp_pdu*>(send_r);
	}
	
	session_context->lock();
	bool down = session_context->get_sender_initiated();
	session_context->unlock();
	
	SignalingMsg* sigmsg = new SignalingMsg();
	sigmsg->set_msg(pdu);
	sigmsg->set_sig_mri(mri);
	sigmsg->set_sid(*sid);
	sigmsg->set_downstream(down);
	sigmsg->send_or_delete();
	
	DLog(state_manager::modname, "END retransmit_message_with_rii()");
}


/** This function retransmits a message with a different RII number (e.g because of RII collision).
 * @param context NSLP context of the current session.
 * @param context_rii the RII object as base for the new RII of the current message to be retransmitted.
 * @param down set to TRUE, if the direction is downstream, otherwise set to FALSE.
 * @param rcvd_sid the session id the current message belongs to.
 * @param rcvd_mri the MRI of the current session.
 */
void  state_manager::retransmit_message_with_different_rii(NSLP_Session_Context *context, rii *context_rii, bool down, 
					const ntlp::sessionid *rcvd_sid, const ntlp::mri_pathcoupled *rcvd_mri)
{
	DLog(state_manager::modname, "retransmit_message_with_different_rii()");

	known_nslp_pdu* pdu = NULL;

	if (!context_rii->get_is_reserve()) {
		DLog(state_manager::modname, "Retransmit QUERY with different RII!");
		
		querymsg *send_q = new querymsg();
		const qspec_object *send_qspec = context_rii->get_qspec();
		send_q->set_qspec(send_qspec->copy());
		
		rii *send_rii = new rii();
		send_rii->generate_new();
		send_rii->set_own(true);
		send_rii->set_is_reserve(false);
		
		send_q->set_rii(send_rii);
		send_q->set_originator(true);
		
		pdu = dynamic_cast<known_nslp_pdu *>(send_q);
	}
	
	if (context_rii->get_is_reserve()) {
		DLog(state_manager::modname,"Retransmit RESERVE with different RII!");
		
		reservereq* send_r = new reservereq();
		
		// I think this isn't correct: RESERVE messages are always sent with the own RSN!
		/*
		if (((rcvd_mri->get_downstream()) && (down)) || ((!rcvd_mri->get_downstream()) && (!down)))  {
		      context->lock();
		      context->get_rsn_own(rsn_number);
		      context->unlock();
		      send_rsn = new rsn(rsn_number);
		      send_r->set_rsn(send_rsn);
		}
		
		if (((!rcvd_mri->get_downstream()) && (down)) || ((rcvd_mri->get_downstream()) && (!down)))  {
		      context->lock();
		      if (down)
		        context->get_rsn_upstream(rsn_number);
		      else
		        context->get_rsn_downstream(rsn_number);
		      context->unlock();
		      send_rsn = new rsn(rsn_number);
		      send_r->set_rsn(send_rsn);
		}
		*/
		context->lock();
		send_r->set_rsn(context->get_own_rsn().copy());
		context->unlock();
		
		const qspec_object* send_qspec = context_rii->get_qspec();
		send_r->set_qspec(send_qspec->copy());
		
		rii* send_rii = new rii();
		send_rii->generate_new();
		send_rii->set_own(true);
		send_rii->set_is_reserve(true);
		send_r->set_rii(send_rii);
		send_r->set_originator(true);
		
		pdu = dynamic_cast<known_nslp_pdu*>(send_r);
	}
	
	SignalingMsg* sigmsg = new SignalingMsg();
	sigmsg->set_msg(pdu);
	sigmsg->set_sig_mri(rcvd_mri);
	sigmsg->set_sid(*rcvd_sid);
	sigmsg->set_downstream(down);
	sigmsg->send_or_delete();
	
	DLog(state_manager::modname, "END retransmit_message_with_different_rii()");
}


/** This function sends a RESERVE message with TEAR_DOWN bit set on the old path.
 * @param context NSLP context of the current session.
 * @param rcvd_sid the session id the current message belongs to.
 * @param rcvd_mri the MRI of the current session.
 */
/*
void state_manager::send_reserve_with_tear_on_the_old_path(NSLP_Context* context, const ntlp::sessionid* rcvd_sid, 
						   const ntlp::mri_pathcoupled* rcvd_mri, bool down)
{
	ILog(state_manager::modname, "send_reserve_with_tear_down_on_the_old_path()");

	uint32 rsn_number, old_sii;
	rsn* send_rsn;
	known_nslp_pdu* pdu = NULL;
	reservereq* send_r = new reservereq();
	context->lock();
	context->get_rsn_own(rsn_number);
	context->unlock();
	send_rsn = new rsn(rsn_number);
	send_r->set_rsn(send_rsn);

	context->lock();
	qspec_object* send_qspec = context->get_context_qspec();
	context->unlock();
	send_r->set_qspec(send_qspec);
	send_r->set_originator(true);
	pdu = dynamic_cast<known_nslp_pdu*>(send_r);
	pdu->set_tear_flag();

	context->lock();
	context->get_old_downstream_sii(old_sii);
	context->unlock();

	SignalingMsg* sigmsg = new SignalingMsg();
	sigmsg->set_msg(pdu);
	sigmsg->set_sig_mri(rcvd_mri);
	sigmsg->set_sid(*rcvd_sid);
	sigmsg->set_downstream(down);
	sigmsg->set_sii_handle(old_sii);
	sigmsg->set_allow_translate(false);
	sigmsg->send_or_delete();
	
	context->lock();
	ntlp::mri_pathcoupled *old_mri = context->get_old_mri();
	context->unlock();
	// XXX: Is this correct? Are the last two checks not exactly the conditions for
	// invalidating the routing state?
	if (!old_mri || old_mri->get_sourceaddress() != rcvd_mri->get_sourceaddress() ||
	    old_mri->get_destaddress() != rcvd_mri->get_destaddress())
		return;
	
	sigmsg = new SignalingMsg();
	sigmsg->set_sig_mri(old_mri);
	sigmsg->set_msg(0);
	sigmsg->set_downstream(true);
	sigmsg->set_invalidate(true);
	sigmsg->send_or_delete();
	
	sigmsg = new SignalingMsg();
	sigmsg->set_sig_mri(old_mri);
	sigmsg->set_msg(0);
	sigmsg->set_downstream(true);
	sigmsg->set_invalidate(true);
	sigmsg->send_or_delete();
}
*/


void state_manager::send_notify_on_the_old_path(const ntlp::sessionid* rcvd_sid, const ntlp::mri_pathcoupled* rcvd_mri, bool down, uint32 old_sii,
		                  info_spec::errorclass_t e_class, info_spec::errorcode_t e_code)
{
	ILog(state_manager::modname, "send_notify_on_the_old_path()");


	notifymsg *send_n = new notifymsg();

	// For simplicity, extract v4-mapped address for v4 case
	in6_addr addr6;
	netaddress *na = addresses->get_first(AddressList::ConfiguredAddr_P,
	                              rcvd_mri->get_destaddress().is_ipv4());
        na->get_ip(addr6);
        delete(na);

        info_spec* send_error = new info_spec(&addr6, e_class, e_code, 0);
        send_n->set_errorobject(send_error);

	known_nslp_pdu *pdu = dynamic_cast<known_nslp_pdu *>(send_n);

	SignalingMsg* sigmsg = new SignalingMsg();
	sigmsg->set_msg(pdu);
	sigmsg->set_sig_mri(rcvd_mri);
	sigmsg->set_sid(*rcvd_sid);
	sigmsg->set_downstream(down);
	sigmsg->set_sii_handle(old_sii);
	sigmsg->set_allow_translate(false);
	sigmsg->send_or_delete();
}


void state_manager::tear_down_other_flow(const ntlp::sessionid* rcvd_sid, const ntlp::mri_pathcoupled* rcvd_mri, bool down,
		NSLP_Session_Context *session_context, NSLP_Flow_Context *flow_context)
{
	ILog(state_manager::modname, "tear_down_other_flow()");

	known_nslp_pdu *pdu;
	SignalingMsg *sigmsg;


	// session_context is already locked!
	NSLP_Session_Context::qn_type_t qn_type = session_context->get_qn_type();

	if (qn_type != NSLP_Session_Context::QNI) {
		// In the first step we send a NOTIFY message to tear down state on all predecessor nodes.
		notifymsg *send_n = new notifymsg();
	
		// For simplicity, extract v4-mapped address for v4 case
		in6_addr addr6;
		netaddress *na = addresses->get_first(AddressList::ConfiguredAddr_P,
		                              rcvd_mri->get_destaddress().is_ipv4());
	        na->get_ip(addr6);
	        delete(na);
	
	        info_spec* send_error = new info_spec(&addr6, info_spec::information, info_spec::TearDownBranch, 0);
	        send_n->set_errorobject(send_error);
	
		pdu = dynamic_cast<known_nslp_pdu *>(send_n);
	
		sigmsg = new SignalingMsg();
		sigmsg->set_msg(pdu);
		sigmsg->set_sig_mri(rcvd_mri);
		sigmsg->set_sid(*rcvd_sid);
		sigmsg->set_downstream(!down);
		sigmsg->send_or_delete();
	}


	// In the second step we send a RESERVE message to tear down state on all successor nodes.
	// This is sometimes redundant, but in case of handovers you cannot know if the predecessor
	// node is yet alive, so to be on the safe side we also tear down state on successor
	// nodes.
	reservereq *send_res = new reservereq();

	// session_context is already locked!
	send_res->set_rsn(session_context->get_own_rsn().copy());

	flow_context->lock();
	qspec_object *send_qspec = new qspec_object(flow_context->get_qspec());
	flow_context->unlock();

	send_res->set_originator(true);
	send_res->set_qspec(send_qspec);

	pdu = dynamic_cast<known_nslp_pdu *>(send_res);
	pdu->set_tear_flag();

	sigmsg = new SignalingMsg();
	sigmsg->set_msg(pdu);
	sigmsg->set_sig_mri(rcvd_mri);
	sigmsg->set_sid(*rcvd_sid);
	sigmsg->set_downstream(down);
	sigmsg->set_invalidate(true);
	sigmsg->send_or_delete();

	ILog(state_manager::modname, "END tear_down_other_flow()");
}


/** This function forwards a RESPONSE message.
 * @param response the RESPONSE message to be forwarded.
 * @param down set to TRUE, if the direction is downstream, otherwise set to FALSE.
 * @param rcvd_sid the session id the current message belongs to.
 * @param rcvd_mri the MRI of the current session.
 */
void state_manager::forward_response_msg(responsemsg* response, const ntlp::sessionid* sid, const ntlp::mri* fwd_mri, bool down) {
	DLog(state_manager::modname, "forward_response_msg() ... I am a QNE");
	uint32 n;
	responsemsg* fwd_response = new responsemsg();
	known_nslp_pdu* pdu = NULL;
	
	
	if(fwd_mri == NULL) {
		ERRLog(state_manager::modname, "forward_response_msg() - fwd_mri == NULL");
		return;
	}


	rsn* r = response->get_rsn();
	if (r) {
		n= r->get();
		rsn* send_rsn = new rsn(n);
		fwd_response->set_rsn(send_rsn);
	}
	
	rii* ri = response->get_rii();
	if (ri) {
		ri->get(n);
		rii* send_rii = new rii(n);
		fwd_response->set_rii(send_rii);
	}
	
	const info_spec* error = response->get_errorobject();
	if (error) {
		fwd_response->set_errorobject(error->copy());
	}

	if (response->get_vlsp_object())
	  fwd_response->set_vlsp_object(response->get_vlsp_object()->copy());

	const session_auth_object* sauthobj= response->get_sessionauth();
	if (sauthobj)
	{
	  fwd_response->set_sessionauth(sauthobj->copy());
	}

	const session_auth_object* sauthobj_hmac_signed= response->get_sessionauth_hmac_signed();
	if (sauthobj_hmac_signed)
	{
	   fwd_response->set_sessionauth_hmac_signed(sauthobj_hmac_signed->copy());
	}
	
	pdu = dynamic_cast<known_nslp_pdu*>(fwd_response);
	
	if(fwd_mri->get_mrm() == mri::mri_t_pathcoupled) {
		const ntlp::mri_pathcoupled* pc_mri = dynamic_cast<const ntlp::mri_pathcoupled *>(fwd_mri);
		assert(pc_mri != NULL);

		SignalingMsg* sigmsg = new SignalingMsg();
		sigmsg->set_msg(pdu);
		sigmsg->set_sig_mri(pc_mri);
		sigmsg->set_sid(*sid);
		sigmsg->set_downstream(down);
		sigmsg->send_or_delete();
	}
	else if(fwd_mri->get_mrm() == mri::mri_t_explicitsigtarget) {
		const ntlp::mri_explicitsigtarget *est_mri = dynamic_cast<const ntlp::mri_explicitsigtarget *>(fwd_mri);
		assert(est_mri != NULL);

		ExplicitSignalingMsg *sigmsg = new ExplicitSignalingMsg();
		sigmsg->set_pdu(pdu);
		sigmsg->set_sig_mri(est_mri);
		sigmsg->set_sid(*sid);
		sigmsg->send_or_delete();
	}
	else {
		ERRLog(state_manager::modname, "forward_response_msg() - Unknown MRI with MRI-ID: " << fwd_mri->get_mrm());
		return;
	}
	
	DLog(state_manager::modname,"END forward_response_msg()");
}


/** This function processes INFO_SPEC for application and sends an internal message to application.
 * @param info_spec received INFO_SPEC.
 * @param context  NSLP context of the current session.
 * @param rcvd_sid the session id the current message belongs to.
 * @param vlspobj  optionally present vlsp object  
 */
void state_manager::process_info_spec_for_appl(const ntlp::sessionid* rcvd_sid, const info_spec* info_spec, const vlsp_object* vlspobj)
{
	DLog(state_manager::modname, "process_info_spec_for_appl()");

	if (!info_spec)
		return;

	info_spec::errorclass_t er_class = (info_spec::errorclass_t) info_spec->get_errorclass();
	switch (er_class) {
	case info_spec::information:
		DLog(state_manager::modname, "InfoSpec Error Class: Informational");
		break;
	case info_spec::success: 
		DLog(state_manager::modname, "InfoSpec Error Class: Success");
		break;
	case info_spec::protocol:
		DLog(state_manager::modname, "InfoSpec Error Class: Protocol Error");
		break;
	case info_spec::transient:
		DLog(state_manager::modname, "InfoSpec Error Class: Transient Failure");
		break;
	case info_spec::permanent:
		DLog(state_manager::modname, "InfoSpec Error Class: Permanent Failure");
		break;
	case info_spec::qos_model_error:
		DLog(state_manager::modname, "InfoSpec Error Class: QoS Model Error");
		break;
	default:
		DLog(state_manager::modname, "InfoSpec Error Class: " << er_class);
	}

	QoS_Appl_Msg* applmsg = new QoS_Appl_Msg();
	applmsg->set_pdu_type(known_nslp_pdu::RESPONSE);
	applmsg->set_sid(*rcvd_sid);
	applmsg->set_info_spec(info_spec);
	applmsg->set_source(message::qaddr_appl_qos_signaling);
	applmsg->set_vlsp_object(vlspobj);
	applmsg->send_or_delete();
}


/** This function sends an internal message to application.
 * @param info the internal info type.
 * @param rcvd_sid the session id the current message belongs to.
 */
void state_manager::notify_application(QoS_Appl_Msg::qos_info_type_t info, const ntlp::sessionid* rcvd_sid)
{
	DLog(state_manager::modname, "notify_application()");

	QoS_Appl_Msg* applmsg = new QoS_Appl_Msg();

	applmsg->set_info_type(info);
	applmsg->set_sid(*rcvd_sid);
	applmsg->set_source(message::qaddr_appl_qos_signaling);
	applmsg->send_or_delete();
}

/** This function processes SII handle either with a message or from NetworkNotification.
 * @param sii_hanlde the received SII handle.
 * @param down set to TRUE, if the SII handle is for the downstream peer.
 * @param my_sid the session id the current message belongs to.
 * @param rcvd_mri the MRI of the current session.
 */
void state_manager::process_sii_handle(const ntlp::sessionid *rcvd_sid, const ntlp::mri_pathcoupled *rcvd_mri, bool down, uint32 sii_handle, APIMsg::error_t status)
{
	DLog(state_manager::modname, "process_sii_handle()");

	NSLP_Session_Context *session_context = rcvd_sid ? session_context_map->find_session_context(*rcvd_sid) : NULL;
	if (session_context != NULL) {
		ILog(state_manager::modname, "process_sii_handle() - Found NSLP_Session_Context with SID [" << rcvd_sid->to_string() << "]");
	
		bool is_sender_initiated;
		session_context->lock();
		is_sender_initiated = session_context->get_sender_initiated();
		session_context->unlock();
	
		ILog(state_manager::modname, "process_sii_handle() - The reservation for the NSLP_Session_Context with SID [" << rcvd_sid->to_string() << "] is "
				<< (is_sender_initiated ? "sender initiated." : "receiver initiated."));
	
		switch (status) {
		case APIMsg::error_msg_delivered:
		case APIMsg::route_changed_status_good:
		{
			if (is_sender_initiated) {
				// sender initiated reservation

				if (down) {
					session_context->lock();
					session_context->set_successor_sii(*rcvd_mri, sii_handle);
					session_context->unlock();
	
					ILog(state_manager::modname, "process_sii_handle() - Set successor SII for Flow ("
							<< rcvd_mri->get_sourceaddress() << ", " << rcvd_mri->get_destaddress()
							<< ") to: " << sii_handle);
				}
				else {
					session_context->lock();
					session_context->set_predecessor_sii(*rcvd_mri, sii_handle);
					session_context->unlock();
	
					ILog(state_manager::modname, "process_sii_handle() - Set predecessor SII for Flow ("
							<< rcvd_mri->get_sourceaddress() << ", " << rcvd_mri->get_destaddress()
							<< ") to: " << sii_handle);
				}
			}
			else {
				// receiver initiated reservation

				if (down) {
					session_context->lock();
					session_context->set_predecessor_sii(*rcvd_mri, sii_handle);
					session_context->unlock();
	
					ILog(state_manager::modname, "process_sii_handle() - Set predecessor SII for Flow ("
							<< rcvd_mri->get_sourceaddress() << ", " << rcvd_mri->get_destaddress()
							<< ") to: " << sii_handle);
				}
				else {
					session_context->lock();
					session_context->set_successor_sii(*rcvd_mri, sii_handle);
					session_context->unlock();
	
					ILog(state_manager::modname, "process_sii_handle() - Set successor SII for Flow ("
							<< rcvd_mri->get_sourceaddress() << ", " << rcvd_mri->get_destaddress()
							<< ") to: " << sii_handle);
				}
			}
	
			break;
		}
		case APIMsg::route_changed_status_bad:
		{
			if (is_sender_initiated) {
				// sender initiated reservation

				if (down) {
					session_context->lock();
					session_context->remove_successor_sii(*rcvd_mri);
					session_context->unlock();
	
					ILog(state_manager::modname, "process_sii_handle() - Remove successor SII for Flow ("
							<< rcvd_mri->get_sourceaddress() << ", " << rcvd_mri->get_destaddress() << ")");
				}
				else {
					session_context->lock();
					session_context->remove_predecessor_sii(*rcvd_mri);
					session_context->unlock();
	
					ILog(state_manager::modname, "process_sii_handle() - Remove predecessor SII for Flow ("
							<< rcvd_mri->get_sourceaddress() << ", " << rcvd_mri->get_destaddress() << ")");
				}
			}
			else {
				// receiver initiated reservation

				if (down) {
					session_context->lock();
					session_context->remove_predecessor_sii(*rcvd_mri);
					session_context->unlock();
	
					ILog(state_manager::modname, "process_sii_handle() - Remove predecessor SII for Flow ("
							<< rcvd_mri->get_sourceaddress() << ", " << rcvd_mri->get_destaddress() << ")");
				}
				else {
					session_context->lock();
					session_context->remove_successor_sii(*rcvd_mri);
					session_context->unlock();
	
					ILog(state_manager::modname, "process_sii_handle() - Remove successor SII for Flow ("
							<< rcvd_mri->get_sourceaddress() << ", " << rcvd_mri->get_destaddress() << ")");
				}
			}




			bool sii_matches = false;

			session_context->lock();
			if ((is_sender_initiated && down) || ((!is_sender_initiated) && (!down))) {
				if (session_context->is_predecessor_sii_set_for_flow(*rcvd_mri)) {
					if (session_context->matches_given_sii_with_saved_predecessor_sii(*rcvd_mri, sii_handle)) {
						sii_matches = true;
					}
				}
				else {
					session_context->unlock();
					return;
				}
			}
			else if (((!is_sender_initiated) && down) || (is_sender_initiated && (!down))) {
				if (session_context->is_successor_sii_set_for_flow(*rcvd_mri)) {
					if (session_context->matches_given_sii_with_saved_successor_sii(*rcvd_mri, sii_handle)) {
						sii_matches = true;
					}
				}
				else {
					session_context->unlock();
					return;
				}
			}
			session_context->unlock();


			if (sii_matches) {
				// If the upstream peer goes away and we are in sender-initiated mode
				if (is_sender_initiated && (!down)) {
					session_context->lock();
					if (session_context->get_is_maybe_dead_end()) {
						session_context->set_is_maybe_dead_end(false);
					}
					else {
						// tear down NSLP_Flow_Context
						NSLP_Flow_Context *flow_context = session_context->find_flow_context(*rcvd_mri);
						if (flow_context != NULL) {
							delete_flow_context(rcvd_sid, rcvd_mri, session_context, flow_context);
						}
						session_context->unlock();

						return;
					}
					session_context->unlock();

					ILog(state_manager::modname, "Current upstream peer is no longer available sending RESERVE(T)");

					session_context->lock();
					NSLP_Flow_Context *flow_context = session_context->find_flow_context(*rcvd_mri);
					if (flow_context != NULL) {
						// Send RESERVE(T)
						uint128 sid_number;

						known_nslp_pdu* pdu = NULL;
						reservereq* send_r = new reservereq();
						session_context->lock();
						send_r->set_rsn(session_context->get_own_rsn().copy());
						session_context->unlock();

						
						flow_context->lock();
						qspec_object *send_qspec = flow_context->get_qspec().copy();
						flow_context->unlock();
						send_r->set_qspec(send_qspec);
						send_r->set_originator(true);
						pdu = dynamic_cast<known_nslp_pdu*>(send_r);
						pdu->set_tear_flag();
						
						SignalingMsg* sigmsg = new SignalingMsg();
						sigmsg->set_msg(pdu);
						sigmsg->set_sig_mri(rcvd_mri);
						sigmsg->set_sid(*rcvd_sid);
						sigmsg->set_downstream(true);
						
						sigmsg->send_or_delete();
					}
					session_context->unlock();
				}

				// tear down NSLP_Flow_Context
				session_context->lock();
				NSLP_Flow_Context *flow_context = session_context->find_flow_context(*rcvd_mri);
				if (flow_context != NULL) {
					delete_flow_context(rcvd_sid, rcvd_mri, session_context, flow_context);
				}
				session_context->unlock();
			}

			break;
		}
		default:
			break;
		}
	}

	DLog(state_manager::modname, "END process_sii_handle()");
}

#ifdef USE_FLOWINFO
void
state_manager::process_mobility(ntlp::mri_pathcoupled *rcvd_mri, ntlp::APIMsg::error_t status)
{
	NSLP_Session_Context_Map::search_context sc;
	NSLP_Session_Context *session_context;
	ntlp::Flowstatus *fs;

	// Lookup the translation for this
	fs = fi_service->get_flowinfo(*rcvd_mri);

	DLog("MOBILITY-DBG 1", "Flowstatus:" << fs->type << endl << fs->orig_mri << *fs->new_mri << endl);

	for (session_context = session_context_map->find_first(*rcvd_mri, sc); session_context != NULL;
	    session_context = session_context_map->find_next(sc)) {
		session_context->lock();
		NSLP_Flow_Context *flow_context = session_context->find_flow_context(*rcvd_mri);
		session_context->unlock();

		if (flow_context == NULL) {
			continue;
		}

		send_mobility_refresh(session_context, flow_context, fs);
	}
}
#endif

/**
 * Check context and flowstatus and send a QUERY/RESERVE if applicable
 * @param context the affected context
 * @param fs the new flowstatus
 */
void
//state_manager::send_mobility_refresh(NSLP_Context *context, ntlp::Flowstatus *fs)
state_manager::send_mobility_refresh(NSLP_Session_Context *session_context, NSLP_Flow_Context *flow_context, ntlp::Flowstatus *fs) {
	NSLP_Session_Context::qn_type_t qn_type;
	ntlp::mri_pathcoupled *my_mri;
	uint128 my_sid;
	qspec_object *my_qspec;
	bool down;
	known_nslp_pdu *pdu;
	rii* send_new_rii;
	bool replace = false;

	flow_context->lock();
	const ntlp::mri_pathcoupled *lmri = flow_context->get_logical_mri();
	flow_context->unlock();

	DLog("MOBILITY-DBG 2", "For " << *lmri << "Flowstatus:" << fs->type << endl << fs->orig_mri << *fs->new_mri << endl);

	switch (fs->type) {
	case ntlp::Flowstatus::fs_tunnel:
	{
		// XXX: Revisit!
		ILog("MOBILITY", "TUNNEL");

		session_context->lock();
		if (!session_context->get_bound_sid(my_sid)) {
			id_t check_id;

			session_context->get_sid(my_sid);

			flow_context->lock();
			my_mri = flow_context->get_mri().copy();
			flow_context->get_timer_id_for_tunnel(check_id);
			flow_context->unlock();

			if (check_id == 0) {
				// Start timer for tunnel reservation
				TimerMsg *tmsg = new TimerMsg(message::qaddr_qos_nslp_timerprocessing);

				id_t check_id = tmsg->get_id();
				flow_context->lock();
				flow_context->set_timer_id_for_tunnel(check_id);
				flow_context->unlock();

				notifymsg* t_notify = new notifymsg();
				ntlp::sessionid* rcvd_sid = new sessionid(my_sid);
				tmsg->start_relative(10, 0, rcvd_sid, my_mri, t_notify);
				std::cout << "QoS Statemodule RELATIVE 9 : " << "10" << std::endl;
				if (tmsg->send_to(message::qaddr_timer)) {
					ILog(state_manager::modname, "send_mobility_refresh, Timer for Tunnel-RESERVE " << check_id << " set to " << 10); 
				}
				tmsg = NULL;

				session_context->unlock();

				return;
			}
		}
		session_context->unlock();

//		// Lookup the context for the bound session
//		sessionid sid(my_sid);
//		context = contextmap.find(sid);
//
//		if (!context)
			return;
		// FALLTHROUGH
	}
	case ntlp::Flowstatus::fs_normal:
	case ntlp::Flowstatus::fs_nothing:
	{
		// Lock context for update and state retrival
		session_context->lock();
		qn_type = session_context->get_qn_type();
		down = session_context->get_sender_initiated();
		session_context->unlock();

		if (qn_type == NSLP_Session_Context::QNE) { 
			ILog("MOBILITY", "I'm QNE - nothing I can do");
			// XXX: This should never happen
			return;
		}
		if (qn_type == NSLP_Session_Context::QNR && down) {
			ILog("MOBILITY", "I'm QNR - nothing I can do (for now)");
			return;
		}
		if (qn_type == NSLP_Session_Context::QNR && !down) {
			ILog("MOBILITY", "I'm QNR - sending QUERY ... maybe");

			/*
			// Get a new lifetime
			uint32 new_refr_p, new_lifetime;
			rp *refr_period = new rp(qosnslpconf.getpar<uint32>(qosnslpconf_refresh_period));
			refr_period->get_rand_wait(qosnslpconf.getpar<uint32>(qosnslpconf_refresh_period), new_refr_p);
			refr_period->get_rand_lifetime(qosnslpconf.getpar<uint32>(qosnslpconf_refresh_period), new_lifetime);
			delete refr_period;
			*/

			flow_context->lock();
			// Update the MRI
			my_mri = flow_context->get_mri().copy();
			if (my_mri->get_sourceaddress() != fs->new_mri->get_sourceaddress()) {
				my_mri->set_sourceaddress(fs->new_mri->get_sourceaddress());
				replace = true;
			}
			if (my_mri->get_destaddress() != fs->new_mri->get_destaddress()) {
				my_mri->set_destaddress(fs->new_mri->get_destaddress());
				replace = true;
			}
			if (!replace) {
				ILog("MOBILITY", "mri unchanged");
				flow_context->unlock();
				return;
			}
			flow_context->unlock();

			// With the separation of the NSLP_Session_Context and NSLP_Flow_Context
			// the reservation of a flow is always existing iff a NSLP_Flow_Context
			// exists. Thus, I think this isn't needed anymore.
			/*
			// REFRESH TIMER
			timer_t t_lifetime = time(NULL) + new_lifetime;
			context->set_time_to_live(t_lifetime);
			if (!context->get_reserved()) {
				id_t query_timer_id;
				context->get_timer_id_for_reserve_or_query(query_timer_id);
				context->unlock();			

				TimerMsg* tmsg = new TimerMsg(message::qaddr_qos_nslp_timerprocessing);
				bool stop;
				stop = tmsg->stop(query_timer_id);
				tmsg->send_to(message::qaddr_timer);
				if (stop) { Log(INFO_LOG,LOG_NORMAL,state_manager::modname,"RESERVE timer " << query_timer_id << " stopped!"); }
			}

			context->set_mri(my_mri);
			context->get_s_id(my_sid);
			context->set_reserved(false);
			my_qspec = context->get_context_qspec();
			context->unlock();
			*/

			session_context->lock();
			session_context->get_sid(my_sid);
			session_context->unlock();

			flow_context->lock();
			my_qspec = flow_context->get_qspec().copy();
			flow_context->lock();


			querymsg *query = new querymsg();
			packet_classifier* new_pc = new packet_classifier();
			new_pc->set_spi_flag();
			query->set_packet_classifier(new_pc);
			query->set_originator(true);
			query->set_qspec(my_qspec);
			query->set_reserve_init_flag();
		
			pdu = dynamic_cast<known_nslp_pdu*>(query);
			SignalingMsg* sigmsg = new SignalingMsg();
			sigmsg->set_msg(pdu);
			sigmsg->set_sig_mri(my_mri);
			sigmsg->set_downstream(!down);
			sigmsg->set_sid(my_sid);
			sigmsg->send_or_delete();

			return;
		}
		if (!down) {
			ILog("MOBILITY", "This might be wrong, but I'll just sit on my hands for the moment");
			return;
		}

		session_context->lock();
		if (session_context->get_bound_sid(my_sid)) {
			session_context->unset_bound_sid();
			session_context->unlock();

			// Tear down bound session
			ntlp::sessionid sid(my_sid);
			NSLP_Session_Context *bound_session_context = session_context_map->find_session_context(sid);
			if (bound_session_context != NULL) {
				bound_session_context->lock();
				NSLP_Session_Context::flowcontext_const_it_t it;
				it = bound_session_context->flowcontext.begin();
				while (it != bound_session_context->flowcontext.end()) {
					const ntlp::mri_pathcoupled flow = it->first;
					NSLP_Flow_Context *flow_context = it->second;
				
					// Since the following code deletes the current (key, value) pair,
					// we jump to the next pair in advance.
					++it;
				
					delete_flow_context(&sid, &flow, bound_session_context, flow_context);
				}
				bound_session_context->unlock();

				session_context_map->erase(sid);
			}
		} else {
			session_context->unlock();
		}

		ILog("MOBILITY", "I'm QNI - sending RESERVE ... maybe");
		// We are QNI and this is a sender-initiated session
		// Let's send a RESERVE

		/*
		// Get a new lifetime
		uint32 new_refr_p, new_lifetime;
		rp *refr_period = new rp(qosnslpconf.getpar<uint32>(qosnslpconf_refresh_period));
		refr_period->get_rand_wait(qosnslpconf.getpar<uint32>(qosnslpconf_refresh_period), new_refr_p);
		refr_period->get_rand_lifetime(qosnslpconf.getpar<uint32>(qosnslpconf_refresh_period), new_lifetime);
		delete refr_period;
		*/

		flow_context->lock();
		// Update the MRI
		my_mri = flow_context->get_mri().copy();
		if (my_mri->get_sourceaddress() != fs->new_mri->get_sourceaddress()) {
			my_mri->set_sourceaddress(fs->new_mri->get_sourceaddress());
			replace = true;
		}
		if (my_mri->get_destaddress() != fs->new_mri->get_destaddress()) {
			my_mri->set_destaddress(fs->new_mri->get_destaddress());
			replace = true;
		}
		flow_context->unlock();

		if (!replace) {
			ILog("MOBILITY", "mri unchanged");
			return;
		}

		// With the separation of the NSLP_Session_Context and NSLP_Flow_Context
		// the reservation of a flow is always existing iff a NSLP_Flow_Context
		// exists. Thus, I think this isn't needed anymore.
		/*
		// REFRESH TIMER
		timer_t t_lifetime = time(NULL) + new_lifetime;
		context->set_time_to_live(t_lifetime);
		if (!context->get_reserved()) {
			id_t query_timer_id;
			context->get_timer_id_for_reserve_or_query(query_timer_id);
			context->unlock();			

			TimerMsg* tmsg = new TimerMsg(message::qaddr_qos_nslp_timerprocessing);
			bool stop;
			stop = tmsg->stop(query_timer_id);
			tmsg->send_to(message::qaddr_timer);
			if (stop) { Log(INFO_LOG,LOG_NORMAL,state_manager::modname,"RESERVE timer " << query_timer_id << " stopped!"); }
		}
		*/

		session_context->lock();
		session_context->get_sid(my_sid);
		// Set RSN = RSN+2
		session_context->increment_own_rsn();
		session_context->increment_own_rsn();
		rsn my_rsn(session_context->get_own_rsn());
		session_context->unlock();

		flow_context->lock();
		my_qspec = flow_context->get_qspec().copy();
		flow_context->unlock();

		reservereq *res = new reservereq();
		res->set_rsn(my_rsn.copy());
		res->set_originator(true);
		send_new_rii = new rii();
		send_new_rii->generate_new();
		send_new_rii->set_own(true);
		send_new_rii->set_is_reserve(true);
		res->set_rii(send_new_rii);
		res->set_qspec(my_qspec);
		res->set_replace_flag();
		
		pdu = dynamic_cast<known_nslp_pdu*>(res);
		SignalingMsg* sigmsg = new SignalingMsg();
		sigmsg->set_msg(pdu);
		sigmsg->set_sig_mri(my_mri);
		sigmsg->set_downstream(down);
		sigmsg->set_sid(my_sid);
		sigmsg->send_or_delete();

	}
	default:
		return;
	}
}

#ifdef USE_FLOWINFO
void
state_manager::create_tunnel_reserve(NSLP_Session_Context *session_context, NSLP_Flow_Context *flow_context) {
	NSLP_Session_Context::qn_type_t qn_type;
	uint128 my_sid;
	bool down;
	ntlp::Flowstatus *fs= NULL;
	ntlp::mri_pathcoupled *lmri;

	if ((!session_context) || (!flow_context)) {
		return;
	}

	flow_context->lock();
	lmri = flow_context->get_logical_mri()->copy();
	flow_context->unlock();

	if (!lmri) {
		return;
	}

	// Lookup the translation for this
	fs = fi_service->get_flowinfo(*lmri);

	if (fs->type != ntlp::Flowstatus::fs_tunnel) {
		flow_context->lock();
		flow_context->set_timer_id_for_tunnel(0);
		flow_context->unlock();
		return;
	}

	flow_context->lock();
	flow_context->set_timer_id_for_tunnel(0);
	flow_context->unlock();

	session_context->lock();
	if (!session_context->get_bound_sid(my_sid)) {
		qn_type = session_context->get_qn_type();
		down = session_context->get_sender_initiated();
		session_context->unlock();

		if (qn_type == NSLP_Session_Context::QNI && down) {
			DLog("MOBILITY-DBG 2", "I'm QNI, we are in tunnel mode and there is no tunnel reservation, creating one");
			uint128 tsid;

		  	reservereq* tunnel_res = new reservereq();

			packet_classifier* new_pc = new packet_classifier();
			new_pc->set_spi_flag();
			tunnel_res->set_packet_classifier(new_pc);

			rsn* new_rsn = new rsn();
			tunnel_res->set_rsn(new_rsn);

			rii* new_rii = new rii();
			new_rii->generate_new();
			new_rii->set_own(true);
			tunnel_res->set_rii(new_rii);

			flow_context->lock();
			qos_nslp::qspec_object* new_qspec = flow_context->get_qspec().copy();
			flow_context->unlock();
			tunnel_res->set_qspec(new_qspec);

			tunnel_res->set_originator(true);

			known_nslp_pdu *pdu = dynamic_cast<known_nslp_pdu*>(tunnel_res);
			SignalingMsg* sigmsg = new SignalingMsg();
			sigmsg->set_msg(pdu);

			ntlp::mri_pathcoupled *logical_tmri = fs->new_mri->copy();
			logical_tmri->set_sourceaddress(fs->orig_mri.get_sourceaddress());
			sigmsg->set_sig_mri(logical_tmri);

			sigmsg->set_downstream(true);

			ntlp::sessionid* sid = new sessionid();
			sid->generate_random();
			tsid = *sid;

			session_context->lock();
			session_context->set_bound_sid(tsid);
			session_context->unlock();
		
			sigmsg->set_sid(tsid);
			sigmsg->send_or_delete();
		}
		if (qn_type == NSLP_Session_Context::QNR && !down) {
		        DLog("MOBILITY-DBG 2", "I'm QNR, we are in tunnel mode and there is no tunnel reservation, creating one");
			uint128 tsid;

		      	querymsg* tunnel_query = new querymsg();
		      	tunnel_query->set_reserve_init_flag();

			packet_classifier* new_pc = new packet_classifier();
			new_pc->set_spi_flag();
			tunnel_query->set_packet_classifier(new_pc);

			flow_context->lock();
			qos_nslp::qspec_object* new_qspec = flow_context->get_qspec().copy();
			flow_context->unlock();
			tunnel_query->set_qspec(new_qspec);

			tunnel_query->set_originator(true);
			known_nslp_pdu *pdu = dynamic_cast<known_nslp_pdu*>(tunnel_query);
			SignalingMsg* sigmsg = new SignalingMsg();
			sigmsg->set_msg(pdu);

			ntlp::mri_pathcoupled *logical_tmri = fs->new_mri->copy();
			logical_tmri->set_sourceaddress(fs->orig_mri.get_sourceaddress());
			sigmsg->set_sig_mri(logical_tmri);

			sigmsg->set_downstream(!down);

			ntlp::sessionid* sid = new sessionid();
			sid->generate_random();
			tsid = *sid;

			session_context->lock();
			session_context->set_bound_sid(tsid);
			session_context->unlock();

			sigmsg->set_sid(tsid);
			sigmsg->send_or_delete();
		}

		return;
	}
	else {
		session_context->unlock();
	}
}
#endif



/* This function creates a QUERY message from a given NSLP_Context.
 * @param context the NSLP Context.
 * @param query the QUERY message.
 */
// TODO: which QSPEC to choose, after modifying context to allow multiple concurrent flows within a session
/*
void state_manager::create_query_message_from_context(NSLP_Context *context, querymsg *query) {
	uint128 n128;
	bool found;

	bound_sessionid *send_b_sid = NULL;
	qspec_object* send_qspec = NULL;

	assert(context != NULL);
	assert(query != NULL);

	// TODO:
	// * only lock once

	// Maybe useless ??
	// QN-Type
	context->lock();
	NSLP_Context::qn_type_t type = context->get_qn_type();
	context->unlock();
	if (type == NSLP_Context::QNI) {
		query->set_originator(true);
	}

	// Bound Session
	context->lock();
	found = context->get_bound_s_id(n128);
	context->unlock();
	if(found) {
		send_b_sid = new bound_sessionid(n128);
		query->set_bound_sid(send_b_sid);
	}

	// QSPEC
	context->lock();
	send_qspec = context->get_context_qspec();
	if(send_qspec != NULL) {
		send_qspec = send_qspec->copy();
	}
	context->unlock();
	query->set_qspec(send_qspec);
}
*/


/**
 * generates actual network pdu from first parameter
 * @param pdu from this NSLP PDU a network message will be generated.
 * @return error code or error_ok on successful encoding
 * @return netmsg pointer to netmsg buffer (which is allocated by this method)
 */
state_manager::error_t
state_manager::generate_pdu(const known_nslp_pdu& pdu, NetMsg*& netmsg)
{
	error_t nslpres = error_ok;
	try {
		// get expected size of pdu
		uint32 pdusize = pdu.get_serialized_size(coding);
		DLog(state_manager::modname, "pdusize after get_serialized_size() is -  " << pdusize);
		if (pdusize>NetMsg::max_size) {
			ERRCLog(state_manager::modname, "state_manager::generate_pdu()."
			<< " Cannot allocate " << pdusize << "bytes NetMsg."
			<< " PDU too big for NetMsg maxsize " << NetMsg::max_size);
			nslpres = error_pdu_too_big;
		}
		else {
			// allocate a netmsg with the corresponding buffer size
			// TODO: make sure netmsg is deleted if no error occured
			netmsg = NULL;
			netmsg = new(nothrow) NetMsg(pdusize);
			if (netmsg) {
				DLog(state_manager::modname, "starting to serialize");

				// write actual pdu octets into netmsg
				uint32 nbytes;
				pdu.serialize(*netmsg, coding, nbytes);
				DLog(state_manager::modname, "PDU serialized.");
				if (nbytes!=pdusize) {
					ERRCLog(state_manager::modname, "1. state_manager::generate_pdu() unspecified serialization error");
					nslpres = error_serialize;
				} // end if serialization error
				if (netmsg->get_bytes_left()) {
					ERRCLog(state_manager::modname, "2. state_manager::generate_pdu() unspecified serialization error");
					nslpres = error_serialize;
				} // end if serialization error
			}
			else {
				ERRCLog(state_manager::modname, "state_manager::generate_pdu() cannot allocate " << pdusize << " bytes NetMsg");
				nslpres = error_no_mem;
			} // end if netmsg
		} // end if too big
	}
	catch(ProtLibException& e) {
		ERRCLog(state_manager::modname, "state_manager::generate_pdu() serialization error: " << e.getstr());
		nslpres = error_serialize;
	} // end try-catch

	// delete and cleanup netmsg if error occured
	if ((nslpres != error_ok) && netmsg) {
		delete netmsg;
		netmsg = NULL;
	} // end if delete netmsg

	return nslpres;
} // end generate_pdu;


} // end namespace qos_nslp


