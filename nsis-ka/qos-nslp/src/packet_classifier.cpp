/// ----------------------------------------*- mode: C++; -*--
/// @file packet_classifier.cpp
/// QoS NSLP Packet Classifier
/// ----------------------------------------------------------
/// $Id: packet_classifier.cpp 5253 2010-04-26 13:41:09Z roehricht $
/// $HeadURL: https://svn.tm.kit.edu/nsis/qos-nslp/trunk/src/packet_classifier.cpp $
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

#include "packet_classifier.h"
#include "nslp_pdu.h"
#include "logfile.h"
#include "ntlp_object.h"

#include <iomanip>

using namespace protlib;
using namespace protlib::log;
using namespace ntlp;

namespace qos_nslp {

/** @addtogroup iepacket_classifier PACKET_CLASSIFIER
 * @ingroup ienslpobject
 * @{
 */

/***** class packet_classifier *****/

/***** IE name *****/

const char* const packet_classifier::iename = "PACKET_CLASSIFIER";

const char* packet_classifier::get_ie_name() const { return iename; }

const uint16 packet_classifier::default_mrm = known_ntlp_object::pathcoupled;
const uint16 packet_classifier::default_flags = 0;

/***** inherited from IE *****/

/** Generate new PACKET_CLASSIFIER instance.
  */
packet_classifier* packet_classifier::new_instance() const {
	packet_classifier* n = NULL;
	catch_bad_alloc(n = new packet_classifier());
	return n;
} // end new_instance

/** Copy function for  PACKET_CLASSIFIERobject. 
  */
packet_classifier* packet_classifier::copy() const {
	packet_classifier* n = NULL;
	catch_bad_alloc(n =  new packet_classifier(*this));
	return n;
} // end copy

/** Deserialize function for PACKET_CLASSIFIER object.
  * @param msg Network message: from this message PACKET_CLASSIFIER object will be deserialized.
  * @param cod it is set to nslp_v1 for this version of QoS NSLP.
  * @param errorlist List to record errors.
  * @param bread reference to integer. Set to the number of bytes read by this call.
  * @param skip skip the IE in case of an error or restore NetMsg position.
  */
packet_classifier* packet_classifier::deserialize(NetMsg& msg, coding_t cod, IEErrorList& errorlist, uint32& bread, bool skip) {
	uint16 t = 0;
	uint16 len = 0;
	uint32 ielen = 0;
	uint32 saved_pos = 0;
	uint32 resume = 0;
	uint16 my_mrm, my_flags;
	//DLog("DESERIALIZE", __PRETTY_FUNCTION__);
	// check arguments
	if (!check_deser_args(cod,errorlist,bread)) return NULL;
	// decode header
	if (!decode_header_nslpv1(msg,t,len,ielen,saved_pos,resume,errorlist,bread,skip)) return NULL;
	// check type
	if (t==PACKET_CLASSIFIER) type = PACKET_CLASSIFIER;
	else {
		// wrong type
		error_wrong_type(cod,t,saved_pos,skip,errorlist,resume,msg);
		return NULL;
	} // end if t
	// check length
	if (len!=4) {
		// wrong length
		error_wrong_length(cod,len,saved_pos,skip,errorlist,resume,msg);
		return NULL;
	} // end if wrong length
	// decode mrmref and flagsref
	my_mrm = msg.decode16();
	my_flags = msg.decode16();
	this->set_mrm(my_mrm);
	this->set_flags(my_flags);
	bread = ielen;
	return this;
} // end deserialize

/** Serialize function for PACKET_CLASSIFIER object.
  * @param netmsg Network message: PACKET_CLASSIFIER object will be serialized and added to this message.
  * @param cod it is set to nslp_v1 for this version of QoS NSLP.
  * @param wbytes Written bytes: the length of added bytes.
  */
void packet_classifier::serialize(NetMsg& msg, coding_t cod, uint32& wbytes) const {
	// check arguments and IE state
	check_ser_args(cod,wbytes);
	encode_header_nslpv1(msg,4);
	// encode mrmref and flagsref
	msg.encode16(mrmref);
	msg.encode16(flagsref);
	wbytes = 4+header_length;
	return;
} // end serialize

/** Check function for PACKET_CLASSIFIER object.
  * @returns TRUE, if the object is of type PACKET_CLASSIFIER.
  */
bool packet_classifier::check() const {
	return ((type==PACKET_CLASSIFIER));
} // end check

/** Function to get the size of the PACKET_CLASSIFIER object (incl. header length).
  */
size_t packet_classifier::get_serialized_size(coding_t cod) const {
	if (supports_coding(cod)) return 4+header_length;
	else throw IEError(IEError::ERROR_CODING);
} // end get_serialized_size

/** Function to compare two objects of type PACKET_CLASSIFIER.
  */
bool packet_classifier::operator==(const IE& ie) const {
	const packet_classifier* t = dynamic_cast<const packet_classifier*>(&ie);
	if (t) {
		return ((type==t->type)
			&& (mrmref==t->mrmref) && (flagsref==t->flagsref)
		);
	} else return false;
} // end operator==

/** Function to print nicely the object of type PACKET_CLASSIFIER on the monitor.
  */
ostream& packet_classifier::print(ostream& os, uint32 level, const uint32 indent, const char* name) const {
        os<<setw(level*indent)<<"";
	Log(INFO_LOG,LOG_NORMAL, "PACKET_CLASSIFIER", "<"<<get_ie_name()<<">: MRM: " << mrmref << ", FLAGS: " << flagsref);
	return os;
} // end print

/** Function for manual input of PACKET_CLASSIFIER object.
  */
istream& packet_classifier::input(istream& is, bool istty, uint32 level, const uint32 indent, const char* name) {
	uint16 i;
	if (istty) {
		cout<<setw(level*indent)<<"";
		if (name && (*name!=0)) cout<<name<<" ";
		cout<<"<"<<get_ie_name()<<"> (MRM for PACKET_CLASSIFIER: ): ";
		is>>i;
		set_mrm(i);
		cout<<"<"<<get_ie_name()<<"> (FLAGS for PACKET_CLASSIFIER: ): ";
		is>>i;
		set_flags(i);
	} // end if istty
	return is;
} // end input

/***** new in packet_classifier *****/

/** Default constructor for PACKET_CLASSIFIER object without any params. The value of the object will be set to the default.
  */
packet_classifier::packet_classifier()
	: known_nslp_object(PACKET_CLASSIFIER, mandatory) {
	mrmref = default_mrm;
	flagsref = default_flags;
	contlen = 4;
} // end constructor

/** Constructor for PACKET_CLASSIFIER object with given param.
  * @param r the request sequence number for new PACKET_CLASSIFIER object.
  */
packet_classifier::packet_classifier(uint16 mrm, uint16 flags)
	: known_nslp_object(PACKET_CLASSIFIER, mandatory) {
	mrmref = mrm;
	flagsref = flags;
	contlen = 4;
} // end constructor

/** Constructor for PACKET_CLASSIFIER object with given PACKET_CLASSIFIER object.
  * @param n the PACKET_CLASSIFIER object to be copied into new PACKET_CLASSIFIER object.
  */
packet_classifier::packet_classifier(const packet_classifier& n)
	: known_nslp_object(n) {
	mrmref = n.mrmref;
	flagsref = n.flagsref;
} // end copy constructor

/** Asign operator for PACKET_CLASSIFIER object.
  * @param n the PACKET_CLASSIFIER object to be asigned to the current PACKET_CLASSIFIER object.
  */
packet_classifier& packet_classifier::operator=(const packet_classifier& n) {
	type = n.type;
	known_nslp_object::operator=(n);
	mrmref = n.mrmref;
	flagsref = n.flagsref;
	return *this;
} // end operator=

/** Function to set the request sequence number to a default.
  */
void packet_classifier::set_default() {
	mrmref = default_mrm;
	flagsref = default_flags;
} // end set_default

/** Get function for MRM of PACKET_CLASSIFIER object.
  * @param r into this variable the value of MRM of PACKET_CLASSIFIER object will be written.
  */
void packet_classifier::get_mrm(uint16& m) const {
	m = mrmref;
} // end get mrm

/** Set function for MRM of PACKET_CLASSIFIER object.
  * @param r the value of the MRM of PACKET_CLASSIFIER object will be set to the value of this variable.
  */
void packet_classifier::set_mrm(uint16 m) {
	mrmref = m;
} // end set mrm

/** Get function for flags of PACKET_CLASSIFIER object.
  * @param r into this variable the value of flags of PACKET_CLASSIFIER object will be written.
  */
void packet_classifier::get_flags(uint16& f) const {
	f = flagsref;
} // end get flags

/** Set function for flags of PACKET_CLASSIFIER object.
  * @param r the value of the flags of PACKET_CLASSIFIER object will be set to the value of this variable.
  */
void packet_classifier::set_flags(uint16 f) {
	flagsref = f;
} // end set flags

/** Check function for PACKET_CLASSIFIER object.
  * @returns TRUE if the current object is of type PACKET_CLASSIFIER.
  */
bool packet_classifier::is_packet_classifier() const {
	return (type==PACKET_CLASSIFIER);
} // end is_packet_classifier

/// set SOURCE_ADDRESS_AND_PREFIX_FLAG
void packet_classifier::set_source_address_and_prefix_flag()
{
  flagsref |= SOURCE_ADDRESS_AND_PREFIX_FLAG;
}
        
/// set DEST_ADDRESS_AND_PREFIX_FLAG
void packet_classifier::set_destination_address_and_prefix_flag()
{
  flagsref |= DEST_ADDRESS_AND_PREFIX_FLAG;
}

/// set PROTOCOL_FLAG 
void packet_classifier::set_protocol_flag()
{
  flagsref |= PROTOCOL_FLAG;
}

/// set TRAFFIC_CLASSL_FLAG
void packet_classifier::set_traffic_class_flag()
{
  flagsref |= TRAFFIC_CLASS_FLAG;
}
        
/// set FLOW_LABLE_FLAG
void packet_classifier::set_flow_label_flag()
{
  flagsref |= FLOW_LABLE_FLAG;
}
	
/// set SPI_FLAG
void packet_classifier::set_spi_flag()
{
  flagsref |= SPI_FLAG;
}
	
/// set SOURCE_PORT_FLAG
void packet_classifier::set_source_port_flag()
{
  flagsref |= SOURCE_PORT_FLAG;
}
	
/// set DESTINATION_PORT_FLAG
void packet_classifier::set_destination_port_flag()
{
  flagsref |= DESTINATION_PORT_FLAG;
}

/// is SOURCE_ADDRESS_AND_PREFIX_FLAG set
bool packet_classifier::is_source_address_and_prefix_flag()
{
  return flagsref & SOURCE_ADDRESS_AND_PREFIX_FLAG;
}
        
/// is DEST_ADDRESS_AND_PREFIX_FLAG set
bool packet_classifier::is_destination_address_and_prefix_flag()
{
  return flagsref & DEST_ADDRESS_AND_PREFIX_FLAG;
}
        
/// is PROTOCOL_FLAG set
bool packet_classifier::is_protocol_flag()
{
  return flagsref & PROTOCOL_FLAG;
}
 
/// is TRAFFIC_CLASSL_FLAG set
bool packet_classifier::is_traffic_class_flag()
{
  return flagsref & TRAFFIC_CLASS_FLAG;
}
        
/// is FLOW_LABLE_FLAG set
bool packet_classifier::is_flow_label_flag()
{
  return flagsref & FLOW_LABLE_FLAG;
}
	
/// is SPI_FLAG set
bool packet_classifier::is_spi_flag()
{
  return flagsref & SPI_FLAG;
}
	
/// is SOURCE_PORT_FLAG set
bool packet_classifier::is_source_port_flag()
{
  return flagsref & SOURCE_PORT_FLAG;
}

/// is DESTINATION_PORT_FLAG set
bool packet_classifier::is_destination_port_flag()
{
  return flagsref & DESTINATION_PORT_FLAG;
}

//@}

} // end namespace qos_nslp
