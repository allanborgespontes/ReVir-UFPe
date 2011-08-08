/// ----------------------------------------*- mode: C++; -*--
/// @file responsemsg.h
/// QoS NSLP Response Message
/// ----------------------------------------------------------
/// $Id: responsemsg.h 6275 2011-06-17 07:16:47Z bless $
/// $HeadURL: https://svn.tm.kit.edu/nsis/qos-nslp/trunk/include/responsemsg.h $
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
/** @ingroup ieresponsemsg
 * @file
 * NSLP ResponseMsg
 */

#ifndef _NSLP__RESPONSEMSG_H_
#define _NSLP__RESPONSEMSG_H_

#include "protlib_types.h"
#include "nslp_ie.h"
#include "nslp_pdu.h"
#include "rsn.h"
#include "packet_classifier.h"
#include "rii.h"
#include "info_spec.h"
#include "qspec.h"
#include "vlsp_object.h"

using namespace protlib;

namespace qos_nslp {

/** @addtogroup ieresponsemsg ResponseMsg
 * @ingroup ienslppdu
 * @{
 */

/// NSLP ResponseMsg
class responsemsg : public known_nslp_pdu {
public:
/***** inherited from IE *****/
	virtual responsemsg* new_instance() const;
	virtual responsemsg* copy() const;
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
	responsemsg();
	responsemsg(
		info_spec* error_object,
		rsn* req_seq_num = NULL,
		rii* req_id_info = NULL,
		qspec_object* q_spec_1 = NULL,
	 	nslp_auth::session_auth_object* session_auth = NULL,
	 	nslp_auth::session_auth_object* session_auth_hmac_signed = NULL,
		vlsp_object* vlsp_obj= NULL
	); // end constructor
	// @}
private:
	static const char* const iename;
	/// deserialization state
	enum deser_state_t {
		wait_nextref,
		deser_done
	} state;
}; // end class responsemsg

//@}

} // end namespace qos_nslp

#endif // _NSLP__RESPONSEMSG_H_
