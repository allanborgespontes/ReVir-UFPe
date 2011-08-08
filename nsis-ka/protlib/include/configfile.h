/// ----------------------------------------*- mode: C++; -*--
/// @file configfile.h
/// Handling of simple (key, value) configuration files
/// to be used with configpar and configpar_repository
/// ----------------------------------------------------------
/// $Id: configfile.h 4107 2009-07-16 13:49:52Z bless $
/// $HeadURL: https://svn.tm.kit.edu/nsis/protlib/trunk/include/configfile.h $
// ===========================================================
//                      
// Copyright (C) 2009, all rights reserved by
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
#ifndef _CONFIGFILE__H
#define _CONFIGFILE__H

#include <fstream>
#include <map>
#include <list>
#include <exception>

#include "configpar_repository.h"

namespace protlib {

/**
 * An exception to be thrown if an invalid configuration is read.
 */
class configfile_error : public std::exception {
  public:
	configfile_error(const std::string &msg="Unspecified configuration file error",
			int line=-1) throw () : msg(msg), line(line) { }
	virtual ~configfile_error() throw () { }

	std::string get_msg() const throw () { return msg; }
	int get_line() const throw () { return line; }
	std::string get_line_str() const throw () { stringstream linestr; linestr << line; return linestr.str(); }
	const char* what() const throw() { string* whatstr= new string(get_msg() + " at line " + get_line_str()); return whatstr->c_str(); }

  private:
	std::string msg;
	int line;
};

inline std::ostream &operator<<(std::ostream &os, const configfile_error &err) {
	if ( err.get_line() > 0 )
		return os << err.get_msg() << " at line " << err.get_line();
	else
		return os << err.get_msg();
}



/**
 * A class for handling simple configuration files.
 *
 * The configuration consists of (key, value) pairs, where both key and value
 * are strings.
 *
 * A configuration file is line-oriented and has the following format:
 *   [space] key [space] = [space] value [space] [# optional comment] EOL
 *
 * Value can be a boolean value, an integer, a float, an IP address (either
 * IPv4 or IPv6), a string, IP address lists, or any other type for which 
 * an output operator exists. String values and address lists have to be 
 * quoted using double quotes. If a double quote should appear in the string, 
 * you have to quote it using a backslash. A backslash in turn has to be quoted 
 * using another backslash.
 *
 * Lines starting with '#' and empty lines are ignored.
 */
class configfile {
  public:
	configfile(configpar_repository* confpar_repository) : confpar_repository(confpar_repository) {};

	void load(const std::string &filename) throw (configfile_error);
	void load(std::istream &in) throw (configfile_error);
	void save(const std::string &filename) throw (configfile_error);
	void dump(std::ostream &out) throw (configfile_error);

private:
	/// fill parname_map with all configpar names of the given realm
	void collectParNames(realm_id_t realm);

	configpar_repository* confpar_repository;
	typedef map<std::string, configpar_id_t> parname_map_t;
	parname_map_t parname_map;
};


} // namespace protlib

#endif // __CONFIGFILE__H
