/// ----------------------------------------*- mode: C++; -*--
/// @file ntlp_errorobject.h
/// GIST Error Object
/// ----------------------------------------------------------
/// $Id: ntlp_errorobject.h 5249 2010-04-26 08:40:59Z roehricht $
/// $HeadURL: https://svn.tm.kit.edu/nsis/ntlp/trunk/src/pdu/ntlp_errorobject.h $
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
#ifndef _NTLP__ERROROBJECT_H_
#define _NTLP__ERROROBJECT_H_

#include "ntlp_object.h"
#include "address.h"
#include "protlib_types.h"
#include "mri.h"
#include "sessionid.h"

namespace ntlp {
    using namespace protlib;
    

class errorobject : public known_ntlp_object {

 public:
  enum errorclass_t {
    Informational    = 0,
    Success          = 1,
    ProtocolError    = 2,
    TransientFailure = 3,
    PermanentFailure = 4,
    max_error_class  = 5
  };

  enum errorcode_t {
    err_CommonHeaderParseError      = 1,
    err_HopLimitExceeded            = 2,
    err_IncorrectEncapsulation      = 3,
    err_IncorrectlyDeliveredMessage = 4,
    err_NoRoutingState              = 5,
    err_UnknownNSLPID               = 6,
    err_EndpointFound               = 7,
    err_MessageTooLarge             = 8,
    err_ObjectTypeError             = 9,
    err_ObjectValueError            = 10,
    err_InvalidIPTTL                = 11,
    err_MRIValidationFailure        = 12,
    err_max_code
  };

  enum errorsubcode_t {
    errsub_Default           = 0,
    // subtypes for err_CommonHeaderParseError
    errsub_UnknownVersion    = 0,
    errsub_UnknownType       = 1,
    errsub_InvalidRFlag      = 2,
    errsub_IncorrectMsgLength= 3,
    errsub_InvalidEFlag	     = 4,
    errsub_InvalidCFlag	     = 5,
    // subtypes for err_ObjectTypeError
    errsub_DuplicateObject   		= 0,
    errsub_UnrecognizedObject		= 1,
    errsub_MissingObject     		= 2,
    errsub_InvalidObject     		= 3,
    errsub_UntranslatedObject		= 4,
    errsub_InvalidExtensibilityFlags	= 5,
    // subtype for err_ObjectValueError
    errsub_IncorrectLength             = 0,
    errsub_ValueNotSupported           = 1,
    errsub_InvalidFlagFieldCombination = 2,
    errsub_EmptyList                   = 3,
    errsub_InvalidCookie               = 4,
    errsub_SP_SCD_Mismatch             = 5,
    // subtype for err_MRIValidationFailure 
    errsub_MRITooWild           = 0,
    errsub_IPVersionMismatch    = 1,
    errsub_IngressFilterFailure = 2
  };
  enum ai_type_t {
  	ai_Message_Length_Info = 1,
  	ai_MTU_Info = 2,
  	ai_Object_Type_Info = 3,
  	ai_Object_Value_Info = 4
  };
 private:
  static const char *const errorclass_string[];
  static const char *const errorcode_string[];

/***** inherited from IE ****/
public:
  virtual errorobject* new_instance() const;
  virtual errorobject* copy() const;
  virtual errorobject* deserialize(NetMsg& msg, coding_t cod, IEErrorList& errorlist, uint32& bread, bool skip = true);
  virtual void serialize(NetMsg& msg, coding_t cod, uint32& wbytes) const;
  virtual void serializeEXT(NetMsg& msg, coding_t cod, uint32& wbytes, bool header) const;
  virtual bool check() const;
  virtual size_t get_serialized_size(coding_t coding) const;
  virtual bool operator==(const IE& ie) const;
  virtual const char* get_ie_name() const;
  virtual ostream& print(ostream& os, uint32 level, const uint32 indent, const char* name = NULL) const;
  virtual istream& input(istream& is, bool istty, uint32 level, const uint32 indent, const char* name = NULL);
  /***** new members *****/

  static const uint16 oti_not_set= 0xfff0; // object type info is only 12-bit, if oti is not present, use this value
  

private:

  // Error class
  uint8 errorclass;

  // Error code
  uint16 errorcode;

  // Error subcode
  uint8 errorsubcode;

  // info count
  uint8 infocount;

  // common header containers
  uint8 ch_version;
  uint8 ch_hops;
  uint16 ch_length;
  uint16 ch_nslpid;
  uint8 ch_type;
  uint8 ch_flags; 

  // Flags
  uint16 flags;

  // sessionid
  sessionid* embedded_sessionid;

  // MRI
  mri* embedded_mri;

  // Additional information fields, these might need to be more flexible (list?)
  // Message Length Info
  uint16 mli;

  // MTU Info
  uint16 mtu;

  // Object Type Info
  uint16 oti;

  // Object Value Info
  uint16 ovi_length;
  uint16 ovi_offset;
  ntlp_object* ovi_object;

  // debug comment
  char* debugcomment;

  // any more fields are not necessary, as we can get the length of the objects via their functions

  static const char* const iename;
  static const size_t contlen;    

  void setup_serialization();

public:    


  enum flags_t {
      SessionID          = 1 << 15,
      MRI                = 1 << 14,
      DebugComment       = 1 << 13,
      DatagramMode       = 1 << 12,
      QueryEncapsulation = 1 << 11
  };


  // stupid constructor, still, it is needed by NTLP_IE's "new_instance()" method, for deserialization (??)
  errorobject();

  // This is the one mighty constructor, via which the object SHOULD be built programmatically!
  errorobject (mri* mr, 
	       sessionid* sid, 
	       uint8 errorclass, 
	       uint8 errorcode, 
	       uint8 errorsubcode, 
	       uint8 ch_version, 
	       uint8 ch_hops, 
	       uint16 ch_length, 
	       uint16 ch_nslpid, 
	       uint8 ch_type, 
	       uint8 ch_flags, 
	       bool dgram, 
	       bool qe, 
	       ntlp_object* ovi_object, 
	       uint16 oti = oti_not_set, 
	       uint16 mtuinfo=0, 
	       uint16 mli=0);

 // copy constructor
 errorobject(const errorobject& eo);

 errorobject& operator=(const errorobject& eo);

 uint8 get_errorclass() const { return errorclass; }

 const char* get_errorclass_str() const { return errorclass_string[(errorclass<max_error_class) ? errorclass : max_error_class]; }

 uint8 get_errorcode() const { return errorcode; }
 const char* get_errorcode_str() const { return errorcode_string[(errorcode>0 && errorcode<err_max_code) ? errorcode : err_max_code ]; }

 const char *const get_errorsubcode_str() const;

 mri* get_embedded_mri() const { return embedded_mri; }

 sessionid* get_embedded_sessionid() const { return embedded_sessionid; }

 uint16 get_referring_nslpid() const { return ch_nslpid; }

 virtual ~errorobject();
};



inline
errorobject::errorobject () :
    known_ntlp_object(ErrorObject, Mandatory),
    errorclass(0),
    errorcode(0),
    errorsubcode(0),
    infocount(0),
    ch_version(0),
    ch_hops(0),
    ch_length(0),
    ch_nslpid(0),
    ch_type(0),
    flags(0),
    embedded_sessionid(NULL),
    embedded_mri(NULL),
    mli(0),
    mtu(0),
    oti(oti_not_set),
    ovi_length(0),
    ovi_offset(0),
    ovi_object(NULL),
    debugcomment(NULL)
{
  setup_serialization();
}

inline
errorobject::errorobject (mri* mr, 
			  sessionid* sid, 
			  uint8 errorclass, 
			  uint8 errorcode, 
			  uint8 errorsubcode, 
			  uint8 ch_version, 
			  uint8 ch_hops, 
			  uint16 ch_length, 
			  uint16 ch_nslpid, 
			  uint8 ch_type, 
			  uint8 ch_flags, 
			  bool dgram, bool qe, 
			  ntlp_object* ovi_object, 
			  uint16 oti, 
			  uint16 mtuinfo, 
			  uint16 mli):
    known_ntlp_object(ErrorObject, Mandatory),
    errorclass(errorclass),
    errorcode(errorcode),
    errorsubcode(errorsubcode),
    infocount (0),
    ch_version(ch_version),
    ch_hops(ch_hops),
    ch_length(ch_length),
    ch_nslpid(ch_nslpid),
    ch_type(ch_type),
    ch_flags(ch_flags),
    flags(0),
    embedded_sessionid(sid),
    embedded_mri(mr),
    mli(mli),
    mtu(mtuinfo),
    oti(oti),
    ovi_length(0),
    ovi_offset(0),
    ovi_object(ovi_object),
    debugcomment(NULL)
{
  setup_serialization();
  if (dgram) flags = flags | DatagramMode;
  if (qe) flags = flags | QueryEncapsulation;
  
  if (mr) flags = flags | MRI;
  if (sid) flags = flags | SessionID;
  
  embedded_mri=mr;
  embedded_sessionid=sid;
  
  // debug comment currently unimplemented
}

inline
errorobject::~errorobject()
{
  if (debugcomment) delete[] debugcomment;
}

} // end namespace

#endif
