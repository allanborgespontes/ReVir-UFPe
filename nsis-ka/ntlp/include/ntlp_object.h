/// ----------------------------------------*- mode: C++; -*--
/// @file ntlp_object.h
/// GIST/NTLP object base classes
/// ----------------------------------------------------------
/// $Id: ntlp_object.h 6149 2011-05-18 12:54:22Z bless $
/// $HeadURL: https://svn.tm.kit.edu/nsis/ntlp/trunk/include/ntlp_object.h $
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
/** @ingroup ientlpobject 
 * @file
 * NTLP object base classes
 */

#ifndef _NTLP__NTLPOBJECT_H_
#define _NTLP__NTLPOBJECT_H_

#include "protlib_types.h"

#include "ntlp_ie.h"


namespace ntlp {
using namespace protlib; 


/** @addtogroup ientlpobject NTLP Objects
 * @ingroup ie
 * @{
 */

/// NTLP object
/** Base class for (known and unknown) objects which can be part of a NTLP PDU. 
 * Note that service specific data have their own category and base
 * class.
 */
class ntlp_object : public NTLP_IE {
/***** inherited from IE *****/
public:
	virtual ntlp_object* new_instance() const = 0;
	virtual ntlp_object* copy() const = 0;
public:
	/// max object content size
	static const uint16 max_size;
	/// object header length
	static const uint32 header_length;

protected:
	/// constructor
	ntlp_object(bool known);
public:
    /// number of object types
    static const uint32 num_of_types;
    /// number of object subtypes per type
    static const uint32 num_of_subtypes;
    /// number of action values
    static const uint32 num_of_actions;
    /// decompose ATH/subtype
    static void decompose_ac_st(uint8 ac_st, uint8& ac, uint8& st);
    /// compose ATH/subtype
    static uint8 compose_ac_st(uint8 ac, uint8 st);
    /// decode header for GIST_v1
    static void decode_header_ntlpv1(NetMsg& m, uint16& t, uint16& st, bool& nat_flag, uint8& ac, uint16& dlen, uint32& ielen);
    /// get type
    virtual uint16 get_type() const = 0;

    static uint16 getTypeFromTLVHeader(const uint8* msgbuf);
    static uint32 getLengthFromTLVHeader(const uint8* msgbuf);
	
}; // end class gist_object

/// Known NTLP object
/** Base class for known objects which can be part of a GIST PDU. 
 */
class known_ntlp_object : public ntlp_object {
/***** inherited from IE *****/
protected:
	/// register this IE
	virtual void register_ie(IEManager *iem) const;
/***** new members *****/
public:
	
 

        /// object type
	/** All NTLP object types have to be listed here and their values should
	 * correspond with those sent over the wire. Otherwise they have to be
	 * mapped when (de)serializing.
	 * Type values must not exceed 12 bits in NTLP_V1.
	 * 
	 * see section 9 (IANA considerations) of GIST specification
	 * +---------+-----------------------------+
	 * | OType   | Object Type                 |
	 * +---------+-----------------------------+
	 * | 0       | Message Routing Information |
	 * | 1       | Session ID                  |
	 * | 2       | Network Layer Information   |
	 * | 3       | Stack Proposal              |
	 * | 4       | Stack Configuration Data    |
	 * | 5       | Query Cookie                |
	 * | 6       | Responder Cookie            |
	 * | 7       | NAT Traversal               |
	 * | 8       | NSLP Data                   |
	 * | 9       | Error                       |
	 * | 10      | Hello ID                    |
	 * +---------+-----------------------------+
	 * Allocation policies for further values are as follows:
	 * 10-1023:  Standards Action
	 * 1024-1999:  Specification Required
	 * 2000-2047:  Private/Experimental Use
	 *
	 */
	enum type_t {
	    MRI                 =   0, 
	    SessionID           =   1, 
	    NLI                 =   2, 
	    StackProposal       =   3, 
	    StackConfiguration  =   4, 
	    QueryCookie         =   5, 
	    RespCookie          =   6, 
	    NatTraversal        =   7, 
	    Nslpdata            =   8, 
	    ErrorObject         =   9, 
	    HelloID	      	=  10
	}; // end type_t
	static bool is_known(uint8 t);
	/// object subtype
	/** Each ntlp object type has its own set of subtypes. These subtypes
	 * are all represented by subtype_t values and the object subtype is
	 * stored here in the base class for better casting performance.
	 */
    enum subtype_t {
	/** 255 is not a valid subtype and is used to register for all 
	 * subtypes.
	 * @note This is no valid subtype because it is not in 0..64. 
	 */
	pathcoupled           = 0,
	looseend              = 1,
	explicitsigtarget     = 125,
	all_subtypes          = 255
	// @{
	/// ContextHandle subtypes
	
    }; // end subtype_t
    /// action
    /** Specifies the action that must be taken if the object type or subtype
     * are unknown. This must not exceed 3 bits.
     */
    enum action_t {
	Mandatory  = 0,
	Ignore     = 1,
	Forward    = 2
    }; // end action_t

       subtype_t subtype;       

protected:
	/// constructor
	known_ntlp_object(type_t t,  action_t a = Mandatory);
	known_ntlp_object(type_t t, subtype_t st, action_t a = Mandatory);
	/// copy constructor
	known_ntlp_object(const known_ntlp_object& n);
	/// assignment
	known_ntlp_object& operator=(const known_ntlp_object& n);
public:
	virtual known_ntlp_object* copy() const = 0;

	/// get type
	virtual uint16 get_type() const;
	/// get action
	action_t get_action() const;
	/// set action
	action_t set_action(action_t ac) ;
	/// a context handle?
	bool is_context() const;
	/// an address?
	bool is_address() const;
	/// a MsgRef?
	bool is_msg_ref() const;
	/// a time object?
	bool is_time() const;
	/// a waiting condition?
	bool is_wait_cond() const;
	/// a refresh interval?
	bool is_refresh_interval() const;
	/// string object?
	bool is_string() const;
	/// an error object
	bool is_error_object() const;
    /// is it a mri object?
    bool is_mri() const;

    /// is it a mri object?
    bool is_nattrav() const;

    ///is it a sessionid?
    bool is_sessionid() const;

    ///is it a nli?
    bool is_nli() const;

    ///is it a responder cookie?
    bool is_respcookie() const;

    ///is it a query cookie?
    bool is_querycookie() const;

    ///is it a stack proposal?
    bool is_stackprop() const;

    ///is it a stack configuration data object?
    bool is_stackconf() const;

    ///is it a NAT traversal object?
    bool is_nattraversal() const;

    ///is it a NSLP data object?
    bool is_nslpdata() const;

    ///is it an ERROR object?
    bool is_errorobject() const;

	///is it an HelloID object?
	bool is_helloid() const;

protected:
	/// object type
	const type_t type;
	/// current object action
	/** This is no constant! */
	action_t action;
	/// decode header for ntlp_v1
	bool decode_header_ntlpv1(NetMsg& msg, uint16& len, uint32& ielen, uint32& saved_pos, uint32& resume, IEErrorList& errorlist, uint32& bread, bool skip = true);
	/// encode header for ntlp_v1
	void encode_header_ntlpv1(NetMsg& msg, uint16 len) const;
	// @{
	/// report error
	
	void error_wrong_length(coding_t cod, uint16 len, uint32 msgpos, bool skip, IEErrorList& elist, uint32 resume, NetMsg& msg);
	// @}
public:
// @{
/// address tests
	/// is it a host address?
	bool is_host_addr() const;
	/// is it a network prefx
	bool is_net_addr() const;
	/// is it an application address
	bool is_appl_addr() const;
	/// is it an AS number?
	bool is_as_num() const;
	/// is it a user identification?
	bool is_user_id() const;
	/// is it an NAI?
	bool is_nai() const;
	/// an X.509?
	bool is_x509() const; 
	/// is it a source or destination address of a reservation?
	bool is_source_or_dest() const;
	/// is it an IP address?
	bool is_ip_addr() const;
// @}
// @{
/// context handle tests
	bool is_single_handle() const;
	bool is_handle_list() const;
	bool is_compact_handle_list() const;
	bool is_continued_handle_list() const;
	bool is_session_id() const;
// @}
// @{
/// encapsulated message tests
	bool is_gist_msg() const;
	bool is_other_msg() const;
// @}
// @{
/// SimpleOption tests
	bool is_padding_option() const;
	bool is_refresh_option() const;
	bool is_keep_option() const;
        bool is_confirmed_option() const;

// @}
}; // end class known_gist_object

/// Raw NTLP object
/** This contains a raw NTLP object and is used tostore unknown GIST
 * objects.
 */ 
class raw_ntlp_object : public ntlp_object {
public:
/***** inherited from IE *****/
	/// get a new instance
	virtual raw_ntlp_object* new_instance() const;
	/// copy instance
	virtual raw_ntlp_object* copy() const;
	/// deserialization
	virtual raw_ntlp_object* deserialize(NetMsg& msg, coding_t cod, IEErrorList& errorlist, uint32& bread, bool skip = true);
	/// serialize
	virtual void serialize(NetMsg& msg, coding_t cod, uint32& wbytes) const;
	/// consistency check
	virtual bool check() const;
	/// serialized size
	virtual size_t get_serialized_size(IE::coding_t c) const;
	/// equality
	virtual bool operator==(const IE& ie) const;
	virtual const char* get_ie_name() const;
	virtual ostream& print(ostream& os, uint32 level, const uint32 indent, const char* name = NULL) const;
/***** new in raw_ntlp_object *****/
	/// constructor
	raw_ntlp_object();
	/// copy constructor
	raw_ntlp_object(const raw_ntlp_object& o);
	/// constructor for raw bytes
        raw_ntlp_object(const uchar *buf, uint32 buflen);
	/// assignment
	raw_ntlp_object& operator=(const raw_ntlp_object& rdo);
	/// destructor
	virtual ~raw_ntlp_object();
	/// get type
	virtual uint16 get_type() const;
	/// get action
	uint8 get_action() const;
	/// get action and subtype byte
	uint8 get_ac_st() const;
	/// get content length
	uint16 get_content_length() const;
	/// get object content
	const uchar* get_content() const;
	/// get coding sheme
	coding_t get_coding() const;
protected:
	/// register a raw_gist_object
	virtual void register_ie(IEManager *iem) const;
private:
	/// buffer length
	uint32 buflen;
	/// raw object buffer
	uchar* buffer;
	/// coding of raw ntlp_object
	/** a raw object can only be serialized in the coding sheme with which it
	 * was deserialized because it contains uninterpreted data.
	 */
	coding_t coding;
	/// object type
	uint16 type;
	/// object action type hint
	uint8 action;
	/// data length
	uint16 datalen;
	/// IE name
	static const char* const iename;
}; // end class raw_ntlp_object

/** @return NTLP object type. */
inline
uint16 
known_ntlp_object::get_type() const { return type; }

/** @return NTLP action. */
inline
known_ntlp_object::action_t 
known_ntlp_object::get_action() const { return action; }

inline
bool 
known_ntlp_object::is_session_id() const {
	return ((type==SessionID)); 
} // end is_session_id

inline
bool
known_ntlp_object::is_mri() const {
    return ((type==MRI));
} // end is_mri

inline
bool
known_ntlp_object::is_nattrav() const {
    return ((type==NatTraversal));
} // end is_mri




inline
bool
known_ntlp_object::is_sessionid() const {
    return ((type==SessionID));
} // end is_sessionid

inline
bool
known_ntlp_object::is_nli() const {
    return ((type==NLI));
} //end is_nli

inline
bool
known_ntlp_object::is_respcookie() const {
    return ((type==RespCookie));
} //end is_resp_cookie

inline
bool
known_ntlp_object::is_querycookie() const {
    return ((type==QueryCookie));
} //end is_query_cookie

inline
bool
known_ntlp_object::is_stackconf() const {
    return ((type==StackConfiguration));
} //end is_query_cookie



inline
bool
known_ntlp_object::is_stackprop() const {
    return ((type==StackProposal));
} //end is_stackprop

inline
bool
known_ntlp_object::is_nattraversal() const {
    return ((type==NatTraversal));
} //end is_nattraversal

inline
bool
known_ntlp_object::is_nslpdata() const {
    return ((type==Nslpdata));
} //end is_nslpdata

inline
bool
known_ntlp_object::is_errorobject() const {
    return ((type==ErrorObject));
} //end is_errorobject

inline
bool
known_ntlp_object::is_helloid() const {
    return ((type==HelloID));
} //end is_helloid



/***** encapsulated message tests *****/

inline
const char* 
raw_ntlp_object::get_ie_name() const 
{ return iename; }

/** Check a raw_gist_object object. */
inline
bool 
raw_ntlp_object::check() const {
	if (buflen && buffer) return true;
	else return false;
} // end check

/** Register a raw_gist_object with the given IE Manager.
 * The IE is registered with its category and type and subtype 0.
 */
inline
void 
raw_ntlp_object::register_ie(IEManager *iem) const {
	iem->register_ie(category,0,0,this);
} // end register_ie

/** Destroy raw_gist_object. */
inline
raw_ntlp_object::~raw_ntlp_object() {
	if (buffer) delete[] buffer;
} // end destructor ~raw_ntlp_object

/** @return type of raw object or 0 if object is not in a valid state. */
inline
uint16 
raw_ntlp_object::get_type() const {
	if (check()) {
		return type;
	} // end if check
	return 0;
} // end get_type


/** @return action value of raw object or 0 if object is not in a valid state. */
inline
uint8 
raw_ntlp_object::get_action() const {
	if (check()) {
		return action;
	} // end if check
	return 0;
} // end get_action

/** @return combined action and subtype value of raw object or 0 if object is not in a valid state. */
inline
uint8 
raw_ntlp_object::get_ac_st() const {
	if (check()) {
		return action;
	} // end if check
	return 0;
} // end get_ac_st

/** @return length of raw_gist_object content (without object header) 
 * or 0 if object state is invalid.
 */
inline
uint16 
raw_ntlp_object::get_content_length() const {
	if (check()) {
		return datalen;
	} // end if check
	return 0;
} // end get_content_length

/** @return pointer to raw content data or NULL if ibject state is invalid. */
inline
const uchar* 
raw_ntlp_object::get_content() const {
	if (check() && (get_content_length()>0)) {
		return buffer+4;
	} // end if check
	return NULL;
} // end get_content()

/** @return object coding sheme.
 * The object can only be serialized in thes coding sheme.
 */
inline
IE::coding_t 
raw_ntlp_object::get_coding() const {
	return coding;
} // end get_coding

//@}

} // end namespace protlib

#endif // _NTLP__NTLPOBJECT_H_

