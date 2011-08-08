/// ----------------------------------------*- mode: C++; -*--
/// @file natfw_notify.cpp
/// Implementation of the NATFW NOTIFY Message.
/// ----------------------------------------------------------
/// $Id: natfw_notify.cpp 2558 2007-04-04 15:17:16Z bless $
/// $HeadURL: https://svn.tm.kit.edu/nsis/natfw-nslp/trunk/src/msg/natfw_notify.cpp $
// ===========================================================
//                      
// Copyright (C) 2005-2007, all rights reserved by
// - Institute of Telematics, Universitaet Karlsruhe (TH)
//
// More information and contact:
// https://projekte.tm.uka.de/trac/NSIS
//                      
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; version 2 of the License
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
//
// ===========================================================
#include "logfile.h"

#include "msg/natfw_ie.h"
#include "msg/natfw_notify.h"
#include "msg/natfw_object.h"


using namespace natfw::msg;
using namespace protlib::log;


/**
 * Constructor.
 *
 * Only basic initialization is done. No NATFW objects exist yet. All other
 * attributes are set to default values.
 */
natfw_notify::natfw_notify() : natfw_msg(MSG_TYPE, false) {
	// nothing to do
}


/**
 * Copy constructor.
 *
 * Makes a deep copy of the object passed as an argument.
 *
 * @param other the object to copy
 */
natfw_notify::natfw_notify(const natfw_notify &other)
		: natfw_msg(other) {
	
	// nothing else to do
}


/**
 * Destructor.
 *
 * Deletes all objects this message contains.
 */
natfw_notify::~natfw_notify() {
	// Nothing to do, parent class handles this.
}


natfw_notify *natfw_notify::new_instance() const {
	natfw_notify *inst = NULL;
	catch_bad_alloc(inst = new natfw_notify());
	return inst;
}


natfw_notify *natfw_notify::copy() const {
	natfw_notify *copy = NULL;
	catch_bad_alloc(copy = new natfw_notify(*this));
	return copy;
}


/* TODO: implement the check() method.
bool natfw_msg::check() const {
	// proxy mode is not allowed for NOTIFY messages
	if ( is_proxy_mode() )
		return false;
}
*/

void natfw_notify::register_ie(IEManager *iem) const {
	iem->register_ie(cat_natfw_msg, get_msg_type(), 0, this);
}


/**
 * Sets the information code.
 *
 * This is used to indicate success or various degrees of failure.
 *
 * @param severity the severity class (4 bit)
 * @param response_code a response code, depending on the severity class
 * @param object_type the object type the response is about (12 bit)
 */
void natfw_notify::set_information_code(uint8 severity, uint8 response_code,
                uint16 object_type) {

	set_object(new information_code(severity, response_code, object_type,
		natfw_object::tr_mandatory));
}


/**
 * Return the object type that triggered this response message.
 *
 * @return a NATFW object type (12 bit)
 */
uint16 natfw_notify::get_object_type() const {
	information_code *ic = dynamic_cast<information_code *>(
		get_object(information_code::OBJECT_TYPE));

	if ( ic == NULL )
		return 0; // TODO: use a default constant here?
	else
		return ic->get_object_type();
}


/**
 * Return the severity class code.
 *
 * @return the severity class as defined in information_code
 */
uint8 natfw_notify::get_severity_class() const {
	information_code *ic = dynamic_cast<information_code *>(
		get_object(information_code::OBJECT_TYPE));

	if ( ic == NULL )
		return 0; // TODO: use a default constant here?
	else
		return ic->get_severity_class();
}

// EOF
