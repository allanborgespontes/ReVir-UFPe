/// ----------------------------------------*- mode: C++; -*--
/// @file nli.h
/// Network Layer Information object
/// ----------------------------------------------------------
/// $Id: nli.h 5249 2010-04-26 08:40:59Z roehricht $
/// $HeadURL: https://svn.tm.kit.edu/nsis/ntlp/trunk/src/pdu/nli.h $
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
#include "ntlp_object.h"
#include "address.h"
#include "protlib_types.h"
#include "logfile.h"
#include <openssl/rand.h>


#ifndef _NTLP__NLI_H_
#define _NTLP__NLI_H_



namespace ntlp {
    using namespace protlib;
    using namespace protlib::log;
    
// type for peer identity
class peer_identity {
  // length in byte words (must be 32-bit word aligned)
  uint8 length;
  uchar* buf;

public:
  // constructor if creating a brand_new initialized one (new PI)
  peer_identity() :
    length(16), // 128 bit
    buf(new uchar[length])
  {
    RAND_bytes(buf, length);
  }

  // constructor if creating a blank one for filling later
  peer_identity(uint8 l) :
    length(l),
    buf(new uchar[l])   
  {  
  }
	
  // copy constructor
  peer_identity(const peer_identity& n)
    : length (n.length),
       buf(new uchar[length])
  {
    memcpy(buf, n.buf, length);
  }
	
  // destructor
  ~peer_identity() 
  {
    if (buf) delete buf;
  }

  uchar* get_buffer() {
    return buf;
  }

  const uchar* get_buffer() const {
    return buf;
  }

  uint8 get_length() const {
    return length;
  }

  void set_peerid(const string& peeridstr);

  bool operator==(const peer_identity& n) const {
    return memcmp(buf, n.buf, length) ? false : true;
  }

  // readable output
  ostream& print (ostream& os) const;

  ostream& operator<<(ostream& os) const
  {
    return print(os);
  }
  
  /// return a hash value
  size_t get_hash() const; 
};


inline
ostream& operator<<(ostream& os, const peer_identity& pi) 
{
  return pi.print(os);
}

istream& 
operator>>(istream&s, peer_identity& pid);


class nli : public known_ntlp_object {


/***** inherited from IE ****/
public:
	virtual nli* new_instance() const;
	virtual nli* copy() const;
	nli* copy2(netaddress if_address) const;
	virtual nli* deserialize(NetMsg& msg, coding_t cod, IEErrorList& errorlist, uint32& bread, bool skip = true);
	virtual void serialize(NetMsg& msg, coding_t cod, uint32& wbytes) const;
	virtual bool check() const;
	virtual size_t get_serialized_size(coding_t coding) const;
	virtual bool operator==(const IE& ie) const;
	virtual const char* get_ie_name() const;
	virtual ostream& print(ostream& os, uint32 level, const uint32 indent, const char* name = NULL) const;
	virtual istream& input(istream& is, bool istty, uint32 level, const uint32 indent, const char* name = NULL);
/***** new members *****/
 private:
    	// IP_TTL
	uint8 ip_ttl;
	// IP_Version
	uint8 ip_version;
	// RS_Validity_Time (in ms) -- Routing State Validity Period
	uint32 rs_validity_time;
	// Peer Identity
	peer_identity* pi;
	// Interface_address
	netaddress if_address;

  static const char* const iename;
  static const size_t contlen;    

public:    
// mri (appladdress sourceaddress, appladdress destaddress, const uint32* flowlabel);
//    bool init_flags();
  nli ();
  nli (const nli& n);
  nli (uint8 ip_ttl, uint32 rs_validity_time, const peer_identity* pi, netaddress if_address);
  virtual ~nli();
  
  const peer_identity* get_pi() const;
  uint32 get_rs_validity_time() const;
  void set_rs_validity_time(uint32 value);
  uint8 get_ip_ttl() const;
  void set_ip_ttl(uint8 ttl);
  const netaddress& get_if_address() const { return if_address; }
#ifdef NSIS_OMNETPP_SIM
  // this is for the OMNeT++ Simulation and is not included otherwise
  // need to set this after the constructor is called (in initialize())
  void set_if_address(const hostaddress& ha) { if_address=ha; }
#endif
}; // end class nli


inline
const peer_identity* 
nli::get_pi() const
{
  return pi;
}

inline
uint32 
nli::get_rs_validity_time() const
{
  return rs_validity_time;
}

inline
void 
nli::set_rs_validity_time(uint32 value) 
{
    rs_validity_time = value;
}

inline
uint8 
nli::get_ip_ttl() const
{
  return ip_ttl;
}

inline
void 
nli::set_ip_ttl(uint8 ttl) 
{
  ip_ttl=ttl;
}




// stupid constructor, still, it is needed by NTLP_IE's "new_instance()" method, for deserialization (??)
inline
nli::nli () :
  known_ntlp_object(NLI, Mandatory),
     ip_ttl(0),
     ip_version(0),
     rs_validity_time(0),
     pi(NULL)
{
}

// copy constructor
inline
nli::nli(const nli& n) :
  known_ntlp_object(n),
     ip_ttl(n.ip_ttl),
     ip_version(n.ip_version),
     rs_validity_time(n.rs_validity_time),
     pi(NULL),
     if_address(n.if_address)
     
{
  if (n.pi) {
    if (n.pi->get_buffer()) {
      pi = new peer_identity(*n.pi);
    } 
    else 
      ERRCLog("NLI copy", "PI exists, but buffer is empty"); 
  }
  
  ip_version= if_address.is_ipv6() ? 6 : 4;
}


// This is the one mighty constructor, via which the object SHOULD be built programmatically!
inline
nli::nli(uint8 ip_ttl, uint32 rs_validity_time, const peer_identity* pi, netaddress if_address):
  known_ntlp_object(NLI, Mandatory),
  ip_ttl(ip_ttl),
  ip_version(if_address.is_ipv6() ? 6 : 4),
  rs_validity_time(rs_validity_time),
  pi(pi ? new peer_identity(*pi) : 0),
  if_address(if_address)
{
}


inline
nli::~nli()
{
  if (pi) delete pi;  
}


} // end namespace ntlp

#endif
