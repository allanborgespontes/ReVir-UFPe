/// ----------------------------------------*- mode: C++; -*--
/// @file configuration.cpp
/// A configuration file parser
/// ----------------------------------------------------------
/// $Id: configfile.cpp 4107 2009-07-16 13:49:52Z bless $
/// $HeadURL: https://svn.tm.kit.edu/nsis/protlib/trunk/src/configfile.cpp $
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
#include <iostream>
#include <sstream>

#include "configfile.h"


namespace protlib {



/** fill parname_map with all configpar names of the given realm
 */
void 
configfile::collectParNames(realm_id_t realm)
{
	// clear the whole map
	parname_map.clear();

	unsigned int max_pars= confpar_repository->getRealmSize(realm);
	configparBase* cfgpar= NULL;
	for (unsigned int i= 0; i < max_pars; i++) {
		try {
			// get config par with id i
			cfgpar= confpar_repository->getConfigPar(realm, i);
			// store mapping from configpar name to parameter id i
			if (cfgpar) 
				parname_map[cfgpar->getName()]= i;
		} // end try
		catch (configParExceptionParNotRegistered) {
			cfgpar= NULL;
			continue;
		} // end catch
		catch (...) {
		  throw;
		}
	} // end for
}

/**
 * Load a configuration file.
 *
 * If there's a parse error or the file can't be opened, a configfile_error
 * exception is thrown.
 *
 * @param filename the file to load
 */
void configfile::load(const std::string &filename) throw (configfile_error) {

	std::ifstream in(filename.c_str());

	if ( ! in )
		throw configfile_error("cannot open file `" + filename + "'");

	try {
		this->load(in);
	}
	catch ( configfile_error &e ) {
		in.close();
		throw;
	}
	catch ( ... ) {
		in.close();
		throw configfile_error("unknown exception thrown");
	}

	in.close();
}


void 
configfile::save(const std::string &filename) throw (configfile_error) {

	std::ofstream outfile(filename.c_str());

	if ( ! outfile )
		throw configfile_error("cannot open file `" + filename + "'");

	try {
		this->dump(outfile);
	}
	catch ( configfile_error &e ) {
		outfile.close();
		throw;
	}
	catch ( ... ) {
		outfile.close();
		throw configfile_error("unknown exception thrown");
	}

	outfile.close();
}


/**
 * Load configuration data from a stream.
 *
 * If there is a parse error, a configfile_error exception is thrown. This method
 * will read until end of file. It is up to the caller to close the stream.
 *
 * @param in_stream the input stream to read data from
 */
void configfile::load(std::istream &in_stream) throw (configfile_error) {
	using namespace std;

	const unsigned int max_keyword_len= 256;
	char keywordbuf[max_keyword_len];

	const unsigned int max_realm_name_len= 256;
	char realmnamebuf[max_realm_name_len];
	realm_id_t current_realm=  confpar_repository->getMaxRealms();

	bool skip_this_realm= false;
	for (int line = 1; in_stream; line++) {
		string buf;
		string key;

		getline(in_stream, buf);

		stringstream in(buf);

		// skip leading whitespace
		configparBase::strip_leading_space(in);

		// skip empty lines and comments
		if ( in.peek() == -1 || in.peek() == '#' )
			continue;

		// check for realm name token [realm]
		if ( in.peek() == '[' )
		{
			in.get(); // read the [
			in.getline(realmnamebuf, max_realm_name_len, ']');
			if ( !in )
				throw configfile_error("parse error - realm name definition must be enclosed like this: [myrealm]", line);
			
			current_realm= confpar_repository->getRealmId( realmnamebuf );
			if ( confpar_repository->existsRealm(current_realm) )
			{ // realm valid or registered
				// prepare map for easier parsing parameter names
				collectParNames(current_realm);
				skip_this_realm= false;
			}
			else // skip over unknown realms
			{
				skip_this_realm= true;
			}
			// read in next line
			continue;
		}

		// if realm should be skipped, proceed reading in the next line
		if (skip_this_realm)
			continue;

		// read the keyword
		in.getline(keywordbuf, max_keyword_len, '=');
		// terminate at first trailing white space
		// getline terminates with \0
		for (char *c= keywordbuf; *c!='\0'; c++)
		{
			if (isspace(*c))
			{
				*c='\0';
			}
		} // end for
		key= keywordbuf;
		if ( key == "")
			throw configfile_error("parse error - expected config parameter name", line);

		parname_map_t::const_iterator cit= parname_map.find(key);
		configpar_id_t configparid= 0;
		if ( cit == parname_map.end() )
			throw configfile_error("invalid/unknown parameter name `" + key + "'", line);
		else
			configparid= cit->second;
		

		// skip space between '=' and value
		configparBase::strip_leading_space(in);

		// no value for this key, we ignore it altogether
		if ( in.peek() == -1 )
			continue;

		// read in the value
		try {
			// get parameter from config repository
			configparBase* cfgpar= confpar_repository->getConfigPar(current_realm, configparid);
			cfgpar->readValFromConfig(in);
		}
		catch ( configParExceptionParseError &e ) {
			throw configfile_error(e.what(), line);
		}
	} // all lines should have been read now

	if ( ! in_stream.eof() )
		throw configfile_error("stream error");

	// check if all required config settings are set.
// 	for ( c_iter i = values.begin(); i != values.end(); i++ ) {
// 		const config_entry &e = i->second;

// 		if ( e.required && ! e.defined )
// 			throw configfile_error(
// 				"key `" + e.key + "' required but not set");
// 	}
}



/**
 * Write the configuration data to a stream.
 *
 * If there is a write error, a configfile_error exception is thrown. This method
 * doesn't close the stream after writing. It is up to the caller to do that.
 *
 * @param out the output stream to read data from
 */
void configfile::dump(std::ostream &out) throw (configfile_error) {
	using namespace std;

	out << "## Configuration dump" << endl;

	realm_id_t max_realms=  confpar_repository->getMaxRealms();

	// outer loop iterate over all realms
	for (unsigned int realm_id= 0; realm_id < max_realms; realm_id++)
	{
		// write realm name first
		if ( confpar_repository->existsRealm(realm_id) )
		{
			out << endl << '[' << confpar_repository->getRealmName(realm_id) << ']' << endl;
		}
		else // skip non existing realms
		  continue;

		// dump every variable of that realm
		unsigned int max_pars= confpar_repository->getRealmSize(realm_id);
		configparBase* cfgpar= NULL;
		for (unsigned int i= 0; i < max_pars; i++) {
			try {
				// get parameter from config repository, config par with id i
				cfgpar= confpar_repository->getConfigPar(realm_id, i);
				// write parameter name and then its value
				if (cfgpar) 
				{
					out << cfgpar->getName() << " = ";
					cfgpar->writeValToConfig(out);
					out << endl;
				}
			} // end try
			catch (configParExceptionParNotRegistered) {
				cfgpar= NULL;
				continue;
			} // end catch
			catch (...) {
				throw;
			}
		} // end for every parameter of that realm
	} // end for (iterate over all realms)

	out << endl << "## End of Configuration dump" << endl;

}



} // end namespace

// EOF
