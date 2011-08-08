/// ----------------------------------------*- mode: C++; -*--
/// @file nat_manager.h
/// The NAT manager.
/// ----------------------------------------------------------
/// $Id: nat_manager.h 6282 2011-06-17 13:31:57Z bless $
/// $HeadURL: https://svn.tm.kit.edu/nsis/natfw-nslp/trunk/include/nat_manager.h $
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
#ifndef NATFW__NAT_MANAGER_H
#define NATFW__NAT_MANAGER_H

#include <queue>
#include <exception>
#include "hashmap"

#include "protlib_types.h"
#include "address.h"

#include "natfw_config.h"
#include "session.h"


namespace natfw {
	using protlib::uint8;
	using protlib::uint16;
	using protlib::hostaddress;
	using protlib::appladdress;

/**
 * An exception to be thrown if a reservation has failed.
 */
class nat_manager_error : public request_error {
  public:
	nat_manager_error(const std::string &msg, uint8 severity=0,
		uint8 response_code=0) throw ()
		: request_error(msg, severity, response_code) { }
	virtual ~nat_manager_error() throw () { }
};

inline std::ostream &operator<<(std::ostream &out, const nat_manager_error &e) {
	return out << e.get_msg();
}


/**
 * The NAT manager.
 *
 * This class manages the pool of external addresses and ports. It is also
 * able to match the tuple (ext. addr, ext. port) to (priv. addr, priv. port).
 *
 * This class is thread-safe.
 */
class nat_manager {

  public:
	nat_manager(natfw_config *conf) throw ();
	nat_manager(const hostaddress &addr,
		uint16 start_port, uint16 stop_port) throw ();

	~nat_manager() throw ();

	appladdress reserve_external_address(const appladdress &priv_addr)
		throw (nat_manager_error);

	void release_external_address(const appladdress &addr) throw ();

	appladdress lookup_private_address(const appladdress &ext_addr)
		throw (nat_manager_error);

  private:
	pthread_mutex_t mutex;

	typedef hashmap_t<appladdress, appladdress> address_map_t;

	hostaddress address;		// only one external address supported
	queue<uint16> available;	// available port numbers

	// map reserved external addresses to private addresses
	address_map_t external_to_private;

	void init();
	void fill_available(uint16 start, uint16 stop);
};


} // namespace natfw

#endif // NATFW__NAT_MANAGER_H
