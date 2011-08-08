/// ----------------------------------------*- mode: C++; -*--
/// @file sessionid.h
/// GIST session id (128 bit)
/// ----------------------------------------------------------
/// $Id: sessionid.h 5249 2010-04-26 08:40:59Z roehricht $
/// $HeadURL: https://svn.tm.kit.edu/nsis/ntlp/trunk/include/sessionid.h $
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
#ifndef NTLP__SESSIONID_H
#define NTLP__SESSIONID_H

#include "protlib_types.h"
#include "ntlp_object.h"


namespace ntlp {
  using namespace protlib;


/**
 * The representation of a GIST 128 Bit Session-ID.
 */
class sessionid : public known_ntlp_object {
  public:
	sessionid();
	sessionid(const sessionid &n);
        sessionid(const uint128& sid);
	sessionid(uint32 a, uint32 b, uint32 c, uint32 d);

	virtual inline ~sessionid() { }

	virtual sessionid *new_instance() const;
	virtual sessionid *copy() const;

	virtual sessionid *deserialize(NetMsg &msg, coding_t cod,
		IEErrorList &errorlist, uint32 &bytes_read, bool skip=true);
	virtual sessionid *deserializeNoHead(NetMsg &msg, coding_t cod,
		IEErrorList &errorlist, uint32 &bytes_read, bool skip=true);

	virtual void serialize(NetMsg &msg, coding_t cod,
		uint32 &bytes_written) const;
	virtual void serializeNoHead(NetMsg &msg, coding_t cod,
		uint32 &bytes_written) const;

	virtual bool operator==(const IE& ie) const;
	virtual bool check() const;
	virtual size_t get_serialized_size(coding_t coding) const;

	virtual const char *get_ie_name() const;
	virtual ostream &print(ostream& os, uint32 level, const uint32 indent,
		const char *name=NULL) const;
	virtual istream &input(istream& is, bool istty, uint32 level,
		const uint32 indent, const char *name=NULL);

        void set_sessionid(uint32 a1, uint32 b1, uint32 c1, uint32 d1) { a= a1; b= b1; c= c1; d= d1; };

	void get_sessionid(uint32 &a1, uint32 &b1, uint32 &c1, uint32 &d1)
		const;
	
	// output session id as readable string
	string to_string() const;

       // returns XOR folded value, used for the routing table.
	long get_hash() const;		
  
        // generate a random session id
        void generate_random();

      
        operator uint128() const { return uint128(a,b,c,d); };
        sessionid&  operator=(const uint128& n) { a=n.w1; b=n.w2; c=n.w3; d=n.w4; return *this; };

  private:
	static unsigned int random_seed;

	uint32 a, b, c, d;	// 32 words 1 to 4.
    
	sessionid *deserializeEXT(NetMsg &msg, coding_t cod,
		IEErrorList &errorlist, uint32 &bytes_read, bool skip=true,
		bool header=true);
	virtual void serializeEXT(NetMsg &msg, coding_t cod,
		uint32 &bytes_written, bool header) const;

	static const char *const iename;
	static const size_t contlen;    
};


inline 
sessionid::sessionid()
		: known_ntlp_object(SessionID, Mandatory),
		  a(0), b(0), c(0), d(0) {

	// nothing to do
}


inline 
sessionid::sessionid(const sessionid &other)
		: known_ntlp_object(SessionID, Mandatory),
		  a(other.a), b(other.b), c(other.c), d(other.d) {

	// nothing to do
}


inline 
sessionid::sessionid(uint32 a, uint32 b, uint32 c, uint32 d)
  : known_ntlp_object(SessionID, Mandatory),
    a(a), b(b), c(c), d(d) {
	// nothing to do
}


inline
sessionid::sessionid(const uint128& sid) 
  : known_ntlp_object(SessionID, Mandatory), 
    a(sid.w1), b(sid.w2), c(sid.w3), d(sid.w4) 
{
}


inline 
long sessionid::get_hash() const {
	return a ^ b ^ c ^ d;
}


istream& 
operator>>(istream&s, sessionid& sid);

ostream& 
operator<<(ostream& os, sessionid& sid);

} // namespace ntlp



#endif // NTLP__SESSIONID_H
