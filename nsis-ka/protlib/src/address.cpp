/// ----------------------------------------*- mode: C++; -*--
/// @file address.cpp
/// GIST address objects
/// ----------------------------------------------------------
/// $Id: address.cpp 6280 2011-06-17 11:38:05Z bless $
/// $HeadURL: https://svn.tm.kit.edu/nsis/protlib/trunk/src/address.cpp $
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
/** @ingroup ieaddress
 * GIST address objects
 */

#include <sys/types.h>
#include <sys/socket.h>

#include "address.h"
#include "threadsafe_db.h"
#include "logfile.h"
#include "rfc5014_hack.h"

#include <net/if.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <iomanip>
#include <netdb.h>
#include <err.h>
#include <string.h>

namespace protlib {

/** @addtogroup ieaddress Address Objects
 * @{
 */
  using namespace log;
/***** class address *****/

/** Set subtype. */
address::address(subtype_t st) 
	: subtype(st) 
{ 
  //Log(DEBUG_LOG,LOG_NORMAL,"address","address constructor called for " << (void *) this); 
}

/** Log and throw a nomem_error */
void address::throw_nomem_error() const {
	try {
	  ERRCLog("address", "Not enough memory for address");
	} catch(...) {}
	throw IEError(IEError::ERROR_NO_MEM);
} // end throw_nomem_error


/***** class hostaddress *****/

/***** inherited from IE *****/

hostaddress* hostaddress::new_instance() const {
	hostaddress* ha = NULL;
	catch_bad_alloc(ha = new hostaddress());
	return ha;
} // end new_instance

hostaddress* hostaddress::copy() const {
	hostaddress* ha = NULL;
	catch_bad_alloc(ha =  new hostaddress(*this));
	return ha;
} // end copy 

bool hostaddress::operator==(const address& ie) const {
	const hostaddress* haddr = dynamic_cast<const hostaddress*>(&ie);
	if (haddr) {
#ifdef DEBUG_HARD
	  Log(DEBUG_LOG,LOG_NORMAL,"hostaddress","::operator==(), v4=" << haddr->get_ip_str()<<"=="<<this->get_ip_str());
	  if (!ipv4flag)
	    Log(DEBUG_LOG,LOG_NORMAL,"hostaddress","::operator==(), v6=" << IN6_ARE_ADDR_EQUAL(&ipv6addr, &haddr->ipv6addr));
#endif
	  return ipv4flag ? (ipv4addr.s_addr == haddr->ipv4addr.s_addr) :
	                    IN6_ARE_ADDR_EQUAL(&ipv6addr, &haddr->ipv6addr);
	} else return false;
} // end operator==

/***** new in hostaddress *****/


/** Initialize hostaddress from string if possible. */
hostaddress::hostaddress(const char *str, bool *res) 
	: address(IPv6HostAddress),
	  ipv4flag(false), 
	  outstring(NULL)
{
	register bool tmpres = false;
	clear_ip();
	tmpres = set_ip(str);
	if (res) *res = tmpres;
} // end string constructor hostaddress


/** Set IPv4 from string or leave object unchanged.
 * This changes object type to IPv4.
 * @return true on success.
 */
bool 
hostaddress::set_ipv4(const char *str) {
	struct in_addr in;
	if (str && (inet_pton(AF_INET,str,&in)>0)) {
		set_ip(in);
		return true;
	} else return false;
} // end set_ipv4

/** Set IPv4 address from struct in_addr. 
 * This changes object type to IPv4. 
 */
void hostaddress::set_ip(const struct in_addr &in) {
	clear_ip();
	ipv4addr.s_addr = in.s_addr;
	// set subtype to IPv4
	set_subtype(true);
	return;
} // end set_ip(in_addr)

/** Set IPv6 from string or leave object unchanged.
 * This changes object type to IPv6.
 * @return true on success.
 */
bool hostaddress::set_ipv6(const char *str) {
	struct in6_addr in;
	if (str && (inet_pton(AF_INET6,str,&in)>0)) {
		set_ip(in);
		return true;
	} else return false;
} // end set_ipv6	

/** Set IPv6 address from struct in6_addr. 
 * This changes object type to IPv6. 
 */
void 
hostaddress::set_ip(const struct in6_addr &in) {
	clear_ip();
	ipv6addr = in;	
	// set subtype to IPv6
	set_subtype(false);
	return;
} // end set_ip(in6_addr)



void hostaddress::set_ip(const hostaddress& h) {
	clear_ip();
	if (h.ipv4flag) {
		ipv4addr = h.ipv4addr;
	} else {
		ipv6addr = h.ipv6addr;
	} // end if ipv4flag
	set_subtype(h.ipv4flag);
} // end set_ip(hostaddress)

/** Check if IP address is unspecified (set to 0). */
bool hostaddress::is_ip_unspec() const {
	if (ipv4flag) return (ipv4addr.s_addr==0);
	else return IN6_IS_ADDR_UNSPECIFIED(&ipv6addr);
	// never reached
	return true;
} // end is_unspec

/** Get IP address as a string. The output string is kept inside the
 * hostaddress object and should be copied if used for a longer time.
 */
const char* hostaddress::get_ip_str() const 
{
  // If outstring exists then it is valid.
  if (outstring) 
    return outstring;
  else
    outstring= (ipv4flag ? new(nothrow) char[INET_ADDRSTRLEN] : 
		           new(nothrow) char[INET6_ADDRSTRLEN]);

  if (hostaddress::get_ip_str(outstring)) 
    return outstring;
  else 
  {
    // error
    if (outstring) 
      delete[] outstring;

    return (outstring = NULL);
  } // end if get_ip_str()
} // end get_ip_str

/** Get IP address as a string.
 * @param str string buffer
 */
const char* hostaddress::get_ip_str(char *str) const 
{
  if (!str) return NULL;
  memset(str,0, ipv4flag ? INET_ADDRSTRLEN : INET6_ADDRSTRLEN);
  return ipv4flag ? inet_ntop(AF_INET,(void*)&ipv4addr,str,INET_ADDRSTRLEN) 
                  : inet_ntop(AF_INET6,(void*)&ipv6addr,str,INET6_ADDRSTRLEN);
} // end get_ip_str(char*)


/** Get IP address as an in_addr if possible.
 * @return true on success.
 */
bool hostaddress::get_ip(struct in_addr& in) const {
	if (ipv4flag) {
		in = ipv4addr;
		return true;
	} else if (IN6_IS_ADDR_V4MAPPED(&ipv6addr)) {
		memcpy(&(in.s_addr),ipv6addr.s6_addr+12,4);
		return true;
	} else return false;
} // get_ip

/** Get IP address as an in6_addr.
 * If this is an IPv4 address, it is mapped to IPv6.
 * @return true since this is always possible.
 */
bool hostaddress::get_ip(struct in6_addr& in) const {
	if (ipv4flag) {
		// map to IPv6
		memset(in.s6_addr,0,10);
		memset(in.s6_addr+10,255,2);
		memcpy(in.s6_addr+12,&(ipv4addr.s_addr),4);
	} else in = ipv6addr;
	return true;
} // get_ip

/// returns false if address is not allowed to be used
/// as source address: loopback, multicast, broadcast, unspecified
bool 
hostaddress::is_bogus_source() const
{
  if (ipv4flag)
  { // IPv4
    if ( IN_MULTICAST(ntohl(ipv4addr.s_addr)) || 
	 ipv4addr.s_addr == htonl(INADDR_LOOPBACK)  ||
         ipv4addr.s_addr == htonl(INADDR_ANY) ||
         ipv4addr.s_addr == htonl(INADDR_BROADCAST)
       )
    {
      return true;
    }
  }
  else
  { // IPv6
	  if ( IN6_ARE_ADDR_EQUAL(&ipv6addr,&in6addr_any) ||
	       IN6_ARE_ADDR_EQUAL(&ipv6addr,&in6addr_loopback) )
		  return true;
  }

  return false;
}


/** Convert the IP address to IPv6 by mapping it from IPv4 to IPv6 if
 * necessary
 */
void hostaddress::convert_to_ipv6() {
	if (ipv4flag) {
		if (is_ip_unspec()) {
			// clear IP and set to IPv6
			clear_ip();
			set_subtype(false);
		} 
		else {
			// map to IPv6
			struct in6_addr in;
			get_ip(in);
			// set IP
			set_ip(in);
		} // end if
	} // end if ipv4flag
} // end convert_to_ipv6



/** Convert the IP address to IPv4 if it is an IPv4 mapped address
 *  plain IPv4 and plain IPv6 addresses remain unchanged
 */
void hostaddress::convert_to_ipv4() {
	if (!ipv4flag) { // only for v6 addresses, v4 address remain unchanged
		if (is_ip_unspec()) {
			// clear IP and set to IPv4
			clear_ip();
			set_subtype(true);
		} else
		if (is_mapped_ip()) {
			struct in_addr in;
			get_ip(in);
			// set IP
			set_ip(in);
		} // end if is_mapped_ip()
	} // end if ipv4flag
} // end convert_to_ipv4


/** Two addresses are equivalent if they are equal after converting both to
 * IPv6.
 */
bool hostaddress::equiv(const hostaddress& h) const {
	bool thisipv4 = is_ipv4();
	bool result = false;
	hostaddress* ipv4haddr = NULL;
	const hostaddress* ipv6haddr = NULL;
	// if both IPv4 or both IPv6 then just test for equality
	if (thisipv4==h.is_ipv4()) return operator==(h);
	// OK, one is IPv6 and the other is IPv4 (to be converted!).
	// Allocating memory dynamically because I do not know the concrete
	// type of *this or h.
	try {
		if (thisipv4) {
			ipv6haddr = &h;
			if (h.is_mapped_ip()) ipv4haddr = this->copy();
		} else {
			ipv6haddr = this;
			if (is_mapped_ip()) ipv4haddr = h.copy();
		} // end if thisipv4
	} catch(IEError&) { ipv4haddr = NULL; }
	if (ipv4haddr) {
		ipv4haddr->convert_to_ipv6();
		result = ((*ipv4haddr)==(*ipv6haddr));
		delete ipv4haddr;
		return result;
	} else return false;
} // end equiv


/** Set subtype and IPv4 flag. This does NOT clear the outstring buffer.
 * Use clear_ip(). 
 */
void hostaddress::set_subtype(bool ipv4) {
	ipv4flag = ipv4; 
	subtype = ipv4flag ? IPv4HostAddress : IPv6HostAddress;
} // end set_subtype

/** Clear the IP address buffer and the outstring if it exists. */
void hostaddress::clear_ip() {
	// only need to clear ipv6 because of union
	ipv6addr = in6addr_any;
	if (outstring) {
		delete[] outstring;
		outstring = NULL;
	} // end if outstring
} // end clear_ip


istream& operator>>(istream& inputstream, hostaddress& ha)
{
  string inputstr;
  
  inputstream >> inputstr;
  
  if (ha.set_ipv6(inputstr.c_str()) == false)
  { // conversion to ipv6 address failed, try ipv4 instead
    ha.set_ipv4(inputstr.c_str());
  }

  return inputstream;
}


istream& operator>>(istream& inputstream, list<hostaddress>& halist)
{
  while ( inputstream ) {
    std::string token;
    inputstream >> token;
    
    bool success;
    hostaddress addr(token.c_str(), &success);
    
    if ( success )
      halist.push_back(addr);
  }

  return inputstream;
}


ostream& operator<<(ostream& outstream, const list<hostaddress>& addresslist)
{
  // algo?  foreach...
  list<hostaddress>::const_iterator haddr_citer= addresslist.begin();
  while ( haddr_citer != addresslist.end() )
  {
    // write address
    outstream << *haddr_citer;
    haddr_citer++;
    // if not the last element, write separating space
    if (haddr_citer != addresslist.end())
      outstream << ' ';
  }
  return outstream;
}

/** Match this IP address against another IP address.
 * Two IP addresses match if they are equal.
 * @return -1 on no match or error, e.g. when IPv4 and IPv6 are matched
 * @return the length of the matched prefix.
 */
int hostaddress::match_against(const hostaddress& ha) const {
	if (ipv4flag==ha.ipv4flag) {
		if (operator==(ha)) {
			if (ipv4flag) return 32;
			else return 128;
		} else return -1;
	} else return -1;
} // end match_against(

/** Match this IP address against the given network prefix.
 * @return -1 on no match or error, e.g. when IPv4 and IPv6 are matched
 * @return the length of the matched prefix.
 */
int hostaddress::match_against(const netaddress& na) const {
	uint32 word1 = 0;
	uint32 word2 = 0;
	const prefix_length_t preflen = na.get_pref_len();
	prefix_length_t lenwords = preflen/32;
	prefix_length_t lenbits = preflen%32;
	// now 0<=lenwords<=4, 0<=lenbits<=31
	prefix_length_t i;
	const hostaddress& ha = dynamic_cast<const hostaddress&>(na);
	if (ipv4flag==na.ipv4flag) {
		if (ipv4flag) {
			if (preflen >= 32)
				lenbits = 32;
			// match IPv4
			word1 = ntohl(ipv4addr.s_addr);
			word2 = ntohl(ha.ipv4addr.s_addr);
			// shift right
			word1 >>= (32-lenbits);
			word2 >>= (32-lenbits);
			if (word1==word2) return preflen;
			else return -1;
		} else {
			if (preflen > 128)
				return -1;
			// match IPv6
			// match words
			for (i=0;i<lenwords;i++) {
			  word1 = ntohl(ipv6addr.s6_addr32[i]);    // RB: as long as both words are in same order we dont need ntohl?!
			  word2 = ntohl(ha.ipv6addr.s6_addr32[i]);
				if (word1!=word2) return -1;
			} // end for i
			// now i points to the next word to be matched
			// match bits
			if (lenbits) {
				word1 = ntohl(ipv6addr.s6_addr32[i]);
				word2 = ntohl(ha.ipv6addr.s6_addr32[i]);
				// shift right
				word1 >>= (32-lenbits);
				word2 >>= (32-lenbits);
				if (word1==word2) return preflen;
				else return -1;
			} else {
				// no extra bits to match and everything OK
				return preflen;
			} // end if lenbits
		} // end if ipv4flag
	} else return -1;
} // end match_against

/** Check whether this IP address is a multicast address.
 * @return true if it is a multicast address, otherwise false
 */
bool hostaddress::is_multicast() const {
	if (ipv4flag)
	{ // IPv4
		if (IN_MULTICAST(ntohl(ipv4addr.s_addr)))
		{
			return true;
		}
	} else { // IPv6
		if (ipv6addr.s6_addr[0] == 0xff)
			return true;
	}
	return false;
}

/***** class appladdress *****/

/***** inherited from IE *****/

appladdress* appladdress::new_instance() const {
	appladdress* aa = NULL;
	catch_bad_alloc(aa = new appladdress());
	return aa;
} // end new_instance

appladdress* appladdress::copy() const {
	appladdress* aa = NULL;
	catch_bad_alloc(aa = new appladdress(*this));
	return aa;
} // end copy 

bool appladdress::operator==(const address& ie) const {
	const appladdress* app = dynamic_cast<const appladdress*>(&ie);
	if (app) {
		// if the IEs are equal when casted to type hostaddress, then
		// only protocols and ports have to be checked.
		// Otherwise they are not equal.
		if (hostaddress::operator==(ie)) {
			//if (proto!=app->proto) cout << "protocols not matching" << endl;
			//if (port !=app->port) cout << "ports not matching" << endl;

		    return ((proto==app->proto) && (port==app->port));
		} else return false;
	} else return false;
} // end operator==


/***** class netaddress *****/

/***** inherited from IE *****/

netaddress* netaddress::new_instance() const {
	netaddress* na = NULL;
	catch_bad_alloc(na = new netaddress());
	return na;
} // end new_instance

netaddress* netaddress::copy() const {
	netaddress* na = NULL;
	catch_bad_alloc(na = new netaddress(*this));
	return na;
} // end copy 

bool netaddress::operator==(const address& ie) const {
	const netaddress* na = dynamic_cast<const netaddress*>(&ie);
	if (na) {
		// if the IEs are equal when casted to type hostaddress, then
		// only prefix lengths have to be checked.
		// Otherwise they are not equal.
		if (hostaddress::operator==(ie)) {
			//if (prefix_length!=na->prefix_length) cout << "Prefix length not matching" << endl;
		    return (prefix_length==na->prefix_length);
		} else return false;
	} else return false;
} // end operator==


/***** inherited from hostaddress *****/

/** Convert the IP address to IPv6 by mapping it from IPv4 to IPv6 if
 * necessary. The prefix length is converted, too.
 */
void netaddress::convert_to_ipv6() {
	if (ipv4flag) {
		// map IP
		hostaddress::convert_to_ipv6();
		// adjust prefix length
		set_pref_len(prefix_length+96);
	} // end if ipv4flag
} // end convert_to_ipv6

/** Set subtype and IPv4 flag. This does NOT clear the outstring buffer.
 * Use clear_ip(). 
 * In addition the prefix length is checked and set to reasonable values.
 */
void netaddress::set_subtype(bool ipv4) {
	ipv4flag = ipv4; 
	if (ipv4) {
		subtype = IPv4NetAddress; 
		if (prefix_length>32) prefix_length = 32;
	} else {
		subtype = IPv6NetAddress;
		if (prefix_length>128) prefix_length = 128;
	} // end if ipv4flag
} // end set_subtype

/***** new members in netaddress *****/

/** Constructor sets address type and clears prefix length. */
netaddress::netaddress() : 
  hostaddress(),
  prefix_length(0)
 {
	set_subtype(ipv4flag);
} // end constructor

netaddress::netaddress(const netaddress& na) : hostaddress(na) {
	prefix_length = na.prefix_length;
	set_subtype(ipv4flag);
} // end copy constructor

/** Initialize with the given host address and prefix length. 
 * Prefix length is optional and set to 0 if missing.
 */
netaddress::netaddress(const hostaddress& h, prefix_length_t len) : hostaddress(h) {
  set_subtype(ipv4flag);
  set_pref_len(len); // this will cut the prefix value accordingly
} // end constructor(hostaddress,len)

/** Initialize from string which contains IP and prefix length separated by
 * '/'.
 */
netaddress::netaddress(const char* str, bool *res) : hostaddress() {
	bool tmpres = true; // MUST BE true
	bool tmpres2 = false;
	long int len = 0;
	uint32 iplen;
	const char* i = NULL;
	char* errptr = NULL;
	char ipstr[INET6_ADDRSTRLEN] = {0};
	// look for /
	i = strchr(str,'/');
	if (i) {
		iplen = i-str;
		i++;
		// decode prefix length
		len = strtol(i,&errptr,10);
		if ((*i) && errptr && ((*errptr)==0)) {
			// prefix OK
			prefix_length = len;
		} else {
			prefix_length = 0;
			tmpres = false;
		} // end if prefix OK
		if (iplen<=INET6_ADDRSTRLEN) {
			// copy IP string
			strncpy(ipstr,str,iplen);
			ipstr[INET6_ADDRSTRLEN-1] = 0;
			// set str to ipstr
			str = ipstr;
		} // end if iplen valid
	} else {
		// no prefix found, OK
		prefix_length = 0;
	} // end if i
	// set IP, this also sets ipvflag.
	tmpres2 = set_ip(str);
	if (res) *res = (tmpres && tmpres2);
	set_subtype(ipv4flag);
} // end constructor(string)

/** Initialize from string and prefix length.
 * If the string does not contain a valid IP address, it is set to all 0 by
 * the hostaddress constructor.
 */
netaddress::netaddress(const char* str, prefix_length_t len, bool *res) : hostaddress(str,res) {
  set_subtype(ipv4flag);
  set_pref_len(len); // this will cut the prefix value accordingly
} // end constructor(string,port)

/** Assigns the given netaddress address by using hostaddress::operator=(). */
netaddress& netaddress::operator=(const netaddress& na) {
  prefix_length = na.prefix_length;
  hostaddress::operator=(na);
  return *this;
} // end operator=


/** Assigns the given netaddress address by using hostaddress::operator=(). */
netaddress& netaddress::operator=(const hostaddress& na) {
	hostaddress::operator=(na);
	set_pref_len(128);
	return *this;
} // end operator=


/** Set prefix length and return old value.
 * will also cut prefix_length if needed (i.e., if len too large for addr type)
 */
prefix_length_t netaddress::set_pref_len(prefix_length_t len) {
	register prefix_length_t olen = prefix_length;
	prefix_length = ipv4flag ? ( (len>32) ? 32 : len ) :
	                           ( (len>128) ? 128 : len );
	return olen;
} // end set_port



/** Compare two netaddress objects. If neither a<b and b<a then a and b are
 * considered equal.
 */
bool netaddress::operator<(const netaddress& na) const {
	uint32 word1 = 0;
	uint32 word2 = 0;
	prefix_length_t lenwords = prefix_length/32;
	prefix_length_t lenbits = prefix_length%32;
	// now 0<=lenwords<=4, 0<=lenbits<=31
	prefix_length_t i;
	// IPv4 is always < IPv6
	if ((!ipv4flag) && na.ipv4flag) return false;
	else if (ipv4flag && (!na.ipv4flag)) return true;
	// now ipv4flag == na.ipv4flag
	else if (prefix_length<na.prefix_length) return true;
	else if (prefix_length>na.prefix_length) return false;
	// now prefix_length == na.prefix_length
	else if (ipv4flag) {
		// compare IPv4 with same prefix length
		word1 = ntohl(ipv4addr.s_addr);
		word2 = ntohl(na.ipv4addr.s_addr);
		// shift right
		word1 >>= (32-lenbits);
		word2 >>= (32-lenbits);
		if (word1<word2) return true;
		else return false;
	} else {
		// compare IPv6 with same prefix length
		// compare words
		for (i=0;i<lenwords;i++) {
			word1 = ntohl(ipv6addr.s6_addr32[i]);
			word2 = ntohl(na.ipv6addr.s6_addr32[i]);
			if (word1<word2) return true;
			if (word1>word2) return false;
		} // end for i
		// now i points to the next word to be compared and previous words are equal
		// compare bits
		if (lenbits) {
			word1 = ntohl(ipv6addr.s6_addr32[i]);
			word2 = ntohl(na.ipv6addr.s6_addr32[i]);
			// shift right
			word1 >>= (32-lenbits);
			word2 >>= (32-lenbits);
			if (word1<word2) return true;
			else return false;
		} else {
			// no extra bist to compare and everything equal
			return false;
		} // end if lenbits
	} // end if ipv4flag, prefox
} // end match_against

/**
 * Compare function for the radix trie:
 *
 * Compare this and na from *pos up to max(this->prefix, na->prefix)
 *
 * In pos return the position where the compare ended.
 *
 * The return value is 0 if the strings are equal and all of this' string
 * was consumed, otherwise the sign will indicate the bit in this' string
 * at pos (i.e. if the search should continue left or right of na).
 *
 * pos < 0 indicates error
 */
int
netaddress::rdx_cmp(const netaddress *na, int *pos) const
{
	if (na == NULL) {
		*pos = -1;
		return 0;
	}

	if (na->ipv4flag != ipv4flag ||
	    *pos > na->prefix_length ||
	    *pos > prefix_length) {
		*pos = -1;
		return 0;
	}

	if (na->prefix_length == 0) {
		*pos = 1;
		if (ipv4flag)
			return ((ntohl(ipv4addr.s_addr) & 0x80000000) == 0 ?
			    -1 : 1);
		else
			return ((htonl(ipv6addr.s6_addr32[0]) & 0x80000000) == 0 ?
			    -1 : 1);
	}

	if (*pos < 0)
		*pos = 0;

	uint32_t w1, w2, w3;
	int diff, i, p1, p2;

	if (ipv4flag) {
		diff = *pos;
		w1 = ntohl(ipv4addr.s_addr);
		w2 = ntohl(na->ipv4addr.s_addr);
		// in w3 store the difference
		w3 = w1 ^ w2;
		// mask out anything after prefix_length and before *pos
		w3 = (w3 >> (32 - prefix_length)) << (32 - prefix_length + diff);
		if (w3 == 0 && prefix_length <= na->prefix_length) {
			*pos = min(prefix_length, na->prefix_length);
			return 0;
		}
		// pos = 0 means start up front, so we need to fix up to that
		diff++;
		while (diff <= prefix_length && diff <= na->prefix_length) {
			if ((w3 & 0x80000000) != 0) {
				*pos = diff;
				return (((w1 & (1 << (32 - diff))) >>
				    (32 - diff)) == 0 ? -1 : 1);
			}
			w3 = w3 << 1;
			diff++;
		}
		// difference past na->prefix_length
		*pos = diff;
                if ((w1 == w2) && (prefix_length > na->prefix_length)) {
                    return 1;
                }
          
		return (((w1 & (1 << (32 - diff))) >>
		    (32 - diff)) == 0 ? -1 : 1);
	}

	diff = *pos;
	for (i = diff / 32; i < 4; i++) {
		diff = diff % 32;
		w1 = ntohl(ipv6addr.s6_addr32[i]);
		w2 = ntohl(na->ipv6addr.s6_addr32[i]);
		w3 = w1 ^ w2;
		p1 = (prefix_length - (i * 32));
		p1 = p1 > 32 ? 32 : p1;
		p2 = (na->prefix_length - (i * 32));
		p1 = p2 > 32 ? 32 : p2;

		w3 = (w3 >> (32 - p1)) << (32 - p1 + diff);
		if (w3 == 0 && prefix_length <= na->prefix_length) {
			*pos = min(prefix_length, na->prefix_length);
			if (prefix_length <= ((i + 1) * 32))
				return 0;
		}
		// pos = 0 means start up front, so we need to fix up to that
		diff++;
		while (diff <= p1 && diff <= p2) {
			if ((w3 & 0x80000000) != 0) {
				*pos = diff + (i * 32);
				return (((w1 & (1 << (32 - diff))) >>
				    (32 - diff)) == 0 ? -1 : 1);
			}
			w3 = w3 << 1;
			diff++;
		}
		if (diff + (32 * i) <= prefix_length &&
		    diff + (32 * i) <= na->prefix_length) {
			diff--;
			continue;
		}
		// difference past na->prefix_length
		*pos = diff + (i * 32);
		if (diff == 33) {
			diff = 1;
			if (i == 3)
				abort();
			w1 = ntohl(ipv6addr.s6_addr32[i+1]);
		}
		return (((w1 & (1 << (32 - diff))) >>
		    (32 - diff)) == 0 ? -1 : 1);
	}

	// Not reachable, but gcc complains
	return 0;
}

const int udsaddress::invalid_socknum= -1;

udsaddress* udsaddress::new_instance() const {
	udsaddress* ha = NULL;
	catch_bad_alloc(ha = new udsaddress());
	return ha;
} // end new_instance

udsaddress* udsaddress::copy() const {
        udsaddress* ha = NULL;
	catch_bad_alloc(ha =  new udsaddress(*this));
	return ha;
} // end copy 

bool udsaddress::operator==(const address& ie) const {
	const udsaddress* app = dynamic_cast<const udsaddress*>(&ie);
	if (app) {
	    return (app->socknum == socknum) && (app->uds_socket == uds_socket);
	} else return false;
} // end operator==


/***** class macaddress *****/

macaddress* macaddress::new_instance() const {
	macaddress* ma = NULL;
	catch_bad_alloc(ma = new macaddress());
	return ma;
} // end new_instance

macaddress* macaddress::copy() const {
	macaddress* ma = NULL;
	catch_bad_alloc(ma =  new macaddress(*this));
	return ma;
} // end copy

bool macaddress::operator==(const address& ie) const {
	const macaddress* ma = dynamic_cast<const macaddress*>(&ie);
	if (ma) {
		return ((initialized == ma->initialized) &&
				(macaddr[0] == ma->macaddr[0]) &&
				(macaddr[1] == ma->macaddr[1]) &&
				(macaddr[2] == ma->macaddr[2]) &&
				(macaddr[3] == ma->macaddr[3]) &&
				(macaddr[4] == ma->macaddr[4]) &&
				(macaddr[5] == ma->macaddr[5]));
	}
	else {
		return false;
	}
} // end operator==

/** Initialize a macaddress object */
macaddress::macaddress()
	:	address(IEEE48),
		initialized(false)
{
	//clear_mac();
} // end constructor macaddress

/** Copy constructor for macaddress objects */
macaddress::macaddress(const macaddress& ma)
	:	address(ma),
		initialized(false)
{
	this->set_mac(ma);
} // end copy constructor macaddress

/** Initialize macaddress from string if possible. */
macaddress::macaddress(const char *str, bool *res)
	:	address(IEEE48),
		initialized(false)
{
	register bool tmpres = false;
	tmpres = set_mac(str);
	if (res) {
		*res = tmpres;
	}
} // end string constructor macaddress

/** Initialize macaddress from byte array */
macaddress::macaddress(const uint8_t macaddr[6])
	:	address(IEEE48),
		initialized(false)
{
	this->set_mac(macaddr);
} // end byte array constructor macaddress

/** Assign ma to this object. */
macaddress& macaddress::operator=(const macaddress& ma) {
	address::operator=(ma);
	this->set_mac(ma);

	return *this;
} // end operator=

/** Set MAC address from macaddress. */
void macaddress::set_mac(const macaddress& ma) {
	uint8_t macaddr[6];

	if (!ma.is_mac_unspec()) {
		ma.get_mac(macaddr);
		set_mac(macaddr);
	}
} // end set_mac(macaddress)

/** Set MAC address from string or leave object unchanged. */
bool macaddress::set_mac(const char *str) {
	uint8_t macaddr[6];
	register uint32_t a, b, c, d, e, f;

	int ret = sscanf(str, "%x:%x:%x:%x:%x:%x", &a, &b, &c, &d, &e, &f);
	if (ret == 6) {
		macaddr[0] = a; macaddr[1] = b; macaddr[2] = c;
		macaddr[3] = d; macaddr[4] = e; macaddr[5] = f;
		set_mac(macaddr);

		return true;
	}

	return false;
} // end set_mac(str)

/** Set MAC address from byte array. */
void macaddress::set_mac(const uint8_t macaddr[6]) {
	memcpy(this->macaddr, macaddr, 6);
	initialized = true;
} // end set_mac(uint8_t)

/** Get MAC address as a string. The output string is kept inside the
 *  macaddress object and should be copied if used for a longer time.
 */
const char* macaddress::get_mac_str() const
{
	if (initialized == false) {
		memset(outstring, 0, sizeof(outstring));
	}
	else {
		sprintf(outstring, "%02x:%02x:%02x:%02x:%02x:%02x",
				macaddr[0], macaddr[1], macaddr[2],
				macaddr[3], macaddr[4], macaddr[5]);
	}

	return outstring;
} // end get_mac_str

/** Get MAC address as a string.
 *  @param str string buffer
 */
void macaddress::get_mac_str(char *str) const
{
	if (str != NULL) {
		const char *src = get_mac_str();

		strcpy(str, src);
	}
} // end get_mac_str(char*)

/** Get MAC address as byte array.
 *  @param byte array
 */
void macaddress::get_mac(uint8_t macaddr[6]) const
{
	memcpy(macaddr, this->macaddr, 6);
} // end get_mac(uint8_t)

bool macaddress::equiv(const macaddress& ma) const
{
	return operator==(ma);
}


} // end namespace protlib
