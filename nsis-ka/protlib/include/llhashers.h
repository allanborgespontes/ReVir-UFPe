/// ----------------------------------------*- mode: C++; -*--
/// @file llhashers.h
/// hash functions for long longs
/// ----------------------------------------------------------
/// $Id: llhashers.h 6282 2011-06-17 13:31:57Z bless $
/// $HeadURL: https://svn.tm.kit.edu/nsis/protlib/trunk/include/llhashers.h $
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

#ifndef PROTLIB_LL_HASHERS_H
#define PROTLIB_LL_HASHERS_H


/** @addtogroup hashers Hash function objects
 *
 * This file contains definitions of hash functions 
 * for long long (64 bit integers)
 *
 * @{
 */

#ifndef USE_UNORDERED_MAP
namespace __gnu_cxx {

/// long long int hasher
template <> struct hash<long long> {
	inline size_t operator()(long long i) const { return (size_t)i; }
}; // end long long hasher

/// unsigned long long int hasher
template <> struct hash<unsigned long long> {
	inline size_t operator()(unsigned long long i) const { return (size_t)i; }
}; // end unsigned long long hasher

} // end namespace __gnu_cxx
#endif

// there are already struct std::hash<long long int> and struct std::hash<long long unsigned int> defined

//@}

#endif
