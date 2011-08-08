/// ----------------------------------------*- mode: C++; -*--
/// @file mri.h
/// MRI (Message Routing Information) base class
/// ----------------------------------------------------------
/// $Id: mri.h 5249 2010-04-26 08:40:59Z roehricht $
/// $HeadURL: https://svn.tm.kit.edu/nsis/ntlp/trunk/include/mri.h $
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
#ifndef NTLP__MRI_H
#define NTLP__MRI_H

#include "ntlp_object.h"
#include "address.h"


namespace ntlp {
    using namespace protlib;

/**
 * @addtogroup mri Message Routing Method Objects
 * @{
 */

class mri : public known_ntlp_object {
  public:
	enum mri_types { 
		mri_t_pathcoupled	= 0,
		mri_t_looseend		= 1,
		mri_t_explicitsigtarget	= 125 // Sec. 9: 120-127 Private/Experimental Use
	};

	virtual ~mri() {};

	virtual mri* new_instance() const = 0;
	virtual mri* copy() const = 0;

	virtual void serialize(NetMsg& msg, coding_t cod, uint32& wbytes)
		const = 0;
	virtual void serializeNoHead(NetMsg& msg, coding_t cod, uint32& wbytes)
		const = 0;

	virtual bool operator==(const IE& ie) const;

	virtual const hostaddress& get_sourceaddress() const = 0 ;
	virtual const hostaddress& get_destaddress() const = 0;

	virtual bool get_downstream() const = 0;

        /// returns the destination address of the signaling packet depending on the MRI
	virtual appladdress* determine_dest_address() const = 0;
    
	/// returns the IP Version
	virtual uint8 get_ip_version() const = 0;

	inline uint8 get_mrm() const { return mrm; }
	inline bool has_nat_flag() const { return nat_flag; }
	virtual void invertDirection() = 0;

  protected:
	// only instantiable from subclasses
	mri(uint8 mrm, bool nat_flag, known_ntlp_object::subtype_t subtype); 

	uint8 mrm;	// ID of the Message Routing Method (all 8 bits)
        bool nat_flag;  /** N flag:  If set (N=1), this means that NATs do not need to translate
                         *  this MRM; if clear (N=0) it means that the method-specific
			 *  information contains network or transport layer information that a
			 *  NAT must process.
			 **/
};   


//@}

} // end namespace

#endif // NTLP__MRI_H
