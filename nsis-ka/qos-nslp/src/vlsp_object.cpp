// ----------------------------------------*- mode: C++; -*--
// vlsp_object.cpp - NSLP BoundSessionIdentification
// ----------------------------------------------------------
/** @file
 * NSLP BoundSessionIdentification
 */

#include "vlsp_object.h"
#include "nslp_pdu.h"
#include "logfile.h"

#include <iomanip>

using namespace protlib;
using namespace protlib::log;

namespace qos_nslp {

/** @addtogroup ievlsp_object VLSP Object
 * @ingroup ienslpobject
 * @{
 */

/***** class vlsp_object *****/

/***** IE name *****/

const char* const vlsp_object::iename = "vlsp_object";

const char* vlsp_object::get_ie_name() const { return iename; }
	// last field is tunnel type, but its padded to a 32bit word
	const uint16 vlsp_object::contlen = sizeof(vnet_id_t) + 2 * sizeof(vnode_id_t) + 2 * sizeof(vif_id_t) + sizeof(vlink_id_t) + sizeof(uint32);

} // end namespace qos_nslp

using namespace qos_nslp;

/***** inherited from IE *****/

/** Generate new VLSP_OBJECT instance.
  */
vlsp_object* 
vlsp_object::new_instance() const {
	vlsp_object* n = NULL;
	catch_bad_alloc(n = new vlsp_object());
	return n;
} // end new_instance

/** Copy function for VLSP_OBJECT object. 
  */
vlsp_object* vlsp_object::copy() const {
	vlsp_object* n = NULL;
	catch_bad_alloc(n =  new vlsp_object(*this));
	return n;
} // end copy


/** Deserialize function for VLSP_OBJECT object.
  * @param msg Network message: from this message VLSP_OBJECT object will be deserialized.
  * @param cod it is set to nslp_v1 for this version of QoS NSLP.
  * @param errorlist List to record errors.
  * @param bread reference to integer. Set to the number of bytes read by this call.
  * @param skip skip the IE in case of an error or restore NetMsg position.
  */
vlsp_object* vlsp_object::deserialize(NetMsg& msg, coding_t cod, IEErrorList& errorlist, uint32& bread, bool skip) {
	uint16 t = 0;
	uint16 len = 0;
	uint32 ielen = 0;
	uint32 saved_pos = 0;
	uint32 resume = 0;
	uint128 my_vlsp_object;

	// check arguments
	if (!check_deser_args(cod,errorlist,bread)) return NULL;
	// decode header
	if (!decode_header_nslpv1(msg,t,len,ielen,saved_pos,resume,errorlist,bread,skip)) return NULL;
	// check type
	if (t == VLSP_OBJECT) 
		type= VLSP_OBJECT;
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

	// decode vlsp_object
	vnet_id= msg.decode128();
	vnode_id_src= msg.decode128();
	vnode_id_dst= msg.decode128();
	vif_id_src= msg.decode64();
	vif_id_dst= msg.decode64();
	vlink_id= msg.decode64();
	// only the last 8 bits are going to be used, upper 24bits are reserved
	tunnel_type= msg.decode32() & 0xff;

	bread = ielen;
	return this;
} // end deserialize


/** Serialize function for VLSP_OBJECT object.
  * @param netmsg Network message: VLSP_OBJECT object will be serialized and added to this message.
  * @param cod it is set to nslp_v1 for this version of QoS NSLP.
  * @param wbytes Written bytes: the length of added bytes.
  */
void vlsp_object::serialize(NetMsg& msg, coding_t cod, uint32& wbytes) const {
	// check arguments and IE state
	check_ser_args(cod,wbytes);
	encode_header_nslpv1(msg,contlen);
	
	msg.encode128(vnet_id);
	msg.encode128(vnode_id_src);
	msg.encode128(vnode_id_dst);
	msg.encode64(vif_id_src);
	msg.encode64(vif_id_dst);
	msg.encode64(vlink_id);
	// tunnel type is only 8 bit, but due to alignment we use 32
	msg.encode32(tunnel_type);

	wbytes = contlen + header_length;

	return;
} // end serialize

/** Check function for VLSP_OBJECT object.
  * @returns TRUE, if the object is of type VLSP_OBJECT.
  */
bool vlsp_object::check() const {
	return type == VLSP_OBJECT;
} // end check

/** Function to get the size of the VLSP_OBJECT object (incl. header length).
  */
size_t vlsp_object::get_serialized_size(coding_t cod) const {
	if (supports_coding(cod)) return contlen + header_length;
	else throw IEError(IEError::ERROR_CODING);
} // end get_serialized_size

/** Function to compare two objects of type VLSP_OBJECT.
  */
bool vlsp_object::operator==(const IE& ie) const {
	const vlsp_object* t = dynamic_cast<const vlsp_object*>(&ie);
	if (t) {
		return ((type == t->type)
			&& vnet_id == t->vnet_id
			&& vnode_id_src == t->vnode_id_src
			&& vnode_id_dst == t->vnode_id_dst
			&& vif_id_src == t->vif_id_src
			&& vif_id_dst == t->vif_id_dst
			&& vlink_id == t->vlink_id
			&& tunnel_type == t->tunnel_type
		);
	} else return false;
} // end operator==

/** Function to print nicely the object of type VLSP_OBJECT on the monitor(or file).
  */
ostream& vlsp_object::print(ostream& os, uint32 level, const uint32 indent, const char* name) const {
	os<<setw(level*indent) 
	  << "Vnet-ID: " << vnet_id
	  << " VNODE-ID-Src: " << vnode_id_src
	  << " VNODE-ID-Dst: " << vnode_id_dst
	  << " VIF-ID-Src: " << vif_id_src
	  << " VIF-ID-Dst: " << vif_id_dst
	  << " VLink-ID: " << vlink_id
	  << " Tunnel type: " << (unsigned int) tunnel_type;

	return os;
} // end print

/** Function for manual input of VLSP_OBJECT object.
  */
istream& vlsp_object::input(istream& is, bool istty, uint32 level, const uint32 indent, const char* name) {
	if (istty) {
		cout<<setw(level*indent)<<"";
		if (name && (*name!=0)) cout<<name<<" ";
		cout<<"<"<<get_ie_name()<<"> (vlsp_object: ): ";
	} // end if istty
	//is >> vnet_id;
	//is >> vnode_id_src;
	//is >> vnode_id_dst;
	//is >> vif_id_src;
	//is >> vif_id_dst;
	//is >> vlink_id;

	return is;
} // end input

/***** new in vlsp_object *****/

/** Default constructor for VLSP_OBJECT object without any params. The value of the object will be set to the all zeros.
  */
vlsp_object::vlsp_object()
	: known_nslp_object(VLSP_OBJECT, forward),
	  vnet_id(0,0,0,0),
	  vnode_id_src(0,0,0,0),
	  vnode_id_dst(0,0,0,0),
	  vif_id_src(0),
	  vif_id_dst(0),
	  vlink_id(0),
	  tunnel_type(tunnel_undefined)
{
} // end constructor


// constructor for setting values
vlsp_object::vlsp_object(vnet_id_t vnetid, vnode_id_t vnode_src, vnode_id_t vnode_dst, vif_id_t vif_src, vif_id_t vif_dst, vlink_id_t vlink_id, tunnel_t tunnel_type) : known_nslp_object(VLSP_OBJECT, forward),
       vnet_id(vnetid),
       vnode_id_src(vnode_src),
       vnode_id_dst(vnode_dst),
       vif_id_src(vif_src),
       vif_id_dst(vif_dst),
       vlink_id(vlink_id),
       tunnel_type(tunnel_type)
{
}


/** Constructor for VLSP_OBJECT object with given VLSP_OBJECT object.
  * @param n the VLSP_OBJECT object to be copied into new VLSP_OBJECT object.
  */
vlsp_object::vlsp_object(const vlsp_object& n)
	: known_nslp_object(n),
	  vnet_id(n.vnet_id),
	  vnode_id_src(n.vnode_id_src),
	  vnode_id_dst(n.vnode_id_dst),
	  vif_id_src(n.vif_id_src),
	  vif_id_dst(n.vif_id_dst),
	  vlink_id(n.vlink_id),
	  tunnel_type(n.tunnel_type)
{
} // end copy constructor


/** Assign operator for VLSP_OBJECT object.
  * @param n the VLSP_OBJECT object to be asigned to the current VLSP_OBJECT object.
  */
vlsp_object& vlsp_object::operator=(const vlsp_object& n) {
	type = n.type;
	known_nslp_object::operator=(n);
	vnet_id= n.vnet_id;
	vnode_id_src= n.vnode_id_src;
	vnode_id_dst= n.vnode_id_dst;
	vif_id_src= n.vif_id_src;
	vif_id_dst= n.vif_id_dst;
	vlink_id= n.vlink_id;
	tunnel_type= n.tunnel_type;

	return *this;
} // end operator=



//@}

