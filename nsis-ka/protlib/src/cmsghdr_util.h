/// ----------------------------------------*- mode: C++; -*--
/// @file cmsghdr_util.h
/// Helper functions for manipulating cmsghdr data structures
/// for internal use by TPoverUDP and TPQueryEncap
/// ----------------------------------------------------------
/// $Id: cmsghdr_util.h 4210 2009-08-06 11:22:29Z bless $
/// $HeadURL: https://svn.tm.kit.edu/nsis/protlib/trunk/src/cmsghdr_util.h $
// ===========================================================
//                      
// Copyright (C) 2009, all rights reserved by
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

#ifndef CMSGHDR_UTIL_H
#define CMSGHDR_UTIL_H

#include <sys/socket.h>	/* msghdr */
#include <address.h>

namespace protlib {

/*
 * Definitions for the IPv6 Router Alert Option, see RFC-2711.
 */
static const uint8_t IP6_RAO_OPT_TYPE		= 0x05;	// hop-by-hop opt. type
static const uint8_t IP6_RAO_OPT_LEN		= 2;
static const uint8_t IP6_RAO_OPT_ALIGN		= 2;	// 2n + 0 alignment

#ifdef HAS_RFC2292
static const uint8_t IP6_RAO_OPT_MULTX		= 2;	// 2n + 0 alignment
static const uint8_t IP6_RAO_OPT_OFFSETY	= 0;
#endif

struct ip6_rao_opt {
	uint8_t		ip6_rao_opt_type;
	uint8_t		ip6_rao_opt_len;
	uint16_t	ip6_rao_opt_val;
};

namespace util {

// returns the size of the CMSGHDR data structure
// needed for the router alert option

size_t
cmsghdr_rao_extension_size();

// Builds a cmsghdr data structure for the Router Alert Option
// with the given RaO value in msg->msg_control.
// This function respects msg->msg_controllen.
// @return zero on success

int
cmsghdr_build_rao(msghdr *msg, cmsghdr *cmsg, int rao);

// Builds a cmsghdr data structure for the IP Hop Limit Option
// with the given hlim value in msg->msg_control.
// This function respects msg->msg_controllen.
// @return zero on success

int
cmsghdr_build_hlim(msghdr *msg, cmsghdr *cmsg, int hlim);

// Builds a cmsghdr data structure for a pkt_info
// with specified optional IP address and/or the outgoing
// interface index set to oif.
// This function respects msg->msg_controllen.
// @return zero on success

int
cmsghdr_build_oif(msghdr *msg, cmsghdr *cmsg, const hostaddress *own_addr, uint16_t oif);

void
set_ancillary_data(struct msghdr *msg, int rao, const hostaddress *own_addr= NULL, uint16_t oif = 0, int hlim = 0);

void
dump_cmsghdr(struct msghdr *c);

} // namespace protlib::util

} // namespace protlib

#endif // CMSGHDR_UTIL_H
