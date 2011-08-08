/// ----------------------------------------*- mode: C++; -*--
/// @file responsemsg.cpp
/// QoS NSLP Response Message
/// ----------------------------------------------------------
/// $Id: responsemsg.cpp 6275 2011-06-17 07:16:47Z bless $
/// $HeadURL: https://svn.tm.kit.edu/nsis/qos-nslp/trunk/src/responsemsg.cpp $
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

#include "responsemsg.h"
#include "logfile.h"
#include <netinet/in.h>
#include <arpa/inet.h>

#include <iomanip>

using namespace protlib;
using namespace protlib::log;

namespace qos_nslp {

/** @addtogroup ieresponsemsg ResponseMsg
 * @ingroup ienslppdu
 * @{
 */

/***** class responsemsg *****/

const char* const responsemsg::iename = "responsemsg";

const char* responsemsg::get_ie_name() const { return iename; }

/***** inherited from IE *****/

/** Generate new ResponseMsg instance
  */
responsemsg* responsemsg::new_instance() const {
	responsemsg* n = NULL;
	catch_bad_alloc(n = new responsemsg());
	return n;
} // end new_instance

/** Copy function for ResponseMsg
  */
responsemsg* responsemsg::copy() const {
	responsemsg* n = NULL;
	catch_bad_alloc(n =  new responsemsg(*this));
	return n;
} // end copy

/** Function to compare two objects of type ResponseMsg
  */
bool responsemsg::operator==(const IE& ie) const {
	const responsemsg* n = dynamic_cast<const responsemsg*>(&ie);
	if (n && known_nslp_pdu::operator==(*n)) {
		return compare_objects(get_errorobject(),n->get_errorobject());
	} else return false;
} // end operator==

/** Function to print nicely the ResponseMsg on the monitor.
  */
ostream& responsemsg::print(ostream& os, uint32 level, const uint32 indent, const char* name) const {
	bool objects = false;
	Log(INFO_LOG,LOG_NORMAL, "RESPONSE", "<"<<get_ie_name()<<">:");
	level++;
	known_nslp_object *tmp;
	if ((tmp = get_rii())) {
		tmp->print(os, level, indent, "rii");
		objects = true;
	}
	if ((tmp = get_rsn())) {
		tmp->print(os, level, indent, "rsn");
		objects = true;
	}
	if ((tmp = get_errorobject())) {
		tmp->print(os, level, indent, "info spec");
		objects = true;
	}
	if ((tmp = get_qspec())) {
		tmp->print(os, level, indent, "qspec");
		objects = true;
	}
	if ((tmp = get_sessionauth_hmac_signed())) { 
	    tmp->print(os, level, indent, "session_auth_hmac_signed");
		objects = true;
 	}
	if (!objects) {
	    Log(INFO_LOG,LOG_NORMAL, "RESPONSE", " <empty>");
	}
	return os;
} // end print

/** Function for manual input of ResponseMsg.
  */
istream& responsemsg::input(istream& is, bool istty, uint32 level, const uint32 indent, const char* name) {
	nslp_object* tmp;
	string s, c;
	int i;
	FILE *fd;
	struct in6_addr peer;
	char readStr[512];	    
	char arg1[100];	    
	char arg2[100];	    
	char arg3[100];
	char arg4[100];
	uint32 e_class, e_code, esi_length;	
	info_spec::errorclass_t er_class;
	info_spec::errorcode_t er_code;
	string e_string;
	catch_bad_alloc(
	    if (istty) {
		cout<<setw(level*indent)<<"";
		if (name && (*name!=0)) cout<<name<<" ";
		cout<<"<"<<get_ie_name()<<">:"<<endl;
	    } // end if istty
	    level++;
	    
	    if (istty) cout<<setw(level*indent)<<""<<"rsn (y/n): ";
	    is>>s;
	    if (s=="y") {
		(tmp = new rsn)->input(is,istty,level,indent,"");
		set_rsn(dynamic_cast<rsn*>(tmp));
	    }
	    else {
		if (istty) cout<<setw(level*indent)<<""<<"rii (y/n): ";
		is>>s;
		if (s=="y") {
		    (tmp = new rii)->input(is,istty,level,indent);		    
			set_rii(dynamic_cast<rii*>(tmp));
		}
	    }

	    fd = fopen("errorobject.input", "r");
	    if (fd == NULL) {
		fprintf(stderr, "Error: Can't open errorobject.input.\n");
		assert(0);
	    }
	    char* resstr= fgets(readStr, 510, fd);
	    if (resstr)
		    sscanf(readStr, "%s %s %s %s", arg1, arg2, arg3, arg4);
	    e_class = atoi(arg1);
	    e_code = atoi(arg2);
	    esi_length = atoi(arg3);
	    cout << "ARGS scanned" << endl;
	    cout << "e_class: " << e_class << endl;
	    cout << "e_code: " << e_code << endl;
	    cout << "esi_length: " << esi_length << endl;

	    i = inet_pton(AF_INET6, arg4, &peer);

	    er_class = (info_spec::errorclass_t) e_class;
	    er_code = (info_spec::errorcode_t) e_code;
	    tmp = new info_spec(&peer, er_class, er_code, 0);
		set_errorobject(dynamic_cast<info_spec*>(tmp));
	    fclose(fd);
	    
	    if (istty) cout<<setw(level*indent)<<""<<"qspec (y/n): ";
	    is>>s;
	    if (s=="y") {
		(tmp = new qspec_object)->input(is,istty,level,indent);
	    set_qspec(dynamic_cast<qspec_object*>(tmp));
	    } 
	); // end catch_bad_alloc
	return is;
} // end input

/** Function to clear all pointers of current ResponseMsg.
  */
void responsemsg::clear_pointers() {
	known_nslp_pdu::clear_pointers();
} // end clear_pointers

/***** inherited from nslp_pdu *****/

/** Function to prepare for deserialization of current ResponseMsg.
  * @param t the PDU type to be checked.
  */
nslp_pdu::deser_error_t responsemsg::accept_type(uint16 t) {
  // prepare for deserialization
  if (t == RESPONSE) {
	state = wait_nextref;
  }
	return deser_ok;
} // end accept_type

/** Function to check the object of current ResponseMsg.
  * @param o the known_nslp_object to be checked whether this object  belongs to a ResponseMsg.
  * @returns TRUE if the given object was accepted, otherwise returns FALSE.
  */
bool responsemsg::accept_object(known_nslp_object* o) {
	if (o->is_rsn()) {
		set_rsn(dynamic_cast<rsn*>(o));
	} else if (o->is_rii()) {
		set_rii(dynamic_cast<rii*>(o));
	} else if (o->is_eo()) {
		set_errorobject(dynamic_cast<info_spec*>(o));
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

/** Function to check at the end of deserialization of current ResponseMsg whether everything was OK.
  * @param errorlist the list with errors that were produced during deserialization of current ResponseMsg.
  * @returns TRUE if there were no errrors, otherwise returns FALSE.
  */
bool responsemsg::deser_end(IEErrorList& errorlist) {
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
responsemsg::build_obj_list() const
{
	known_nslp_object *tmp;
/*
   RESPONSE = COMMON_HEADER
              [ RII / RSN ] INFO_SPEC [SESSION_ID_LIST [ RSN_LIST ] ]
              [ QSPEC ] [VLSP OBJECT] [SESSION AUTHORIZATION OBJECT]
 */
	obj_list.clear();

	// NOTE: draft isn't clear if RII *and* RSN
	// are forbidden or just discouraged.
	if ((tmp = get_rii()))
		obj_list.push_back(tmp);

	if ((tmp = get_rsn()))
		obj_list.push_back(tmp);

	/* INFO_SPEC is mandatory */
	if (!(tmp = get_errorobject()))
		return (false); 
	obj_list.push_back(tmp);

	if ((tmp = get_qspec()))
		obj_list.push_back(tmp);

	// experimental additional VLSP Object
	if ((tmp = get_vlsp_object()))
		obj_list.push_back(tmp);

	// additional session authorization object
	if ((tmp = get_sessionauth()))
		obj_list.push_back(tmp);

	if ((tmp = get_sessionauth_hmac_signed()))
		obj_list.push_back(tmp);

	return (nslp_pdu::build_obj_list());

}

/***** new members *****/

/** Default constructor for ResponseMsg without any params. All values will be set to the default values or NULLs.
  */
responsemsg::responsemsg() : known_nslp_pdu(RESPONSE,"RESPONSE",specific_not_set,generic_not_set) {
} // end constructor

/** Constructor for ResponseMsg. All objects of ResponseMsg will be set to the objects given as parameters.
  * @param error_object the error object of the new ResponseMsg will be set to this error_object.
  * @param req_seq_num the rsn object of the new ResponseMsg will be set to this rsn_seq_num.
  * @param req_id_info the rii object of the new ResponseMsg will be set to this req_id_info.
  * @param q_spec the qspec object of the new ResponseMsg will be set to this q_spec.
  */
responsemsg::responsemsg(
                info_spec* error_object,
		rsn* req_seq_num,
		rii* req_id_info,
		qspec_object* q_spec_1,
	 	nslp_auth::session_auth_object* session_auth,
	 	nslp_auth::session_auth_object* session_auth_hmac_signed,
		vlsp_object* vlsp_obj
			 ) : known_nslp_pdu(RESPONSE,"RESPONSE",specific_not_set,generic_not_set)
{
	set_rsn(dynamic_cast<rsn*>(req_seq_num));
	set_rii(dynamic_cast<rii*>(req_id_info));
	set_errorobject(dynamic_cast<info_spec*>(error_object));
	set_qspec(dynamic_cast<qspec_object*>(q_spec_1));
	set_sessionauth(dynamic_cast<nslp_auth::session_auth_object*>(session_auth)); 
	set_sessionauth_hmac_signed(dynamic_cast<nslp_auth::session_auth_object*>(session_auth_hmac_signed)); 
	set_vlsp_object(dynamic_cast<vlsp_object*>(vlsp_obj));
} // end constructor

} // end namespace qos_nslp
