/// ----------------------------------------*- mode: C++; -*--
/// @file nli.cpp
/// GIST NLI (Network Layer Information)
/// ----------------------------------------------------------
/// $Id: nli.cpp 5249 2010-04-26 08:40:59Z roehricht $
/// $HeadURL: https://svn.tm.kit.edu/nsis/ntlp/trunk/src/pdu/nli.cpp $
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
#include "nli.h"
#include <iostream>
#include <errno.h>
#include <string>
#include <sstream>
#include "logfile.h"
#include <iomanip>

#include <sstream>
#include <cctype>

namespace ntlp {

using namespace protlib;

/** @defgroup ienli NLI Object
 *  @ingroup ientlpobject
 * @{
 */
/***** class peer_identity *****/

void 
peer_identity::set_peerid(const string& peeridstr)
{
	delete buf;
	length= peeridstr.length();
	if (length)
	{
		buf= new uchar[length];
		memcpy(buf, peeridstr.c_str(), length);
	}
	else
		buf= NULL;
}


// print readable content
ostream& 
peer_identity::print(ostream& os) const
{
  ios_base::fmtflags flags = os.flags();    // save stream flags
  os << hex << noshowbase;

  string printablestr;
  // if all printable, print it ascii
  int c;
  int i = 0; 
  while (i < length)
  {
    c= buf[i];
    
    os << setw(2) << setfill('0') << c;

    if (isprint(c))
      printablestr+= buf[i];
    else
      printablestr+= '.';
    i++;
  }
  os << " (" << printablestr << ')' << flush;

  os.width(0);
  os << dec << setfill(' ');
  os.setf(flags);               // reset stream flags

  return os;
}


istream& 
operator>>(istream&s, peer_identity& pid)
{
	string peerid_str;
  
	s >> peerid_str;

	pid.set_peerid(peerid_str);

	return s;
}



size_t
peer_identity::get_hash() const
{ 
  // sums up the XOR value of unsigned longs within peer identity
  size_t hashval= 0; 
  for (unsigned long* wordp= reinterpret_cast<unsigned long *>(buf); reinterpret_cast<uchar *>(wordp)+sizeof(unsigned long) <= buf+length; wordp++) { hashval= hashval ^ *wordp; }; 
  return hashval; 
}

/***** class nli *****/

/***** IE name *****/

const char* const nli::iename = "nli";

const char* nli::get_ie_name() const { return iename; }

//contlen is absolutely irrelevant, get_serialized_size will NOT use it for objects of variable length!
const size_t nli::contlen = 24;

/***** inherited from IE *****/

nli* nli::new_instance() const {
	nli* sh = NULL;
	catch_bad_alloc(sh = new nli());
	return sh;
} // end new_instance

nli* nli::copy() const {
	nli* sh = NULL;
	catch_bad_alloc(sh =  new nli(*this));
	return sh;
} // end copy 

nli* nli::copy2(netaddress if_address) const {
	nli* sh = NULL;
	catch_bad_alloc(sh =  new nli(*this));
	sh->if_address = if_address;
	sh->ip_version= if_address.is_ipv6() ? 6 : 4;
	return sh;
} // end copy 

nli* 
nli::deserialize(NetMsg& msg, coding_t cod, IEErrorList& errorlist, uint32& bread, bool skip) {
	
    uint16 len = 0;
    uint32 ielen = 0;
    uint32 saved_pos = 0;
    uint32 resume = 0;
    uint8 msg_pi_length=0;
    

    // check arguments
    if (!check_deser_args(cod,errorlist,bread)) 
	return NULL;
    // decode header
    if (!decode_header_ntlpv1(msg,len,ielen,saved_pos,resume,errorlist,bread,skip)) 
	return NULL;
    
    // decode PI Length
    msg_pi_length = msg.decode8();
    //DLog("NLI", "decoded PI length of " << (int) msg_pi_length);

    pi = new peer_identity(round_up4(msg_pi_length));

    //DLog("NLI", "new PI object has length of " << (int) pi->length);
    
    // decode IP_TTL
    ip_ttl = msg.decode8();
    
    // decode ip version
    ip_version = msg.decode16()>>12;
    
    // decode RS Validity Time
    rs_validity_time = msg.decode32();

    // decode PI Padding is included in peer_identity
    msg.decode(pi->get_buffer(), round_up4(msg_pi_length));

    //DLog("NLI", "decoded " << msg_pi_length << " byte of PI");

    //decode Interface address
    in_addr tmpaddr4;

    in6_addr tmpaddr6;

    if (ip_version == 6) {
	msg.decode(tmpaddr6);
	if_address.set_ip(tmpaddr6);
    } else {
	msg.decode(tmpaddr4);
	if_address.set_ip(tmpaddr4);
    }
 
    // There is no padding.
    bread = ielen;

    //check for correct length
    if (ielen != get_serialized_size(cod)) {
	ERRCLog("NLI", "Incorrect Object Length Error");
	errorlist.put(new GIST_IncorrectObjectLength(protocol_v1, this));
    }

    return this;
} // end deserialize

void nli::serialize(NetMsg& msg, coding_t cod, uint32& wbytes) const {
    uint32 ielen = get_serialized_size(cod); //+header_length;
    // check arguments and IE state
    check_ser_args(cod,wbytes);
 
    // calculate length and encode header
    encode_header_ntlpv1(msg,get_serialized_size(cod)-4);

    //encode pi_length
    msg.encode8(pi->get_length());

    //encode ip_ttl
    msg.encode8(ip_ttl);

    //encode ip_version
    msg.encode16(ip_version <<12);
	
    //encode RS Validity Time
    msg.encode32(rs_validity_time);

    // encode pi
    msg.encode(pi->get_buffer(), pi->get_length());

    struct in_addr ip4addr;
    if (ip_version==6) 
    {
      const struct in6_addr *ip6addr=if_address.get_ip();
      if (ip6addr)
	msg.encode(*ip6addr);
      else
      {
	ERRCLog("respcookie::serialize()", "No valid IPv6 address in if_address");
	msg.encode(in6addr_any);
      }
    } 
    else 
    {
      if_address.get_ip(ip4addr);
      msg.encode(ip4addr);
    }
	
    wbytes = ielen;

    //ostringstream tmpostr;
    //msg.hexdump(tmpostr,0,wbytes);
    //Log(DEBUG_LOG,LOG_NORMAL, "nli::serialize", "netmsg pos:" << msg.get_pos() << " netmsg:" << tmpostr.str().c_str());
    return;
} // end serialize

bool nli::check() const {
	return (true);
} // end check

size_t nli::get_serialized_size(coding_t cod) const {
    int size;

    if (!supports_coding(cod)) throw IEError(IEError::ERROR_CODING);
    
    // 2 x 32bits header
    size=64;
    
    size=size + round_up4(pi->get_length())*8;
    
    if (ip_version==6) size+=128; else size+=32;
  
    return (size/8)+header_length;

} // end get_serialized_size

bool nli::operator==(const IE& ie) const {
	const nli* sh = dynamic_cast<const nli*>(&ie);
	if (sh) return ((ip_ttl==sh->ip_ttl)
			&& (rs_validity_time==sh->rs_validity_time)
			&& (*pi==*(sh->pi))
			&& (if_address==sh->if_address));
	return false;
} // end operator==
 

ostream& 
nli::print(ostream& os, uint32 level, const uint32 indent, const char* name) const 
{
    	if (name && (*name!=0)) os<<name<<" ";
	os << '<' << get_ie_name() << ">: (" << action << ")\n" << endl;
	os << "IP_TTL: " << (int)ip_ttl << endl;
	os << "IP_Version: " << (int)ip_version << endl;
	os << "RS-Validity-Time: " << (int)rs_validity_time << endl;
	if (pi) os << "PI length: " << (int)pi->get_length() << endl; 
	os << "PI: "; 
	if (pi) 
	{ 
	  os << *pi;
	} 
	else 
	  os << "<Empty>";
	os << endl;
	os << "IF_Address: " << if_address.get_ip_str() << endl;

	return os;
} // end print

istream& nli::input(istream& is, bool istty, uint32 level, const uint32 indent, const char* name) {
    
        string s;
	unsigned short in_no;

	if (istty) {
		cout<<"";
		if (name && (*name!=0)) cout<<name<<" ";
		cout<<"<"<<get_ie_name()<<"> IP TTL: ";
	} // end if istty
	is>>in_no;
	ip_ttl=in_no;
	cout << endl;
	if (istty) {
		cout<<"";
		if (name && (*name!=0)) cout<<name<<" ";
		cout<<"<"<<get_ie_name()<<"> IP Version: ";
	} // end if istty
	is>>in_no;
	ip_version= in_no;
	cout << endl;
	if (istty) {
		cout<<"";
		if (name && (*name!=0)) cout<<name<<" ";
		cout<<"<"<<get_ie_name()<<"> RS Time (32bit): ";
	} // end if istty
	is>>rs_validity_time;
	cout << endl;
	if (istty) {
		cout<<"";
		if (name && (*name!=0)) cout<<name<<" ";
		cout<<"<"<<get_ie_name()<<"> 1st 32bit of PI: ";
	} // end if istty

	pi = new peer_identity(4);
	
	is>>pi->get_buffer();
		
	if (istty) cout<<""<<"IF_Address: ";
	
	cin >> s;

	if_address=hostaddress(s.c_str());

	return is;
} // end input

//@}

} // end namespace protlib
