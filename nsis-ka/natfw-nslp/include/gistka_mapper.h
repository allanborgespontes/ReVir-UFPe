/// ----------------------------------------*- mode: C++; -*--
/// @file gistka_mapper.h
/// Utilities for mapping between GISTka/protlib and NATFW. 
/// ----------------------------------------------------------
/// $Id: gistka_mapper.h 2558 2007-04-04 15:17:16Z bless $
/// $HeadURL: https://svn.tm.kit.edu/nsis/natfw-nslp/trunk/include/gistka_mapper.h $
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
#ifndef NATFW__GISTKA_MAPPER_H
#define NATFW__GISTKA_MAPPER_H

#include "timer_module.h"

#include "apimessage.h"		// from NTLP

#include "events.h"
#include "natfw_timers.h"
#include "msg/ntlp_msg.h"


namespace natfw {

/**
 * Utilities for mapping between GISTka/protlib and NATFW. 
 *
 * This class maps GISTka's APIMsg objects to NATFW events, creates APIMsg
 * objects from a given ntlp_msg etc.
 *
 * It provides functionality used in the dispatcher but has been factored out
 * to keep the dispatcher small and clean.
 */
class gistka_mapper {
	
  public:
	event *map_to_event(const protlib::message *msg) const;

	ntlp::APIMsg *create_api_msg(msg::ntlp_msg *msg) const throw ();

	ntlp::sessionid *create_ntlp_session_id(const session_id &sid) const;
	session_id *create_natfw_session_id(ntlp::sessionid *sid) const;

  private:
	event *map_api_message(const ntlp::APIMsg *msg) const;
	event *map_api_receive_message(const ntlp::APIMsg *msg) const;
	event *map_api_network_notification(const ntlp::APIMsg *msg) const;
	event *map_api_message_status(const ntlp::APIMsg *msg) const;
	event *map_timer_message(const NatFwTimerMsg *msg) const;
};


} // namespace natfw

#endif // NATFW__GISTKA_MAPPER_H
