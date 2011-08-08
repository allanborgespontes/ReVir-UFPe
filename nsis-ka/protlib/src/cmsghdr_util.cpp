/// ----------------------------------------*- mode: C++; -*--
/// @file cmsghdr_util.cpp
/// Helper functions for manipulating cmsghdr data structures
/// for internal use by TPoverUDP and TPQueryEncap
/// ----------------------------------------------------------
/// $Id: cmsghdr_util.cpp 4210 2009-08-06 11:22:29Z bless $
/// $HeadURL: https://svn.tm.kit.edu/nsis/protlib/trunk/src/cmsghdr_util.cpp $
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

extern "C" {
#include <assert.h>
#ifdef HAS_RFC2292
#include <sys/types.h>
#endif
#include <netinet/in.h>
#include <netinet/ip6.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
}

#include "logfile.h"
#include "cmsghdr_util.h"

using namespace std;

namespace protlib {
using namespace log;

namespace util {

/** Calculates the size of a cmsghdr needed for the Router Alert Option
 *
 * @return	the calculated size on success, or negative value on error
 */

size_t
cmsghdr_rao_extension_size()
{
#ifdef HAS_RFC2292
    return inet6_option_space(sizeof(ip6_rao_opt));
#else
    int currentlen;

    currentlen = inet6_opt_init(NULL, 0);
    if (currentlen == -1)
    	return -1;

    currentlen = inet6_opt_append(NULL, 0, currentlen,
				      IP6_RAO_OPT_TYPE,
				      IP6_RAO_OPT_LEN,
				      IP6_RAO_OPT_ALIGN, NULL);
    if (currentlen == -1)
        return -2;

    currentlen = inet6_opt_finish(NULL, 0, currentlen);
    if (currentlen == -1)
	return -3;
    return currentlen;
#endif
}

/** Adds a Router Alert Option to a msghdr
 *
 * This function adds a IPv6 hop by hop extension header containing the Router
 * Alert Option (rao) to the msg_control member of the given msghdr.  For this
 * purpose it writes a cmsghdr struct to pointer cmsg, which MUST point to the
 * memory area allocated in msg->msg_control of size msg->msg_controllen. It
 * is the caller's duty to allocate this memory in advance and to initialize
 * msg->msg_controllen accordingly.
 *
 * @param msg	a pointer to the msghdr struct which this RaO should be set for
 * @param cmsg	the location where the cmsghdr structure for the RaO should
 *              be written to
 * @rao         the desired value of the Router Alert Option
 * @return	zero on success, -1 on error
 */

int
cmsghdr_build_rao(msghdr *msg, cmsghdr *cmsg, int rao)
{
    const char *const methodname = "protlib::util::cmsghdr_build_rao()";
    assert(msg);
    assert(cmsg != NULL);
    assert(rao >= 0);
    unsigned int space_left = msg->msg_controllen - (cmsg - ((cmsghdr*)msg->msg_control));
    const int extlen = cmsghdr_rao_extension_size();
    if (extlen < 0)
      return -1;
    if (space_left < CMSG_LEN(extlen))
      return -1;

#ifdef HAS_RFC2292
    struct ip6_rao_opt opt_rao;
    struct cmsghdr *cmsgptr= NULL;
    int rv;
    rv = inet6_option_init(cmsg, &cmsg, IPV6_HOPOPTS);
    if (rv != 0) return -1;
    opt_rao.ip6_rao_opt_type = IP6_RAO_OPT_TYPE;
    opt_rao.ip6_rao_opt_len = IP6_RAO_OPT_LEN;
    opt_rao.ip6_rao_opt_val = htons(rao);
    rv = inet6_option_append(cmsgptr, &opt_rao.ip6_rao_opt_type,
				IP6_RAO_OPT_MULTX, IP6_RAO_OPT_OFFSETY);
    if (rv != 0) return -1;
#else
    cmsg->cmsg_level = IPPROTO_IPV6;
    cmsg->cmsg_type  = IPV6_HOPOPTS;
    cmsg->cmsg_len = CMSG_LEN(extlen);
    void *extbuf = CMSG_DATA(cmsg);
    assert(extbuf != NULL);

    int currentlen = inet6_opt_init(extbuf, extlen);
    if (currentlen == -1) {
	ERRCLog(methodname, "Failure while setting Hop-by-Hop Router Alert Option, inet6_opt_init()");
	return -1;
    }
    // inet6_opt_init initializes the value wrongly (glibc2.6.1):
    // it divides extlen by 8, but passsing an extlen of 0 is not possible
    // so we must fix the setting now
    struct ip6_hbh *extp = reinterpret_cast<struct ip6_hbh *>(extbuf);
    // inet6_opt_init did already some sanity checks (extlen is multiple of 8)
    extp->ip6h_len= (extlen / 8) - 1; // should be zero then, indicating a length of 8 bytes

    void *databuf;
    currentlen = inet6_opt_append(extbuf, extlen, currentlen,
		IP6_RAO_OPT_TYPE, IP6_RAO_OPT_LEN, IP6_RAO_OPT_ALIGN,
		&databuf);
    if (currentlen == -1) {
	ERRCLog(methodname, "Failure while setting Hop-by-Hop Router Alert Option, does not fit into extension header");
	return -1;
    }
    uint16 val = htons(rao);
    (void)inet6_opt_set_val(databuf, 0, &val, sizeof(val));

    currentlen = inet6_opt_finish(extbuf, extlen, currentlen);
    if (currentlen == -1) {
	ERRCLog(methodname, "Failure while setting Hop-by-Hop Router Alert Option, inet6_opt_finish()");
	return -1;
    }
#endif
    return 0;
}


/** Adds a Hop Limit Option to a msghdr
 *
 * This function adds a IPv6 hop limit to the msg_control member of the given
 * msghdr.  For this purpose it writes a cmsghdr struct to pointer cmsg, which
 * MUST point to the memory area allocated in msg->msg_control of size
 * msg->msg_controllen. It is the caller's duty to allocate this memory in
 * advance and to initialize msg->msg_controllen accordingly.
 *
 * @param msg	a pointer to the msghdr struct which the hop limit should be
 *              set for
 * @param cmsg	the location where the cmsghdr structure for the hop limit
 *              should be written to
 * @hlim        the desired hop limit
 * @return	zero on success, -1 on error
 */
int
cmsghdr_build_hlim(msghdr *msg, cmsghdr *cmsg, int hlim)
{
    assert(msg);
    assert(cmsg != NULL);
    assert(hlim > 0);

    unsigned int space_left = msg->msg_controllen - (cmsg - ((cmsghdr*)msg->msg_control));
    if (space_left < CMSG_LEN(sizeof(int)))
      return -1;

    int hop_limit = hlim;
    cmsg->cmsg_level = IPPROTO_IPV6;
    cmsg->cmsg_type = IPV6_HOPLIMIT;
    cmsg->cmsg_len = CMSG_LEN(sizeof(int));
    memcpy(CMSG_DATA(cmsg), &hop_limit, sizeof(int));
    return 0;
}

/** Adds the outgoing interface index or outgoing address to a msghdr
 *
 * This function adds a pkt_info for setting the outgoing interface/address to the
 * msg_control member of the given msghdr.  For this purpose it writes a
 * cmsghdr struct to pointer cmsg, which MUST point to the memory area
 * allocated in msg->msg_control of size msg->msg_controllen. It is the
 * caller's duty to allocate this memory in advance and to initialize
 * msg->msg_controllen accordingly.
 *
 * @param msg	a pointer to the msghdr struct which the hop limit should
 *              be set for
 * @param cmsg	the location where the cmsghdr structure for the hop limit
 *              should be written to
 * @param oif   the desired outgoing interface index, may be 0 if unset (usually if outgoing_address is given)
 * @param outgoing_address  the ip source address, may be NULL if unset (usually if oif is given)
 * @return	zero on success, -1 on error
 */
int
cmsghdr_build_oif(msghdr *msg, cmsghdr *cmsg, const hostaddress* outgoing_address, uint16_t oif)
{
    assert(msg);
    assert(cmsg != NULL);

    in6_pktinfo pktinfo;
    unsigned int space_left = msg->msg_controllen - (cmsg - ((cmsghdr*)msg->msg_control));
    if (space_left < CMSG_LEN(sizeof(pktinfo)))
      return -1;

    if (outgoing_address)
	    outgoing_address->get_ip(pktinfo.ipi6_addr);
    else
	    pktinfo.ipi6_addr = in6addr_any;

    pktinfo.ipi6_ifindex = oif;

    cmsg->cmsg_level = IPPROTO_IPV6;
    cmsg->cmsg_type = IPV6_PKTINFO;
    cmsg->cmsg_len=CMSG_LEN(sizeof(pktinfo));
    memcpy(CMSG_DATA(cmsg), &pktinfo, sizeof(pktinfo));
    return 0;
}



/** Set ancillary data to configure Routing Alert Option, outgoing interface and IP hop limit
 *
 * This function sets ancillary data to configure the Router Alert Option, the
 * outgoing interface and an IP hop limit. This ancillary data is stored in
 * msg->msg_control, which MUST be initialized to a NULL pointer.
 * msg->msg_controllen MUST be set to zero accordinlgy.
 *
 * The function constructs the complete ancillary data needed for the given
 * parameters rao (Router Alert Option), oif (Outgoing Interface), and hlim
 * (IP hop limit). After calling this funktion the ancillary data is stored
 * in the new allocated memory pointed to by msg->msg_control of size
 * msg->msg_controllen.
 *
 * It is the callers responsibility to free the memory allocated in
 * msg->msg-control by this function. All msghdr struct members except
 * msg_control and msg_controllen are preserved.
 *
 * @param msg	A pointer to the msghdr struct the ancillary data should
 *              be set for
 * @param rao	The desired Router Alert Option value (or -1 if none wanted)
 * @param outgoing_address The desired outgoing address
 *              (or NULL if the kernel should choose the outgoing address)
 * @param oif	The desired outgoing interface index (> 0)
 *              (or zero if the kernel should choose the outgoing interface)
 * @param hlim	The desired IP hop limit (> 0)
 *              (or zero if the kernel should set the IP hop limit)
 */
void
set_ancillary_data(struct msghdr *msg, int rao, const hostaddress* outgoing_address, uint16_t oif, int hlim)
{
  const char *const methodname= "protlib::util::set_ancillary_data()";

  assert(msg);
  assert(msg->msg_control == NULL);
  assert(msg->msg_controllen == 0);

  // Phase 1: Calculate needed buffer size
  uint32 buflength = 0;
  int rao_extlen = 0;

  if (rao != -1)
  {
    rao_extlen = cmsghdr_rao_extension_size();
    if (rao_extlen < 0)
    {
      ERRCLog(methodname, "Failure while calculating Hop-by-Hop "
			  "Router Alert Option size: " + rao_extlen);
      return;
    }
    buflength += CMSG_SPACE(rao_extlen);
  }
  if (hlim > 0) {
    buflength += CMSG_SPACE(sizeof(int));
  }
  if (oif > 0 || outgoing_address != NULL) {
    in6_pktinfo pktinfo;
    buflength += CMSG_SPACE(sizeof(pktinfo));
  }

  if (buflength == 0)
    return;

  // Phase 2: Allocate the buffer in calculated size
  msg->msg_control = static_cast<cmsghdr*>(malloc(buflength));
  memset(msg->msg_control, 0, buflength);
  msg->msg_controllen = buflength;
  assert(msg->msg_control != NULL);

  // Phase 3: Initialize the buffer contents
  cmsghdr *cmsgptr = CMSG_FIRSTHDR(msg);
  if (rao != -1)
  {
    DLog(methodname, "Adding IP ROA " << rao);
    int rv = cmsghdr_build_rao(msg, cmsgptr, rao);
    if (rv != 0) {
      ERRCLog(methodname, "Failure while building Hop-by-Hop "
			  "Router Alert Option: " + rv);
      free(msg->msg_control);
      msg->msg_control = NULL;
      msg->msg_controllen = 0;
      return;
    }
    cmsgptr = CMSG_NXTHDR(msg, cmsgptr);
  }
  if (hlim > 0)
  {
    DLog(methodname, "Adding IP hop limit " << hlim);
    int rv = cmsghdr_build_hlim(msg, cmsgptr, hlim);
    if (rv != 0) {
      ERRCLog(methodname, "Failure while building "
			  "IP TTL limit option: " + rv);
      free(msg->msg_control);
      msg->msg_control = NULL;
      msg->msg_controllen = 0;
      return;
    }
    cmsgptr = CMSG_NXTHDR(msg, cmsgptr);
  }
  if (oif > 0 || outgoing_address != NULL)
  {
    if (oif > 0)
	    DLog(methodname, "Adding IP outgoing interface " << oif);
    if (outgoing_address != NULL)
	    DLog(methodname, "Adding outgoing IP address " << *outgoing_address);

    int rv = cmsghdr_build_oif(msg, cmsgptr, outgoing_address, oif);
    if (rv != 0) {
    }
    cmsgptr = CMSG_NXTHDR(msg, cmsgptr);
  }

#ifdef DEBUG_HARD
  dump_cmsghdr(msg); // for debugging purposes (declaration see below)
#endif
  if (cmsgptr != NULL)
    ERRCLog(methodname, "Memory left over after building options???");
}

inline const char *level_p(int level) {
	if (level == IPPROTO_IPV6) return "IPv6";
	return "unknown";
}

inline const char *type_p(int type) {
	if (type == IPV6_HOPOPTS)  return "IPV6_HOPOPTS";
	if (type == IPV6_HOPLIMIT) return "IPV6_HOPLIMIT";
	if (type == IPV6_PKTINFO)  return "IPV6_PKTINFO";
	return "unknown";
}

#ifdef DEBUG_HARD
void
dump_cmsghdr(struct msghdr *c)
{
  struct cmsghdr *cmsg;
  cmsg = CMSG_FIRSTHDR(c);
  while (cmsg != NULL) {
    printf("CMSG[\tcmsg_len:   0x%02X (= %u bytes)\n"
  		"\tcmsg_level: 0x%02X (= %s)\n"
  		"\tcmsg_type:  0x%02X (= %s)\n\tdmesg_data:",
  		cmsg->cmsg_len, cmsg->cmsg_len,
		cmsg->cmsg_level, level_p(cmsg->cmsg_level),
		cmsg->cmsg_type,  type_p(cmsg->cmsg_type));
    int datalen = cmsg->cmsg_len - sizeof(cmsg->cmsg_len);
    datalen -= sizeof(cmsg->cmsg_level);
    datalen -= sizeof(cmsg->cmsg_type);
    unsigned char *p = CMSG_DATA(cmsg);
    for (int i = 0; i < datalen ; i++)
      printf(" %02X", p[i]);
    printf(" ]\n");
    cmsg = CMSG_NXTHDR(c, cmsg);
  }
}
#endif


} // namespace protlib::util

} // namespace protlib
