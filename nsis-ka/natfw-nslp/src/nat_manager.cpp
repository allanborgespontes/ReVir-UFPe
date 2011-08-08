/// ----------------------------------------*- mode: C++; -*--
/// @file nat_manager.cpp
/// The nat_manager class.
/// ----------------------------------------------------------
/// $Id: nat_manager.cpp 2558 2007-04-04 15:17:16Z bless $
/// $HeadURL: https://svn.tm.kit.edu/nsis/natfw-nslp/trunk/src/nat_manager.cpp $
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

#include "address.h"
#include "logfile.h"

#include "nat_manager.h"
#include "msg/information_code.h"


using namespace natfw;
using natfw::msg::information_code;
using namespace protlib::log;
using protlib::uint32;


#define LogError(msg) ERRLog("nat_manager", msg)
#define LogWarn(msg) WLog("nat_manager", msg)
#define LogInfo(msg) ILog("nat_manager", msg)
#define LogDebug(msg) DLog("nat_manager", msg)

#define LogUnimp(msg) Log(ERROR_LOG, LOG_UNIMP, "nat_manager", \
	msg << " at " << __FILE__ << ":" << __LINE__)


#define install_cleanup_handler(m) \
    pthread_cleanup_push((void (*)(void *)) pthread_mutex_unlock, (void *) m)

#define uninstall_cleanup_handler()     pthread_cleanup_pop(0);


/**
 * Contructor.
 */
nat_manager::nat_manager(natfw_config *conf) throw () {
	assert( conf != NULL );

	if ( ! conf->is_nf_nat() ) {
		LogDebug("node is no NAT, leaving nat_manager uninitialized");
		return;
	}

	init();

	address = conf->get_nf_nat_public_address();
	uint16 begin = conf->get_nf_nat_public_port_begin();
	uint16 end = conf->get_nf_nat_public_port_end();
	
	fill_available(begin, end);

	LogInfo("external address pool: " << address << ", ports "
		<< begin << "-" << end);
}


/**
 * Contructor.
 */
nat_manager::nat_manager(const hostaddress &addr, uint16 start_port,
		uint16 stop_port) throw () : address(addr) {

	assert( start_port <= stop_port );

	init();

	fill_available(start_port, stop_port);
}


/**
 * Destructor.
 *
 * Deletes all reservations, but does *not* touch the external NAT.
 */
nat_manager::~nat_manager() throw () {
	pthread_mutex_destroy(&mutex);
}


/**
 * A helper method for the constructors, to avoid code duplication.
 */
void nat_manager::init() {
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


void nat_manager::fill_available(uint16 start, uint16 stop) {
	for ( int i = start; i <= stop; i++ )
		available.push(i);
}


/**
 * Reserve an external address and port from the address pool.
 *
 * The parameter has to be the application address (with a set IP address, 
 * port number, and protocol) of the application that is requesting the
 * reservation. In NATFW speak, this is the NI+.
 *
 * @param priv_addr the NI+'s address, port number, and protocol
 * 
 * @return a reserved external address
 */
appladdress nat_manager::reserve_external_address(const appladdress &priv_addr)
		throw (nat_manager_error) {

	bool address_reserved = false;
	appladdress ext_addr;
	uint16 port;

	install_cleanup_handler(&mutex);
	pthread_mutex_lock(&mutex);

	if ( ! available.empty() ) {
		port = available.front();
		available.pop();

		ext_addr = appladdress(address, priv_addr.get_protocol(), port);
		external_to_private[ext_addr] = priv_addr;

		address_reserved = true;
	}

	pthread_mutex_unlock(&mutex);
	uninstall_cleanup_handler();

	if ( ! address_reserved )
		throw nat_manager_error("no port number available",
			information_code::sc_transient_failure,
			information_code::tfail_resources_unavailable);

	return ext_addr;
}


/**
 * Release a previously reserved external address and port.
 */
void nat_manager::release_external_address(const appladdress &ext_addr)
		throw () {

	install_cleanup_handler(&mutex);
	pthread_mutex_lock(&mutex);

	available.push(ext_addr.get_port());
	external_to_private.erase(ext_addr);

	pthread_mutex_unlock(&mutex);
	uninstall_cleanup_handler();
}


/**
 * Return the private address for a reserved external address.
 *
 * This returns the corresponding private address and port for a given and
 * previously reserved external address. If the external address and port
 * are unknown to the NAT manager, a nat_manager_error exception is thrown.
 */
appladdress nat_manager::lookup_private_address(const appladdress &ext_addr)
		throw (nat_manager_error) {

	bool private_address_found = false;
	appladdress priv_addr;

	install_cleanup_handler(&mutex);
	pthread_mutex_lock(&mutex);

	LogDebug("lookup_private_address() " << ext_addr);

	address_map_t::const_iterator i = external_to_private.find(ext_addr);

	if ( i != external_to_private.end() ) {
		priv_addr = i->second;
		private_address_found = true;
	}

	pthread_mutex_unlock(&mutex);
	uninstall_cleanup_handler();

	if ( ! private_address_found )
		throw nat_manager_error("no matching reservation found",
			information_code::sc_signaling_session_failures,
			information_code::sigfail_no_reservation_found);

	return priv_addr;
}

// EOF
