/// ----------------------------------------*- mode: C++; -*--
/// @file qspec.cpp
/// QoS NSLP Quality-of-Service Sepcification Object
/// ----------------------------------------------------------
/// $Id: qspec.cpp 6159 2011-05-18 15:24:52Z bless $
/// $HeadURL: https://svn.tm.kit.edu/nsis/qos-nslp/trunk/src/qspec.cpp $
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

#include "qspec.h"
#include "nslp_pdu.h"
#include "logfile.h"

#include "qspec_ie.h"
#include "qspec_pdu.h"
#include "qspec_param.h"

#include <iomanip>

using namespace qspec;
using namespace protlib;
using namespace protlib::log;

namespace qos_nslp {

/** @addtogroup ieqspec Quality Specification
 * @ingroup ienslpobject
 * @{
 */

/***** class qspec_object *****/

/***** IE name *****/

const char* const qspec_object::iename = "qspec_object";

const char* qspec_object::get_ie_name() const { return iename; }

/***** inherited from IE *****/

/** Generate new QSPEC_OBJECT instance.
  */
qspec_object* qspec_object::new_instance() const {
	qspec_object* n = NULL;
	catch_bad_alloc(n = new qspec_object());
	return n;
} // end new_instance

/** Copy function for QSPEC_OBJECT object. 
  */
qspec_object* qspec_object::copy() const {
	qspec_object* n = NULL;
	catch_bad_alloc(n =  new qspec_object(*this));
	return n;
} // end copy

/** Deserialize function for QSPEC_OBJECT object.
  * @param msg Network message: from this message RSN object will be deserialized.
  * @param cod it is set to nslp_v1 for this version of QoS NSLP.
  * @param errorlist List to record errors.
  * @param bread reference to integer. Set to the number of bytes read by this call.
  * @param skip skip the IE in case of an error or restore NetMsg position.
  */
qspec_object* qspec_object::deserialize(NetMsg& msg, coding_t cod, IEErrorList& errorlist, uint32& bread, bool skip) {
	uint16 t = 0;
	uint16 len = 0;
	uint32 ielen = 0;
	uint32 saved_pos = 0;
	uint32 resume = 0;

	//DLog("DESERIALIZE", __PRETTY_FUNCTION__);
	// check arguments
	if (!check_deser_args(cod,errorlist,bread)) return NULL;
	// decode header
	if (!decode_header_nslpv1(msg,t,len,ielen,saved_pos,resume,errorlist,bread,skip)) return NULL;
	// check type
	if (t==QSPEC) type = QSPEC;
	else {
		// wrong type
		error_wrong_type(cod,t,saved_pos,skip,errorlist,resume,msg);
		return NULL;
	} // end if t
	// decode qspec data
	NetMsg* netmsg= new NetMsg(ielen-header_length);
	uchar* buffer = netmsg->get_buffer();
	msg.decode(buffer, ielen-header_length);
	netmsg->to_start();
	uint32 bytes_read = 0;
	IEManager *iem = QSPEC_IEManager::instance();
	IE *ie = iem->deserialize(*netmsg, (short unsigned int) cat_qspec_pdu, IE::protocol_v1, errorlist, bytes_read, true);
	qspec::qspec_pdu *my_qspec = dynamic_cast<qspec_pdu *>(ie);
	set_qspec_data(my_qspec);
	
	bread= ielen; // return length read in buffer (bytes_read may return value w/o padding bytes)

	return this;
} // end deserialize

/** Serialize function for QSPEC object.
  * @param netmsg Network message: RSN object will be serialized and added to this message.
  * @param cod it is set to nslp_v1 for this version of QoS NSLP.
  * @param wbytes Written bytes: the length of added bytes.
  */
void qspec_object::serialize(NetMsg& msg, coding_t cod, uint32& wbytes) const{
	// check arguments and IE state
	check_ser_args(cod,wbytes);
	uint32 written_bytes = 0;
	// encode qspec data
	if (qspec_data) {
	  NetMsg netmsg(qspec_data->get_serialized_size(cod)+header_length);
	  qspec_data->serialize(netmsg, cod, written_bytes);
	  netmsg.truncate();
	  uchar* buffer = netmsg.get_buffer();
	  // encode header
	  encode_header_nslpv1(msg,written_bytes);
	  
	  // encode qspec data
	  msg.encode(buffer, written_bytes);
	  wbytes = written_bytes+header_length;
	}
	return;
} // end serialize

/** Check function for QSPEC_OBJECT object.
  * @returns TRUE, if the object is of type QSPEC.
  */
bool qspec_object::check() const {
	return ((type==QSPEC));
} // end check

/** Function to get the size of the QSPEC object (incl. header length).
  */
size_t qspec_object::get_serialized_size(coding_t cod) const {
  if (qspec_data == NULL)
    return 0;
  if (supports_coding(cod)) return (qspec_data->get_serialized_size(cod)+header_length);
	else throw IEError(IEError::ERROR_CODING);
} // end get_serialized_size

/** Function to compare two objects of type QSPEC_OBJECT.
  */
bool qspec_object::operator==(const IE& ie) const {
	const qspec_object* t = dynamic_cast<const qspec_object*>(&ie);
	if (t) {
		return ((type==t->type)	&& (qspec_data==t->qspec_data)
		);
	} else return false;
} // end operator==

/** Function to print nicely the object of type QSPEC_OBJECT on the monitor.
  */
ostream& qspec_object::print(ostream& os, uint32 level, const uint32 indent, const char* name) const {
	os<<setw(level*indent)<<"";
	if (qspec_data) {
	  qspec_data->print(os, level, indent, name);
	}
	return os;
} // end print

/** Function for manual input of QSPEC_OBJECT object.
  */
istream& qspec_object::input(istream& is, bool istty, uint32 level, const uint32 indent, const char* name) {
	qspec_pdu* tmp = new qspec_pdu();
	if (istty) {
		cout<<setw(level*indent)<<"";
		if (name && (*name!=0)) cout<<name<<" ";
		cout<<"<"<<get_ie_name()<<">:"<<endl;
	} // end if istty
	level++;	
	tmp->input(is,istty,level,indent,"");
	set_qspec_data(tmp);
	return is;
} // end input

/***** new in qspec *****/

/** Default constructor for QSPEC object without any params. The values of the object will be set to the default.
  */
qspec_object::qspec_object()
	: known_nslp_object(QSPEC, mandatory),
	  qspec_data(NULL),
	  contlen(0)
{
} // end constructor

/** Constructor for QSPEC object with given QSPEC pdu form namespace qspec.
  * @param q_pdu the QSPEC pdu to be copied into new QSPEC pdu of the new QSPEC object.
  */
qspec_object::qspec_object(const qspec::qspec_pdu* q_pdu)
        : known_nslp_object(QSPEC, mandatory),
	  qspec_data(q_pdu ? q_pdu->copy() : NULL),
	  contlen(0)
{
} // end constructor


/** Constructor for QSPEC object with given QSPEC object.
  * @param n the QSPEC object to be copied into new QSPEC object.
  */
qspec_object::qspec_object(const qspec_object& n)
	: known_nslp_object(n) {
	qspec_data = n.qspec_data;
} // end copy constructor

/** Asign operator for QSPEC object.
  * @param n the QSPEC object to be asigned to the current QSPEC object.
  */
qspec_object& qspec_object::operator=(const qspec_object& n) {
	type = n.type;
	known_nslp_object::operator=(n);
	qspec_data = n.qspec_data;
	return *this;
} // end operator=


/** Get function for qspec data of the current QSPEC object.
  * @return the qspec data of the current QSPEC object is returned (as read-only reference)
  */
const qspec::qspec_pdu* 
qspec_object::get_qspec_data() const {
  return qspec_data;
} // end get

/** Set function for qspec class of the current QSPEC object.
  * @param data the value of the qspec class of the current QSPEC object will be set to the value of this variable. If NULL, the current qspec will get deleted
  */
void qspec_object::set_qspec_data(qspec::qspec_pdu* data) {
  delete qspec_data;
  if (data) {
    qspec_data = data->copy();
  }
  else
    qspec_data = NULL;
} // end set

/** Check function for QSPEC object.
  * @returns TRUE if the current object is of type QSPEC.
  */
bool qspec_object::is_qspec() const {
	return (type==QSPEC);
} // end is_qspec

void qspec_object::set_contlen(uint32 len)
{
  contlen = len;
}

//@}

} // end namespace qos_nslp
