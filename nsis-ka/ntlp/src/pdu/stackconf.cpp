/// ----------------------------------------*- mode: C++; -*--
/// @file stackconf.cpp
/// GIST Stack Configuration Data
/// ----------------------------------------------------------
/// $Id: stackconf.cpp 5250 2010-04-26 11:26:28Z roehricht $
/// $HeadURL: https://svn.tm.kit.edu/nsis/ntlp/trunk/src/pdu/stackconf.cpp $
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
#include "stackconf.h"
#include <iostream>
#include <errno.h>
#include <string>
#include <sstream>
#include "logfile.h"
#include <iomanip>

namespace ntlp {

using namespace protlib;

/** @defgroup iestackconf Stackconf Objects
 *  @ingroup ientlpobject
 * @{
 */

/***** class singlehandle *****/

/***** IE name *****/

const char* const stack_conf_data::iename = "stack_conf_data";

const char* stack_conf_data::get_ie_name() const { return iename; }

const size_t stack_conf_data::contlen = 16;

/***** inherited from IE *****/

stack_conf_data* 
stack_conf_data::new_instance() const
{
	stack_conf_data* sh = NULL;
	catch_bad_alloc(sh = new stack_conf_data());
	return sh;
} // end new_instance

stack_conf_data* 
stack_conf_data::copy() const
{
	stack_conf_data* sh = NULL;
	catch_bad_alloc(sh =  new stack_conf_data(*this));
	return sh;
} // end copy 

stack_conf_data* 
stack_conf_data::deserialize(NetMsg& msg, coding_t cod, IEErrorList& errorlist, uint32& bread, bool skip)
{

	uint16 len = 0;
	uint32 ielen = 0;
	uint32 saved_pos = 0;
	uint32 resume = 0;

	// check arguments
	if (!check_deser_args(cod, errorlist, bread)) 
		return NULL;

	// decode header
	if (!decode_header_ntlpv1(msg, len, ielen, saved_pos, resume, errorlist, bread, skip)) 
		return NULL;

	// get profile count
	uint profile_count = msg.decode8();

	// read reserved bits to "nowhere"
	uint tmp = msg.decode8();
	tmp = msg.decode16();

	// read MA_hold_time
	ma_hold_time = msg.decode32();

	ma_protocol_option temp(0, 0, false);

	// iterate over profile_count to read whole profiles
	for (uint i = 0; i < profile_count; i++) {
		temp.clear();

		// read protocol
		temp.set_protocol(msg.decode8());

		// read profile numer
		temp.set_profile(msg.decode8());

		// read count of fields:
		uint count = msg.decode8();

		// read flags
		temp.flags = msg.decode8();

		//iterate through Higher Level Information
		for (uint j = 0; j < count; j++) 
			temp.add(msg.decode8());

		// read padding
		for (uint f = 0; f < (round_up4(count) - (count)); f++)
			tmp=msg.decode8();

		add_protoption(temp);
	}


	// There is no padding.
	bread = ielen;

	//check for correct length
	if (ielen != get_serialized_size(cod)) {
		ERRCLog("Stackconf", "Incorrect Object Length Error. Size encoded in PDU: " << ielen 
				<< " Size calculated from object: " << get_serialized_size(cod));
		errorlist.put(new GIST_IncorrectObjectLength(protocol_v1, this));
	}

	return this;
} // end deserialize


// interface function wrapper
void 
stack_conf_data::serialize(NetMsg& msg, coding_t cod, uint32& wbytes) const 
{
	serializeEXT(msg, cod, wbytes, true);
}

// special serialization function with the ability to omit the TLV header
void 
stack_conf_data::serializeEXT(NetMsg& msg, coding_t cod, uint32& wbytes, bool header) const 
{
	uint32 ielen = get_serialized_size(cod);

	// check arguments and IE state
	check_ser_args(cod,wbytes);

	// calculate length and encode header
	encode_header_ntlpv1(msg,get_serialized_size(cod)-header_length);

	//encode Prof_Count
	msg.encode8(ma_prot_options.size());
	msg.encode8(0);
	msg.encode16(0);

	//encode MA_Hold_Time
	msg.encode32(ma_hold_time);

	//iterate ma_prot_options,  
	for (uint i = 0; i < ma_prot_options.size(); i++) {
		//encode protocol
		msg.encode8(ma_prot_options[i].protocol);
		//encode profile
		msg.encode8(ma_prot_options[i].profile);
		//encode MA field count
		msg.encode8(ma_prot_options[i].options_data.size());
		//encode flags
		msg.encode8(ma_prot_options[i].flags);

		uint idx = 0;      

		for (uint j = 0; j < ma_prot_options[i].options_data.size(); j++) {
			msg.encode8(ma_prot_options[i].options_data[j]);
			idx++;
		}

		if (idx % 4) {
			if (idx % 4 == 3) msg.encode8(0);
			if (idx % 4 == 2) msg.encode16(0);
			if (idx % 4 == 1) { msg.encode16(0); msg.encode8(0); }
		}
	}

	wbytes = ielen;

	return;
} // end serializeEXT

bool 
stack_conf_data::check() const 
{
	return (true);
} // end check

size_t stack_conf_data::get_serialized_size(coding_t cod) const 
{
	if (!supports_coding(cod))
		throw IEError(IEError::ERROR_CODING);

	// Prof-Count field and reserved
	uint size = 32;

	// MA_Hold_Time
	size += 32;

	// iterate over ma_prot_options, add their sizes, round up to word boundary 
	for (uint i = 0; i < ma_prot_options.size(); i++)
		size += round_up4(ma_prot_options[i].options_data.size() + 4) * 8;

	return (size / 8) + header_length;

} // end get_serialized_size

bool stack_conf_data::operator==(const IE& ie) const 
{
	const stack_conf_data* sh = dynamic_cast<const stack_conf_data*>(&ie);
	if (sh)
		return ((ma_prot_options.size() == sh->ma_prot_options.size()));

	return false;
} // end operator==


ostream& 
stack_conf_data::print(ostream& os, uint32 level, const uint32 indent, const char* name) const 
{
	os << setw(level * indent) << "";
	if (name && (*name != 0))
		os << name << endl;

	os << "Profiles: " << (int) ma_prot_options.size() << endl;
	os << "MA-Hold Time: " << ma_hold_time << " ms" << endl;

	ios_base::fmtflags flags = os.flags();	// save stream flags

	// iterate through profile config data
	for (uint i = 0; i < ma_prot_options.size(); i++) {
		// print MA field count
		os << "Protocol: " << (int) ma_prot_options[i].protocol
		   << " Profile: " << (int) ma_prot_options[i].profile
		   << " Option Data: ";

		if (ma_prot_options[i].options_data.size() > 0) {
			uint j;
			for (j = 0; j < ma_prot_options[i].options_data.size(); j++) {
				os << "<" << hex << setw(2) << setfill('0')
					<< (int) ma_prot_options[i].options_data[j]
					<< "> ";
			}
			// print additionally as integer if 16-bit
			if (j == 2) {
				int portno = (ma_prot_options[i].options_data[0] << 8) + ma_prot_options[i].options_data[1];
				os << "(" << dec << portno << ")";
			}
		}
		else
			os << "<empty>";

		os.setf(flags); // reset stream flags

		os << endl;
	} // end for

	return os;
} // end print

istream& stack_conf_data::input(istream& is, bool istty, uint32 level, const uint32 indent, const char* name)
{
	if (istty)
		cout << "" << "hostaddress: ";

	if (istty) {
		cout << "";
		if (name && (*name != 0))
			cout << name << " ";
		cout << "<" << get_ie_name() << "> 1st 32bit: ";
	} // end if istty

	return is;
} // end input

	void 
stack_conf_data::add_protoption(const ma_protocol_option& protopt)
{ 
	ma_prot_options.push_back(protopt);
	// fix index
	ma_prot_options[ma_prot_options.size()-1].set_profile(ma_prot_options.size());
}

//@}

} // end namespace protlib
