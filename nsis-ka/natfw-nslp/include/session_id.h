/// ----------------------------------------*- mode: C++; -*--
/// @file session_id.h
/// The session classes.
/// ----------------------------------------------------------
/// $Id: session_id.h 6282 2011-06-17 13:31:57Z bless $
/// $HeadURL: https://svn.tm.kit.edu/nsis/natfw-nslp/trunk/include/session_id.h $
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
#ifndef NATFW__SESSION_ID_H
#define NATFW__SESSION_ID_H

#include <iomanip>
#include "hashmap"

#include "protlib_types.h"
#include "msg/natfw_msg.h"


namespace natfw {
    using protlib::uint128;

class dispatcher;

/**
 * A 128 Bit Session ID.
 *
 * TODO: move to protlib. don't forget iomanip!
 */
class session_id {
  public:
	explicit session_id();

	explicit session_id(uint128 value) : id(value) { }

	inline ~session_id() { }

	inline uint128 get_id() const throw () {
		return id;
	}

	inline bool operator==(const session_id &other) const {
		return id.w1 == other.id.w1 && id.w2 == other.id.w2
			&& id.w3 == other.id.w3 && id.w4 == other.id.w4;
	}

	inline bool operator!=(const session_id &other) const {
		return ! (*this == other);
	}
  private:
	uint128 id;
};

std::ostream &operator<<(std::ostream &out, const session_id &sid);

} // namespace natfw

#ifndef USE_UNORDERED_MAP
namespace __gnu_cxx {
#else
namespace std {
#endif

template <> struct hash<natfw::session_id> {
	inline size_t operator()(const natfw::session_id& id) const {
		protlib::uint128 i = id.get_id();
		return i.w1 ^ i.w2 ^ i.w3 ^ i.w4; // bitwise XOR
	}
};

} // namespace __gnu_cxx


#endif // NATFW__SESSION_ID_H
