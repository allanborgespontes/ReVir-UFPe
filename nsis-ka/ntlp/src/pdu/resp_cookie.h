/// ----------------------------------------*- mode: C++; -*--
/// @file resp_cookie.h
/// GIST Responder Cookie
/// ----------------------------------------------------------
/// $Id: resp_cookie.h 5249 2010-04-26 08:40:59Z roehricht $
/// $HeadURL: https://svn.tm.kit.edu/nsis/ntlp/trunk/src/pdu/resp_cookie.h $
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
#ifndef _NTLP__RESPCOOKIE_H_
#define _NTLP__RESPCOOKIE_H_

#include "ntlp_object.h"
#include "address.h"
#include "protlib_types.h"

#include "nli.h"
#include "nattraversal.h"



namespace ntlp {
    using namespace protlib;


class respcookie : public known_ntlp_object {


/***** inherited from IE ****/
public:
	virtual respcookie* new_instance() const;
	virtual respcookie* copy() const;
	virtual respcookie* deserialize(NetMsg& msg, coding_t cod, IEErrorList& errorlist, uint32& bread, bool skip = true);
	respcookie* deserializeNoHead(NetMsg& msg, coding_t cod, IEErrorList& errorlist, uint32& bread, bool skip = true);
	virtual void serialize(NetMsg& msg, coding_t cod, uint32& wbytes) const;
	virtual void serializeNoHead(NetMsg& msg, coding_t cod, uint32& wbytes) const;
	virtual void serializeEXT(NetMsg& msg, coding_t cod, uint32& wbytes, bool header) const;
	virtual bool check() const;
	virtual size_t get_serialized_size(coding_t coding) const;
	virtual bool operator==(const IE& ie) const;
	virtual const char* get_ie_name() const;
	virtual ostream& print(ostream& os, uint32 level, const uint32 indent, const char* name = NULL) const;
	virtual istream& input(istream& is, bool istty, uint32 level, const uint32 indent, const char* name = NULL);
/***** new members *****/

  uint32 get_generationnumber() const { return buf ? ntohl(*(reinterpret_cast<uint32*>(buf))) : 0; }

  // decode interface id (16bit), located after the generation number field
  uint32 get_if_index() const { return buf ? ntohs(*(reinterpret_cast<uint16*>(buf+sizeof(uint32)))) : 0; }

  // decode transparent data length field (16bit) expressed as number of bytes, located after the generation num and interface id
  uint16 get_transparent_data_len() const { return (buf && buf_length>=8) ? ntohs(*(reinterpret_cast<uint16*>(buf+sizeof(uint32)+sizeof(uint16)))) : 0; }

  // return start of the transparent data part or 0 if not present
  uchar* get_transparent_data() const { return (get_transparent_data_len() > 0) ? buf+sizeof(uint32)+2*sizeof(uint16) : NULL; }

  string to_str() const;

 private:
	/// buffer for data
	uchar *buf;
	/// buffer length
	uint32 buf_length;
    
  static const char* const iename;
  static const size_t contlen;    
  respcookie* deserializeEXT(NetMsg& msg, coding_t cod, IEErrorList& errorlist, uint32& bread, bool skip = true, bool header = true);
	

public:    

  /// stupid constructor, still, it is needed by NTLP_IE's "new_instance()" method, for deserialization (??)
  respcookie () :
    known_ntlp_object(RespCookie, Mandatory),
    buf(NULL),
    buf_length(0)
  {
  };


  /// Returns a handle to the buffer containing the cookie
  uchar* get_buffer() const { return buf; }

  /// Returns buffer length
  uint32 get_size() const { return buf_length; }

  /// This is the one mighty constructor, via which the object SHOULD be built programmatically!
  respcookie (uchar *buffer, uint32 l):
    known_ntlp_object(RespCookie, Mandatory),
    buf_length(l)
    {
      buf = new(nothrow) uchar[l];
      if (!buf) throw NetMsgError(NetMsgError::ERROR_NO_MEM);
      if (buffer) {
	memcpy(buf,buffer,l);
      } else {
	throw NetMsgError(NetMsgError::ERROR_NULL_POINTER); 
      }
    };
  


  /// copy constructor
  respcookie(const respcookie& n): 
    known_ntlp_object(RespCookie, Mandatory)
  {
    buf_length=n.buf_length;
    
    if (buf_length) {
      
      buf = new(nothrow) uchar[buf_length];
      if (!buf) throw NetMsgError(NetMsgError::ERROR_NO_MEM);
      if (buf) {
	if (n.buf) {
	  memcpy(buf,n.buf,buf_length);
	} else {
	  throw NetMsgError(NetMsgError::ERROR_NULL_POINTER); 
	}
      } else {
	throw NetMsgError(NetMsgError::ERROR_NULL_POINTER); 
      }
      
    } else {
      buf = NULL;
    }
  };

  virtual ~respcookie() { delete buf; }

  nli*	get_nli() const;
  nattraversal* get_nto(uint16 byteoffset) const;

};

} // end namespace

#endif
