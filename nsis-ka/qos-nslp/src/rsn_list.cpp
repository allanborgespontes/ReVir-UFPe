/// ----------------------------------------*- mode: C++; -*--
/// @file rsn_list.cpp
/// QoS NSLP Reservation Sequence Number List
/// ----------------------------------------------------------
/// $Id: rsn_list.cpp 5253 2010-04-26 13:41:09Z roehricht $
/// $HeadURL: https://svn.tm.kit.edu/nsis/qos-nslp/trunk/src/rsn_list.cpp $
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

#include "rsn_list.h"
#include "nslp_pdu.h"
#include "logfile.h"

#include <iomanip>

using namespace protlib;
using namespace protlib::log;

namespace qos_nslp {

/** @addtogroup iersn_list
 * @ingroup ienslpobject
 * @{
 */

/***** class rsn_list *****/

/***** IE name *****/

const char* const rsn_list::iename = "rsn_list";

const uint16 rsn_list::minlen = 8;		// 2 * sizeof(uint32)

const char* rsn_list::get_ie_name() const { return iename; }

/***** inherited from IE *****/

rsn_list*
rsn_list::new_instance() const {
	rsn_list* n = NULL;
	catch_bad_alloc(n = new rsn_list());
	return n;
}

rsn_list*
rsn_list::copy() const {
	rsn_list* n = NULL;
	catch_bad_alloc(n =  new rsn_list(*this));
	return n;
}

/** Deserialize function for SESSION_ID_LIST object
  * @param msg Network message: from this message SESSION_ID_LIST object will be deserialized.
  * @param cod it is set to nslp_v1 for this version of QoS NSLP.
  * @param errorlist List to record errors.
  * @param bread reference to integer. Set to the number of bytes read by this call.
  * @param skip skip the IE in case of an error or restore NetMsg position.
  */
rsn_list*
rsn_list::deserialize(NetMsg& msg, coding_t cod, IEErrorList& errorlist, uint32& bread, bool skip)
{
	uint16 t = 0;
	uint16 len = 0;
	uint32 ielen = 0;
	uint32 saved_pos = 0;
	uint32 resume = 0;
	uint32 my_rsn;

	//DLog("DESERIALIZE", __PRETTY_FUNCTION__);
	// check arguments
	if (!check_deser_args(cod, errorlist, bread))
		return NULL;

	// decode header
	if (!decode_header_nslpv1(msg, t, len, ielen, saved_pos, resume,
	    errorlist, bread, skip))
		return NULL;

	// check type
	if (t == RSN_LIST)
		type = RSN_LIST;
	else {
		// wrong type
		error_wrong_type(cod, t, saved_pos, skip, errorlist, resume, msg);
		return NULL;
	}

	// check length
	if (len % sizeof(uint32) != 0 || len < minlen) {
		// wrong length
		error_wrong_length(cod, len, saved_pos, skip, errorlist, resume, msg);
		return NULL;
	}

	my_rsn = msg.decode32();
	set_epoch_id(my_rsn);
	for (int i = 1; i < (int)(len % sizeof(uint32)); i++) {
		my_rsn = msg.decode32();
		add_rsn(my_rsn);
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
void rsn_list::serialize(NetMsg& msg, coding_t cod, uint32& wbytes) const
{
	uint16 len;
	// check arguments and IE state
	check_ser_args(cod, wbytes);

	len = (rsn_list_list.size() + 1) * sizeof(uint32);
	encode_header_nslpv1(msg, len);
	msg.encode32(get_epoch_id());
	for (list<uint32>::const_iterator it = rsn_list_list.begin();
	    it != rsn_list_list.end(); it++) {
		msg.encode32((*it));
	}
	wbytes = len + header_length;
	return;
}

bool rsn_list::check() const
{
	return (type == RSN_LIST);
}

size_t rsn_list::get_serialized_size(coding_t cod) const
{
	if (supports_coding(cod))
		return (((rsn_list_list.size() + 1) * sizeof(uint32)) + header_length);
	else
		throw IEError(IEError::ERROR_CODING);
}

bool rsn_list::operator==(const IE& ie) const {
	const rsn_list* t = dynamic_cast<const rsn_list*>(&ie);
	if (t) {
		if (t->rsn_list_list.size() != rsn_list_list.size())
			return false;
/*
 * Do not test for epoch_id here - RSNs are significant
 * 
 *		if (t->get_epoch_id() != get_epoch_id())
 *			return false;
 */

		list<uint32>::const_iterator i1, i2;
		i1 = t->rsn_list_list.begin();
		i2 = rsn_list_list.begin();
		while (i1 != t->rsn_list_list.end() &&
		    i2 != rsn_list_list.end() &&
		    (*i1) == (*i2)) {
			i1++;
			i2++;
		}
		return (i1 == t->rsn_list_list.end() && i2 == rsn_list_list.end()); 
	} else
		return false;
}

/** Function to print nicely the object of type RII on the monitor
  */
ostream& rsn_list::print(ostream& os, uint32 level, const uint32 indent, const char* name) const {
        os<<setw(level*indent)<<"";
	Log(INFO_LOG,LOG_NORMAL, "RSN_LIST:", "");
	for (list<uint32>::const_iterator it = rsn_list_list.begin(); it != rsn_list_list.end(); it++)
		Log(INFO_LOG,LOG_NORMAL, "         ", "[" << (*it) << "]");
	return os;
} // end print

/** Function for manual input for RII object
  */
istream& rsn_list::input(istream& is, bool istty, uint32 level, const uint32 indent, const char* name) {
	char out[100];
	if (istty) {
		cout<<setw(level*indent)<<"";
		if (name && (*name!=0)) cout<<name<<" ";
		cout<<"<"<<get_ie_name()<<"> (epoch id: ): ";
	} // end if istty
	is>>out;
	set_epoch_id(atoi(out));
	for (;;) {
	if (istty) {
		cout<<setw(level*indent)<<"";
		if (name && (*name!=0)) cout<<name<<" ";
		cout<<"<"<<get_ie_name()<<"> (rsn or '.': ): ";
	} // end if istty
	is>>out;
	if (out[0] == '.')
		break;		
	add_rsn(atoi(out));
	}
	return is;
} // end input

/***** new in rsn_list *****/

/** Default constructor for SESSION_ID_LIST object without any params
  */
rsn_list::rsn_list()
	: known_nslp_object(SESSION_ID_LIST, mandatory) {
} // end constructor

/** Constructor for SESSION_ID_LIST object
  * @param r the request identification number for new RII object
  */
rsn_list::rsn_list(uint32 *rsn_arr, int count)
	: known_nslp_object(SESSION_ID_LIST, mandatory) {
} // end constructor

/** Constructor for SESSION_ID_LIST object
  * @param n the SESSION_ID_LIST object to be copied into new object
  */
rsn_list::rsn_list(const rsn_list& n)
	: known_nslp_object(n) {
	rsn_list_list = n.rsn_list_list;
} // end copy constructor

/** Equality operator for RII object
  * @param n the RII object to be compared with the current RII object
  */
rsn_list&
rsn_list::operator=(const rsn_list& n) {
	rsn_list_list = n.rsn_list_list;
	return *this;
} // end operator=

void
rsn_list::add_rsn(uint32 rsn)
{
	rsn_list_list.push_back(rsn);
}

list<uint32>
rsn_list::get_rsn_list()
{
	return rsn_list_list;
}

int
rsn_list::get_rsn_count()
{
	return rsn_list_list.size();
}

void
rsn_list::set_epoch_id(uint32 r)
{
	epoch_id = r;
}

uint32
rsn_list::get_epoch_id() const
{
	return epoch_id;
}

//@}

} // end namespace qos_nslp
