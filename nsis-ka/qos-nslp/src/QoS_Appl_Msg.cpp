/// ----------------------------------------*- mode: C++; -*--
/// @file QoSApplMsg.cpp
/// QoS NSLP Application Message
/// ----------------------------------------------------------
/// $Id: QoS_Appl_Msg.cpp 6211 2011-06-03 19:58:27Z bless $
/// $HeadURL: https://svn.tm.kit.edu/nsis/qos-nslp/trunk/src/QoS_Appl_Msg.cpp $
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
/** @file
 * This Signaling Application allows to send PDUs and manage delivered information.
 */

#include <unistd.h>
#include <iostream>
#include <string>
#include <sstream>
#include <netdb.h>

#include "QoS_Appl_Msg.h"
#include "sessionid.h"
#include "messages.h"
#include "queuemanager.h"
#include "logfile.h"
#include "info_spec.h"

#include "qspec_ie.h"
#include "qspec_pdu.h"
#include "qspec_param.h"

using namespace protlib::log;
using namespace protlib;
using namespace qspec;

namespace qos_nslp   {

/***** class QoS-Appl-Msg *****/
  
/** 
 * Default Constructor for QoS_Appl_Msg without any params. All values of QoS_Appl_Msg will be set to the default.
 */
QoS_Appl_Msg::QoS_Appl_Msg() : message(type_signaling,message::qaddr_qos_appl_signaling),
			       vlsp_objectp(NULL),
			       session_auth_objectp(NULL),
			       session_auth_objectp_hmac_signed(NULL),
			       q(NULL),
			       info_type(not_set),
			       infospec(NULL),
			       replace(false),
			       acknowledge(false),
			       scoping(false),
			       tear(false),
			       reserve_init(false),
			       direction(true),
			       set_rii(false),
			       reliable(false),
			       secure(false),
			       pdu_type(known_nslp_pdu::UNDEFINED)
{
} 


/// copy constructor
QoS_Appl_Msg::QoS_Appl_Msg(const QoS_Appl_Msg& qam) : message(qam),
						      vlsp_objectp(qam.vlsp_objectp ? qam.vlsp_objectp->copy() : NULL),
						      session_auth_objectp(qam.session_auth_objectp ? qam.session_auth_objectp->copy() : NULL),
						      session_auth_objectp_hmac_signed(qam.session_auth_objectp_hmac_signed ? qam.session_auth_objectp_hmac_signed->copy() : NULL),
						      q(qam.q ? qam.q->copy() : NULL),
						      info_type(qam.info_type),
						      infospec(qam.infospec ? qam.infospec->copy() : NULL),
						      replace(qam.replace),
						      acknowledge(qam.acknowledge),
						      scoping(qam.scoping),
						      tear(qam.tear),
						      reserve_init(qam.reserve_init),
						      direction(qam.direction),
						      set_rii(qam.set_rii),
						      sig_sid(qam.sig_sid),
						      source_addr(qam.source_addr),
						      dest_addr(qam.dest_addr),
						      reliable(qam.reliable),
						      secure(qam.secure),
						      pdu_type(qam.pdu_type)
{
} 



/** Set function for INFO_SPEC object of the current QoS_Appl_Msg.
  * @param in_sp the value of the INFO_SPEC of the current QoS_Appl_Msg will be set to the value of this variable.
  */
void QoS_Appl_Msg::set_info_spec(const info_spec* in_sp)
{
	delete infospec;
	infospec = (in_sp != NULL) ? in_sp->copy() : NULL;
}
	
/** Set function for the source address of the current QoS_Appl_Msg.
  * @param addr the source address of the current QoS_Appl_Msg will be set to the value of this variable.
  */
void QoS_Appl_Msg::set_source_addr(const appladdress& addr)
{
	source_addr = addr;
}
	
/** Get function for the source address of the current QoS_Appl_Msg.
  * @return the source address of the current QoS_Appl_Msg.
  */
appladdress QoS_Appl_Msg::get_source_addr() const
{
	return source_addr;
}
	
/** Set function for the destination address of the current QoS_Appl_Msg.
  * @param addr the destination address of the current QoS_Appl_Msg will be set to the value of this variable.
  */
void QoS_Appl_Msg::set_dest_addr(const appladdress& addr)
{
	dest_addr = addr;
}
	
/** Get function for the destination address of the current QoS_Appl_Msg.
  * @return the destination address of the current QoS_Appl_Msg.
  */
appladdress QoS_Appl_Msg::get_dest_addr() const
{
	return dest_addr;
}

/** Set function for the PDU type of the current QoS_Appl_Msg.
  * @param t the PDU type of the current QoS_Appl_Msg will be set to the value of this variable.
  */
void QoS_Appl_Msg::set_pdu_type(qos_nslp::known_nslp_pdu::type_t t)
{
	pdu_type = t;
}
	
/** Get function for the PDU type of the current QoS_Appl_Msg.
  * @return the PDU type of the current QoS_Appl_Msg.
  */
qos_nslp::known_nslp_pdu::type_t QoS_Appl_Msg::get_pdu_type() const
{
	return pdu_type;
}



/** Clear all pointers.
 * @param deletobjs if set to true will also delete all objects
 * */
void QoS_Appl_Msg::clear_pointers(bool deleteobjs) {
	if (deleteobjs)
	{
		delete q; 
		delete infospec;
		delete vlsp_objectp;
		delete session_auth_objectp;
		delete session_auth_objectp_hmac_signed;
	}
	q = NULL;
	infospec = NULL;
	vlsp_objectp = NULL;
	session_auth_objectp = NULL;
	session_auth_objectp_hmac_signed = NULL;
} // end clear_pointers


/** this function sends the current  QoS_Appl_Msg to appropriate queue.
  */
bool QoS_Appl_Msg::send(bool exp) {
	qaddr_t s = get_source();

	if (s == qaddr_qos_appl_signaling) {
		DLog("QoS ApplMsg", "Sending to qaddr_appl_qos_signaling");
		return send_to(qaddr_appl_qos_signaling,exp);
	}

	if (s == qaddr_appl_qos_signaling) {
		DLog("QoS ApplMsg", "Sending to qaddr_qos_appl_signaling");
		return send_to(qaddr_qos_appl_signaling,exp);
	}

	ERRLog("QoS ApplMsg Signaling", "Unknown qaddr");
	return false;
} // end send


}  //end namespace qos_nslp

//@}

