/// ----------------------------------------*- mode: C++; -*--
/// @file session_id_list.cpp
/// QoS NSLP Session ID List
/// ----------------------------------------------------------
/// $Id: session_id_list.cpp 5253 2010-04-26 13:41:09Z roehricht $
/// $HeadURL: https://svn.tm.kit.edu/nsis/qos-nslp/trunk/src/session_id_list.cpp $
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

#include "session_id_list.h"
#include "nslp_pdu.h"
#include "logfile.h"

#include <iomanip>

using namespace protlib;
using namespace protlib::log;

namespace qos_nslp {

/** @addtogroup iesid_list
 * @ingroup ienslpobject
 * @{
 */

/***** class sid_list *****/

/***** IE name *****/

const char* const sid_list::iename = "sid_list";

const char* sid_list::get_ie_name() const { return iename; }

/***** inherited from IE *****/

sid_list*
sid_list::new_instance() const {
	sid_list* n = NULL;
	catch_bad_alloc(n = new sid_list());
	return n;
}

sid_list*
sid_list::copy() const {
	sid_list* n = NULL;
	catch_bad_alloc(n =  new sid_list(*this));
	return n;
}

/** Deserialize function for SESSION_ID_LIST object
  * @param msg Network message: from this message SESSION_ID_LIST object will be deserialized.
  * @param cod it is set to nslp_v1 for this version of QoS NSLP.
  * @param errorlist List to record errors.
  * @param bread reference to integer. Set to the number of bytes read by this call.
  * @param skip skip the IE in case of an error or restore NetMsg position.
  */
sid_list*
sid_list::deserialize(NetMsg& msg, coding_t cod, IEErrorList& errorlist, uint32& bread, bool skip)
{
	uint16 t = 0;
	uint16 len = 0;
	uint32 ielen = 0;
	uint32 saved_pos = 0;
	uint32 resume = 0;
	uint128 my_sid;

	//DLog("DESERIALIZE", __PRETTY_FUNCTION__);
	// check arguments
	if (!check_deser_args(cod, errorlist, bread))
		return NULL;

	// decode header
	if (!decode_header_nslpv1(msg, t, len, ielen, saved_pos, resume,
	    errorlist, bread, skip))
		return NULL;

	// check type
	if (t == SESSION_ID_LIST)
		type = SESSION_ID_LIST;
	else {
		// wrong type
		error_wrong_type(cod, t, saved_pos, skip, errorlist, resume, msg);
		return NULL;
	}

	// check length
	if (len % sizeof(uint128) != 0) {
		// wrong length
		error_wrong_length(cod, len, saved_pos, skip, errorlist, resume, msg);
		return NULL;
	}

	for (int i = 0; i < (int)(len % sizeof(uint128)); i++) {
		my_sid = msg.decode128();
		add_sid(my_sid);
	}

	// There is no padding.
	bread = ielen;
	return this;
} // end deserialize

/** Serialize function for SESSION_ID_LIST object
  * @param netmsg Network message: SESSION_ID_LIST object will be serialized and added to this message
  * @param cod it is set to nslp_v1 for this version of QoS NSLP
  * @param wbytes Written bytes: the length of added bytes
  */
void sid_list::serialize(NetMsg& msg, coding_t cod, uint32& wbytes) const
{
	uint16 len;
	// check arguments and IE state
	check_ser_args(cod, wbytes);

	len = sid_list_list.size() * sizeof(uint128);
	encode_header_nslpv1(msg, len);
	for (list<uint128>::const_iterator it = sid_list_list.begin();
	    it != sid_list_list.end(); it++) {
		msg.encode128((*it));
	}
	wbytes = len + header_length;
	return;
}

bool sid_list::check() const
{
	return (type == SESSION_ID_LIST);
}

size_t sid_list::get_serialized_size(coding_t cod) const
{
	if (supports_coding(cod))
		return ((sid_list_list.size() * sizeof(uint128)) + header_length);
	else
		throw IEError(IEError::ERROR_CODING);
}

bool sid_list::operator==(const IE& ie) const {
	const sid_list* t = dynamic_cast<const sid_list*>(&ie);
	if (t) {
		if (t->sid_list_list.size() != sid_list_list.size())
			return false;

		list<uint128>::const_iterator i1, i2;
		i1 = t->sid_list_list.begin();
		i2 = sid_list_list.begin();
		while (i1 != t->sid_list_list.end() &&
		    i2 != sid_list_list.end() &&
		    (*i1) == (*i2)) {
			i1++;
			i2++;
		}
		return (i1 == t->sid_list_list.end() && i2 == sid_list_list.end()); 
	} else
		return false;
}

/** Function to print nicely the object of type RII on the monitor
  */
ostream& sid_list::print(ostream& os, uint32 level, const uint32 indent, const char* name) const {
        os<<setw(level*indent)<<"";
	Log(INFO_LOG,LOG_NORMAL, "SESSION_ID_LIST:", "");
	for (list<uint128>::const_iterator it = sid_list_list.begin(); it != sid_list_list.end(); it++)
		Log(INFO_LOG,LOG_NORMAL, "                ", "[" << (*it).w1 << ", " << (*it).w2 <<", " << (*it).w3 << ", " << (*it).w4 << "]");
	return os;
} // end print

/** Function for manual input for RII object
  */
istream& sid_list::input(istream& is, bool istty, uint32 level, const uint32 indent, const char* name) {
	uint128 i;
	char out[100];
	char arg1[10];
	char arg2[10];
	char arg3[10];
	char arg4[10];
	for (;;) {
	if (istty) {
		cout<<setw(level*indent)<<"";
		if (name && (*name!=0)) cout<<name<<" ";
		cout<<"<"<<get_ie_name()<<"> (session_id or '.': ): ";
	} // end if istty
	is>>out;
	if (*out == '.')
		break;		
	sscanf(out, "%s %s %s %s", arg1, arg2, arg3, arg4);
	i.w1 = atoi(arg1);
	i.w2 = atoi(arg2);
	i.w3 = atoi(arg3);
	i.w4 = atoi(arg4);
	add_sid(i);
	}
	return is;
} // end input

/***** new in sid_list *****/

/** Default constructor for SESSION_ID_LIST object without any params
  */
sid_list::sid_list()
	: known_nslp_object(SESSION_ID_LIST, mandatory) {
} // end constructor

/** Constructor for SESSION_ID_LIST object
  * @param r the request identification number for new RII object
  */
sid_list::sid_list(uint128 *sid_arr, int count)
	: known_nslp_object(SESSION_ID_LIST, mandatory) {
} // end constructor

/** Constructor for SESSION_ID_LIST object
  * @param n the SESSION_ID_LIST object to be copied into new object
  */
sid_list::sid_list(const sid_list& n)
	: known_nslp_object(n) {
	sid_list_list = n.sid_list_list;
} // end copy constructor

/** Equality operator for RII object
  * @param n the RII object to be compared with the current RII object
  */
sid_list&
sid_list::operator=(const sid_list& n) {
	sid_list_list = n.sid_list_list;
	return *this;
} // end operator=

void
sid_list::add_sid(uint128 sid)
{
	sid_list_list.push_back(sid);
}

list<uint128>
sid_list::get_sid_list()
{
	return sid_list_list;
}

int
sid_list::get_sid_count()
{
	return sid_list_list.size();
}

//@}

} // end namespace qos_nslp
