/// ----------------------------------------*- mode: C++; -*--
/// @file stackprop.h
/// Stack Proposal
/// ----------------------------------------------------------
/// $Id: stackprop.h 6282 2011-06-17 13:31:57Z bless $
/// $HeadURL: https://svn.tm.kit.edu/nsis/ntlp/trunk/src/pdu/stackprop.h $
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
#ifndef _NTLP__STACKPROP_H_
#define _NTLP__STACKPROP_H_

#include "ntlp_object.h"
#include "address.h"
#include "protlib_types.h"
#include <vector>

namespace ntlp {
    using namespace protlib;

/// each profile is a sequence of protocol layers
/// so each profile this is a list of MA-Protocol-IDs, e.g. describing TCP, TLS/TCP, SCTP, SCTP/TLS stacking etc.
class stackprofile {
 public: 
  static const uint8 MA_Protocol_ID_FORWARDS_TCP = 1;
  static const uint8 MA_Protocol_ID_TLS          = 2;
  static const uint8 MA_Protocol_ID_SCTP         = 3;
 public:
    std::vector <uint8> prof_vec;
    stackprofile() {};
    ~stackprofile() {};
    void addprotocol(uint8 protocol) { prof_vec.push_back(protocol); }
    void clear() { prof_vec.clear(); }
    uint16 length() { return prof_vec.size(); }

};


/// stack proposal
/// this is a list of profiles (see stackprofile)
class stackprop : public known_ntlp_object 
{
/***** inherited from IE ****/
public:
  virtual stackprop* new_instance() const;
  virtual stackprop* copy() const;
  virtual stackprop* deserialize(NetMsg& msg, coding_t cod, IEErrorList& errorlist, uint32& bread, bool skip = true);
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
  // the vector holding our profiles
  std::vector<stackprofile> profiles;
  
  // any more fields are not necessary, as we can get the length of the vector via it's functions
  
  static const char* const iename;
  static const size_t contlen;    

public:    
// mri (appladdress sourceaddress, appladdress destaddress, const uint32* flowlabel);
//    bool init_flags();
  // stupid constructor, still, it is needed by NTLP_IE's "new_instance()" method, for deserialization (??)
  stackprop () :
    known_ntlp_object(StackProposal, Mandatory)
    {
      profiles.clear();
    };
  

  // This is the one mighty constructor, via which the object SHOULD be built programmatically!
  stackprop(std::vector<stackprofile> profiles): known_ntlp_object(StackProposal, Mandatory), profiles(profiles) {};
  
  virtual ~stackprop() {}


  // add a profile
  void add_profile(stackprofile profile) { profiles.push_back(profile); }

  // clear profiles
  void clear() { profiles.clear(); }

  // get a profile
  const stackprofile* get_profile(uint32 index) const;

  // return profile count
  uint32 get_count() const { return profiles.size(); }
  // get a profile with topmost protocol id #id
  const stackprofile* find_profile(protocol_t proto, uint32& index) const;

}; // end class stackprop


/** this function returns the profile and profile index
 *  for the first profile with a matching protocol id
 *  at the first index.
 */ 
inline
const stackprofile* 
stackprop::find_profile(protocol_t proto, uint32& index) const
{
  for (unsigned int i=0; i<profiles.size(); i++) 
  {
    // save the index (profile index counts from 1)
    index = i+1;
    if (profiles[i].prof_vec[0] == proto) return &(profiles[i]);
  } // end for

  return NULL;
}


} // end namespace ntlp

#endif
