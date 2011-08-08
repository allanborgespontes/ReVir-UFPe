/// ----------------------------------------*- mode: C++; -*--
/// @file rsn.cpp
/// QoS NSLP Reservation Sequence Number
/// ----------------------------------------------------------
/// $Id: rsn.cpp 5253 2010-04-26 13:41:09Z roehricht $
/// $HeadURL: https://svn.tm.kit.edu/nsis/qos-nslp/trunk/src/rsn.cpp $
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

#include "rsn.h"
#include "nslp_pdu.h"
#include "logfile.h"

#include <iomanip>

using namespace protlib;
using namespace protlib::log;
using namespace qos_nslp;

namespace qos_nslp {

/** @addtogroup iersn Reservation Sequence Number
 * @ingroup ienslpobject
 * @{
 */

/***** class rsn *****/

/***** IE name *****/

const char* const rsn::iename = "rsn";

const uint16 rsn::contlen = 8;

const uint32 rsn::rsndefault = 0;

const uint32 rsn::rsn_max_increment = (1UL << 31)-1;

// this one should be set at start
uint32 rsn::local_epoch_id = 0;

} // end namespace qos_nslp

/***** inherited from IE *****/

/** Generate new RSN instance.
  */
rsn* rsn::new_instance() const {
	rsn* n = NULL;
	catch_bad_alloc(n = new rsn());
	return n;
} // end new_instance

/** Copy function for RSN object. 
  */
rsn* rsn::copy() const {
	rsn* n = NULL;
	catch_bad_alloc(n =  new rsn(*this));
	return n;
} // end copy

/** Deserialize function for RSN object.
  * @param msg Network message: from this message RSN object will be deserialized.
  * @param cod it is set to nslp_v1 for this version of QoS NSLP.
  * @param errorlist List to record errors.
  * @param bread reference to integer. Set to the number of bytes read by this call.
  * @param skip skip the IE in case of an error or restore NetMsg position.
  */
rsn* rsn::deserialize(NetMsg& msg, coding_t cod, IEErrorList& errorlist, uint32& bread, bool skip) {
	uint16 t = 0;
	uint16 len = 0;
	uint32 ielen = 0;
	uint32 saved_pos = 0;
	uint32 resume = 0;
	uint32 my_rsn;
	//DLog("DESERIALIZE", __PRETTY_FUNCTION__);

	// check arguments
	if (!check_deser_args(cod,errorlist,bread)) return NULL;
	// decode header
	if (!decode_header_nslpv1(msg,t,len,ielen,saved_pos,resume,errorlist,bread,skip)) return NULL;
	// check type
	if (t==RSN) type = RSN;
	else {
		// wrong type
		error_wrong_type(cod,t,saved_pos,skip,errorlist,resume,msg);
		return NULL;
	} // end if t
	// check length
	if (len!=contlen) {
		// wrong length
		error_wrong_length(cod,len,saved_pos,skip,errorlist,resume,msg);
		return NULL;
	} // end if wrong length
	// decode reservation_seq_no
	my_rsn = msg.decode32();
	this->set(my_rsn);
	// decode epoch_id
	my_rsn = msg.decode32();
	set_epoch_id(my_rsn);
	bread = ielen;
	return this;
} // end deserialize

/** Serialize function for RSN object.
  * @param netmsg Network message: RSN object will be serialized and added to this message.
  * @param cod it is set to nslp_v1 for this version of QoS NSLP.
  * @param wbytes Written bytes: the length of added bytes.
  */
void rsn::serialize(NetMsg& msg, coding_t cod, uint32& wbytes) const {
	// check arguments and IE state
	check_ser_args(cod,wbytes);
	encode_header_nslpv1(msg,contlen);
	// encode reservation_seq_no
	msg.encode32(reservation_seq_no);
	// encode epoch_id
	msg.encode32(epoch_id);
	wbytes = contlen+header_length;
	return;
} // end serialize

/** Check function for RSN object.
  * @returns TRUE, if the object is of type RSN.
  */
bool rsn::check() const {
	return ((type==RSN));
} // end check

/** Function to get the size of the RSN object (incl. header length).
  */
size_t rsn::get_serialized_size(coding_t cod) const {
	if (supports_coding(cod)) return contlen+header_length;
	else throw IEError(IEError::ERROR_CODING);
} // end get_serialized_size

/** Function to compare two objects of type RSN.
  */
bool rsn::operator==(const IE& ie) const {
	const rsn* t = dynamic_cast<const rsn*>(&ie);
	if (t) {
		return ((type==t->type)
			&& (reservation_seq_no==t->reservation_seq_no)
		);
	} else return false;
} // end operator==

/** Function to print nicely the object of type RSN on the monitor(or file).
  */
ostream& rsn::print(ostream& os, uint32 level, const uint32 indent, const char* name) const {
        os << setw(level*indent) << "RSN(" << hex << epoch_id << dec << "," << reservation_seq_no << ")";
	return os;
} // end print

/** Function for manual input of RSN object.
  */
istream& rsn::input(istream& is, bool istty, uint32 level, const uint32 indent, const char* name) {
	uint32 i;
	if (istty) {
		cout<<setw(level*indent)<<"";
		if (name && (*name!=0)) cout<<name<<" ";
		cout<<"<"<<get_ie_name()<<"> (rsn: ): ";
	} // end if istty
	is>>i;
	set(i);
	if (istty) {
		cout<<setw(level*indent)<<"";
		if (name && (*name!=0)) cout<<name<<" ";
		cout<<"<"<<get_ie_name()<<"> (epoch_id: ): ";
	} // end if istty
	is>>i;
	set_epoch_id(i);
	return is;
} // end input

/***** new in rsn *****/

/** Constructor for RSN object with given RSN object.
  * @param n the RSN object to be copied into new RSN object.
  */
rsn::rsn(const rsn& n)
	: known_nslp_object(n), 
	  epoch_id(n.epoch_id),
	  reservation_seq_no(n.reservation_seq_no)
{
} // end copy constructor

/** Assign operator for RSN object.
  * @param n the RSN object to be asigned to the current RSN object.
  */
rsn& rsn::operator=(const rsn& n) {
	type = n.type;
	known_nslp_object::operator=(n);
	reservation_seq_no = n.reservation_seq_no;
	epoch_id = n.epoch_id;
	return *this;
} // end operator=

/** Function to set the request sequence number to a default.
  */
void rsn::set_default() {
	reservation_seq_no = rsndefault;
} // end set_default



//@}

