/// ----------------------------------------*- mode: C++; -*--
/// @file querymsg.cpp
/// QoS NSLP Query Message
/// ----------------------------------------------------------
/// $Id: querymsg.cpp 6159 2011-05-18 15:24:52Z bless $
/// $HeadURL: https://svn.tm.kit.edu/nsis/qos-nslp/trunk/src/querymsg.cpp $
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

#include "querymsg.h"
#include "logfile.h"

#include <iomanip>

using namespace protlib;
using namespace protlib::log;

namespace qos_nslp {

/** @addtogroup iequerymsg QueryMsg
 * @ingroup ienslppdu
 * @{
 */

/***** class querymsg *****/

const char* const querymsg::iename = "querymsg";

const char* querymsg::get_ie_name() const { return iename; }

/***** inherited from IE *****/

/** Generate new QueryMsg instance.
  */
querymsg* querymsg::new_instance() const {
	querymsg* n = NULL;
	catch_bad_alloc(n = new querymsg());
	return n;
} // end new_instance

/** Copy function for QueryMsg.
  */
querymsg* querymsg::copy() const {
	querymsg* n = NULL;
	catch_bad_alloc(n =  new querymsg(*this));
	return n;
} // end copy

/** Function to compare two objects of type QueryMsg.
  */
bool querymsg::operator==(const IE& ie) const {
	const querymsg* n = dynamic_cast<const querymsg*>(&ie);
	if (n && known_nslp_pdu::operator==(*n)) {
		return compare_objects(get_rii(),n->get_rii());
	} else return false;
} // end operator==

/** Function to print nicely the QueryMsg on the monitor.
  */
ostream& querymsg::print(ostream& os, uint32 level, const uint32 indent, const char* name) const {
	bool objects = false;
	Log(INFO_LOG,LOG_NORMAL, "QUERY", "<"<<get_ie_name()<<">:");
	level++;
	known_nslp_object *tmp;
	if ((tmp = get_packet_classifier())) {
		tmp->print(os, level, indent, "packet_classifier");
		objects = true;
	}
	if ((tmp = get_rii())) {
		tmp->print(os, level, indent, "rii");
		objects = true;
	}
	// TODO: [ *BOUND_SESSION_ID ]
	if ((tmp = get_bound_sid())) {
		tmp->print(os, level, indent, "bound session id");
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
	if (!objects) {
	    Log(INFO_LOG,LOG_NORMAL, "QUERY", " <empty>");
	}
	return os;
} // end print

/** Function for manual input of QueryMsg.
  */
istream& querymsg::input(istream& is, bool istty, uint32 level, const uint32 indent, const char* name) {
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

/** Function to clear all pointers of current QueryMsg.
  */
void querymsg::clear_pointers() {
	known_nslp_pdu::clear_pointers();
} // end clear_pointers

/***** inherited from nslp_pdu *****/

/** Function to prepare for deserialization of current QueryMsg.
  * @param t the PDU type to be checked.
  */
nslp_pdu::deser_error_t querymsg::accept_type(uint16 t) {
	state = wait_nextref;
	return deser_ok;
} // end accept_type

/** Function to check the object of current QueryMsg.
  * @param o the known_nslp_object to be checked whether this object  belongs to a QueryMsg.
  * @returns TRUE if the given object was accepted, otherwise returns FALSE.
  */
bool querymsg::accept_object(known_nslp_object* o) {
	if (o->is_packet_classifier()) {
		set_packet_classifier(dynamic_cast<packet_classifier*>(o));
	} else if (o->is_rii()) {
		set_rii(dynamic_cast<rii*>(o));
	} else if (o->is_bound_sid()) {
		set_bound_sid(dynamic_cast<bound_sessionid*>(o));
	} else if (o->is_qspec()) {
		set_qspec(dynamic_cast<qspec_object*>(o));
	} else if (o->is_sessionauth()) {
	 	set_sessionauth(dynamic_cast<nslp_auth::session_auth_object*>(o));
	} else
		return false;
	// When this is reached, everything is allright.
	return true;
} // end accept_object

/** Function to check at the end of deserialization of current QueryMsg whether everything was OK.
  * @param errorlist the list with errors that were produced during deserialization of current QueryMsg.
  * @returns TRUE if there were no errrors, otherwise returns FALSE.
  */
bool querymsg::deser_end(IEErrorList& errorlist) {
	switch (state) {
		case wait_nextref:
			errorlist.put(new PDUSyntaxError("No additional information found."));
		case deser_done: return true;
		default:
			errorlist.put(new IEError(IEError::ERROR_INVALID_STATE));
			return false;
	} // end switch state
} // end deser_end

bool
querymsg::build_obj_list() const
{
	known_nslp_object *tmp;
/*
   QUERY = COMMON_HEADER
           [ RII ][ *BOUND_SESSION_ID ]
           [ PACKET_CLASSIFIER ] QSPEC
 */
	obj_list.clear();
	/* QSPEC is mandatory */
	if (!(tmp = get_qspec()))
		return (false); 
	obj_list.push_back(tmp);

	if ((tmp = get_packet_classifier()))
		obj_list.push_front(tmp);

	list<bound_sessionid *>::const_reverse_iterator it;
	for (it = bsid_list.rbegin(); it != bsid_list.rend(); it++)
		obj_list.push_front((*it));

	if ((tmp = get_rii()))
		obj_list.push_front(tmp);

	if ((tmp = get_sessionauth()))
		obj_list.push_back(tmp);			

	return (nslp_pdu::build_obj_list());
}	

/***** new members *****/

/** Default constructor for QueryMsg without any params. All values will be set to the default values or NULLs.
  */
querymsg::querymsg() : known_nslp_pdu(QUERY,"QUERY",specific_not_set,generic_not_set) {
//	set_x_flag();
} // end constructor

/** Constructor for QueryMsg. All objects of the current QueryMsg will be set to the objects given as parameters.
  * @param req_id_info the RII object of the new QueryMsg will be set to this req_id_info.
  * @param bound_session_id the BOUND_SESSIONID object of the new QueryMsg will be set to this bound_session_id.
  * @param q_spec the QSPEC object of the new QueryMsg will be set to this q_spec.
  */
querymsg::querymsg(
		packet_classifier* pac_cla,
		rii* req_id_info,
		bound_sessionid* bound_session_id,
		qspec_object* q_spec_1,
	 	nslp_auth::session_auth_object* session_auth
		   ) : known_nslp_pdu(QUERY,"QUERY",specific_not_set,generic_not_set)
{
	set_packet_classifier(pac_cla);
	set_rii(req_id_info);
	set_bound_sid(bound_session_id);
	set_qspec(q_spec_1);
	set_sessionauth(dynamic_cast<nslp_auth::session_auth_object*>(session_auth)); 
//	set_x_flag();
} // end constructor

} // end namespace qos_nslp
