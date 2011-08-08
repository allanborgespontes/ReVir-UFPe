/// ----------------------------------------*- mode: C++; -*--
/// @file nslp_pdu.cpp
/// NSLP PDU object class
/// ----------------------------------------------------------
/// $Id: nslp_pdu.cpp 6187 2011-05-23 12:27:11Z bless $
/// $HeadURL: https://svn.tm.kit.edu/nsis/qos-nslp/trunk/src/nslp_pdu.cpp $
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

#include "nslp_pdu.h"
#include "logfile.h"

using namespace protlib;
using namespace protlib::log;

namespace qos_nslp {

/** @addtogroup ienslppdu NSLP PDUs
 * @ingroup ie
 * @{
 */

/***** class nslp_pdu *****/
/***** inherited from IE *****/

/** Deserialize function for NSLP PDUs.
  * @param msg Network message: from this message NSLP PDU will be deserialized.
  * @param coding it is set to nslp_v1 for this version of QoS NSLP.
  * @param errorlist List to record errors.
  * @param bread reference to integer. Set to the number of bytes read by this call.
  * @param skip skip the IE in case of an error or restore NetMsg position.
  */
nslp_pdu*
nslp_pdu::deserialize(NetMsg& msg, coding_t coding, IEErrorList& errorlist, uint32& bread, bool skip)
{
	IEError *iee = NULL;
	uint32 ielen = 0;
	uint32 saved_pos = msg.get_pos();
	uint32 bytes_left = msg.get_bytes_left();
	uint8 t = 0;
	uint8 s_f = 0;
	uint16 g_f = 0;
	uint32 len = 0;
	uint32 resume = 0;

	// check arguments
	if (!check_deser_args(coding, errorlist, bread))
		return NULL;

	// check if we have enough to decode header
	if (bytes_left < common_header_length) {
		catch_bad_alloc(iee = new IEMsgTooShort(coding,category,saved_pos));
		Log(ERROR_LOG,LOG_NORMAL, "NSLP_pdu", "1. nslp_pdu::deserialize: IE " << get_ie_name() << " error, " << iee->getstr() << ", coding " << coding << ", category " << category << ", position " << saved_pos);
		errorlist.put(iee);
		msg.set_pos(saved_pos);
		return NULL;
	}

	// decode header
	try {
		decode_common_header_nslpv1(msg, t, s_f, g_f, len);
	} catch (IEError& ignored) {
		catch_bad_alloc(iee = new IETooBigForImpl(coding,category,saved_pos));
		Log(ERROR_LOG,LOG_NORMAL, "NSLP_pdu" ,"2. nslp_pdu::deserialize: IE " << get_ie_name() << ", error " << iee->getstr() << ", category " << category << ", position " << saved_pos);
		errorlist.put(iee);
		msg.set_pos(saved_pos);
		return NULL;
	}

	set_specific_flags(s_f);
	set_generic_flags(g_f);

	// check NetMsg buffer
	ielen = len + common_header_length;
	if (bytes_left < ielen) {
		catch_bad_alloc(iee = new IEMsgTooShort(nslp_v1,category,saved_pos));
		Log(ERROR_LOG,LOG_NORMAL, "NSLP_pdu", "3. known_nslp_object::decode_header_nslpv1: IE " << get_ie_name() << ", ielen: "<< ielen << ", bytes_left " << bytes_left << ", error " << iee->getstr() << ", category " << category << ", position " << saved_pos);
		errorlist.put(iee);
		msg.set_pos(saved_pos);
		return false;
	}

	if (skip) {
		resume = saved_pos + ielen;
		bread = ielen;
	} else {
		resume = saved_pos;
		bread = 0;
	}

	// check type
	switch (accept_type(t)) {
	case type_error:
		catch_bad_alloc(iee = new IEWrongType(coding,category,saved_pos));
		Log(ERROR_LOG,LOG_NORMAL, "NSLP_pdu", "nslp_pdu::deserialize: IE " << get_ie_name() << ", error " << iee->getstr() << ", coding " << coding << ", wrong_type " << (int)t << (skip ? ", skipping IE." : "."));
		if (skip) {
			bread = ielen;
		}
		errorlist.put(iee);
		msg.set_pos(resume);
		return NULL;
	default: // do nothing
		break;
	}

	// XXX: hack, hack, hack
	switch (t) {
	case known_nslp_pdu::RESERVE:
		break_flag_set = (specific_flags & RESV_FLAG_BREAK);
		break;
	case known_nslp_pdu::QUERY:
		break_flag_set = (specific_flags & QUERY_FLAG_BREAK);
		break;
	case known_nslp_pdu::RESPONSE:
		break_flag_set = (specific_flags & RESP_FLAG_BREAK);
		break;
	default:
		break_flag_set = false;
	}

	// decode NSLP objects
	IE* tmpie = NULL;
	nslp_object *tmpobj;
	known_nslp_object *knownobj;
	uint32 objbread = 0;
	uint32 tmpbread = 1;
	while ((objbread < len) && (tmpbread != 0)) {

		tmpie = NSLP_IEManager::instance()->deserialize(msg, cat_known_nslp_object, coding, errorlist, tmpbread, skip);
		objbread += tmpbread;

		if (tmpie == NULL) {
			ERRLog("NSLP_pdu", "nslp_pdu::deserialize: error unknown object, coding " << coding << (skip ? ", skipped." : "."));

			if (skip)
				continue;
			msg.set_pos(resume);
			bread= objbread;
			return this;
		}

		tmpobj = dynamic_cast<nslp_object *>(tmpie);
		if (tmpobj == NULL) {
			ERRLog("NSLP_pdu", "nslp_pdu::deserialize: error: not an NSLP object, coding " << coding << (skip ? ", skipped." : "."));

			delete tmpie;
			if (skip)
				continue;
			msg.set_pos(resume);
			bread= objbread;
			return this;
		}

		knownobj = dynamic_cast<known_nslp_object *>(tmpobj);
		if (knownobj == NULL) {
			ERRLog("NSLP_pdu", "nslp_pdu::deserialize: error: unknown NSLP object, coding " << coding << (skip ? ", skipped." : "."));
			delete tmpie;
			if (skip)
				continue;
			msg.set_pos(resume);
			bread= objbread;
			return this;			
		}

		// We have a "known" object.  accept_object() handles the rest
		if (!accept_object(knownobj)) {
			catch_bad_alloc(iee = new IEWrongType(coding,category,saved_pos));
			ERRLog("NSLP_pdu", "1. nslp_pdu::deserialize: error " << iee->getstr() << ", coding " << coding << (skip ? ", appending IE anyway." : "."));
			errorlist.put(iee);
			if (!skip) {
				delete knownobj;
				msg.set_pos(resume);
				bread= objbread;
				return this;
			}
			fwd_list.push_back(knownobj);
		}
		else
		{
		  DLog("NSLP_pdu","found " << color[blue] << get_ie_name() << color[off] << " object " << color[blue] << "[" << knownobj->get_ie_name() << "]" << color[off] << " read: " << tmpbread << " bytes");
		}

	} // end while

	// finish PDU deserialization
	if (!deser_end(errorlist)) {
		msg.set_pos(resume);
		return this;
	}

	// check objbread against len
	if (objbread != len) {
		catch_bad_alloc(iee = new IEWrongLength(coding,category,t,0,saved_pos));
		ERRLog("NSLP_pdu", "3. nslp_pdu::deserialize(): IE " << get_ie_name() << ", error " << iee->getstr() << ", coding " << coding << ", type " << (int)t << ", length " << len << ", read " << objbread << ", position " << saved_pos << (skip ? ", skipping IE." : "."));
		errorlist.put(iee);
		msg.set_pos(resume);
		return this;
	}

	// padding is included in objects
	bread = ielen;
	return this;
}

/** Serialize function for NSLP PDUs.
  * @param netmsg Network message: NSLP PDU will be serialized and added to this message.
  * @param cod it is set to nslp_v1 for this version of QoS NSLP.
  * @param wbytes Written bytes: the length of added bytes.
  */
void
nslp_pdu::serialize(NetMsg& msg, coding_t coding, uint32& wbytes) const 
{
	uint32 objwbytes = 0;
	uint32 len = 0;
	uint32 pdulen = 0;

	// ensure the obj_list is set
	if (obj_list_stale && !(build_obj_list())) {
		IEProtocolSpecific err(coding,category,msg.get_pos());
		ERRLog("NSLP_pdu", "nslp_pdu::serialize: build_obj_list() failed");
		throw err;
	}

	// check arguments and IE state
	check_ser_args(coding, wbytes);

	// calculate length and encode header
	pdulen = get_serialized_size(coding);
	len = pdulen - common_header_length;
	// check if len is a multiple of 32bit words
	if (len % sizeof(uint32)) {
		IEWrongLength err(coding,category,type,0,msg.get_pos());
		ERRLog("NSLP_pdu", "nslp_pdu::serialize: IE " << get_ie_name() << ", error " << err.getstr() << ", length " << len << " - not 32-bit aligned" );
		throw err;
	}

	// Is msg big enough?
	if (msg.get_bytes_left() < pdulen) {
		IEMsgTooShort err(coding,category,msg.get_pos());
		ERRLog("NSLP_pdu", "nslp_pdu::serialize: IE " << get_ie_name() << ", error " << err.getstr() << ", required " << pdulen << ", length " << msg.get_bytes_left());
		throw err;
	}

	// encode header
	msg.encode8(type);

	// XXX: hack, hack, hack
	uint8 tmp_flags = specific_flags;
	if (break_flag_set) {
		switch (type) {
		case known_nslp_pdu::RESERVE:
			tmp_flags |= RESV_FLAG_BREAK;
			break;
		case known_nslp_pdu::QUERY:
			tmp_flags |= QUERY_FLAG_BREAK;
			break;
		case known_nslp_pdu::RESPONSE:
			tmp_flags |= RESP_FLAG_BREAK;
			break;
		default:
			Log(ERROR_LOG,LOG_NORMAL, "NSLP_pdu", "nslp_pdu::serialize: BREAK flag requested in message of unknown type: " << type);
		}
	}
	msg.encode8(tmp_flags);

	msg.encode16(generic_flags);

	uint32 tmpwbytes = 0;
	// encode objects
	list<nslp_object *>::const_iterator it;
	for (it = obj_list.begin(); it != obj_list.end(); it++) {
		try {
			tmpwbytes= 0;
			if (*it != NULL)
			{
				//DLog("NSLP_pdu","serialize() serializing - " << (*it)->get_ie_name());
				(*it)->serialize(msg,coding,tmpwbytes);
				//DLog("NSLP_pdu", "serialize() serialized - " << (*it)->get_ie_name() << " wrote " << tmpwbytes << " bytes");
			}
			else
			{
				cerr << "nslp_pdu serialize() - ignored NULL object" << endl;
			}
		} catch (IEError& e) {
			ERRCLog("NSLP_pdu", "while processing " << (*it)->get_ie_name() << ":" << e.getstr());
			throw;
		} catch(...) {
			throw;
		}
		objwbytes += tmpwbytes;
		if (objwbytes > pdulen)
			break;
	}

	// check length of encoded objects
	if ((objwbytes != len) || (it != obj_list.end())) {
		//cerr << "nslp_pdu serialize(): objwbytes ==" << objwbytes << " len==" << len << endl;
		IEWrongLength err(coding,category,type,0,msg.get_pos());
		ERRLog("NSLP_pdu", "serialize(): IE " << get_ie_name() << ", error " << err.getstr() << ", length " << len << ", wrote " << objwbytes);
		throw err;
	}

	// padding is included in service object
	wbytes = pdulen;
	return;	
}

/** Check function for NSLP PDU.
  * @returns TRUE, if all object are legal objects.
  */
bool 
nslp_pdu::check() const
{
	// ensure the obj_list is set
	if (obj_list_stale && !build_obj_list()) {
		ERRLog("NSLP_pdu","check(): build_obj_list() failed");
		return false;		
	}

	list<nslp_object *>::const_iterator it;
	for (it = obj_list.begin(); it != obj_list.end(); it++)
	{
		if ( (*it != NULL) && !(*it)->check()) {
			ERRLog("NSLP_pdu","check(): failed for " << ((*it) ? (*it)->get_ie_name() : "-NULL-"));
			return false;
		}
	} // end for
	return true;
}


/** Function to get the size of the NSLP PDU.
  */
size_t 
nslp_pdu::get_serialized_size(coding_t coding) const
{
	size_t res = common_header_length;

	// ensure the obj_list is set
	if (obj_list_stale && !build_obj_list())
	{
		ERRLog("NSLP_pdu","get_serialized_size(): build_obj_list() failed");
		return 0;		
	}

	list<nslp_object*>::const_iterator it;
	for (it = obj_list.begin(); it != obj_list.end(); it++) {
		//#ifdef _DEBUG_IE
		//if ((*it) != NULL) // cannot use DLog() due to recursive lock in Logging
			//cerr << "NSLP_pdu get_serialized_size(): " << "ie:" << (*it)->get_ie_name() << '[' << (*it)->get_serialized_size(coding) << ']' << endl;
		//#endif

		res += ((*it) != NULL) ? (*it)->get_serialized_size(coding) : 0;
	}

	return res;  
} // end get_serialized_size

/** Only clear those pointers that are accessible from outside. The other
 * NSLP Objects must be deleted when the PDU is deleted and therefore must
 * not be cleared.
 */
void 
nslp_pdu::clear_pointers()
{
	obj_list_stale = true;
	obj_list.clear();
}

/** Only compare objects that are accessible from outside.
 * For a NSLP PDU this is the basic header and the integrity object.
 */
bool 
nslp_pdu::operator==(const IE& n) const
{
	const nslp_pdu* p = dynamic_cast<const nslp_pdu*>(&n);
	if (p)
		return (compare_objects(get_rsn(),p->get_rsn()));
	else
		return false;
}

/***** new members *****/

/** Function to compare two objects.
  */
bool 
nslp_pdu::compare_objects(const nslp_object* o1, const nslp_object* o2)
{
	if (!(o1 || o2))
		return true;
	else if (o1 && o2)
		return ((*o1) == (*o2));
	else
		return false;
}

/** Maximum content size of a NSLP PDU for this implementation. */
const uint32 nslp_pdu::max_size = 0xFFFFFFF4;

/** Length of common header in bytes. */
const uint32 nslp_pdu::common_header_length = 4;

/** The type field is 16 bit long, so there are 2^16 types. */
const uint32 nslp_pdu::num_of_types = 0x1000;

/** Decode common header without doing any sanity checks. 
  * @param m Network message: PDU header will be decoded from there.
  * @param t decoded PDU type from the header will be written into this variable.
  * @param s_f decoded special PDU flags from the header will be written into this variable.
  * @param g_f decoded generic PDU flags from the header will be written into this variable.
  * @param clenbytes number of undecoded words after the decoding the header will be written into this variable.
  */
void 
nslp_pdu::decode_common_header_nslpv1(NetMsg& m, uint8& t, uint8& s_f, uint16& g_f, uint32& clenbytes)
{
	(void)m.decode32();		//size
	t = m.decode8();
	// NSLP message specific flags
	s_f = m.decode8();
	// NSLP generic flags
	g_f = m.decode16();

	// Message size is not provided within NSLP PDU, only via the NTLP API
	clenbytes = m.get_bytes_left();
}

/** Constructor for a NSLP PDU: set category to known_nslp_pdu or unknown_nslp_pdu.
 * @param known unknown or known PDU?
 * @param t PDU type.
 * @param s_f specific PDU flags.
 * @param g_f generic PDU flags.
 * @param numobj number of objects excluding the basic header.
 */
  nslp_pdu::nslp_pdu(bool known, uint8 t, const char *const name, uint8 s_f, uint16 g_f)
  : NSLP_IE(known?(NSLP_IE::cat_known_nslp_pdu):(NSLP_IE::cat_unknown_nslp_pdu)),
    type(t), name(name), specific_flags(s_f), generic_flags(g_f),
    rsn_p(NULL),
    rii_p(NULL),
    rp_p(NULL),
    pc_p(NULL),
    qs_p(NULL),
    sauth_p(NULL),
    sauth_hmac_signed_p(NULL),
    vlsp_p(NULL),
    is_p(NULL)
{
	switch (type) {
	case known_nslp_pdu::RESERVE:
		break_flag_set = (specific_flags & RESV_FLAG_BREAK);
		break;
	case known_nslp_pdu::QUERY:
		break_flag_set = (specific_flags & QUERY_FLAG_BREAK);
		break;
	case known_nslp_pdu::RESPONSE:
		break_flag_set = (specific_flags & RESP_FLAG_BREAK);
		break;
	default:
		break_flag_set = false;
	}


	obj_list.clear();
	obj_list_stale = true;

	originator = false;	// XXX: there is no right default

	retry_counter = 0;
}

/** Constructor for NSLP PDU with given NSLP PDU.
  * @param n the NSLP PDU to be copied into new NSLP PDU.
  */
nslp_pdu::nslp_pdu(const nslp_pdu& n)
  : NSLP_IE(n), type(n.type), name(n.name), specific_flags(n.specific_flags), generic_flags(n.generic_flags),
    rsn_p(n.rsn_p),
    rii_p(n.rii_p),
    rp_p(n.rp_p),
    pc_p(n.pc_p),
    qs_p(n.qs_p),
    sauth_p(n.sauth_p),
    sauth_hmac_signed_p(n.sauth_hmac_signed_p),
    vlsp_p(n.vlsp_p),
    is_p(n.is_p),
    bsid_list(n.bsid_list),
    fwd_list(n.fwd_list),
    obj_list(n.obj_list),
    obj_list_stale(n.obj_list_stale),
    originator(n.originator),
    retry_counter(n.retry_counter)
{
	
	// XXX: hack, hack, hack
	switch (type) {
	case known_nslp_pdu::RESERVE:
		break_flag_set = (specific_flags & RESV_FLAG_BREAK);
		break;
	case known_nslp_pdu::QUERY:
		break_flag_set = (specific_flags & QUERY_FLAG_BREAK);
		break;
	case known_nslp_pdu::RESPONSE:
		break_flag_set = (specific_flags & RESP_FLAG_BREAK);
		break;
	default:
		break_flag_set = false;
	}
}

/** Assign operator for NSLP PDU: assigns type, flags and objects.
  * @param n the NSLP PDU to be assigned to the current NSLP PDU.
  */
nslp_pdu&
nslp_pdu::operator=(const nslp_pdu& n) 
{
	type = n.type;
	specific_flags = n.specific_flags;
	generic_flags = n.generic_flags;

	// XXX: hack, hack, hack
	switch (type) {
	case known_nslp_pdu::RESERVE:
		break_flag_set = (specific_flags & RESV_FLAG_BREAK);
		break;
	case known_nslp_pdu::QUERY:
		break_flag_set = (specific_flags & QUERY_FLAG_BREAK);
		break;
	case known_nslp_pdu::RESPONSE:
		break_flag_set = (specific_flags & RESP_FLAG_BREAK);
		break;
	default:
		break_flag_set = false;
	}

	rsn_p = n.rsn_p;
	rii_p = n.rii_p;
	rp_p = n.rp_p;
	pc_p = n.pc_p;
	qs_p = n.qs_p;
	sauth_p = n.sauth_p;
	sauth_hmac_signed_p = n.sauth_hmac_signed_p;
	vlsp_p = n.vlsp_p;
	is_p = n.is_p;
	bsid_list = n.bsid_list;
	fwd_list = n.fwd_list;

	obj_list = n.obj_list;
	obj_list_stale = n.obj_list_stale;
	originator = n.originator;

	retry_counter = n.retry_counter;

	return *this;
}

rsn*
nslp_pdu::get_rsn() const
{
	return rsn_p;
}

void
nslp_pdu::set_rsn(rsn* r)
{
	rsn_p = r;
	obj_list_stale = true;
}

packet_classifier*
nslp_pdu::get_packet_classifier() const
{
	return pc_p;
}

void
nslp_pdu::set_packet_classifier(packet_classifier* r)
{
	pc_p = r;
	obj_list_stale = true;
}

rii*
nslp_pdu::get_rii() const
{
	return rii_p;
}

void
nslp_pdu::set_rii(rii* r)
{
	rii_p = r;
	obj_list_stale = true;
}

rp*
nslp_pdu::get_rp() const
{
	return rp_p;
}

void
nslp_pdu::set_rp(rp* r)
{
	rp_p = r;
	obj_list_stale = true;
}

nslp_pdu::bsids_t
nslp_pdu::get_bound_sids() const
{
	return bsid_list.begin();
}

void
nslp_pdu::add_bound_sid(bound_sessionid* r)
{
	bsid_list.push_back(r);
	obj_list_stale = true;
}

bound_sessionid*
nslp_pdu::get_bound_sid() const
{
	if (bsid_list.empty())
		return NULL;

	return bsid_list.front();
}

void
nslp_pdu::set_bound_sid(bound_sessionid* r)
{
	bsid_list.clear();
	bsid_list.push_front(r);
	obj_list_stale = true;
}

info_spec*
nslp_pdu::get_errorobject() const
{
	return is_p;
}

void
nslp_pdu::set_errorobject(info_spec* error_object)
{
	is_p = error_object;
	obj_list_stale = true;
}

qspec_object*
nslp_pdu::get_qspec() const
{
	return qs_p;
}

void
nslp_pdu::set_qspec(qspec_object* r)
{
	qs_p = r;
	obj_list_stale = true;
}


void 
nslp_pdu::set_sessionauth(session_auth_object* as) 
{        
    sauth_p=as;         
    obj_list_stale = true; 
} 
	 

session_auth_object*  
nslp_pdu::get_sessionauth() const 
{ 
    return sauth_p; 
}

void 
nslp_pdu::set_sessionauth_hmac_signed(session_auth_object* as) 
{        
    sauth_hmac_signed_p=as;
    obj_list_stale = true; 
} 
	 

session_auth_object*  
nslp_pdu::get_sessionauth_hmac_signed() const 
{ 
    return sauth_hmac_signed_p; 
}


vlsp_object*
nslp_pdu::get_vlsp_object() const
{
	return vlsp_p;
}

void
nslp_pdu::set_vlsp_object(vlsp_object* r)
{
	vlsp_p = r;
	obj_list_stale = true;
}

void
nslp_pdu::set_bool_rii(bool r)
{
	set_rii_timer = r;
}

bool
nslp_pdu::get_bool_rii()
{
	return set_rii_timer;
}

void
nslp_pdu::set_bool_rp(bool r)
{
	set_rp_timer = r;
}

bool
nslp_pdu::get_bool_rp()
{
	return set_rp_timer;
}

bool
nslp_pdu::get_originator() const
{
	return originator;
}

void
nslp_pdu::set_originator(bool orig)
{
	originator = orig;
}

void
nslp_pdu::set_retry_counter(uint32 counter) {
	retry_counter = counter;
}

void
nslp_pdu::get_retry_counter(uint32 &counter) const {
	counter = retry_counter;
}

bool
nslp_pdu::build_obj_list() const
{
	list<nslp_object *>::const_iterator it;

	for (it = fwd_list.begin(); it != fwd_list.end(); it++)
	{
		if (*it != NULL) 
			obj_list.push_back((*it));
	}

	obj_list_stale = false;
	return (true);
}

/** Function to check the PDU type.
  * @param t PDU type to be checked.
  * @returns TRUE if this type is known to the implementation and the system can handle it at least
 */
bool 
known_nslp_pdu::is_known(uint16 t)
{
	switch (t) {
	case RESERVE:
	case QUERY:
	case RESPONSE:
	case NOTIFY:
		return true;
	default:
		return false;
	}
}

//@}

} // end namespace qos_nslp
