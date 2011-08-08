/// ----------------------------------------*- mode: C++; -*--
/// @file refresh_period.cpp
/// QoS NSLP Refresh Period
/// ----------------------------------------------------------
/// $Id: refresh_period.cpp 5253 2010-04-26 13:41:09Z roehricht $
/// $HeadURL: https://svn.tm.kit.edu/nsis/qos-nslp/trunk/src/refresh_period.cpp $
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

#include "refresh_period.h"
#include "nslp_pdu.h"
#include "logfile.h"

#include <iomanip>

using namespace protlib;
using namespace protlib::log;

namespace qos_nslp {

/** @addtogroupgroup ierp Refresh Period
 * @ingroup ienslpobject
 * @{
 */

/***** class rp *****/

/***** IE name *****/

const char* const rp::iename = "refresh_period";

const char* rp::get_ie_name() const { return iename; }

const uint16 rp::contlen = 4;

const uint32 rp::rpdefault = 30;

/***** inherited from IE *****/

/** Generate new Refresh Period instance
  */
rp* rp::new_instance() const {
	rp* n = NULL;
	catch_bad_alloc(n = new rp());
	return n;
} // end new_instance

/** Copy function for Refresh Period object 
  */
rp* rp::copy() const {
	rp* n = NULL;
	catch_bad_alloc(n =  new rp(*this));
	return n;
} // end copy

/** Deserialize function for Refresh Period object
  * @param netmsg Network message: from this message RSN object will be deserialized.
  * @param cod it is set to nslp_v1 for this version of QoS NSLP.
  * @param errorlist List to record errors.
  * @param bread reference to integer. Set to the number of bytes read by this call.
  * @param skip skip the IE in case of an error or restore NetMsg position.
  */
rp* rp::deserialize(NetMsg& msg, coding_t cod, IEErrorList& errorlist, uint32& bread, bool skip) {
	uint16 t = 0;
	uint16 len = 0;
	uint32 ielen = 0;
	uint32 saved_pos = 0;
	uint32 resume = 0;
	uint32 my_rp;
	//DLog("DESERIALIZE", __PRETTY_FUNCTION__);
	// check arguments
	if (!check_deser_args(cod,errorlist,bread)) return NULL;
	// decode header
	if (!decode_header_nslpv1(msg,t,len,ielen,saved_pos,resume,errorlist,bread,skip)) return NULL;
	// check type
	if (t==REFRESH_PERIOD) type = REFRESH_PERIOD;
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
	// decode rpref
	my_rp = msg.decode32();
	this->set(my_rp);
	// There is no padding.
	bread = ielen;
	return this;
} // end deserialize

/** Serialize function for Refresh Period object.
  * @param netmsg Network message: RSN object will be serialized and added to this message.
  * @param cod it is set to nslp_v1 for this version of QoS NSLP.
  * @param wbytes Written bytes: the length of added bytes.
  */
void rp::serialize(NetMsg& msg, coding_t cod, uint32& wbytes) const {
	// check arguments and IE state
	check_ser_args(cod,wbytes);
	encode_header_nslpv1(msg,contlen);
	// encode rpref
	msg.encode32(rpref);
	wbytes = contlen+header_length;
	return;
} // end serialize

/** Check function for Refresh Period object: returns true, if the object is of type REFRESH PERIOD
  */
bool rp::check() const {
	return ((type==REFRESH_PERIOD));
} // end check

/** Function to get the size of the Refresh Period object (incl. header length)
  */
size_t rp::get_serialized_size(coding_t cod) const {
	if (supports_coding(cod)) return contlen+header_length;
	else throw IEError(IEError::ERROR_CODING);
} // end get_serialized_size

/** Function to compare two objects of type Refresh Period
  */
bool rp::operator==(const IE& ie) const {
	const rp* t = dynamic_cast<const rp*>(&ie);
	if (t) {
		return ((type==t->type)
			&& (rpref==t->rpref)
		);
	} else return false;
} // end operator==

/** Function to print nicely the object of type Refresh Period on the monitor
  */
ostream& rp::print(ostream& os, uint32 level, const uint32 indent, const char* name) const {
	os<<setw(level*indent)<<"";
	Log(INFO_LOG,LOG_NORMAL, "REFRESH_PERIOD", "<"<<get_ie_name()<<">: " << rpref);
	return os;
} // end print

/** Function for manual input for Refresh Period object
  */
istream& rp::input(istream& is, bool istty, uint32 level, const uint32 indent, const char* name) {
	uint32 i;
	if (istty) {
		cout<<setw(level*indent)<<"";
		if (name && (*name!=0)) cout<<name<<" ";
		cout<<"<"<<get_ie_name()<<"> (refresh period: ): ";
	} // end if istty
	is>>i;
	set(i);
	return is;
} // end input

/***** new in rp *****/

/** Default constructor for REFRESH PERIOD object without any params. The value of the object will be set to the default.
  */
rp::rp()
	: known_nslp_object(REFRESH_PERIOD, mandatory) {
	rpref = rpdefault;
} // end constructor

/** Constructor for REFRESH PERIOD object
  * @param r the value for new REFRESH PERIOD object
  */
rp::rp(uint32 r)
	: known_nslp_object(REFRESH_PERIOD, mandatory) {
	rpref = r;
} // end constructor

/** Constructor for REFRESH PERIOD object
  * @param n the REFRESH PERIOD object to be copied into new for REFRESH PERIOD object
  */
rp::rp(const rp& n)
	: known_nslp_object(n) {
	rpref = n.rpref;
} // end copy constructor

/** Equality operator for REFRESH PERIOD object
  * @param n the REFRESH PERIOD object to be compared with the current REFRESH PERIOD object
  */
rp& rp::operator=(const rp& n) {
	type = n.type;
	known_nslp_object::operator=(n);
	rpref = n.rpref;
	return *this;
} // end operator=

/** Function to calculate the wait period based on current value of the REFRESH PERIOD object
  * @param period the value of the current REFRESH PERIOD objects
  * @param new_period the new calculated value to wait based on current value of the REFRESH PERIOD object
  */
void rp::get_rand_wait(uint32 period, uint32& new_period)
{
    double minrp = 0.5*period;
    double maxrp = 1.5*period;
    new_period = static_cast<uint32>(minrp+( (double)(maxrp-minrp)* rand()/(RAND_MAX+1.0) ));
}

/** Function to calculate the session lifetime based on current value of the REFRESH PERIOD object
  * @param period the value of the current REFRESH PERIOD objects
  * @param new_lifetime the new calculated value to live based on current value of the REFRESH PERIOD object
  */
void rp::get_rand_lifetime(uint32 period, uint32& new_lifetime)
{
    double minrp = (3 + 0.5) * 1.5 * period;
    double maxrp = (4 + 0.5) * 1.5 * period;
    new_lifetime = static_cast<uint32>(minrp+( (double)(maxrp-minrp)* rand()/(RAND_MAX+1.0) ));
}

/** Function to set the refresh period value to a default
  */
void rp::set_default() {
	rpref = rpdefault;
} // end set_default

/** Get function for REFRESH PERIOD object
  * @param r into this variable the value of REFRESH PERIOD object will be written.
  */
void rp::get(uint32& r) const {
	r = rpref;
} // end get

/** Set function for REFRESH PERIOD object
  * @param r the value of the REFRESH PERIOD object will be set to the value of this variable.
  */
void rp::set(uint32 r) {
	rpref = r;
} // end set

/** Check function for REFRESH PERIOD object
  * This function returns true if the current object is of type REFRESH PERIOD.
  */
bool rp::is_rp() const {
	return (type==REFRESH_PERIOD);
} // end is_rp

//@}

} // end namespace qos_nslp
