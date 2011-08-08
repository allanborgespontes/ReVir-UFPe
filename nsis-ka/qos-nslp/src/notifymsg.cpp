/// ----------------------------------------*- mode: C++; -*--
/// @file notifymsg.cpp
/// NSLP NotifyMsg
/// ----------------------------------------------------------
/// $Id: notifymsg.cpp 6176 2011-05-20 07:39:55Z bless $
/// $HeadURL: https://svn.tm.kit.edu/nsis/qos-nslp/trunk/src/notifymsg.cpp $
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

#include "notifymsg.h"
#include "logfile.h"

#include <iomanip>

using namespace protlib;

namespace qos_nslp {

/** @addtogroup ienotifymsg NotifyMsg
 * @ingroup ienslppdu
 * @{
 */

/***** class notifymsg *****/

const char* const notifymsg::iename = "notifymsg";

const char* notifymsg::get_ie_name() const { return iename; }

/***** inherited from IE *****/

/** Generate new NotifyMsg instance.
  */
notifymsg* notifymsg::new_instance() const {
	notifymsg* n = NULL;
	catch_bad_alloc(n = new notifymsg());
	return n;
} // end new_instance

/** Copy function for NotifyMsg.
  */
notifymsg* notifymsg::copy() const {
	notifymsg* n = NULL;
	catch_bad_alloc(n =  new notifymsg(*this));
	return n;
} // end copy

/** Function to compare two objects of type NotifyMsg.
  */
bool notifymsg::operator==(const IE& ie) const {
	const notifymsg* n = dynamic_cast<const notifymsg*>(&ie);
	if (n && known_nslp_pdu::operator==(*n)) {
		return compare_objects(get_errorobject(),n->get_errorobject());
	} else return false;
} // end operator==

/** Function to print nicely the NotifyMsg on the monitor(or file).
  */
ostream& notifymsg::print(ostream& os, uint32 level, const uint32 indent, const char* name) const {
	bool objects = false;
	os<<setw(level*indent)<<"";
	if (name && (*name!=0)) os<<name<<" ";
	os<<"<"<<get_ie_name()<<">:";
	level++;
	known_nslp_object *tmp;
	if ((tmp = get_packet_classifier())) {
		tmp->print(os, level, indent, "packet_classifier");
		objects = true;
	}
	if ((tmp = get_errorobject())) {
		tmp->print(os, level, indent, "error_object");
		objects = true;
	}
	if ((tmp = get_qspec())) {
		tmp->print(os, level, indent, "qspec");
		objects = true;
	}
	if (!objects) cout<<" <empty>";
	return os;
} // end print

/** Function for manual input of NotifyMsg.
  */
istream& notifymsg::input(istream& is, bool istty, uint32 level, const uint32 indent, const char* name) {
	nslp_object* tmp;
	string s, c;
	FILE *fd;
	int i;
	struct in6_addr peer;
	char readStr[512];	    
	char arg1[100];	    
	char arg2[100];	    
	char arg3[100];
	char arg4[100];
	//port_t port;
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

	    if (istty) cout<<setw(level*indent)<<""<<"packet_classifier (y/n): ";
	    is>>s;
	    if (s=="y") {
	      (tmp = new packet_classifier)->input(is,istty,level,indent);
	      set_packet_classifier(dynamic_cast<packet_classifier*>(tmp));
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
	    
	    i = inet_pton(AF_INET6, arg4, &peer);

	    er_class = (info_spec::errorclass_t) e_class;
	    er_code = (info_spec::errorcode_t) e_code;
	    tmp = new info_spec(&peer, er_class, er_code, 0);
	    set_errorobject(dynamic_cast<info_spec*>(tmp));

	    if (istty) cout<<setw(level*indent)<<""<<"qspec (y/n): ";
	    is>>s;
	    if (s=="y") {
		(tmp = new qspec_object)->input(is,istty,level,indent);
		set_qspec(dynamic_cast<qspec_object*>(tmp));
	    }

	); // end catch_bad_alloc
	return is;
} // end input

/** Function to clear all pointers of current NotifyMsg.
  */
void notifymsg::clear_pointers() {
	known_nslp_pdu::clear_pointers();
} // end clear_pointers

/***** inherited from nslp_pdu *****/

/** Function to prepare for deserialization of current NotifyMsg.
  * @param t the PDU type to be checked.
  */
nslp_pdu::deser_error_t notifymsg::accept_type(uint16 t) {
	// prepare for deserialization
	state = wait_nextref;
	return deser_ok;
} // end accept_type

/** Function to check the object of current NotifyMsg.
  * @param o the known_nslp_object to be checked whether this object belongs to a NotifyMsg.
  * @returns TRUE if the given object was accepted, otherwise returns FALSE.
  */
bool notifymsg::accept_object(known_nslp_object* o) {
	if (o->is_packet_classifier()) {
		set_packet_classifier(dynamic_cast<packet_classifier*>(o));
	} else if (o->is_eo()) {
		set_errorobject(dynamic_cast<info_spec*>(o));
	} else if (o->is_qspec()) {
		set_qspec(dynamic_cast<qspec_object*>(o));
	} else
		return false;
// When this is reached, everything is allright.
return true;
} // end accept_object

/** Function to check at the end of deserialization of current NotifyMsg whether everything was OK.
  * @param errorlist the list with errors that were produced during deserialization of current NotifyMsg.
  * @returns TRUE if there were no errrors, otherwise returns FALSE.
  */
bool notifymsg::deser_end(IEErrorList& errorlist) {
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
notifymsg::build_obj_list() const
{
	known_nslp_object *tmp;
/*
   NOTIFY = COMMON_HEADER
            INFO_SPEC [ QSPEC ]
 */
	obj_list.clear();
	if (!(tmp = get_errorobject()))
		return (false); 
	obj_list.push_back(tmp);

	if ((tmp = get_qspec()))
		obj_list.push_back(tmp);

	return (nslp_pdu::build_obj_list());
}

/***** new members *****/

/** Default constructor for NotifyMsg without any params. All values will be set to the default values or NULLs.
  */
notifymsg::notifymsg() : known_nslp_pdu(NOTIFY, "NOTIFY", 0, 0) {
} // end constructor

/** Constructor for NotifyMsg. All objects of the current NotifyMsg will be set to the objects given as parameters.
  * @param pac_cla the PACKE_CLASSIFIER object of the new NotifyMsg will be set to this pac_cla.
  * @param error_object the INFO_SPEC object of the new NotifyMsg will be set to this error_object.
  * @param q_spec the QSPEC object of the new NotifyMsg will be set to this q_spec.
  */
notifymsg::notifymsg(
    packet_classifier* pac_cla,
    info_spec* error_object,
    qspec_object* q_spec_1
		     ) : known_nslp_pdu(NOTIFY, "NOTIFY", specific_not_set, generic_not_set)
{
	set_packet_classifier(pac_cla);
	set_errorobject(error_object);
	set_qspec(q_spec_1);
} // end constructor

} // end namespace qos_nslp
