/// ----------------------------------------*- mode: C++; -*--
/// @file nattraversal.h
/// GIST NAT Traversal
/// ----------------------------------------------------------
/// $Id: nattraversal.h 6282 2011-06-17 13:31:57Z bless $
/// $HeadURL: https://svn.tm.kit.edu/nsis/ntlp/trunk/src/pdu/nattraversal.h $
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
#ifndef NTLP_NATTRAV_H
#define NTLP_NATTRAV_H

#include "ntlp_object.h"
#include "address.h"
#include "protlib_types.h"
#include "mri.h"
#include "nli.h"
#include "ie.h"
#include <vector>

namespace ntlp {
    using namespace protlib;
    


/**
 * list of translated NTLP objects / information per NAT, 
 * this information is opaque and specific per NAT
 **/
class natinfo {
 public:
	/// constructor
	natinfo() : netmsgbuf(NULL) {};

	/// copy constructor
	natinfo(const natinfo& nat_info) {
		netmsgbuf= nat_info.get() ? new NetMsg(*nat_info.get()) : NULL;
	}

	/// destructor
	~natinfo() { clear(); }

	// assignment operator
	natinfo& operator=(const natinfo& nat_info) { 
		if (netmsgbuf) { delete netmsgbuf; }
		netmsgbuf= nat_info.get() ? new NetMsg(*nat_info.get()) : NULL;
		return *this;
	}

	bool operator==(const natinfo& ni) const { 
		if (netmsgbuf==NULL && ni.get()==NULL)
			return true;
		else
		if (netmsgbuf!=NULL && ni.get()!=NULL)
			return *netmsgbuf == *(ni.get());
		else
			return false;
	}

	/// add a copy of the translated NAT information
	void set(const NetMsg& givennetmsgbuf) { 
		if (netmsgbuf) { delete netmsgbuf; }
		netmsgbuf= new NetMsg(givennetmsgbuf);
	}

	/// add the netmsg as translated NAT information (object will be taken over!)
	void set(NetMsg* givennetmsgbuf) { 
		if (netmsgbuf) { delete netmsgbuf; }
		netmsgbuf= givennetmsgbuf;
	}
	
	/// get opaque NAT info back (as NetMsg buffer)
	const NetMsg* get() const { return netmsgbuf; }

	/// remove all contents
	void clear() { delete netmsgbuf; }

	uint16 length() const { return netmsgbuf ? netmsgbuf->get_size() : 0; }

private:
	NetMsg* netmsgbuf;
};

/**
 *
 *  0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   | MRI-Length    | Type-Count    |  NAT-Count    |  Reserved     |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   //            Original Message-Routing-Information             //
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   //                 List of translated objects                  //
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   | Length of opaque information  |                               |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+                              //
   //                Information replaced by NAT #1                |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   :                                                               :
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   | Length of opaque information  |                               |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+                              //
   //                Information replaced by NAT #N                |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

   MRI-Length (8 bits):  The length of the included MRI payload in 32-
      bit words.

   Original Message-Routing-Information (variable):  The MRI data from
      when the message was first sent, not including the object header.

   Type-Count (8 bits):  The number of objects in the 'List of
      translated objects' field.

   List of translated objects (variable):  This field lists the types of
      the objects that were translated by every NAT through which the
      message has passed.  Each element in the list is a 16-bit field
      containing the first 16 bits of the object TLV header, including
      the AB extensibility flags, two reserved bits, and 12 bit object
      type.  The list is initialised by the first NAT on the path;
      subsequent NATs may delete elements in the list.  Padded with 2
      null bytes if necessary.

   NAT-Count (8 bits):  The number of NATs traversed by the message, and
      the number of opaque payloads at the end of the object.  The
      length fields for each opaque payload are byte counts, not
      including the 2 bytes of the length field itself.  Note that each
      opaque information field is zero-padded to the next 32-bit word
      boundary if necessary.
*
**/

class nattraversal : public known_ntlp_object {

/***** inherited from IE ****/
public:
	virtual nattraversal* new_instance() const;
	virtual nattraversal* copy() const;
	virtual nattraversal* deserialize(NetMsg& msg, coding_t cod, IEErrorList& errorlist, uint32& bread, bool skip = true);
	virtual void serialize(NetMsg& msg, coding_t cod, uint32& wbytes) const;
	virtual void serializeEXT(NetMsg& msg, coding_t cod, uint32& wbytes, bool header) const;
	virtual bool check() const;
	virtual size_t get_serialized_size(coding_t coding) const;
	virtual bool operator==(const IE& ie) const;
	virtual const char* get_ie_name() const;
	virtual ostream& print(ostream& os, uint32 level, const uint32 indent, const char* name = NULL) const;
	virtual istream& input(istream& is, bool istty, uint32 level, const uint32 indent, const char* name = NULL);
/***** new members *****/

	const mri* get_embedded_mri() const { return embedded_mri; }

 private:
	typedef std::vector<natinfo> nat_information_t;

	// the vector holding NAT specific opaque information 
	// (one information object per traversed NAT)
	nat_information_t nat_information;
	
	// the vector holding our list of translated object types
	std::vector <uint16> translated_object_types;


	// MRI
	mri* embedded_mri;

	// mri length will be derived from embedded_mri
        // any further fields are not necessary, as we can get the length of the vectors via their functions

    
  static const char* const iename;
  static const size_t contlen;    

public:    

	// add information that have been translated by a NAT
	void push_natinfo(const natinfo& info) { nat_information.push_back(info); };

	// clear profiles
	inline void clear() { nat_information.clear(); };

	// add a translated object
	inline void add_translation(uint16 type) { translated_object_types.push_back(type); }

	// clear translated objects
	inline void clear_translations() { translated_object_types.clear(); }

	// get translated objects
	inline const vector<uint16> get_translations() const { return translated_object_types; }

	// stupid constructor, still, it is needed by NTLP_IE's "new_instance()" method, for deserialization (??)
	inline nattraversal () : known_ntlp_object(NatTraversal, Mandatory),
				 embedded_mri(0)
		{
			nat_information.clear();
		};


	// This is the one mighty constructor, via which the object SHOULD be built programmatically!
	inline
	nattraversal (const mri* mr, nat_information_t nat_information, std::vector <uint16> translated_object_types):
		known_ntlp_object(NatTraversal, Mandatory),
		nat_information(nat_information),
		translated_object_types(translated_object_types),
		embedded_mri(mr ? mr->copy() : 0)
		{};


	// Copy constructor
	inline
	nattraversal(const nattraversal& n) :
		known_ntlp_object(NatTraversal, Mandatory),
		nat_information(n.nat_information),
		translated_object_types(n.translated_object_types),
		embedded_mri(n.embedded_mri ? n.embedded_mri->copy() : NULL)
		{
		}
	

	inline
	nattraversal& operator=(const nattraversal& n)
	{
		nat_information.clear();
		nat_information= n.nat_information;
		translated_object_types.clear();
		translated_object_types=n.translated_object_types;
		delete embedded_mri;
		embedded_mri= n.embedded_mri ? n.embedded_mri->copy() : NULL;
		
		return *this;
	}

	virtual inline ~nattraversal() { delete embedded_mri; }

};

}


#endif
