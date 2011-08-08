/// ----------------------------------------*- mode: C++; -*--
/// @file mri_le.h
/// GIST LE MRI Loose End MRI (Message Routing Information)
/// ----------------------------------------------------------
/// $Id: mri_le.h 5249 2010-04-26 08:40:59Z roehricht $
/// $HeadURL: https://svn.tm.kit.edu/nsis/ntlp/trunk/include/mri_le.h $
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
#ifndef NTLP__MRI_LE_H
#define NTLP__MRI_LE_H

#include "mri.h"


namespace ntlp {
    using namespace protlib;

/**
 * @ingroup mri
 * @{
 */

class mri_looseend : public mri {
  public:
	mri_looseend();
	mri_looseend(hostaddress sourceaddress, hostaddress destaddress,
		bool downstream);

	virtual inline ~mri_looseend() { }

	virtual mri_looseend* new_instance() const;
	virtual mri_looseend* copy() const;

	virtual mri_looseend* deserialize(NetMsg& msg, coding_t cod,
		IEErrorList& errorlist, uint32& bread, bool skip = true);
	virtual mri_looseend* deserializeNoHead(NetMsg& msg, coding_t cod,
		IEErrorList& errorlist, uint32& bread, bool skip = true);
	virtual mri_looseend* deserializeEXT(NetMsg& msg, coding_t cod,
		IEErrorList& errorlist, uint32& bread, bool skip = true,
		bool header = true);

	virtual void serialize(NetMsg& msg, coding_t cod, uint32& wbytes) const;
	virtual void serializeNoHead(NetMsg& msg, coding_t cod, uint32& wbytes)
		const;
	virtual void serializeLooseEnd(NetMsg& msg, coding_t cod,
		uint32& wbytes, bool header) const;

	virtual bool check() const;
	virtual size_t get_serialized_size(coding_t coding) const;

	virtual bool operator==(const IE& ie) const;
	virtual const char* get_ie_name() const;

	virtual ostream& print(ostream& os, uint32 level, const uint32 indent,
		const char* name = NULL) const;
	virtual istream& input(istream& is, bool istty, uint32 level,
		const uint32 indent, const char* name = NULL);

	virtual appladdress* determine_dest_address() const;
	virtual void invertDirection();

	inline const hostaddress& get_sourceaddress() const { return sourceaddress; }
	inline const hostaddress& get_destaddress() const { return destaddress; }

	inline uint8 get_ip_version() const { return ip_version; }
	inline bool get_downstream() const { return ! (flags & Direction); }
	inline uint16 get_flags() const { return flags; }

	enum flags_t {
		Direction        = 1 << 11
	};
    
  private:
	uint8 ip_version;	// IP Version (only 4 bit used)
	uint16 flags;		// Flags field

	hostaddress sourceaddress, destaddress;

	bool rebuild_flags();
 
	static const char* const iename;
	static const size_t contlen;    
};


/**
 * Reset the flags based on the object's attributes.
 *
 * Used in the constructors, can be called any time to set the flags field
 * according to current data in the mri_looseend object
 */
inline bool mri_looseend::rebuild_flags() {

	// set the IP Version to "6" if one of the addresses is a native v6 or
	// IPv4-mapped
	if ( destaddress.is_ipv6() || sourceaddress.is_ipv6() )
		ip_version = 6;
	else
		ip_version = 4;

	return 1;
}


/**
 * Default constructor, only used for registering with the IE manager.
 */
inline mri_looseend::mri_looseend()
		: mri(mri_t_looseend, false, looseend),
		  ip_version(0), flags(0) {

	rebuild_flags();
}


/**
 * Constructor.
 */
inline mri_looseend::mri_looseend(hostaddress sourceaddress,
		hostaddress destaddress, bool downstream)
		: mri(mri_t_looseend, false, looseend), flags(0),
		  sourceaddress(sourceaddress), destaddress(destaddress) {

	if ( ! downstream )
		flags |= Direction;

	rebuild_flags();
}

//@}

}

#endif // NTLP__MRI_LE_H
