/// ----------------------------------------*- mode: C++; -*--
/// @file nwn_uds_msg.h
/// Provides basic structures for injecting network events
/// into GIST.
/// ----------------------------------------------------------
/// $Id$
/// $HeadURL$
// ===========================================================
//                      
// Copyright (C) 2005-2010, all rights reserved by
// - Institute of Telematics, Karlsruhe Institute of Technology
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
#ifndef NWN_UDS_MSG_H
#define NWN_UDS_MSG_H


#include <linux/types.h>
#include <netinet/in.h>
#include <linux/if_ether.h>
#include <linux/if.h>


namespace ntlp {


struct aho_msg_body {
  char iface[IFNAMSIZ];
  uint8_t l2_ap_n[ETH_ALEN];

  uint32_t l3_af;
  union {
    struct in_addr	ipv4;
    struct in6_addr	ipv6;
  } l3_ar_n;
#define	l3_ar_n_ipv4	l3_ar_n.ipv4
#define	l3_ar_n_ipv6	l3_ar_n.ipv6
  union {
    struct in_addr	ipv4;
    struct in6_addr	ipv6;
  } l3_mn;
#define	l3_mn_ipv4	l3_mn.ipv4
#define	l3_mn_ipv6	l3_mn.ipv6
};


struct ho_msg_body {
  char iface[IFNAMSIZ];
  uint8_t l2_ap_n[ETH_ALEN];

  uint32_t l3_af;
  union {
    struct in_addr	ipv4;
    struct in6_addr	ipv6;
  } l3_ar_n;
#define	l3_ar_n_ipv4	l3_ar_n.ipv4
#define	l3_ar_n_ipv6	l3_ar_n.ipv6
  union {
    struct in_addr	ipv4;
    struct in6_addr	ipv6;
  } l3_mn;
#define	l3_mn_ipv4	l3_mn.ipv4
#define	l3_mn_ipv6	l3_mn.ipv6
};


struct nwn_uds_msg {
  uint32_t nwn_msg_type;
#define	NWN_MSG_TYPE_AHO        1
#define	NWN_MSG_TYPE_HO         2

  union {
    struct aho_msg_body	aho;
    struct ho_msg_body	ho;
  } nwn_msg_body;
#define	aho_msg		nwn_msg_body.aho
#define	ho_msg		nwn_msg_body.ho
};


} // end namespace ntlp

#endif // NWN_UDS_MSG_H

// vi: expandtab softtabstop=2 tabstop=2 shiftwidth=2
