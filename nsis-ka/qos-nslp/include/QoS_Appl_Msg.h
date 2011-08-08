/// ----------------------------------------*- mode: C++; -*--
/// @file QoS_Appl_Msg.h
/// QoS NSLP Application Message
/// ----------------------------------------------------------
/// $Id: QoS_Appl_Msg.h 6211 2011-06-03 19:58:27Z bless $
/// $HeadURL: https://svn.tm.kit.edu/nsis/qos-nslp/trunk/include/QoS_Appl_Msg.h $
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
/** @ingroup signaling_appl
 * @file
 * Internal QoS NSLP API Messages
 */

#ifndef _QOS_APPL_MSG_H_
#define _QOS_APPL_MSG_H_

#include "protlib_types.h"
#include "address.h"
#include "messages.h"
#include "threads.h"
#include "logfile.h"

#include "qspec.h"
#include "nslp_pdu.h"
#include "vlsp_object.h"

using namespace protlib::log;
using namespace protlib;

namespace qos_nslp {


/// signaling message
/** Messages of this type are exchanged between the signaling module and
 * the coordination module.
 */
class QoS_Appl_Msg : public message {
public:
	// static version number
	static const uint32 version_magic;

	enum qos_info_type_t {
		not_set,
		type_response_for_rii_not_received,
		type_not_enough_resources
	}; // end qos_info_type_t

	/// default constructor
	QoS_Appl_Msg();
        /// copy constructor
	QoS_Appl_Msg(const QoS_Appl_Msg& qam);

	/// destructor
	virtual ~QoS_Appl_Msg();

	/// set sid
	void set_sid(uint128 r) { 
		sig_sid.w1 = r.w1;
		sig_sid.w2 = r.w2;
		sig_sid.w3 = r.w3;
		sig_sid.w4 = r.w4;
	}
	/// get sid
	void get_sid(uint128& r) { 
		r.w1 = sig_sid.w1;
		r.w2 = sig_sid.w2;
		r.w3 = sig_sid.w3;
		r.w4 = sig_sid.w4;
	}
	// @{
	/// set up qspec
	void set_qspec(const qspec::qspec_pdu* set_q);
	const qspec::qspec_pdu* get_qspec() const;
	// @}
	void set_info_spec(const info_spec* in_sp);
	const info_spec* get_info_spec() const;

	/// get info type
	qos_info_type_t get_info_type() const;
	/// set info type
	void set_info_type(qos_info_type_t i_type);

	void set_replace_flag(bool b);
	void set_acknowledge_flag(bool b);
	void set_scoping_flag(bool b);
	void set_tear_flag(bool b);
	void set_reserve_init_flag(bool b);
	bool get_replace_flag() const;
	bool get_acknowledge_flag() const;
	bool get_scoping_flag() const;
	bool get_tear_flag() const;
	bool get_reserve_init_flag() const;

	void set_direction(bool b)
	{
		direction = b;
	}
	bool get_direction() const
	{
		return direction;
	}
	void set_request_response_with_rii(bool b)
	{
		set_rii = b;
	}
	bool get_request_response_with_rii() const
	{
		return set_rii;
	}
	void set_source_addr(const appladdress& addr);
	appladdress get_source_addr() const;
	void set_dest_addr(const appladdress& addr);
	appladdress get_dest_addr() const;
	void set_reliable(bool reliability);
	bool get_reliable() const;
	void set_secure(bool security);
	bool get_secure() const;
	void set_pdu_type(qos_nslp::known_nslp_pdu::type_t t);
	qos_nslp::known_nslp_pdu::type_t get_pdu_type() const;

	void set_vlsp_object(const vlsp_object* vlspobj) { delete vlsp_objectp; vlsp_objectp= vlspobj ? vlspobj->copy() : NULL; }
	const vlsp_object* get_vlsp_object() const { return vlsp_objectp; }

	void set_session_auth_object_hmac_signed(const session_auth_object* sauthobj) { delete session_auth_objectp_hmac_signed; session_auth_objectp_hmac_signed= sauthobj ? sauthobj->copy() : NULL; }
	const session_auth_object* get_session_auth_object_hmac_signed() const { return session_auth_objectp_hmac_signed; }

	void set_session_auth_object(const session_auth_object* sauthobj) { delete session_auth_objectp; session_auth_objectp= sauthobj ? sauthobj->copy() : NULL; }
	const session_auth_object* get_session_auth_object() const { return session_auth_objectp; }

	/// clear pointers
	virtual void clear_pointers(bool delete_objects= true);
	//@{
	/// send message to coordination or signaling module
	bool send(bool exp = false);
	void send_or_delete(bool exp = false) { if (!send(exp)) delete this; }
	//@}
private:
	vlsp_object* vlsp_objectp;
	session_auth_object* session_auth_objectp;
	session_auth_object* session_auth_objectp_hmac_signed;
	qspec::qspec_pdu* q;
	qos_info_type_t info_type;
	info_spec* infospec;
	bool replace, acknowledge, scoping, tear, reserve_init;
	bool direction;
	bool set_rii;
	uint128 sig_sid;
	appladdress source_addr;
	appladdress dest_addr;
	bool reliable;
	bool secure;
	qos_nslp::known_nslp_pdu::type_t pdu_type;

}; // end class QoS_Appl_Msg


/** Destructor for QoS_Appl_Msg.
  */
inline 
QoS_Appl_Msg::~QoS_Appl_Msg() {
	delete q; 
	delete infospec;
	delete vlsp_objectp;
	delete session_auth_objectp;
	delete session_auth_objectp_hmac_signed;
} // end destructor

/// get info type
inline 
QoS_Appl_Msg::qos_info_type_t QoS_Appl_Msg::get_info_type() const
{
	return info_type;
}
   
/// set info type
inline 
void QoS_Appl_Msg::set_info_type(qos_info_type_t i_type)
{
	info_type = i_type;
}

/** Get function for INFOSPEC object of the current QoS_Appl_Msg.
  * @returns the INFOSPEC object of the current QoS_Appl_Msg.
  */
inline 
const info_spec* QoS_Appl_Msg::get_info_spec() const
{
	return infospec;
}

/** Set function for QSPEC object of the current QoS_Appl_Msg.
  * @param set_q the QSPEC object that will be copied into the current QoS_Appl_Msg
  */
inline 
void QoS_Appl_Msg::set_qspec(const qspec::qspec_pdu* set_q) {
	delete q; q = set_q ? set_q->copy() : NULL;
}

/** Get function for QSPEC object of the current QoS_Appl_Msg.
  * @returns the QSPEC object of the current QoS_Appl_Msg.
  */
inline 
const qspec::qspec_pdu* QoS_Appl_Msg::get_qspec() const
{
	return q;
}

inline 
void QoS_Appl_Msg::set_replace_flag(bool b)
{
	replace = b;
}

inline 
void QoS_Appl_Msg::set_acknowledge_flag(bool b)
{
	acknowledge = b;
}

inline 
void QoS_Appl_Msg::set_scoping_flag(bool b)
{
	scoping = b;
}

inline 
void QoS_Appl_Msg::set_tear_flag(bool b)
{
	tear = b;
}

inline 
void QoS_Appl_Msg::set_reserve_init_flag(bool b)
{
	reserve_init = b;
}

inline 
bool QoS_Appl_Msg::get_replace_flag() const
{
	return replace;
}

inline 
bool QoS_Appl_Msg::get_acknowledge_flag() const
{
	return acknowledge;
}

inline 
bool QoS_Appl_Msg::get_scoping_flag() const
{
	return scoping;
}

inline 
bool QoS_Appl_Msg::get_tear_flag() const
{
	return tear;
}

inline 
bool QoS_Appl_Msg::get_reserve_init_flag() const
{
	return reserve_init;
}

/** Set function for the QoS application's reliability request (i.e.,
 * udp, tcp, or sctp) regarding the GIST message transport.
 * @param reliability the QoS application's reliability request regarding
 * the GIST message transport (translates to UDP, TCP, or SCTP
 * internally).
 */
inline void QoS_Appl_Msg::set_reliable(bool reliability)
{
	reliable = reliability;
}
	
/** Get function for the QoS application's reliability request (i.e.,
 * udp, tcp, or sctp) regarding the GIST message transport.
 * @return the QoS application's reliability request regarding
 * the GIST message transport (translates to UDP, TCP, or SCTP
 * internally).
 */
inline bool QoS_Appl_Msg::get_reliable() const
{
	return reliable;
}

/** Set function for the QoS application's security request (i.e.,
 * tcp with tls) regarding the GIST message transport.
 * @param security the QoS application's security request regarding
 * the GIST message transport (translates to TCP with TLS internally).
 */
inline void QoS_Appl_Msg::set_secure(bool security)
{
	secure = security;
}
	
/** Get function for the QoS application's security request (i.e.,
 * tcp with tls) regarding the GIST message transport.
 * @return security the QoS application's security request regarding
 * the GIST message transport (translates to TCP with TLS internally).
 */
inline bool QoS_Appl_Msg::get_secure() const
{
	return secure;
}

} // end namespace qos_nslp

#endif // _QOS_APPL_MSG_H_
