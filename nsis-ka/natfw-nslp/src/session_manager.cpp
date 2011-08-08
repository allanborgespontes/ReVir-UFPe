/// ----------------------------------------*- mode: C++; -*--
/// @file session_manager.cpp
/// The session_manager class.
/// ----------------------------------------------------------
/// $Id: session_manager.cpp 2558 2007-04-04 15:17:16Z bless $
/// $HeadURL: https://svn.tm.kit.edu/nsis/natfw-nslp/trunk/src/session_manager.cpp $
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
#include <assert.h>
#include <openssl/rand.h>

#include "logfile.h"

#include "session.h"
#include "session_manager.h"


using namespace natfw;
using namespace protlib::log;
using protlib::uint32;


#define LogError(msg) ERRLog("session_manager", msg)
#define LogWarn(msg) WLog("session_manager", msg)
#define LogInfo(msg) ILog("session_manager", msg)
#define LogDebug(msg) DLog("session_manager", msg)


#define install_cleanup_handler(m) \
    pthread_cleanup_push((void (*)(void *)) pthread_mutex_unlock, (void *) m)

#define uninstall_cleanup_handler()	pthread_cleanup_pop(0);


/**
 * Contructor.
 */
session_manager::session_manager(natfw_config *conf)
		: config(conf), session_table(SESSION_TABLE_SIZE) {

	pthread_mutexattr_t mutex_attr;

	pthread_mutexattr_init(&mutex_attr);

#ifdef _DEBUG
	pthread_mutexattr_settype(&mutex_attr, PTHREAD_MUTEX_ERRORCHECK);
#else
	pthread_mutexattr_settype(&mutex_attr, PTHREAD_MUTEX_NORMAL);
#endif

	pthread_mutex_init(&mutex, &mutex_attr);

	pthread_mutexattr_destroy(&mutex_attr); // valid, doesn't affect mutex
}


/**
 * Destructor.
 *
 * Deletes all sessions in the session table.
 */
session_manager::~session_manager() {
	for ( c_iter i = session_table.begin(); i != session_table.end(); i++ )
		delete i->second;

	pthread_mutex_destroy(&mutex);
}


/**
 * Create a unique session ID.
 *
 * This only works in a multithreaded environment if the caller takes care
 * of locking.
 */
session_id session_manager::create_unique_id() const {
	session_id id;

	while ( session_table.find(id) != session_table.end() )
		id = session_id();

	return id;
}


/**
 * Creates an initiator session and adds it to the session table.
 */
ni_session *session_manager::create_ni_session() {
	ni_session *s;

	install_cleanup_handler(&mutex);
	pthread_mutex_lock(&mutex);

	s = new ni_session(create_unique_id(), config);
	session_table[s->get_id()] = s;

	LogInfo("created new NI session " << s->get_id());

	pthread_mutex_unlock(&mutex);
	uninstall_cleanup_handler();

	return s;
}


/**
 * Creates an initiator proxy session and adds it to the session table.
 *
 * @param the nonce to send along with each NATFW CREATE message
 */
ni_session *session_manager::create_ni_session(uint32 nonce) {

	ni_session *s = create_ni_session();
	s->set_nonce(nonce);

	return s;
}


/**
 * Creates a forwarder session and adds it to the session table.
 */
nf_session *session_manager::create_nf_session(const session_id &sid) {
	nf_session *s;

	install_cleanup_handler(&mutex);
	pthread_mutex_lock(&mutex);

	s = new nf_session(sid, config);
	session_table[s->get_id()] = s;

	LogInfo("created new NF session " << s->get_id());

	pthread_mutex_unlock(&mutex);
	uninstall_cleanup_handler();

	return s;
}


/**
 * Creates a responder session and adds it to the session table.
 */
nr_session *session_manager::create_nr_session(const session_id &sid) {
	nr_session *s;

	install_cleanup_handler(&mutex);
	pthread_mutex_lock(&mutex);

	s = new nr_session(sid, config);
	session_table[s->get_id()] = s;

	LogInfo("created new NR session " << s->get_id());

	pthread_mutex_unlock(&mutex);
	uninstall_cleanup_handler();

	return s;
}


/**
 * Creates an NI+ session and adds it to the session table.
 */
nr_ext_session *session_manager::create_nr_ext_session() {
	nr_ext_session *s;

	install_cleanup_handler(&mutex);
	pthread_mutex_lock(&mutex);

	s = new nr_ext_session(create_unique_id(), config);
	session_table[s->get_id()] = s;

	LogInfo("created new NR EXT session " << s->get_id());

	pthread_mutex_unlock(&mutex);
	uninstall_cleanup_handler();

	return s;
}


/**
 * Creates an edge NF EXT responder session and adds it to the session table.
 */
nf_edge_ext_session *session_manager::create_nf_edge_ext_session(
		const session_id &sid) {

	nf_edge_ext_session *s;

	install_cleanup_handler(&mutex);
	pthread_mutex_lock(&mutex);

	s = new nf_edge_ext_session(sid, config);
	session_table[s->get_id()] = s;

	LogInfo("created new edge NF EXT session " << s->get_id());

	pthread_mutex_unlock(&mutex);
	uninstall_cleanup_handler();

	return s;
}


/**
 * Creates a non-edge NF EXT responder session and adds it to the session table.
 */
nf_non_edge_ext_session *session_manager::create_nf_non_edge_ext_session(
		const session_id &sid) {

	nf_non_edge_ext_session *s;

	install_cleanup_handler(&mutex);
	pthread_mutex_lock(&mutex);

	s = new nf_non_edge_ext_session(sid, config);
	session_table[s->get_id()] = s;

	LogInfo("created new non-edge NF EXT session " << s->get_id());

	pthread_mutex_unlock(&mutex);
	uninstall_cleanup_handler();

	return s;
}


/**
 * Retrieve a session by its session ID.
 *
 * The returned session may be changed, but it is still stored in the session
 * table.
 *
 * Note that this method is almost a pure "const" implementation. Even if
 * the session isn't found, the session table won't be changed. This is in
 * contrast to the standard library's map implementation that inserts an
 * entry with a value of NULL if an entry isn't found. We do this to prevent
 * attackers from trying to overflow the session table.
 * However, we have to lock the session table, so the mutex variable is
 * modifed.
 *
 * @param sid the session ID
 * @return the session, or NULL if it isn't found
 */
session *session_manager::get_session(const session_id &sid) {
	session *s = NULL;

	install_cleanup_handler(&mutex);
	pthread_mutex_lock(&mutex);

	c_iter i = session_table.find(sid);

	if ( i != session_table.end() )
		s = i->second;

	pthread_mutex_unlock(&mutex);
	uninstall_cleanup_handler();

	return s;
}


/**
 * Remove a session from the session table.
 *
 * Remove the session with the given session ID from the session table. If
 * there is no session with this ID, NULL is returned.
 *
 * The returned session itself is *not* deleted.
 *
 * @param sid a session ID
 * @return the session, or NULL if it isn't found
 */
session *session_manager::remove_session(const session_id &sid) {
	session *s;

	install_cleanup_handler(&mutex);
	pthread_mutex_lock(&mutex);

	s = session_table[sid];

	// Note: session_table[sid] creates a session if none existed before
	session_table.erase(sid);

	LogInfo("removed session " << s->get_id());

	pthread_mutex_unlock(&mutex);
	uninstall_cleanup_handler();

	return s; // either the session or NULL
}


// EOF
