/// ----------------------------------------*- mode: C++; -*--
/// @file ntlp_errorobject.cpp
/// GIST Error Object
/// ----------------------------------------------------------
/// $Id: ntlp_errorobject.cpp 5249 2010-04-26 08:40:59Z roehricht $
/// $HeadURL: https://svn.tm.kit.edu/nsis/ntlp/trunk/src/pdu/ntlp_errorobject.cpp $
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
#include "ntlp_errorobject.h"
#include <iostream>
#include <errno.h>
#include <string>
#include <sstream>
#include "logfile.h"
#include <iomanip>
#include "ntlp_pdu.h"

#include "mri_pc.h"
#include "mri_le.h"


namespace ntlp {

using namespace protlib;
    using namespace protlib::log;
/** @defgroup ieerrorobject Errorobject Objects
 *  @ingroup ientlpobject
 * @{
 */

/***** class errorobject *****/

  const char *const errorobject::errorclass_string[]= {
    "Informational",
    "Success",
    "ProtocolError",
    "TransientFailure",
    "PermanentFailure",
    "(Undefined ErrorClass)"
  };

  const char *const errorobject::errorcode_string[] =  {
    "Errorcode 0",
    "Common Header Parse Error",
    "Hop Limit Exceeded",
    "Incorrect Encapsulation",
    "Incorrectly Delivered Message",
    "No Routing State",
    "Unknown NSLPID",
    "Endpoint Found",
    "Message Too Large",
    "Object Type Error",
    "Object Value Error",
    "Invalid IP TTL",
    "MRI Validation Failure",
    "(Undefined ErrorCode)"
  };

/***** IE name *****/

const char* const errorobject::iename = "errorobject";

const char* errorobject::get_ie_name() const { return iename; }

const size_t errorobject::contlen = 16;

/***** inherited from IE *****/

errorobject* errorobject::new_instance() const {
	errorobject* sh = NULL;
	catch_bad_alloc(sh = new errorobject());
	return sh;
} // end new_instance

errorobject* errorobject::copy() const {
	errorobject* sh = NULL;
	catch_bad_alloc(sh =  new errorobject(*this));
	return sh;
} // end copy 

errorobject* 
errorobject::deserialize(NetMsg& msg, coding_t cod, IEErrorList& errorlist, uint32& bread, bool skip) {
	
    uint16 len = 0;
    uint32 ielen = 0;
    uint32 saved_pos = 0;
    uint32 resume = 0;

    DLog("errorobject", "starting to deserialize, bread:" << bread);
    
    // check arguments
    if (!check_deser_args(cod,errorlist,bread)) 
	return NULL;

    // decode header
    if (!decode_header_ntlpv1(msg,len,ielen,saved_pos,resume,errorlist,bread,skip))
    {
      DLog("errorobject", "failed to deserialize");

      return NULL;
    }

    // DLog("errorobject", "after parsing header @pos " << msg.get_pos() << ", saved_pos:" << saved_pos << " length: " << ielen);

    // decode errorclass
    errorclass=msg.decode8();

    // decode errorcode
    errorcode=msg.decode16();

    // decode errorsubcode
    errorsubcode=msg.decode8();

    // decode flags
    flags=msg.decode16();
    
    // decode mrilength (actually not used, MRI has its own length calculation)
    int mri_length=msg.decode8();
    DLog("errorobject", "MRI length: " << mri_length);

    // decode infocount
    infocount=msg.decode8();

    DLog("errorobject", "info count: " << (uint16) infocount);

    //decode common header    
    ch_version = msg.decode8();
    ch_hops = msg.decode8();
    ch_length = msg.decode16();
    ch_nslpid = msg.decode16();
    ch_type = msg.decode8();
    ch_flags = msg.decode8();

    //DLog("errorobject", "after parsing common header @pos " << msg.get_pos() << ", saved_pos:" << saved_pos << " length: " << ielen);

    // Now we must check the flags because the contents from now on are optional
    //decode sessionid
     if (flags & SessionID) {
       embedded_sessionid = new sessionid();
       embedded_sessionid->deserializeNoHead(msg, cod, errorlist, bread, skip);
    };


     //DLog("errorobject", "after parsing session id @pos " << msg.get_pos() << ", saved_pos:" << saved_pos << " length: " << ielen);
  
    // read mri
   
    // read MRM and instantiate correct MRI subclass
    if (flags & MRI) {
      int mrm = msg.decode16(false);

      if (mrm == 0) {

	embedded_mri = new mri_pathcoupled;
        dynamic_cast<mri_pathcoupled*>(embedded_mri)->deserializeNoHead(msg, cod, errorlist, bread, skip);
      }

      if (mrm == 1) {

	embedded_mri = new mri_looseend;
	dynamic_cast<mri_looseend*>(embedded_mri)->deserializeNoHead(msg, cod, errorlist, bread, skip);
      }
 
    }

    //DLog("errorobject", "after parsing MRI @pos " << msg.get_pos() << ", saved_pos:" << saved_pos << " length: " << ielen);

	// Collect additional information fields as present
	uint16 ai_type, ai_len;
	for (int i = infocount; i > 0; i--) {
		// decode TL-header
		ai_type = msg.decode16();
		ai_len = msg.decode16();
		switch (ai_type) {
		case ai_Message_Length_Info:
			if (ai_len != 1) {
				DLog("errorobject", "wrong length of ai_Message_Length_Info field:" << ai_len);
				// XXX: Fail gracefully?!?
				while (ai_len--)
					(void)msg.decode32();
				break;
			}
			mli = msg.decode16();
			(void)msg.decode16();		//reserved
			break;
		case ai_MTU_Info:
			if (ai_len != 1) {
				DLog("errorobject", "wrong length of ai_MTU_Info field:" << ai_len);
				// XXX: Fail gracefully?!?
				while (ai_len--)
					(void)msg.decode32();
				break;
			}
			mtu = msg.decode16();
			(void)msg.decode16();	//reserved
			break;
		case ai_Object_Type_Info:
			if (ai_len != 1) {
				DLog("errorobject", "wrong length of ai_Object_Type_Info field:" << ai_len);
				// XXX: Fail gracefully?!?
				while (ai_len--)
					(void)msg.decode32();
				break;
			}
			oti = msg.decode16() & 0x0fff; // only 12 bit
			(void)msg.decode16();	//reserved
			break;
		case ai_Object_Value_Info:
			if (ai_len == 0) {
				DLog("errorobject", "empty ai_Object_Value_Info");
				break;
			}
		    ovi_length = msg.decode16() & 0x0fff;	// real object length, 12-bit value, number of 32-bit words
		    ovi_offset = msg.decode16();
		    //      We can't be sure to be able to decode this,
		    //      after all there has been an error with it before!
		    //      On top of that we should consider ovi_length.
		    ovi_object = dynamic_cast<known_ntlp_object*>(NTLP_IEManager::instance()->deserialize(msg, cat_known_pdu_object, cod, errorlist, bread, skip));
		    // previous if parsing failed, try to deliver as raw_ntlp_object at least
		    if (ovi_object == NULL)
		      ovi_object = new raw_ntlp_object(msg.get_buffer() + msg.get_pos(), ovi_length*4);

		    break;
		default:
			DLog("errorobject", "Unknown Additional Information field: " << ai_type << " len: " << ai_len);
			// skip
			while (ai_len--)
				(void)msg.decode32();
			break;
		}
	}
   
    // if comment flag is set, decode debugging comment
    if (flags & DebugComment) {
      uint16 dc_length= len - msg.get_pos();
      DLog("errorobject", "Debug Comment is present, we decode it, length=" << dc_length);
      if (dc_length >0)
      {
	debugcomment = new(nothrow) char[dc_length];
	// read debug comment
	// copy the contents of netmsg from current position 
	if (debugcomment)
	  msg.copy_to(reinterpret_cast<uchar *>(debugcomment), dc_length);
      }
    }

    // There is no padding.
    bread = ielen;

    DLog("errorobject", "deserialize finish.");

    return this;
} // end deserialize


// setup serialization, set "present" flags/error class according to error code
void errorobject::setup_serialization()  {

    if (embedded_mri) flags = flags | MRI;
    if (embedded_sessionid) flags = flags | SessionID;

    switch (errorcode) {
	case 0:
		// Unrecognized Message Type
		errorclass=ProtocolError;
		mli=0;
		mtu=0;
		oti= oti_not_set;
		break;
	case err_CommonHeaderParseError:
		// Common Header Parse Error (Unknown Version/Type, Incorrect Message Length)
		errorclass=ProtocolError;
		mtu=0;
		oti= oti_not_set;
		// mli should be given
		infocount++;
		// Note that in this case the original MRI and Session ID
		// MUST NOT be included in the Error Object.
		if (embedded_mri || embedded_sessionid) {
			// throw?
			flags &= ~( MRI | SessionID);
		}
		break;	    
	case err_HopLimitExceeded:
		// Hop Limit Exceeded
		errorclass=PermanentFailure;
		mli=0;
		mtu=0;
		oti= oti_not_set;
		break;
	case err_IncorrectEncapsulation:
		// Incorrect Encapsulation
		errorclass=ProtocolError;
		mli=0;
		mtu=0;
		oti= oti_not_set;
		break;
	case err_IncorrectlyDeliveredMessage:
		// Incorrectly delivered Message
		errorclass=ProtocolError;
		mli=0;
		mtu=0;
		oti= oti_not_set;
		break;
	case err_NoRoutingState:
		// No Routing State
		errorclass=ProtocolError;
		mli=0;
		mtu=0;
		oti= oti_not_set;
		break;
	case err_UnknownNSLPID:
		// Unknown NSLPID
		errorclass=PermanentFailure;
		mli=0;
		mtu=0;
		oti= oti_not_set;
		break;
	case err_EndpointFound:
		//Endpoint Found
		errorclass=PermanentFailure;
		mli=0;
		mtu=0;
		oti= oti_not_set;
		break;
	case err_MessageTooLarge:
		// Message too large
		errorclass=PermanentFailure;
		mli=0;
		oti= oti_not_set;
		// MTU should be present
		infocount++;
		break;
	case err_ObjectTypeError:
		// Object Type Error
		errorclass=ProtocolError;
		mli=0;
		mtu=0;
		// oti should be present
		infocount++;
		break;
	case err_ObjectValueError:
		// Object Value Error
		errorclass=ProtocolError;
		mli=0;
		mtu=0;
		oti= oti_not_set;
		// ovi should be present
		if (ovi_object)	{
			ovi_length=ovi_object->get_serialized_size(protocol_v1); 
			infocount++;
		}
		break;
	case err_InvalidIPTTL:
		//Invalid IP-TTL
		errorclass=PermanentFailure;
		mli=0;
		mtu=0;
		oti= oti_not_set;
		break;
	case err_MRIValidationFailure:
		//MRI too wild
		errorclass=PermanentFailure;
		mli=0;
		mtu=0;
		oti= oti_not_set;
		if (ovi_object) {
			ovi_length=ovi_object->get_serialized_size(protocol_v1); 
			infocount++;
		}
		break;
    } // end switch

    if (debugcomment)
      flags|= DebugComment;
}


// interface function wrapper
void errorobject::serialize(NetMsg& msg, coding_t cod, uint32& wbytes) const {

    serializeEXT(msg, cod, wbytes, true);

}


// special serialization function with the ability to omit the TLV header
void errorobject::serializeEXT(NetMsg& msg, coding_t cod, uint32& wbytes, bool header) const {
	//uint32 save_pos= msg.get_pos();

    // calculate length of object body
    uint32 ielen = get_serialized_size(cod); // includes object TLV header (header_length)
    if (!header)
      ielen-= header_length;

    //DLog("errorobject::serialize()","SHOULD be of length: " << ielen << " bytes" << (header ? "(incl. object header)" : "(no object header)"));
    // check arguments and IE state
    check_ser_args(cod,wbytes);
    //DLog("errorobject::serialize()", "Bytes left in NetMsg: " << msg.get_bytes_left());
    // Encode object TLV header
    if (header)
    {
      encode_header_ntlpv1(msg, ielen - header_length);
      //DLog("errorobject::serialize()", "NTLP Object Header encoded (4 byte) bytes left in NetMsg: " << msg.get_bytes_left());
    }
    //DLog("errorobject::serialize()", "Begin of serialization");

    // encode errorclass
    msg.encode8(errorclass);

    // encode errorcode
    msg.encode16(errorcode);

    // encode errorsubcode
    msg.encode8(errorsubcode);

    // encode flags (4 bit + 12 Reserved)
    msg.encode16(flags);

    // encode mri length (get_serialized_size currently returns byte size INCLUDING TLV header, we must subtract 1 32bit word = 4 byte)
    // The MRI Length field gives the length of the MRI object in 32-bit words.
    if (flags & MRI) {
      msg.encode8((embedded_mri->get_serialized_size(cod)-header_length) / 4);
      //DLog("errorobject::serialize()", "Expected MRI length (#bytes w/o TLV header):" << embedded_mri->get_serialized_size(cod)-header_length);
    } 
    else 
      msg.encode8(0);

    // encode infocount
    msg.encode8(infocount);

    //DLog("errorobject::serialize()", "2nd word encoded (now: 12 bytes incl. object header), starting now common header, bytes left in NetMsg: " << msg.get_bytes_left());

    // encode common header

    msg.encode8(ch_version);
    msg.encode8(ch_hops);
    msg.encode16(ch_length);
    msg.encode16(ch_nslpid);
    msg.encode8(ch_type);
    msg.encode8(ch_flags);

    //DLog("errorobject::serialize()", "common header encoded (now: 20 bytes incl. object header), starting now common header, bytes left in NetMsg: " << msg.get_bytes_left());

    uint32 written=0;

    // the following objects are included without TLV headers

    // if sessionid flag is set, encode the sessionid

    if (flags & SessionID) {
	   
      if (embedded_sessionid) 
      {
	embedded_sessionid->serializeNoHead(msg, cod, written);
	//DLog("errorobject::serialize()", "SessionID encoded, "<< written << " bytes, bytes left in NetMsg: " << msg.get_bytes_left());
      }
      else 
	ERRCLog("errorobject::serialize()", "Tried to serialize NULL sessionid object!");
    }

    // if MRIO flag is set, encode the MRI

    if (flags & MRI) {
	    
      DLog("errorobject::serialize()", "encoding MRI, should be " << (embedded_mri->get_serialized_size(cod))-header_length << " bytes, bytes left in NetMsg: " << msg.get_bytes_left());
      // encode MRI 
      embedded_mri->serializeNoHead(msg, cod, written);
	    
      //DLog("errorobject::serialize()", "MRI encoded, "<< written << " bytes written, bytes left in NetMsg: " << msg.get_bytes_left());
	
    };

    // now encode the additional information

    // Message length info: length of original message calculated by adding up all objects
    //                      in the message
    if (mli) {
	    //DLog("errorobject::serialize()", "Encoding Msg Len Info (4 + 4 bytes), bytes left in NetMsg: " << msg.get_bytes_left());
      msg.encode16(ai_Message_Length_Info);
      msg.encode16(1);						// 32 bit words
      msg.encode16(mli);
      msg.encode16(0);   // lower 16 bits currently reserved
      //DLog("errorobject::serialize()", "Msg Len Info encoded, 4 + 4 bytes, bytes left in NetMsg: " << msg.get_bytes_left());
    }

    // MTU info: MTU for a link along which a message could not be sent
    if (mtu) {
	    //DLog("errorobject::serialize()", "Encoding MTU (4 + 4 bytes), bytes left in NetMsg: " << msg.get_bytes_left());
      msg.encode16(ai_MTU_Info);
      msg.encode16(1);						// 32 bit words
      msg.encode16(mtu);
      msg.encode16(0);   // lower 16 bits currently reserved
      //DLog("errorobject::serialize()", "MTU encoded, 4 + 4 bytes, bytes left in NetMsg: " << msg.get_bytes_left());
    }

    // Object Type Info: provides info about the object that caused the error
    if (oti != oti_not_set) {
	    //DLog("errorobject::serialize()", "Encoding Object Type Info (4 + 4 bytes), bytes left in NetMsg: " << msg.get_bytes_left());
      msg.encode16(ai_Object_Type_Info);
      msg.encode16(1);						// 32 bit words

      if (oti > 4095)  // oti must be only 12 bit!
	ERRLog("errorobject::serialize()", "oti is not valid (must be in range [0..4095]), will be cut down to allowed range" << oti);

      msg.encode16(oti & 0x0fff);
      msg.encode16(0);   // lower 16 bits currently reserved
      //DLog("errorobject::serialize()", "Object Type Info encoded, 4 + 4 bytes, bytes left in NetMsg: " << msg.get_bytes_left());

    }

    // for ovi info, we need not call IEManager like for deserialization
    if (ovi_object) {
      uint16 ai_len;
      //DLog("errorobject::serialize()", "Encoding Object value info 4 + " << ovi_object->get_serialized_size(protocol_v1)+4 <<" bytes, bytes left in NetMsg: " << msg.get_bytes_left());
      msg.encode16(ai_Object_Value_Info);
      ai_len = ovi_object->get_serialized_size(protocol_v1) + 1;
      msg.encode16(ai_len);
      msg.encode16(ai_len - 1);		// gratuitous and redundant ;)
      msg.encode16(ovi_offset);
      ovi_object->serialize(msg, cod, written);
      //DLog("errorobject::serialize()", "Object value info encoded, "<< written << " bytes, bytes left in NetMsg: " << msg.get_bytes_left());
    }

    wbytes = ielen;
	
    //ostringstream tmpostr;
    //msg.hexdump(tmpostr,msg.get_buffer()+save_pos,wbytes);
    //DLog("errorobject::serialize()", "netmsg pos:" << msg.get_pos() << " netmsg @" << save_pos << ":" << tmpostr.str().c_str());
    return;
} // end serialize


bool 
errorobject::check() const {
	return (true);
} // end check

size_t 
errorobject::get_serialized_size(coding_t cod) const 
{
  int size= header_length;
  
  if (!supports_coding(cod)) throw IEError(IEError::ERROR_CODING);

  // Error Class,Code,Subcode,Flags,MRI Length, Info Count: 2x32bit
  size+= 8;

  // Common Header, 2x32bit
  size+= ntlp_pdu::common_header_length;

  // Session ID
  if (embedded_sessionid) {if (flags & SessionID) size+= embedded_sessionid->get_serialized_size(cod) - header_length;}
  
  // MRI length in 8 = bit length - 4 byte (TLV header)
  if (embedded_mri) {if (flags & MRI) size+= (embedded_mri->get_serialized_size(cod)) - header_length;}
  
  // Additional Info
  if (mli) size+= 8; // 4 bytes header + 2 + 2 (reserved)
  if (mtu) size+= 8; // 4 bytes header + 2 + 2 (reserved)
  if (oti != oti_not_set) size+= 8; // 4 bytes header + 2 + 2 (reserved)

  // Object Value Error Information
  if (ovi_object) 
  {
    size+= 8 + ovi_object->get_serialized_size(cod) ; //  header (Real Object length + Offset) and invalid object (incl. TLV)
  }
  
  return size;

} // end get_serialized_size


// copy constructor
errorobject::errorobject(const errorobject& eo)
  : known_ntlp_object(eo),
  errorclass   (eo.errorclass),
  errorcode    (eo.errorcode),
  errorsubcode (eo.errorsubcode),
  infocount    (eo.infocount),
  ch_version   (eo.ch_version),
  ch_hops      (eo.ch_hops),
  ch_length    (eo.ch_length),
  ch_nslpid    (eo.ch_nslpid),
  ch_type      (eo.ch_type),
  ch_flags     (eo.ch_flags),
  flags        (eo.flags),
  embedded_sessionid(eo.embedded_sessionid ? eo.embedded_sessionid->copy() : 0),
  embedded_mri( eo.embedded_mri ? eo.embedded_mri->copy() : 0 ),
  mli (eo.mli),
  mtu (eo.mtu),
  oti (eo.oti),
  ovi_length (eo.ovi_length),
  ovi_offset (eo.ovi_offset),
  ovi_object(eo.ovi_object ? eo.ovi_object->copy() : 0),
  debugcomment(NULL)
{
  if (eo.debugcomment != NULL)
  {
    int slen= strlen(eo.debugcomment);
    debugcomment= new char[slen+1];
    strcpy(debugcomment, eo.debugcomment);
  }
} // end copy constructor

errorobject&
errorobject::operator=(const errorobject& eo)
{
  errorclass   = eo.errorclass;
  errorcode    = eo.errorcode;
  errorsubcode = eo.errorsubcode;
  infocount    = eo.infocount;
  ch_version   = eo.ch_version;
  ch_hops      = eo.ch_hops;
  ch_length    = eo.ch_length;
  ch_nslpid    = eo.ch_nslpid;
  ch_type      = eo.ch_type;
  ch_flags     = eo.ch_flags;
  flags        = eo.flags;
  if(embedded_sessionid != NULL)
  {
    delete embedded_sessionid;
  } 
  embedded_sessionid=  (eo.embedded_sessionid !=NULL) ? eo.embedded_sessionid->copy() : 0;

  if (embedded_mri != NULL)
  {
    delete embedded_mri;
  } 
  embedded_mri=  (eo.embedded_mri !=NULL) ? eo.embedded_mri->copy() : 0;

  mli = eo.mli;
  mtu = eo.mtu;
  oti = eo.oti;
  ovi_length = eo.ovi_length;
  ovi_offset = eo.ovi_offset;

  if (ovi_object != NULL)
  {
    delete ovi_object;
  } 
  ovi_object=  (eo.ovi_object !=NULL) ? eo.ovi_object->copy() : 0;


  if (debugcomment != NULL)
  {
    delete debugcomment;
    debugcomment= 0;
  }
  if (eo.debugcomment)
  {
    int slen= strlen(eo.debugcomment);
    debugcomment= new char[slen+1];
    strcpy(debugcomment, eo.debugcomment);
  }

  return *this;
} // end assignment operator


bool 
errorobject::operator==(const IE& ie) const {
  const errorobject* eo = dynamic_cast<const errorobject*>(&ie);
  if (eo)
  {
    return 
      errorclass   == eo->errorclass
      &&
      errorcode    == eo->errorcode
      &&
      errorsubcode == eo->errorsubcode
      &&
      infocount    == eo->infocount
      &&
      ch_version   == eo->ch_version
      &&
      ch_hops      == eo->ch_hops
      &&
      ch_length    == eo->ch_length
      &&
      ch_nslpid    == eo->ch_nslpid
      &&
      ch_type      == eo->ch_type
      &&
      ch_flags     == eo->ch_flags
      &&
      flags        == eo->flags
      &&
      ((embedded_sessionid != NULL && eo->embedded_sessionid !=NULL && (*embedded_sessionid == *(eo->embedded_sessionid)))
       || (embedded_sessionid == NULL && eo->embedded_sessionid == NULL))
      &&
      ((embedded_mri != NULL && eo->embedded_mri != NULL && *embedded_mri == *(eo->embedded_mri))
       || (embedded_mri == NULL && eo->embedded_mri==NULL))
      &&
      mli == eo->mli
      &&
      mtu == eo->mtu
      &&
      oti == eo->oti
      &&
      ovi_length == eo->ovi_length 
      &&
      ovi_offset == eo->ovi_offset
      &&
      ((ovi_object != NULL && eo->ovi_object !=NULL && (*ovi_object == *(eo->ovi_object)))
       || (ovi_object == NULL && eo->ovi_object == NULL))
      &&
      ((debugcomment != NULL && eo->debugcomment != NULL && (strcmp(debugcomment,eo->debugcomment)==0))
       || (debugcomment == NULL && eo->debugcomment == NULL));
  }
  else
      return false;
} // end operator==



const char *const
errorobject::get_errorsubcode_str() const
{
	switch (errorcode) {

	case err_CommonHeaderParseError:
		
		switch (errorsubcode) {
		case errsub_UnknownVersion : 
			return "Unknown Version";
			
		case    errsub_UnknownType :
			return "Unknown Type";

		case    errsub_InvalidRFlag :
			return "Invalid R Flag";

		case    errsub_IncorrectMsgLength : 
			return "Incorrect Message Length";

		case    errsub_InvalidEFlag :
			return "Invalid E Flag";
			
		case    errsub_InvalidCFlag :
			return "Invalid C Flag";
		default:
			return "Unknown/Unimplemented Error Subcode";
		} // end switch errsub
		break;

	case err_ObjectTypeError:
		switch (errorsubcode) {
		case errsub_DuplicateObject : 
			return "Duplicate Object";

		case errsub_UnrecognizedObject	:
			return "Unrecognized Object";

		case errsub_MissingObject     	:
			return "Missing Object";

		case errsub_InvalidObject     	:
			return "Invalid Object";

		case errsub_UntranslatedObject	:
			return "Untranslated Object";

		case errsub_InvalidExtensibilityFlags :
			return "Invalid Extensibility Flags";

		default:
			return "Unknown/Unimplemented Error Subcode";
		} // end switch errsub
		break;

	case err_ObjectValueError:
		switch (errorsubcode) {

		case    errsub_IncorrectLength :
			return "Incorrect Length";

		case    errsub_ValueNotSupported :
			return "Value Not Supported";

		case    errsub_InvalidFlagFieldCombination :
			return "Invalid Flag Field Combination";

		case    errsub_EmptyList :
			return "Empty List";

		case    errsub_InvalidCookie :
			return "Invalid Cookie";

		case    errsub_SP_SCD_Mismatch :
			return "Stack Proposal/Stack Configuration Data Mismatch";

		default:
			return "Unknown/Unimplemented Error Subcode";

		} // end switch errsub
		break;

	case err_MRIValidationFailure :
		switch (errorsubcode) {

		case errsub_MRITooWild :
			return "MRI too wild";

		case errsub_IPVersionMismatch :
			return "IP Version Mismatch";

		case errsub_IngressFilterFailure :
			return "Ingress Filter Failure";


		default:
			return "Unknown/Unimplemented Error Subcode";
		} // end switch errsub
		break;

	default:
		switch (errorsubcode) {
		case errsub_Default :
			return "(Plain/Default Subcode)";
		default:
			return "Unknown/Unimplemented Error Subcode";
		} // end switch errsub
		
	} // end switch error code
}

ostream& 
errorobject::print(ostream& os, uint32 level, const uint32 indent, const char* name) const 
{
    os<<setw(level*indent)<<"";

    if (name && (*name!=0)) os << name << endl;

    os << get_errorclass_str() << "(" << static_cast<unsigned int>(errorclass) << "): " 
       << get_errorcode_str() << " / " << get_errorsubcode_str() 
       << " (" << static_cast<unsigned int>(errorcode)  
       << "/" << static_cast<unsigned int>(errorsubcode) << ")" << endl;

    os << "Common header: ver=" << (short int) ch_version 
       << " hops=" << (short int) ch_hops
       << " length=" << ch_length
       << " nslpID=" << ch_nslpid
       << " type=" << (short int) ch_type
       << " flags=" << ((flags & SessionID) ? 'S' : '-')
                    << ((flags & MRI) ? 'M' : '-')
                    << ((flags & DebugComment) ? 'C' : '-')
                    << ((flags & DatagramMode) ? 'D' : '-')
                    << ((flags & QueryEncapsulation) ? 'Q' : '-')
       << " (" << hex << (short int) ch_flags << dec << ")" << endl;

    if (embedded_sessionid)
      os << *embedded_sessionid << endl;
    else
      os << "No Session ID present" << endl;

    if (flags & MRI) {

      os << "MRI-Length:   " << static_cast<int>(embedded_mri->get_serialized_size(protocol_v1)-header_length) << endl;

      // print MRI
      os << "Embedded MRI: ";
      if (embedded_mri)
	os << *embedded_mri << endl;
      else
	os << "n/a" << endl;
    }

    // print out additional objects if there are any

    os << "Number of additional info objects: " << (int) infocount << endl;

    os << "Message length info:" << mli << endl;

    os << "MTU info:" << mtu << endl;

    if (oti != oti_not_set) 
      os << "Object Type info:" << oti << endl;
    
    if (ovi_object)
    {
      os << "Object Value Info: len=" << ovi_length << " offset=" << ovi_offset << " object[" << ovi_object->get_ie_name() << "]=" << (*ovi_object) << endl;
    }

    if (flags & DebugComment)
    {
      os << "Debug comment: " << debugcomment << endl;
    }

    return os;
} // end print
 
istream& errorobject::input(istream& is, bool istty, uint32 level, const uint32 indent, const char* name) {
    //ntlp_object* tmp;
        string s;

	embedded_mri = new mri_pathcoupled();
	flags = flags | MRI;

	embedded_sessionid = new sessionid();
	flags = flags | SessionID;

	if (istty) {
		cout<<"";
		if (name && (*name!=0)) cout<<name<<" ";
		cout<<"<"<<get_ie_name()<<"> 1st 32bit: ";
	} // end if istty
	return is;
} // end input



//@}

} // end namespace protlib
