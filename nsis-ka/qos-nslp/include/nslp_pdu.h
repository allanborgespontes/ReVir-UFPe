/// ----------------------------------------*- mode: C++; -*--
/// @file nslp_pdu.h
/// NSLP pdu object classes
/// ----------------------------------------------------------
/// $Id: nslp_pdu.h 6187 2011-05-23 12:27:11Z bless $
/// $HeadURL: https://svn.tm.kit.edu/nsis/qos-nslp/trunk/include/nslp_pdu.h $
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
/** @ingroup ienslppdu
 * @file
 * NSLP PDUs
 */

#ifndef _NSLP__NSLPPDU_H_
#define _NSLP__NSLPPDU_H_

#include "protlib_types.h"
#include "nslp_ie.h"
#include "nslp_object.h"
#include "session_auth_object.h"
#include "rsn.h"
#include "packet_classifier.h"
#include "rii.h"
#include "refresh_period.h"
#include "bound_sessionid.h"
#include "qspec.h"
#include "info_spec.h"
#include "vlsp_object.h"

#include <list>

using namespace protlib;
using namespace nslp_auth; 

namespace qos_nslp {

/** @addtogroup ienslppdu NSLP PDUs
 * @{
 */

/** NSLP protocol data unit
 * Base class for NSLP PDU, representing common NSLP header and NSLP Objects.
 */

class nslp_pdu : public NSLP_IE {
/***** inherited from NSLP_IE *****/
public:
	nslp_pdu* new_instance() const = 0;
	nslp_pdu* copy() const = 0;
	virtual nslp_pdu* deserialize(NetMsg& msg, coding_t coding, IEErrorList& errorlist, uint32& bread, bool skip = true);
	virtual void serialize(NetMsg& msg, coding_t coding, uint32& wbytes) const;
	virtual bool check() const;
	virtual size_t get_serialized_size(coding_t coding) const;
	virtual void clear_pointers();
	virtual bool operator==(const IE& n) const;
/***** new members *****/
public:
	/* Namespace these at least a bit */
	/* Note: Looking at this, it seems stupid not to have the BREAK flag in GENERIC */ 
	/* generic flags (16 bit) */
	static const uint16 GEN_FLAG_SCOPING			= (1<<0);
	static const uint16 GEN_FLAG_PROXY			= (1<<1);
	static const uint16 GEN_FLAG_ACKNOWLEDGE		= (1<<2);

	/* RESERVE specific flags (8 bit) */
	static const uint8 RESV_FLAG_REPLACE			= (1<<0);
	static const uint8 RESV_FLAG_TEAR			= (1<<1);
	static const uint8 RESV_FLAG_REQ_REDUCED_REFRESH	= (1<<2);	// ugly name
	static const uint8 RESV_FLAG_BREAK			= (1<<3);

	/* QUERY specific flags (8 bit) */
	static const uint8 QUERY_FLAG_RESERVE_INIT		= (1<<0);
	static const uint8 QUERY_FLAG_BREAK			= (1<<1);
	static const uint8 QUERY_FLAG_X				= (1<<2);	// see Anticipated Handover

	/* RESPONSE specific flags (8bit) */
	static const uint8 RESP_FLAG_BREAK			= (1<<0);

	/* NOTIFY specific flags (8bit) */
	static const uint8 NOTIFY_FLAG_X			= (1<<0);	// see Anticipated Handover


	static const uint8 specific_not_set = 0;
	static const uint16 generic_not_set = 0;

	/// max PDU size
	static const uint32 max_size;
	static const uint32 common_header_length;
	/// number of type values
	static const uint32 num_of_types;

protected:
        nslp_pdu(bool known, uint8 t = 0, const char* name="unspecified", uint8 s_f = 0, uint16 g_f = 0);
	nslp_pdu(const nslp_pdu& n);

	nslp_pdu& operator=(const nslp_pdu& n);

	/// check two objects for equality and handle NULL pointers.
	static bool compare_objects(const nslp_object* o1, const nslp_object* o2);

public:
	virtual ~nslp_pdu();

	/// decode header for msg content length (excluding common header)
	static bool decode_common_header_nslpv1_clen(NetMsg& m, uint32& clen_words);
	/// decode header
	static void decode_common_header_nslpv1(NetMsg& m, uint8& t, uint8& s_f, uint16& g_f, uint32& clen_bytes);

	uint8 get_type() const;

	uint16 get_generic_flags();
	void set_generic_flags(uint16 f);

	uint8 get_specific_flags();
	void set_specific_flags(uint8 f);

	void set_replace_flag();
	void set_acknowledge_flag();
	void set_scoping_flag();
	void set_tear_flag();
	void set_reserve_init_flag();
	void set_proxy_flag();
	void set_req_reduced_refresh_flag();
	void set_break_flag(); // XXX: should this be namespaced?

	bool is_replace_flag() const;
	bool is_acknowledge_flag() const;
	bool is_scoping_flag() const;
	bool is_tear_flag() const;
	bool is_reserve_init_flag() const;
	bool is_proxy_flag() const;
	bool is_req_reduced_refresh_flag() const;
	bool is_break_flag() const;

// @{
	rsn* get_rsn() const;
	void set_rsn(rsn* r);

	packet_classifier* get_packet_classifier() const;
	void set_packet_classifier(packet_classifier* r);

	rii* get_rii() const;
	void set_rii(rii* r);

	rp* get_rp() const;
	void set_rp(rp* r);

	typedef list<bound_sessionid *> bsid_list_t;
	typedef bsid_list_t::const_iterator bsids_t;

	bsids_t get_bound_sids() const;
	void add_bound_sid(bound_sessionid* r);
	// compat wrapper for just one bsid
	bound_sessionid* get_bound_sid() const;
	void set_bound_sid(bound_sessionid* r);

	info_spec* get_errorobject() const;
	void set_errorobject(info_spec* error_object);

	qspec_object* get_qspec() const;
	void set_qspec(qspec_object* r);

	void set_bool_rii(bool r);
	bool get_bool_rii();

	void set_bool_rp(bool r);
	bool get_bool_rp();

	bool get_originator() const;
	void set_originator(bool orig);

	void set_sessionauth(session_auth_object* as); 
	session_auth_object* get_sessionauth() const;

	void set_sessionauth_hmac_signed(session_auth_object* as); 
	session_auth_object* get_sessionauth_hmac_signed() const;

	// currently only used for QUERY messages without
	// RII object (QUERY with Reserve-Init-Flag set)
	void set_retry_counter(uint32 counter);
	void get_retry_counter(uint32 &counter) const;

        const char* get_name() const { return name; }

        // additional experimental object for virtual link setup
	vlsp_object* get_vlsp_object() const;
	void set_vlsp_object(vlsp_object* r);


// @}

protected:
	uint8 type;
        const char *const name;

	uint8 specific_flags;
	uint16 generic_flags;
	bool break_flag_set;	// XXX: This is a hack!

/*
   RESERVE = COMMON_HEADER
             RSN [ RII ] [ REFRESH_PERIOD ] [ *BOUND_SESSION_ID ]
             [ SESSION_ID_LIST [ RSN_LIST ] ]
             [ [ PACKET_CLASSIFIER ] QSPEC ]

   QUERY = COMMON_HEADER
           [ RII ][ *BOUND_SESSION_ID ]
           [ PACKET_CLASSIFIER ] QSPEC

   RESPONSE = COMMON_HEADER
              [ RII / RSN ] INFO_SPEC [SESSION_ID_LIST [ RSN_LIST ] ]
              [ QSPEC ]

   NOTIFY = COMMON_HEADER
            INFO_SPEC [ QSPEC ]
*/
	rsn *rsn_p;
	rii *rii_p;
	rp *rp_p;
	packet_classifier *pc_p;
	qspec_object *qs_p;
	session_auth_object* sauth_p;
	session_auth_object* sauth_hmac_signed_p;
        vlsp_object *vlsp_p;
	info_spec *is_p;
	list<bound_sessionid *> bsid_list;
	// objects marked "forward", need to be kept
	// XXX: order requirements?
	list<nslp_object *> fwd_list;

	// serialized object list
	mutable list<nslp_object *> obj_list;
	mutable bool obj_list_stale;
	// the parent version takes care of appending objs in fwd_list
	virtual bool build_obj_list() const;

	bool set_rii_timer;
	bool set_rp_timer;
	bool originator;

	uint32 retry_counter;

/***** for deserialization *****/
	enum deser_error_t { deser_ok, type_error };
	/** Check and set type and prepare PDU for deserialization. */
	virtual deser_error_t accept_type(uint16 t) = 0;
	/** Check the given object and append it to the object array. */
	virtual bool accept_object(known_nslp_object* o) = 0;
	/** Finish deserialization, set all fields and generate errors.
	 * @returns true if PDU state OK, false otherwise.
	 */
	virtual bool deser_end(IEErrorList& errorlist) = 0;
}; // end class nslp_pdu

/// Known NSLP PDUs
class known_nslp_pdu : public nslp_pdu {
/***** inherited from IE *****/
public:
	known_nslp_pdu* new_instance() const = 0;
	known_nslp_pdu* copy() const = 0;
protected:
	virtual void register_ie(IEManager* iem) const;
/***** new members *****/
public:
	/// message type
	/** All NSLP message types have to be listed here and their values should
	 * correspond with those sent over the wire. Otherwise they have to be
	 * mapped when (de)serializing.
	 * Type values must not exceed 16 bits in NSLP_V1.
	 */

	enum type_t {
		UNDEFINED       = 0,
		RESERVE		= 1,
		QUERY		= 2,
		RESPONSE	= 3,
		NOTIFY		= 4,
		reserved	= 255
	};

	static bool is_known(uint16 t);

protected:
        known_nslp_pdu(type_t t, const char *const name, uint8 s_f, uint16 g_f);
	known_nslp_pdu(const known_nslp_pdu& n);
public:
	bool is_reserve() const;
	bool is_query() const;
	bool is_response() const;
	bool is_notify() const;

	void set_x_flag();
	bool is_x_flag() const;
}; // end class known_nslp_pdu

/** Decode common header for getting msg content length without doing any sanity checks. Resets position pointer in m to start.
 * @return true if NSLP header seems to be ok, false if not a NSLP header
 */
inline
bool
nslp_pdu::decode_common_header_nslpv1_clen(NetMsg& m, uint32& clen_words)
{
	clen_words = ntohl(*(reinterpret_cast<uint32 *>(m.get_buffer())));
	clen_words = (clen_words - (sizeof(uint32)/4));
	if (clen_words)
		return true;
	else
		return false;
}

/***** class known_nslp_pdu *****/

/***** inherited from IE *****/

/** Register this nslp_pdu with current type. */
inline
void known_nslp_pdu::register_ie(IEManager* iem) const
{
	iem->register_ie(category, type, 0, this);
}

/***** new members *****/

inline
nslp_pdu::~nslp_pdu()
{
}

/** set PDU type.
 * @param t message type
 */
inline
known_nslp_pdu::known_nslp_pdu(type_t t, const char *const name, uint8 s_f, uint16 g_f)
  : nslp_pdu(true, t, name, s_f, g_f)
{
}

inline
known_nslp_pdu::known_nslp_pdu(const known_nslp_pdu& n) : nslp_pdu(n)
{
}

/** @return type of the NSLP PDU. */
inline uint8 nslp_pdu::get_type() const { return type; }

/** @return generic flags of the NSLP PDU. */
inline uint16 nslp_pdu::get_generic_flags() { return generic_flags; }

/** @return specific flags of the NSLP PDU. */
inline uint8 nslp_pdu::get_specific_flags() { return specific_flags; }

/** @set flags of the NSLP PDU. */

inline void nslp_pdu::set_generic_flags(uint16 f) { generic_flags = f; }

// XXX: We can't be certain to have the right type set when calling
// set_specific_flags.  Thus we can't figure if the break flag is set
// or not.  There is a lot of special casing for this one in the .cpp
inline void nslp_pdu::set_specific_flags(uint8 f) { specific_flags = f; }
inline void nslp_pdu::set_replace_flag() { specific_flags |= RESV_FLAG_REPLACE; }
inline void nslp_pdu::set_acknowledge_flag() { generic_flags |= GEN_FLAG_ACKNOWLEDGE; }
inline void nslp_pdu::set_scoping_flag() { generic_flags |= GEN_FLAG_SCOPING; }
inline void nslp_pdu::set_tear_flag() { specific_flags |= RESV_FLAG_TEAR; }
inline void nslp_pdu::set_reserve_init_flag() { specific_flags |= QUERY_FLAG_RESERVE_INIT; }
inline void nslp_pdu::set_proxy_flag() { generic_flags |= GEN_FLAG_PROXY; } 
inline void nslp_pdu::set_req_reduced_refresh_flag() { specific_flags |= RESV_FLAG_REQ_REDUCED_REFRESH; } 
inline void nslp_pdu::set_break_flag() { break_flag_set = true; }
//inline void nslp_pdu::set_x_flag() { specific_flags |= QUERY_FLAG_X; } 
inline bool nslp_pdu::is_replace_flag() const { return (specific_flags & RESV_FLAG_REPLACE); }
inline bool nslp_pdu::is_acknowledge_flag() const { return (generic_flags & GEN_FLAG_ACKNOWLEDGE); }
inline bool nslp_pdu::is_scoping_flag() const { return (generic_flags & GEN_FLAG_SCOPING); }
inline bool nslp_pdu::is_tear_flag() const { return (specific_flags & RESV_FLAG_TEAR); }
inline bool nslp_pdu::is_reserve_init_flag() const { return (specific_flags & QUERY_FLAG_RESERVE_INIT); }
inline bool nslp_pdu::is_proxy_flag() const { return (generic_flags & GEN_FLAG_PROXY); }
inline bool nslp_pdu::is_req_reduced_refresh_flag() const { return (specific_flags & RESV_FLAG_REQ_REDUCED_REFRESH); }
inline bool nslp_pdu::is_break_flag() const { return break_flag_set; }
//inline bool nslp_pdu::is_x_flag() const { return (is_query() ? (specific_flags & QUERY_FLAG_X) : false); }

/** true if this is a RESERVE PDU. */
inline bool known_nslp_pdu::is_reserve() const { return (type == RESERVE); }

/** true if this is a QUERY PDU. */
inline bool known_nslp_pdu::is_query() const { return (type == QUERY); }

/** true if this is a RESPONSE PDU. */
inline bool known_nslp_pdu::is_response() const { return (type == RESPONSE); }

/** true if this is a NOTIFY PDU. */
inline bool known_nslp_pdu::is_notify() const { return (type == NOTIFY); }


inline void known_nslp_pdu::set_x_flag() { if(is_query()) { specific_flags |= QUERY_FLAG_X; }
					   if(is_notify()) { specific_flags |= NOTIFY_FLAG_X; } } 
inline bool known_nslp_pdu::is_x_flag() const { return ( is_query() ? (specific_flags & QUERY_FLAG_X) : (is_notify() ? specific_flags & NOTIFY_FLAG_X : false) ); }
//@}

} // end namespace qos_nslp

#endif // _NSLP__NSLPPDU_H_
