/// ----------------------------------------*- mode: C++; -*--
/// @file stackconf.h
/// GIST Stack Configuration Data
/// ----------------------------------------------------------
/// $Id: stackconf.h 6282 2011-06-17 13:31:57Z bless $
/// $HeadURL: https://svn.tm.kit.edu/nsis/ntlp/trunk/src/pdu/stackconf.h $
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
#ifndef _NTLP__STACKCONF_H_
#define _NTLP__STACKCONF_H_

#include "ntlp_object.h"
#include "address.h"
#include "protlib_types.h"

#include <stdexcept> // for std::out_of_range
#include <vector>

namespace ntlp {
    using namespace protlib;
    
/**
 * Represents an MA-Protocol-Options field (part of Stack-Configuration-Data).
 *   The MA-protocol-options fields are formatted as follows:
 *
 *  0                   1                   2                   3
 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |MA-Protocol-ID |     Profile   |    Length     |D|  Reserved   |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * //                         Options Data                        //
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *   MA-Protocol-ID (8 bits):  Protocol identifier as described in
 *     Section 5.7.
 *
 * Profile (8 bits):  Tag indicating which profile from the accompanying
 *    Stack-Proposal object this applies to.  Profiles are numbered from
 *    1 upwards; the special value 0 indicates 'applies to all
 *    profiles'.
 *
 *
 */

static const uint8 d_flag= 1 << 7;

class ma_protocol_option {
public:
	protocol_t protocol;
	uint8 profile;
	uint8 flags;
	std::vector <uint8> options_data;

	void add(uint8 field) { options_data.push_back(field); };

	void add16(uint16 field) { options_data.push_back(field / 256); options_data.push_back(field % 256); };

	uint16 get16(uint16 index) const { return options_data[index * 2] * 256 + options_data[index * 2 + 1]; };

	void clear() { options_data.clear(); };

	ma_protocol_option(protocol_t proto, uint8 profile, bool DoNotUseMA):
		protocol(proto),
		profile(profile) {
		flags= DoNotUseMA ? d_flag : 0;
	};

	uint16 length() const {return options_data.size();};

	protocol_t get_protocol() const { return protocol; }

	void set_protocol(protocol_t prot) { protocol=prot; }

	/// returns number of profile
	uint8 get_profile() const { return profile; }

	/// sets number of profile
	void set_profile(uint8 prop) { profile=prop; }

	bool get_DoNotUseMA() const {return flags & d_flag;}

	void set_DoNotUseMA() { flags= flags | d_flag; }

	~ma_protocol_option() {};
};



/**
* Represents the GIST Stack-Configuration-Data object.
*/
class stack_conf_data : public known_ntlp_object {


/***** inherited from IE ****/
public:
	virtual stack_conf_data* new_instance() const;
	virtual stack_conf_data* copy() const;
	virtual stack_conf_data* deserialize(NetMsg& msg, coding_t cod, IEErrorList& errorlist, uint32& bread, bool skip = true);
	virtual void serialize(NetMsg& msg, coding_t cod, uint32& wbytes) const;
	virtual void serializeEXT(NetMsg& msg, coding_t cod, uint32& wbytes, bool header) const;
	virtual bool check() const;
	virtual size_t get_serialized_size(coding_t coding) const;
	virtual bool operator==(const IE& ie) const;
	virtual const char* get_ie_name() const;
	virtual ostream& print(ostream& os, uint32 level, const uint32 indent, const char* name = NULL) const;
	virtual istream& input(istream& is, bool istty, uint32 level, const uint32 indent, const char* name = NULL);
	/***** new members *****/


private:
	// the vector holding our MA Protocol Options
	std::vector <ma_protocol_option> ma_prot_options;

	uint32 ma_hold_time;
	// any more fields are not necessary, as we can get the length of the vector via it's functions


	static const char* const iename;
	static const size_t contlen;    

public:    
	/// set ma_hold_time
	void set_ma_hold_time(uint32 time) { ma_hold_time=time;}

	/// get ma_hold_time
	uint32 get_ma_hold_time() const { return ma_hold_time; }


	/// add protocol option data
	void add_protoption(const ma_protocol_option& protopt);

	/// clear prot options
	void clear() { ma_prot_options.clear(); }

	/// get a protocol option
	const ma_protocol_option* get_protoption(uint32 index) const { try { return &ma_prot_options.at(index); } catch (std::out_of_range& e) { return NULL; } }

	/// get length
	uint32 length() const {return ma_prot_options.size(); }

	/// stupid constructor, still, it is needed by NTLP_IE's "new_instance()" method, for deserialization (??)
	stack_conf_data () : known_ntlp_object(StackConfiguration, Mandatory) {ma_prot_options.clear();};

	/// This constructor includes only MA_HOLD_TIME
	stack_conf_data (uint32 ma_hold_time): known_ntlp_object(StackConfiguration, Mandatory), ma_hold_time(ma_hold_time) {};

	/// This is the one mighty constructor, via which the object SHOULD be built programmatically!
	stack_conf_data (uint32 ma_hold_time, const std::vector <ma_protocol_option>& ma_prot_options): 
		known_ntlp_object(StackConfiguration, Mandatory), ma_prot_options(ma_prot_options), ma_hold_time(ma_hold_time) {};

	virtual ~stack_conf_data() {} 
}; // end class stack_conf_data

}


#endif
