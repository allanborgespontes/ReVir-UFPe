/// ----------------------------------------*- mode: C++; -*--
/// @file sessionid.cpp
/// GIST 128-bit session ID
/// ----------------------------------------------------------
/// $Id: sessionid.cpp 5249 2010-04-26 08:40:59Z roehricht $
/// $HeadURL: https://svn.tm.kit.edu/nsis/ntlp/trunk/src/pdu/sessionid.cpp $
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
#include <iostream>
#include <sstream>
#include <string>
#include <cerrno>
#include <iomanip>

#include <cstdlib>
#include <sys/time.h>

#include "sessionid.h"

#include "logfile.h"

namespace ntlp {

using namespace protlib;
using namespace protlib::log;


/**
 * @defgroup iesessionid SessionID Objects
 *  @ingroup ientlpobject
 * @{
 */


const size_t sessionid::contlen = 16;
const char *const sessionid::iename = "sessionid";
unsigned int sessionid::random_seed = 0;


const char *sessionid::get_ie_name() const {
	return iename;
}


sessionid *sessionid::new_instance() const {
	sessionid* sh = NULL;
	catch_bad_alloc(sh = new sessionid());
	return sh;
}


sessionid *sessionid::copy() const {
	sessionid* sh = NULL;
	catch_bad_alloc(sh = new sessionid(*this));
	return sh;
}



sessionid *sessionid::deserialize(NetMsg &msg, coding_t cod,
		IEErrorList &errorlist, uint32 &bytes_read, bool skip) {

	return deserializeEXT(msg, cod, errorlist, bytes_read, skip, true);
}


sessionid *sessionid::deserializeNoHead(NetMsg &msg, coding_t cod,
		IEErrorList &errorlist, uint32 &bytes_read, bool skip) {

	return deserializeEXT(msg, cod, errorlist, bytes_read, skip, false);
}


// Backend for the other two deserialization methods.
sessionid *sessionid::deserializeEXT(NetMsg &msg, coding_t cod,
		IEErrorList &errorlist, uint32 &bytes_read, bool skip,
		bool header) {
	
	uint16 len = 0;
	uint32 ielen = 0;
	uint32 saved_pos = 0;
	uint32 resume = 0;

	// check arguments
	if ( ! check_deser_args(cod, errorlist, bytes_read) ) 
		return NULL;

	if ( header ) {
		if ( ! decode_header_ntlpv1(msg, len, ielen, saved_pos,
				resume, errorlist, bytes_read, skip) ) 
			return NULL;
	}

	// Decode all 128 Bits of the session ID.
	a = msg.decode32();
	b = msg.decode32();
	c = msg.decode32();
	d = msg.decode32();

	// There is no padding.
	bytes_read = ielen;

	// Check for correct length.
	if ( header && (ielen != get_serialized_size(cod)) ) {
		ERRCLog("SessionID", "Incorrect Object Length Error");
		errorlist.put(
			new GIST_IncorrectObjectLength(protocol_v1, this)
		);
	}

	return this;
}


void sessionid::serialize(NetMsg &msg, coding_t cod, uint32 &bytes_written)
		const {

	serializeEXT(msg, cod, bytes_written, true);
}


void sessionid::serializeNoHead(NetMsg &msg, coding_t cod,
		uint32 &bytes_written) const {

	serializeEXT(msg, cod, bytes_written, false);
}


// Backend for the other two serialization methods.
void sessionid::serializeEXT(NetMsg &msg, coding_t cod, uint32 &bytes_written,
		bool header) const {

	uint32 ielen = get_serialized_size(cod); // includes object header

	// check arguments and IE state
	check_ser_args(cod, bytes_written);

	// If header is desired, calculate length and encode header.
        if ( header )
		encode_header_ntlpv1(msg, ielen - header_length);

	msg.encode32(a);
	msg.encode32(b);
	msg.encode32(c);
	msg.encode32(d);
	
	bytes_written = header ? ielen : ielen - header_length;
}


bool sessionid::check() const {
	return true;
}


size_t sessionid::get_serialized_size(coding_t cod) const {

	if ( ! supports_coding(cod) )
		throw IEError(IEError::ERROR_CODING);
    
	int size = 128;		// 4 x 32 Bits

	return (size/8) + header_length;
}


bool sessionid::operator==(const IE &ie) const {
	const sessionid *other = dynamic_cast<const sessionid *>(&ie);

	return other != NULL && a == other->a && b == other->b
		&& c == other->c && d == other->d;
}


string 
sessionid::to_string() const 
{
  ostringstream os;
  os  << setw(8) << setfill('0') << hex << a << '-' 
      << setw(8) << setfill('0') << hex << b << '-' 
      << setw(8) << setfill('0') << hex << c << '-' 
      << setw(8) << setfill('0') << hex << d << setfill(' ') << dec; 

  return os.str();
}

ostream &sessionid::print(ostream &os, uint32 level, const uint32 indent,
		const char *name) const {

	os << setw(level*indent);

	if ( name != NULL && name[0] != '\0' )
		os << name << " ";

	os << '<' << get_ie_name() << ">: "
		<< setw(8) << setfill('0') << hex << a << '-' 
		<< setw(8) << setfill('0') << hex << b << '-' 
		<< setw(8) << setfill('0') << hex << c << '-' 
		<< setw(8) << setfill('0') << hex << d
		<< setfill(' ') << dec << endl; 

	return os;
}


istream &sessionid::input(istream &is, bool istty, uint32 level,
		const uint32 indent, const char *name) {

	std::string s;
   
	if ( istty ) {
		if ( name != NULL && name[0] != '\0' )
			cout << name << " ";

		cout << "<" << get_ie_name() << "> 1st 32bit: ";
	}

	is >> a;


	if ( istty ) {
		if ( name != NULL && name[0] != '\0' )
			cout << name << " ";

		cout << "<" << get_ie_name() << "> 2nd 32bit: ";
	}

	is >> b;


	if ( istty ) {
		if ( name != NULL && name[0] != '\0' )
			cout << name << " ";

		cout << "<" << get_ie_name() << "> 3rd 32bit: ";
	}

	is >> c;


	if ( istty ) {
		if ( name != NULL && name[0] != '\0' )
			cout << name << " ";

		cout << "<" << get_ie_name() << "> 4th 32bit: ";
	}

	is >> d;

	return is;
}


void 
sessionid::get_sessionid(uint32 &a1, uint32 &b1, uint32 &c1, uint32 &d1)
		const {

	a1 = a;
	b1 = b;
	c1 = c;
	d1 = d;
}


void 
sessionid::generate_random() {
	if (random_seed == 0) {
		// use milliseconds precision for random
		// number initialization and XOR values
		// with current PID
		struct timeval tv;
		gettimeofday(&tv, 0);

		random_seed = (tv.tv_sec ^ tv.tv_usec) ^ getpid();
	}

	a = rand_r(&random_seed);
	b = rand_r(&random_seed);
	c = rand_r(&random_seed);
	d = rand_r(&random_seed);
}


istream& 
operator>>(istream&s, sessionid& sid)
{
  uint32 a,b,c,d;
  char ch;

  
  s >> hex >> a >> ch;
  if (ch == '-')
  {
    s >> hex >> b >> ch;
    if (ch == '-')
    {
      s >> hex >> c >> ch;
      if (ch == '-')
      {
	s >> hex >> d >> ch;
      }
      else
      {
	s.putback(ch);
	s.clear(ios_base::badbit);
	return s;
      }
    }
    else
    {
      s.putback(ch);
      s.clear(ios_base::badbit);
      return s;
    }
  }
  else
  {
    s.putback(ch);
    s.clear(ios_base::badbit);
    return s;
  }

  sid.set_sessionid(a,b,c,d);

  return s;
}


ostream& operator<<(ostream& os, sessionid& sid)
{
  uint32 a,b,c,d;
  sid.get_sessionid(a,b,c,d);

  os  << setw(8) << setfill('0') << hex << a << '-' 
      << setw(8) << setfill('0') << hex << b << '-' 
      << setw(8) << setfill('0') << hex << c << '-' 
      << setw(8) << setfill('0') << hex << d; 

  return os;
}

// @}

} // namespace ntlp

// EOF
