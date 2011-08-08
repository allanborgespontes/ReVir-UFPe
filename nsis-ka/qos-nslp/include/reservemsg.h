/// ----------------------------------------*- mode: C++; -*--
/// @file reservemsg.h
/// QoS NSLP Reserve Message
/// ----------------------------------------------------------
/// $Id: reservemsg.h 6187 2011-05-23 12:27:11Z bless $
/// $HeadURL: https://svn.tm.kit.edu/nsis/qos-nslp/trunk/include/reservemsg.h $
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
/** @ingroup iereservemsg
 * @file
 * NSLPP ReserveReq
 */

#ifndef _NSLP__RESERVEMSG_H_
#define _NSLP__RESERVEMSG_H_

#include "protlib_types.h"
#include "nslp_ie.h"
#include "nslp_pdu.h"
#include "rsn.h"
#include "packet_classifier.h"
#include "rii.h"
#include "refresh_period.h"
#include "bound_sessionid.h"
#include "qspec.h"

using namespace protlib;

namespace qos_nslp {

/** @addtogroup iereservemsg ReserveReq
 * @ingroup ienslppdu
 * @{
 */

/// NSLP ReserveReq
class reservereq : public known_nslp_pdu {
public:
/***** inherited from IE *****/
	virtual reservereq* new_instance() const;
	virtual reservereq* copy() const;
	virtual bool operator==(const IE& ie) const;
	virtual const char* get_ie_name() const;
	virtual ostream& print(ostream& os, uint32 level, const uint32 indent, const char* name = NULL) const;
	virtual istream& input(istream& is, bool istty, uint32 level, const uint32 indent, const char* name = NULL);
	virtual void clear_pointers();
/***** inherited from nslp_pdu *****/
//protected:
	virtual deser_error_t accept_type(uint16 t);
	virtual bool accept_object(known_nslp_object* o);
	virtual bool deser_end(IEErrorList& errorlist);
protected:
	virtual bool build_obj_list() const;
/***** new members *****/
public:
	// @{
	/// constructor
	reservereq();
	reservereq(
		rsn* req_seq_num,
		packet_classifier* pac_cla,
		rii* req_id_info = NULL,
		rp* ref_period = NULL,
		bound_sessionid* bound_session_id = NULL,
		qspec_object* q_spec_1 = NULL,
		nslp_auth::session_auth_object* session_auth = NULL,
		nslp_auth::session_auth_object* session_auth_hmac_signed = NULL,
		vlsp_object* vlsp_obj= NULL
	); // end constructor
	// @}

	void set_anticipated_reservation(bool val);
	bool get_anticipated_reservation();
private:
	static const char* const iename;

	/// deserialization state
	enum deser_state_t {
		wait_rsnref,
		wait_nextref,
		deser_done
	} state;

	bool anticipated_reservation;
}; // end class reservereq

//@}

} // end namespace qos_nslp

#endif // _NSLP__RESERVEMSG_H_
