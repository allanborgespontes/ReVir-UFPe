/// ----------------------------------------*- mode: C++; -*--
/// @file reservemsg.cpp
/// QoS NSLP Reserve Message
/// ----------------------------------------------------------
/// $Id: reservemsg.cpp 6260 2011-06-14 13:47:24Z bless $
/// $HeadURL: https://svn.tm.kit.edu/nsis/qos-nslp/trunk/src/reservemsg.cpp $
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

#include "reservemsg.h"
#include "logfile.h"

#include <iomanip>

using namespace protlib;
using namespace protlib::log;

namespace qos_nslp {

/** @addtogroup iereservemsg ReserveReq
 * @ingroup ienslppdu
 * @{
 */

/***** class reservereq *****/

const char* const reservereq::iename = "reservereq";

const char* reservereq::get_ie_name() const { return iename; }

/***** inherited from IE *****/

/** Generate new ReserveMsg instance.
  */
reservereq* reservereq::new_instance() const {
	reservereq* n = NULL;
	catch_bad_alloc(n = new reservereq());
	return n;
} // end new_instance

/** Copy function for ReserveMsg.
  */
reservereq* reservereq::copy() const {
	reservereq* n = NULL;
	catch_bad_alloc(n =  new reservereq(*this));
	return n;
} // end copy

/** Function to compare two objects of type ReserveMsg.
  */
bool reservereq::operator==(const IE& ie) const {
	const reservereq* n = dynamic_cast<const reservereq*>(&ie);
	if (n && known_nslp_pdu::operator==(*n)) {
		return compare_objects(get_rsn(),n->get_rsn());
	} else return false;
} // end operator==

/** Function to print nicely the ReserveMsg on the monitor(or file).
  */
ostream& reservereq::print(ostream& os, uint32 level, const uint32 indent, const char* name) const {
	bool objects = false;
	os<<setw(level*indent)<<"";
	Log(INFO_LOG,LOG_NORMAL, "RESERVE", "<"<<get_ie_name()<<">:");
	level++;
	known_nslp_object *tmp;
	if ((tmp = get_rsn())) {
		tmp->print(os, level, indent, "rsn");
		objects = true;
	}
	if ((tmp = get_rii())) {
		tmp->print(os, level, indent, "rii");
		objects = true;
	}
	if ((tmp = get_rp())) {
		tmp->print(os, level, indent, "refresh period");
		objects = true;
	}
	// TODO: [ *BOUND_SESSION_ID ]
	if ((tmp = get_bound_sid())) {
		tmp->print(os, level, indent, "bound session id");
		objects = true;
	}
	// TODO: [ SESSION_ID_LIST [ RSN_LIST ] ]
	if ((tmp = get_packet_classifier())) {
		tmp->print(os, level, indent, "packet_classifier");
		objects = true;
	}
	if ((tmp = get_qspec())) {
		tmp->print(os, level, indent, "qspec");
		objects = true;
	}
	if ((tmp = get_sessionauth())) { 
	    tmp->print(os, level, indent, "session_auth");
		objects = true;
 	}
	if ((tmp = get_sessionauth_hmac_signed())) { 
	    tmp->print(os, level, indent, "session_auth_hmac_signed");
		objects = true;
 	}
	if (!objects) {
	    Log(INFO_LOG,LOG_NORMAL, "RESERVE", " <empty>");
	}
	return os;
} // end print

/** Function for manual input of ReserveMsg.
  */
istream& reservereq::input(istream& is, bool istty, uint32 level, const uint32 indent, const char* name) {
	nslp_object* tmp;
	string s;
	uint128 i;
	string c;
	catch_bad_alloc(
	if (istty) {
		cout<<setw(level*indent)<<"";
		if (name && (*name!=0)) cout<<name<<" ";
		cout<<"<"<<get_ie_name()<<">:"<<endl;
	} // end if istty
	level++;

	(tmp = new rsn)->input(is,istty,level,indent,"");
	set_rsn(dynamic_cast<rsn*>(tmp));

	if (istty) cout<<setw(level*indent)<<""<<"packet_classifier (y/n): ";
	is>>s;
	if (s=="y") {
		(tmp = new packet_classifier)->input(is,istty,level,indent);
		set_packet_classifier(dynamic_cast<packet_classifier*>(tmp));
	}
	
	if (istty) cout<<setw(level*indent)<<""<<"rii (y/n): ";
	is>>s;
	if (s=="y") {
		(tmp = new rii)->input(is,istty,level,indent);
		set_rii(dynamic_cast<rii*>(tmp));
	}
	
	if (istty) cout<<setw(level*indent)<<""<<"refresh period (y/n): ";
	is>>s;
	if (s=="y") {
		(tmp = new rp)->input(is,istty,level,indent);
		set_rp(dynamic_cast<rp*>(tmp));
	}

	if (istty) cout<<setw(level*indent)<<""<<"bound session id (y/n): ";
	is>>s;
	if (s=="y") {
	    cout << "\nbound session id: ";
	    cout<<"\nword[1]: ): ";
	    is>>i.w1;
	    cout<<"word[2]: ): ";
	    is>>i.w2;
	    cout<<"word[3]: ): ";
	    is>>i.w3;
	    cout<<"word[4]: ): ";
	    is>>i.w4;
	    tmp = new bound_sessionid(i);
		set_bound_sid(dynamic_cast<bound_sessionid*>(tmp));
	}

	if (istty) cout<<setw(level*indent)<<""<<"qspec (y/n): ";
	is>>s;
	if (s=="y") {
		(tmp = new qspec_object)->input(is,istty,level,indent);
		set_qspec(dynamic_cast<qspec_object*>(tmp));
	}

	); // end catch_bad_alloc
	originator = true;
	return is;
} // end input

/** Function to clear all pointers of current ReserveMsg.
  */
void reservereq::clear_pointers() {
	known_nslp_pdu::clear_pointers();
} // end clear_pointers

/***** inherited from nslp_pdu *****/

/** Function to prepare for deserialization of current ReserveMsg.
  * @param t the PDU type to be checked.
  */
nslp_pdu::deser_error_t reservereq::accept_type(uint16 t) {
	// prepare for deserialization
	state = wait_rsnref;
	return deser_ok;
} // end accept_type

/** Function to check the object of current ReserveMsg.
  * @param o the known_nslp_object to be checked whether this object belongs to a ReserveMsg.
  * @returns TRUE if the given object was accepted, otherwise returns FALSE.
  *
  *       RESERVE = COMMON-HEADER
  *              RSN [ RII ] [ REFRESH-PERIOD ] [ *BOUND-SESSION-ID ]
  *              [ SESSION-ID-LIST [ RSN-LIST ] ]
  *              [ MSG-ID / BOUND-MSG-ID ] [ INFO-SPEC ]
  *              [ [ PACKET-CLASSIFIER ] QSPEC ]
  *
  */
bool reservereq::accept_object(known_nslp_object* o) {
	if (o->is_rsn()) {
		set_rsn(dynamic_cast<rsn*>(o));
		state = deser_done; // RSN is the only required object
	} else if (o->is_packet_classifier()) {
		set_packet_classifier(dynamic_cast<packet_classifier*>(o));
	} else if (o->is_rii()) {
		set_rii(dynamic_cast<rii*>(o));
	} else if (o->is_rp()) {
		set_rp(dynamic_cast<rp*>(o));
	} else if (o->is_bound_sid()) {
		set_bound_sid(dynamic_cast<bound_sessionid*>(o));
	} else if (o->is_qspec()) {
		set_qspec(dynamic_cast<qspec_object*>(o));
	} else if (o->is_vlsp_object()) {
		set_vlsp_object(dynamic_cast<vlsp_object*>(o));
	} else if (o->is_sessionauth()) {
		nslp_auth::session_auth_object* sao= dynamic_cast<nslp_auth::session_auth_object*>(o);
		if (sao)
		{
			if (sao->is_hmac_signed())
				set_sessionauth_hmac_signed(dynamic_cast<nslp_auth::session_auth_object*>(o));
			else
				set_sessionauth(dynamic_cast<nslp_auth::session_auth_object*>(o));
		}
	} else
		return false;

	// When this is reached, everything is allright.
	return true;
} // end accept_object

/** Function to check at the end of deserialization of current ReserveMsg whether everything was OK.
  * @param errorlist the list with errors that were produced during deserialization of current ReserveMsg.
  * @returns TRUE if there were no errors, otherwise returns FALSE.
  */
bool reservereq::deser_end(IEErrorList& errorlist) {
	bool have_errors = false;
	switch (state) {
	case wait_rsnref:
		errorlist.put(new PDUSyntaxError("No request sequence number found."));
		have_errors = true;
		break;
	case wait_nextref:
		errorlist.put(new PDUSyntaxError("No additional information found."));
		have_errors = true;
		break;
	case deser_done:
		break;
	default:
		errorlist.put(new IEError(IEError::ERROR_INVALID_STATE));
		have_errors = true;
	} // end switch state

	if ( get_packet_classifier() && (get_qspec() == NULL) ) {
		errorlist.put(new PDUSyntaxError("Found a packet classifier, but no qspec object."));
		have_errors = true;
	}
	if ( (get_rii() != NULL) && is_acknowledge_flag() ) {
		errorlist.put(new PDUSyntaxError("Found RII object and ACK_REQ flag is set."));
		have_errors = true;
	}
	return (have_errors == false);
} // end deser_end

bool
reservereq::build_obj_list() const
{
	known_nslp_object *tmp= NULL;
/*
   RESERVE = COMMON_HEADER
             RSN [ RII ] [ REFRESH_PERIOD ] [ *BOUND_SESSION_ID ]
TODO:        [ SESSION_ID_LIST [ RSN_LIST ] ]
             [ [ PACKET_CLASSIFIER ] QSPEC ] [ SESSION_AUTH_OBJECT ]
 */
	obj_list.clear();
	/* RSN is mandatory */
	if (!(tmp = get_rsn()))
		return (false); 
	else
		obj_list.push_front(tmp);

	if ((tmp = get_rii()))
		obj_list.push_back(tmp);

	if ((tmp = get_rp()))
		obj_list.push_back(tmp);

	list<bound_sessionid *>::const_iterator it;
	for (it = bsid_list.begin(); it != bsid_list.end(); it++)
		if (*it) obj_list.push_back((*it));

	if ((tmp = get_packet_classifier())) {
		obj_list.push_back(tmp);
//   Subsequent RESERVE messages meant as reduced refreshes, where no
//   QSPEC is provided, MUST NOT include a PACKET_CLASSIFIER either.
		if (!(tmp = get_qspec())) {
			return (false);
		} else
			obj_list.push_back(tmp);

	} 
	else 
	if ((tmp = get_qspec()))
		obj_list.push_back(tmp);

	if ((tmp = get_vlsp_object()))
		obj_list.push_back(tmp);

	if ((tmp = get_sessionauth()))
		obj_list.push_back(tmp);

	if ((tmp = get_sessionauth_hmac_signed()))
		obj_list.push_back(tmp);

	return (nslp_pdu::build_obj_list());
}

/***** new members *****/

/** Default constructor for ReserveMsg without any params. All values will be set to the default values or NULLs.
  */
reservereq::reservereq() : known_nslp_pdu(RESERVE,"RESERVE",specific_not_set, generic_not_set) {
	rp_p = NULL;

	anticipated_reservation = false;
} // end constructor

/** Constructor for ReserveMsg. All objects of the current ReserveMsg will be set to the objects given as parameters.
  * @param req_seq_num the RSN object of the new ReserveMsg will be set to this rsn_seq_num.
  * @param req_id_info the RII object of the new ReserveMsg will be set to this req_id_info.
  * @param ref_period the REFRESH_PERIOD object of the new ReserveMsg will be set to this ref_period.
  * @param bound_session_id the BOUND_SESSIONID object of the new ReserveMsg will be set to this bound_session_id.
  * @param q_spec the QSPEC object of the new ReserveMsg will be set to this q_spec.
  */
reservereq::reservereq(
		rsn* req_seq_num,
		packet_classifier* pac_cla,
		rii* req_id_info,
		rp* ref_period,
		bound_sessionid* bound_session_id,
		qspec_object* q_spec_1,
	 	nslp_auth::session_auth_object* session_auth,
	 	nslp_auth::session_auth_object* session_auth_hmac_signed,
		vlsp_object* vlsp_obj
		       ) : known_nslp_pdu(RESERVE,"RESERVE",specific_not_set,generic_not_set) {
	set_rsn(dynamic_cast<rsn*>(req_seq_num));
	set_packet_classifier(dynamic_cast<packet_classifier*>(pac_cla));
	set_rii(dynamic_cast<rii*>(req_id_info));
	set_rp(dynamic_cast<rp*>(ref_period));
	set_bound_sid(dynamic_cast<bound_sessionid*>(bound_session_id));
	set_qspec(dynamic_cast<qspec_object*>(q_spec_1));
	set_sessionauth(dynamic_cast<nslp_auth::session_auth_object*>(session_auth)); 
	set_sessionauth_hmac_signed(dynamic_cast<nslp_auth::session_auth_object*>(session_auth_hmac_signed)); 
	set_vlsp_object(dynamic_cast<vlsp_object*>(vlsp_obj));
	anticipated_reservation = false;
} // end constructor

void reservereq::set_anticipated_reservation(bool val) {
	anticipated_reservation = val;
}

bool reservereq::get_anticipated_reservation() {
	return anticipated_reservation;
}

} // end namespace qos_nslp
