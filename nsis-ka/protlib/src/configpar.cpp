/// ----------------------------------------*- mode: C++; -*--
/// @file configpar.cpp
/// Configuration parameters and their repository
/// ----------------------------------------------------------
/// $Id: configpar.cpp 4107 2009-07-16 13:49:52Z bless $
/// $HeadURL: https://svn.tm.kit.edu/nsis/protlib/trunk/src/configpar.cpp $
// ===========================================================
//                      
// Copyright (C) 2008, all rights reserved by
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
#include "configpar.h"

namespace protlib {
  using namespace std;

/**
 * constructor for generic base class
 */
configparBase::configparBase(realm_id_t realm, configpar_id_t configparid, const char* name, const char* description, bool changeable_while_running, const char* unitinfo) :
    realm_id(realm),
    par_id(configparid),
    changeable_while_running(changeable_while_running),
    name(name),
    description(description),
    unit(unitinfo)
{
}


bool 
configparBase::operator==(const configparBase& cfgpar) const
{
  return realm_id  == cfgpar.realm_id
    &&     par_id  == cfgpar.par_id
    &&       name  == cfgpar.name
    && description == cfgpar.description;
}



void 
configparBase::strip_leading_space(std::istream &in) {
	while ( in && ( in.peek() == ' ' || in.peek() == '\t' ) )
		in.get();
}


// Write the string to the stream, adding quotation marks and escape sequences
void 
configparBase::write_quoted_string(std::ostream &out, const std::string &str) {

	std::stringstream stream(str);

	out << '"';

	char c;
	while ( stream.get(c) ) {
		switch ( c ) {
			case '\\':	// fallthrough
			case '"':	out << '\\' << c; break;
			default:	out << c;
		}
	}

	out << '"';
}


// throw an exception if the rest of the line doesn't only contain whitespace or a comment starting with #
void 
configparBase::skip_rest_of_line(std::istream &in) {
	char c;
	bool ignore= false;
	while ( in.get(c) ) {
		if (ignore)
			continue;
		else
		if (c == '#')
			ignore= true;
		else
		if ( c != ' ' && c != '\t' )
			throw configParExceptionParseError("junk after value");
	}
}

/** returns inner content of a quoted string
 *  Matches pattern "[^"]*"\s* and returns matching part without quotes
 *
 */
std::string 
configparBase::parse_quoted_string(std::istream &in) {

	if ( in.get() != '"' )
		throw configParExceptionParseError("string doesn't start with a quotation mark");

	bool escape = false;
	std::string tmp;
	char c;

	while ( in.get(c) ) {
		if ( escape ) {
			if ( c == '\\' || c == '"' )
				tmp += c;
			else
				throw configParExceptionParseError("invalid escape sequence");

			escape = false;
		}
		else {
			if ( c == '"' )
				break;
			else if ( c == '\\' )
				escape = true;
			else
				tmp += c;
		}
	}

	// we should be on the closing quotation mark
	if ( c != '"' )
		throw configParExceptionParseError("unterminated string");

	skip_rest_of_line(in);

	return tmp;
}


/**
 * appends a comment with informational text about the unit
 * if available
 */
ostream& 
configparBase::appendUnitInfoComment(ostream& outstream) const
{
	if (unit)
		return outstream << "      ## unit [" << unit << ']'; 
	else
		return outstream;
}


/** template specializations for certain parameter types **/
template <>
istream& 
configpar<bool>::readVal(std::istream &in) {
	std::string tmp;

	in >> tmp;

	if ( tmp == "true" || tmp == "on" || tmp == "yes" || tmp == "1" )
		value = true;
	else if ( tmp == "false" || tmp == "off" || tmp == "no" || tmp == "0" )
		value = false;
	else
		throw configParExceptionParseError("parsing boolean type failed");

	skip_rest_of_line(in);

	return in;
}


template <>
ostream& 
configpar<bool>::writeVal(std::ostream &out) const
{
	return out << (value ? "yes" : "no");
}


template <>
istream& 
configpar<int>::readVal(std::istream &in) {
	int tmp = -1;

	in >> tmp;

	if ( ! in.good() && ! in.eof() )
		throw configParExceptionParseError("parsing integer failed");

	skip_rest_of_line(in);

	value= tmp;

	return in;
}


template <>
istream& 
configpar<float>::readVal(std::istream &in) {
	float tmp = 0.0;

	in >> tmp;

	if ( ! in.good() && ! in.eof() )
		throw configParExceptionParseError("parsing float failed");

	skip_rest_of_line(in);

	value= tmp;

	return in;
}


template <>
istream& 
configpar< list<hostaddress> >::readVal(std::istream &in) {
	std::string tmp;
	bool success= false;
	while ( in >> tmp ) {
		hostaddress addr(tmp.c_str(), &success);
		if ( success )
			value.push_back(addr);
		else
			throw configParExceptionParseError("invalid host address `" + tmp + "'");
	}
	return in;
}


template <>
istream& 
configpar< string >::readValFromConfig(std::istream &in) {
	value= parse_quoted_string(in);
	return in;
}


// address lists will be quoted in config files
template <>
istream& 
configpar< list<hostaddress> >::readValFromConfig(std::istream &in) {
	std::string tmp= parse_quoted_string(in);
	stringstream inputstr(tmp);

	bool success= false;
	while ( inputstr >> tmp ) {
		hostaddress addr(tmp.c_str(), &success);
		if ( success )
			value.push_back(addr);
		else
			throw configParExceptionParseError("invalid host address `" + tmp + "'");
	}
	return in;
}



// write address list quoted to a config file
template <>
ostream& 
configpar< list<hostaddress> >::writeValToConfig(ostream& outstream) const
{
	std::stringstream addressliststr;
	addressliststr << value;
	write_quoted_string(outstream, addressliststr.str());
        return outstream;
}

// write a string quoted (with escape chars) to a config file
template <>
ostream& 
configpar<string>::writeValToConfig(ostream& outstream) const
{
	write_quoted_string(outstream, value);
	return outstream;
}

} // end namespace
