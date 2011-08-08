/// ----------------------------------------*- mode: C++; -*--
/// @file session_manager.h
/// The session manager.
/// ----------------------------------------------------------
/// $Id: session_manager.h 6282 2011-06-17 13:31:57Z bless $
/// $HeadURL: https://svn.tm.kit.edu/nsis/natfw-nslp/trunk/include/session_manager.h $
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
#ifndef NATFW__SESSION_MANAGER_H
#define NATFW__SESSION_MANAGER_H

#include "hashmap"

#include "protlib_types.h"

#include "session.h"
#include "ni_session.h"
#include "nf_session.h"
#include "nf_edge_ext_session.h"
#include "nf_non_edge_ext_session.h"
#include "nr_session.h"
#include "nr_ext_session.h"


namespace natfw {

/**
 * The session manager.
 *
 * The session manager is the interface to NATFW's session table. It can be
 * used to get a session using its session ID. This class also serves as a
 * session factory, because it can verify that a created session_id is really
 * unique on this node.
 *
 * Instances of this class are thread-safe.
 */
class session_manager {

  public:
	session_manager(natfw_config *conf);
	~session_manager();

	ni_session *create_ni_session();
	ni_session *create_ni_session(uint32 nonce);
	nf_session *create_nf_session(const session_id &sid);
	nf_edge_ext_session *create_nf_edge_ext_session(const session_id &sid);
	nf_non_edge_ext_session *create_nf_non_edge_ext_session(
		const session_id &sid);
	nr_session *create_nr_session(const session_id &sid);
	nr_ext_session *create_nr_ext_session();

	session *get_session(const session_id &sid);
	session *remove_session(const session_id &sid);

  private:
	pthread_mutex_t mutex;
	natfw_config *config; // shared by many objects, don't delete
	hashmap_t<session_id, session *> session_table;
	typedef hashmap_t<session_id, session *>::const_iterator c_iter;

	session_id create_unique_id() const;

	// Large initial size to avoid resizing of the session table.
	static const int SESSION_TABLE_SIZE = 500000;
};


} // namespace natfw

#endif // NATFW__SESSION_MANAGER_H
