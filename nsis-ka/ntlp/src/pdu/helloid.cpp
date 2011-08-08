/// ----------------------------------------*- mode: C++; -*--
/// @file helloid.cpp
/// GIST 32-bit hello ID
/// ----------------------------------------------------------
/// $Id: helloid.cpp 5249 2010-04-26 08:40:59Z roehricht $
/// $HeadURL: https://svn.tm.kit.edu/nsis/ntlp/trunk/src/pdu/helloid.cpp $
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

#include "helloid.h"

#include "logfile.h"


using namespace ntlp;
using namespace protlib;
using namespace protlib::log;


/**
 * @defgroup iehelloid helloID Objects
 *  @ingroup ientlpobject
 * @{
 */


const size_t helloid::contlen = 4;
const char *const helloid::iename = "helloid";

const char *helloid::get_ie_name() const {
	return iename;
}


helloid *helloid::new_instance() const {
	helloid* sh = NULL;
	catch_bad_alloc(sh = new helloid());
	return sh;
}


helloid *helloid::copy() const {
	helloid* sh = NULL;
	catch_bad_alloc(sh = new helloid(*this));
	return sh;
}



helloid *helloid::deserialize(NetMsg &msg, coding_t cod,
		IEErrorList &errorlist, uint32 &bytes_read, bool skip) {

	return deserializeEXT(msg, cod, errorlist, bytes_read, skip, true);
}


helloid *helloid::deserializeNoHead(NetMsg &msg, coding_t cod,
		IEErrorList &errorlist, uint32 &bytes_read, bool skip) {

	return deserializeEXT(msg, cod, errorlist, bytes_read, skip, false);
}


// Backend for the other two deserialization methods.
helloid *helloid::deserializeEXT(NetMsg &msg, coding_t cod,
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

	// Decode the hello ID.
	a = msg.decode32();

	// There is no padding.
	bytes_read = ielen;

	// Check for correct length.
	if ( header && (ielen != get_serialized_size(cod)) ) {
		ERRCLog("helloID", "Incorrect Object Length Error");
		errorlist.put(
			new GIST_IncorrectObjectLength(protocol_v1, this)
		);
	}

	return this;
}


void helloid::serialize(NetMsg &msg, coding_t cod, uint32 &bytes_written)
		const {

	serializeEXT(msg, cod, bytes_written, true);
}


void helloid::serializeNoHead(NetMsg &msg, coding_t cod,
		uint32 &bytes_written) const {

	serializeEXT(msg, cod, bytes_written, false);
}


// Backend for the other two serialization methods.
void helloid::serializeEXT(NetMsg &msg, coding_t cod, uint32 &bytes_written,
		bool header) const {

	uint32 ielen = get_serialized_size(cod); // includes object header

	// check arguments and IE state
	check_ser_args(cod, bytes_written);

	// If header is desired, calculate length and encode header.
        if ( header )
		encode_header_ntlpv1(msg, ielen - header_length);

	msg.encode32(a);
	
	bytes_written = header ? ielen : ielen - header_length;
}


bool helloid::check() const {
	return true;
}


size_t helloid::get_serialized_size(coding_t cod) const {

	if ( ! supports_coding(cod) )
		throw IEError(IEError::ERROR_CODING);
    
	return contlen + header_length;
}


bool helloid::operator==(const IE &ie) const {
	const helloid *other = dynamic_cast<const helloid *>(&ie);

	return other != NULL && a == other->a;
}


string 
helloid::to_string() const 
{
  ostringstream os;
  os  << "0x" << setw(8) << setfill('0') << hex << a << dec;
  return os.str();
}

ostream &helloid::print(ostream &os, uint32 level, const uint32 indent,
		const char *name) const {

	os << setw(level*indent);

	if ( name != NULL && name[0] != '\0' )
		os << name << " ";

	os << '<' << get_ie_name() << ">: 0x"
		<< setw(8) << setfill('0') << hex << a 
		<< setfill(' ') << dec << endl; 

	return os;
}


istream &helloid::input(istream &is, bool istty, uint32 level,
		const uint32 indent, const char *name) {

	std::string s;
   
	if ( istty ) {
		if ( name != NULL && name[0] != '\0' )
			cout << name << " ";

		cout << "<" << get_ie_name() << "> 32bit: ";
	}

	is >> a;

	return is;
}


uint32 helloid::get_helloid() const
{
	return a;
}


// @}

// EOF
