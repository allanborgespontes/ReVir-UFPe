/// ----------------------------------------*- mode: C++; -*--
/// @file session.cpp
/// The session class.
/// ----------------------------------------------------------
/// $Id: session.cpp 2558 2007-04-04 15:17:16Z bless $
/// $HeadURL: https://svn.tm.kit.edu/nsis/natfw-nslp/trunk/src/session.cpp $
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

#include "logfile.h"

#include "session.h"
#include "dispatcher.h"


using namespace natfw;
using namespace protlib::log;
using protlib::uint32;


/**
 * Constructor.
 *
 * A random session ID is created and the message sequence number is set to 0.
 * Additionally, the mutex is initialized.
 */
session::session() : id(), msn(0) {
	init();
}


/**
 * Constructor.
 *
 * Like the default constructor, but uses the given session id and msgn instead
 * of generating them.
 *
 * @param sid a hopefully unique session id
 */
session::session(const session_id &sid) : id(sid), msn(0) {
	init();
}


/**
 * Destructor.
 */
session::~session() {
	pthread_mutex_destroy(&mutex);
}


/**
 * A helper method for the constructors, to avoid code duplication.
 */
void session::init() {
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
 * Process the given event.
 *
 * This method calls the user-defined process_event method and takes care of
 * locking.
 */
void session::process(dispatcher *d, event *evt) {
	typedef void (*cleanup_t)(void *);
	int ret;

	pthread_cleanup_push((cleanup_t) pthread_mutex_unlock, (void *) &mutex);
	ret = pthread_mutex_lock(&mutex);
	assert( ret == 0 );

	process_event(d, evt);	// implemented by child classes

	ret = pthread_mutex_unlock(&mutex);
	assert( ret == 0 );
	pthread_cleanup_pop(0);
}


/**
 * Increment the Message Sequence Number.
 *
 * @return the new (incremented) MSN
 */
uint32 session::next_msg_sequence_number() {
	return ++msn;	// UINT_MAX+1 = 0, so wrap arounds as per RFC-1982 work
}


/****************************************************************************
 *
 * Utilities
 *
 ****************************************************************************/


void session::check_lifetime(uint32 lifetime, uint32 max_lifetime)
		const throw (request_error) {

	if ( lifetime > max_lifetime )
		throw request_error("requested session lifetime too long",
			information_code::sc_signaling_session_failures,
			information_code::sigfail_lifetime_too_big);
}


void session::check_authorization(dispatcher *d, const msg_event *e)
		const throw (request_error) {

	if ( ! d->is_authorized(e) )
		throw request_error("authorization/authentication failed",
			information_code::sc_permanent_failure,
			information_code::fail_authentication_failed);
}


/**
 * Get the NR/NI+'s address and port.
 */
appladdress session::get_nr_address(msg_event *e) const {
	using namespace ntlp;

	/*
	 * If this is path-coupled MRM, we get the NR's port number from
	 * the MRI.
	 */
	mri_pathcoupled *mri_pc = dynamic_cast<mri_pathcoupled *>(e->get_mri());

	if ( mri_pc != NULL )
		return appladdress(mri_pc->get_destaddress(),
			mri_pc->get_protocol(), mri_pc->get_destport());

	/*
	 * If it's loose-end MRM, we have to get the port and protocol numbers
	 * from the data terminal info object.
	 */
	mri_looseend *mri_le = dynamic_cast<mri_looseend *>(e->get_mri());

	if ( mri_le != NULL ) {
		natfw_ext *ext = e->get_ext();
		assert( ext != NULL );

		return appladdress(mri_le->get_destaddress(),
				ext->get_protocol(), ext->get_dr_port());
	}


	/*
	 * Unknown routing method.
	 */
	throw request_error("unknown MRM: " + int(e->get_mri()->get_mrm()));
}


// EOF
