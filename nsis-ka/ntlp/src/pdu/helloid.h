/// ----------------------------------------*- mode: C++; -*--
/// @file helloid.h
/// GIST hello id (32 bit)
/// ----------------------------------------------------------
/// $Id: helloid.h 5249 2010-04-26 08:40:59Z roehricht $
/// $HeadURL: https://svn.tm.kit.edu/nsis/ntlp/trunk/src/pdu/helloid.h $
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
#ifndef NTLP__helloID_H
#define NTLP__helloID_H

#include "protlib_types.h"
#include "ntlp_object.h"


namespace ntlp {
  using namespace protlib;


/**
 * The representation of a GIST 32 Bit hello-ID.
 */
class helloid : public known_ntlp_object {
  public:
	helloid();
	helloid(const helloid &n);
	helloid(uint32 a);

	virtual inline ~helloid() { }

	virtual helloid *new_instance() const;
	virtual helloid *copy() const;

	virtual helloid *deserialize(NetMsg &msg, coding_t cod,
		IEErrorList &errorlist, uint32 &bytes_read, bool skip=true);
	virtual helloid *deserializeNoHead(NetMsg &msg, coding_t cod,
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


	uint32 get_helloid() const;
	
	// output hello id as readable string
	string to_string() const;

  private:
	uint32 a;	// The actual ID
    
	helloid *deserializeEXT(NetMsg &msg, coding_t cod,
		IEErrorList &errorlist, uint32 &bytes_read, bool skip=true,
		bool header=true);
	virtual void serializeEXT(NetMsg &msg, coding_t cod,
		uint32 &bytes_written, bool header) const;

	static const char *const iename;
	static const size_t contlen;    
};



inline helloid::helloid()
		: known_ntlp_object(HelloID, Mandatory),
		  a(0) {

	// nothing to do
}


inline helloid::helloid(const helloid &other)
		: known_ntlp_object(HelloID, Mandatory),
		  a(other.a) {

	// nothing to do
}


inline helloid::helloid(uint32 a)
		: known_ntlp_object(HelloID, Mandatory),
		  a(a) {

	// nothing to do
}


} // namespace ntlp

#endif // NTLP__helloID_H
