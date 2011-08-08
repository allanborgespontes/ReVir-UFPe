/// ----------------------------------------*- mode: C++; -*--
/// @file rii.cpp
/// QoS NSLP Request Identification Information Object
/// ----------------------------------------------------------
/// $Id: rii.cpp 5253 2010-04-26 13:41:09Z roehricht $
/// $HeadURL: https://svn.tm.kit.edu/nsis/qos-nslp/trunk/src/rii.cpp $
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

#include "rii.h"
#include "nslp_pdu.h"
#include "logfile.h"

#include <iomanip>

using namespace protlib;
using namespace protlib::log;

namespace qos_nslp {

/** @addtogroup ierii Request Identification Information
 * @ingroup ienslpobject
 * @{
 */

/***** class rii *****/

/***** IE name *****/

const char* const rii::iename = "rii";

const char* rii::get_ie_name() const { return iename; }

const uint16 rii::contlen = 4;

const uint32 rii::riidefault = 20000;

/***** inherited from IE *****/

/** Generate new RII instance
  */
rii* rii::new_instance() const {
	rii* n = NULL;
	catch_bad_alloc(n = new rii());
	return n;
} // end new_instance

/** Copy function for RII object 
  */
rii* rii::copy() const {
	rii* n = NULL;
	catch_bad_alloc(n =  new rii(*this));
	return n;
} // end copy

/** Deserialize function for RII object
  * @param msg Network message: from this message RII object will be deserialized.
  * @param cod it is set to nslp_v1 for this version of QoS NSLP.
  * @param errorlist List to record errors.
  * @param bread reference to integer. Set to the number of bytes read by this call.
  * @param skip skip the IE in case of an error or restore NetMsg position.
  */
rii* rii::deserialize(NetMsg& msg, coding_t cod, IEErrorList& errorlist, uint32& bread, bool skip) {
	uint16 t = 0;
	uint16 len = 0;
	uint32 ielen = 0;
	uint32 saved_pos = 0;
	uint32 resume = 0;
	uint32 my_rii;
	//DLog("DESERIALIZE", __PRETTY_FUNCTION__);
	// check arguments
	if (!check_deser_args(cod,errorlist,bread)) return NULL;
	// decode header
	if (!decode_header_nslpv1(msg,t,len,ielen,saved_pos,resume,errorlist,bread,skip)) return NULL;
	// check type
	if (t==RII) type = RII;
	else {
		// wrong type
		error_wrong_type(cod,t,saved_pos,skip,errorlist,resume,msg);
		return NULL;
	} // end if st
	// check length

	if (len!=contlen) {
		// wrong length
		error_wrong_length(cod,len,saved_pos,skip,errorlist,resume,msg);
		return NULL;
	} // end if wrong length
	// decode riiref
	my_rii = msg.decode32();
	this->set(my_rii);
	// There is no padding.
	bread = ielen;
	return this;
} // end deserialize

/** Serialize function for RII object
  * @param netmsg Network message: RII object will be serialized and added to this message
  * @param cod it is set to nslp_v1 for this version of QoS NSLP
  * @param wbytes Written bytes: the length of added bytes
  */
void rii::serialize(NetMsg& msg, coding_t cod, uint32& wbytes) const {
	// check arguments and IE state
	check_ser_args(cod,wbytes);
	encode_header_nslpv1(msg,contlen);
	// encode riiref
	msg.encode32(riiref);
	wbytes = contlen+header_length;
	return;
} // end serialize

/** Check function for RII object: returns true, if the object is of type RII
  */
bool rii::check() const {
	return ((type==RII));
} // end check

/** Function to get the size of the RII object (incl. header length)
  */
size_t rii::get_serialized_size(coding_t cod) const {
	if (supports_coding(cod)) return contlen+header_length;
	else throw IEError(IEError::ERROR_CODING);
} // end get_serialized_size

/** Function to compare two objects of type RII
  */
bool rii::operator==(const IE& ie) const {
	const rii* t = dynamic_cast<const rii*>(&ie);
	if (t) {
		return ((type==t->type)
			&& (riiref==t->riiref)
		);
	} else return false;
} // end operator==

/** Function to print nicely the object of type RII on the monitor
  */
ostream& rii::print(ostream& os, uint32 level, const uint32 indent, const char* name) const {
        os<<setw(level*indent)<<"";
	Log(INFO_LOG,LOG_NORMAL, "RII", "<"<<get_ie_name()<<">: " << riiref);
	return os;
} // end print

/** Function for manual input for RII object
  */
istream& rii::input(istream& is, bool istty, uint32 level, const uint32 indent, const char* name) {
	uint32 i;
	if (istty) {
		cout<<setw(level*indent)<<"";
		if (name && (*name!=0)) cout<<name<<" ";
		cout<<"<"<<get_ie_name()<<"> (rii: ): ";
	} // end if istty
	is>>i;
	set(i);
	is_own = true;
	return is;
} // end input

/***** new in rii *****/

/** Default constructor for RII object without any params
  */
rii::rii()
	: known_nslp_object(RII, mandatory),
	  riiref(rand()),
	  retry_counter(0),
	  is_own(false),
	  is_reserve(true),
	  q(NULL)
{
} // end constructor

/** Constructor for RII object
  * @param r the request identification number for new RII object
  */
rii::rii(uint32 r)
	: known_nslp_object(RII, mandatory),
	  riiref(r),
	  retry_counter(0),
	  is_own(false),
	  is_reserve(true),
	  q(NULL)
{
} // end constructor

/** Constructor for RII object
  * @param n the RII object to be copied into new for RII object
  */
rii::rii(const rii& n)
	: known_nslp_object(n),
	  riiref(n.riiref),
	  retry_counter(n.retry_counter),
	  is_own(n.is_own),
	  is_reserve(n.is_reserve),
	  q((n.q!=NULL) ? n.q->copy() : NULL)
{
} // end copy constructor

/** Equality operator for RII object
  * @param n the RII object to be compared with the current RII object
  */
rii& rii::operator=(const rii& n) {
	type = n.type;
	known_nslp_object::operator=(n);
	riiref = n.riiref;
	is_own = n.is_own;
	is_reserve = n.is_reserve;
	delete q;
	if (n.q)
	{
		q= n.q->copy();
	}
	return *this;
} // end operator=


//@}

} // end namespace qos_nslp
