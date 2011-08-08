/// ----------------------------------------*- mode: C++; -*--
/// @file querymsg.h
/// QoS NSLP Query Message
/// ----------------------------------------------------------
/// $Id: querymsg.h 6159 2011-05-18 15:24:52Z bless $
/// $HeadURL: https://svn.tm.kit.edu/nsis/qos-nslp/trunk/include/querymsg.h $
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
/** @ingroup iequerymsg
 * @file
 * NSLP QueryMsg
 */

#ifndef _NSLP__QUERYMSG_H_
#define _NSLP__QUERYMSG_H_

#include "protlib_types.h"
#include "nslp_ie.h"
#include "nslp_pdu.h"
#include "packet_classifier.h"
#include "rii.h"
#include "bound_sessionid.h"
#include "qspec.h"

using namespace protlib;

namespace qos_nslp {

/** @addtogroup iequerymsg QueryMsg
 * @ingroup ienslppdu
 * @{
 */

/// NSLP QueryMsg
class querymsg : public known_nslp_pdu {
public:
/***** inherited from IE *****/
	virtual querymsg* new_instance() const;
	virtual querymsg* copy() const;
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
	querymsg();
	querymsg(
	        packet_classifier* pac_cla,
		rii* req_id_info = NULL,
		bound_sessionid* bound_session_id = NULL,
		qspec_object* q_spec_1 = NULL,
		nslp_auth::session_auth_object* session_auth = NULL
	); // end constructor
	// @}
private:
	static const char* const iename;
	/// deserialization state
	enum deser_state_t {
		wait_nextref,
		deser_done
	} state;
}; // end class querymsg

//@}

} // end namespace qos_nslp

#endif // _NSLP__QUERYMSG_H_
