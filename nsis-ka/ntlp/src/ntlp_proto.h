/// ----------------------------------------*- mode: C++; -*--
/// ntlp_proto.h
/// GIST packets preprocessor
/// ----------------------------------------------------------
/// $Id: ntlp_proto.h 5249 2010-04-26 08:40:59Z roehricht $
/// $HeadURL: https://svn.tm.kit.edu/nsis/ntlp/trunk/src/ntlp_proto.h $
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

/** @ingroup ntlpproto 
 * @file ntlp_proto.h
 * NTLP packets preprocessor
 */

#ifndef _NTLP__NTLPPROTO_H_
#define _NTLP__NTLPPROTO_H_

#include "protlib_types.h"
#include "ntlp_pdu.h"
//#include "authentication.h"
#include "network_message.h"
#include "threadsafe_db.h"
#include "logfile.h"

#include "addresslist.h"

#ifndef NSIS_OMNETPP_SIM
  // don't want this in the OMNet++ simulation
  #include <pthread.h>
#endif

namespace ntlp {
    using namespace protlib;
    using namespace protlib::log;

class NTLP {
public:
	/// GIST protocol errors (generated here, read in Signaling)
	enum error_t {
		error_ok,
		error_ok_in_auth,
		error_ok_out_auth,
		error_ok_forward,
		error_internal,
		error_wrong_pdu_type,
		error_no_request_found,
		error_no_mem,
		error_serialize,
		error_pdu_too_big,
		error_reference_in_use,
		error_send_error_pdu,
		error_log_incpduerror_only,
		error_ref_not_found,

		// GIST errorcodes

		error_gist_unknown_version,
		error_gist_unknown_type,
		error_gist_inconsistent_r_flag,
		error_gist_incorrect_msg_length,
		error_gist_hoplimit_exceeded,
		error_gist_incorrect_encapsulation,
		error_gist_incorrectly_delivered,
		error_gist_no_routing_state,
		error_gist_unknown_nslpid,
		error_gist_endpoint_found,
		error_gist_message_too_large,
		error_gist_duplicate_object,
		error_gist_unrecognised_object,
		error_gist_missing_object,
		error_gist_invalid_object,
		error_gist_untranslated_object,
		error_gist_incorrect_object_length,
		error_gist_value_not_supported,
		error_gist_invalid_flag_field_combination,
		error_gist_empty_list,
		error_gist_invalid_cookie,
		error_gist_sp_scd_mismatch,
		error_gist_MRI_ip_version_mismatch
	}; // end error_t

    static const char *const errstr[];
    static const char *const default_host_signaling_transport_protocol;
    
    

  /// constructor
  NTLP(AddressList &addresses, uint16 wkp, IE::coding_t c = IE::protocol_v1, bool acct_unauth = true);
  ~NTLP();
  error_t process_tp_recv_msg(NetMsg& msg, appladdress& addr,
			      gp_id_t& resid, known_ntlp_pdu*& respdu,
			      gp_id_t& refid, known_ntlp_pdu*& encappdu
			      ); // end process_tp_recv_msg
  error_t process_response(gp_id_t reqid, known_ntlp_pdu& rsppdu, gp_id_t encapid,
			   NetMsg*& msg, appladdress& addr);
  error_t process_outgoing(known_ntlp_pdu& rsppdu, const appladdress& addr,
			   gp_id_t resid, gp_id_t refid, NetMsg*& msg);
  bool revoke(gp_id_t reqid);

  /// checks whether address belongs to pool of own addresses
  bool isown_address(const hostaddress* ha) const;

private:
    static const char *const modname;
    
    bool set_orig_msgref(known_ntlp_pdu& pdu);
    
    AddressList &addresses;

    error_t generate_errorpdu(NetMsg& msg, 
			      IEErrorList& errorlist, known_ntlp_pdu* pdu, known_ntlp_pdu*& respdu, appladdress& peer);
    error_t generate_pdu(const known_ntlp_pdu& pdu, NetMsg*& msg, uint32 magic_number= 0); 
    known_ntlp_pdu* process_encap_msg(const known_ntlp_pdu& pdu, const appladdress& peer,
				     gp_id_t& refid);
    bool get_ref_id(known_ntlp_pdu*& pdu, const appladdress& peer, gp_id_t& refid);
    bool fill_in_msg_ref(known_ntlp_pdu& pdu, gp_id_t refid) const;
    void fill_in_encap_pdu(known_ntlp_pdu& pdu, gp_id_t encapid) const;
    /// signaling data table for remote messages
    //SignalingData sdtable;
    /// coding sheme to use
    IE::coding_t coding;
    /// accept unauthenticated messages
    const bool accept_unauthenticated;
    ///the well-known-GIST-port
    uint16 wkp;

}; // end class NTLP
    


/** inline methods **/
inline
bool 
NTLP::set_orig_msgref(known_ntlp_pdu& pdu)
{
  // if msgreference not present, create a new one

    return true;

}

//@}

} // end namespace ntlp

#endif // _NTLP__NTLPPROTO_H_

