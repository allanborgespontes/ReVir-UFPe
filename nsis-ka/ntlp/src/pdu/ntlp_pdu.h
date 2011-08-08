/// ----------------------------------------*- mode: C++; -*--
/// @file ntlp_pdu.h
/// GIST pdu object classes
/// ----------------------------------------------------------
/// $Id: ntlp_pdu.h 6195 2011-05-25 13:31:46Z bless $
/// $HeadURL: https://svn.tm.kit.edu/nsis/ntlp/trunk/src/pdu/ntlp_pdu.h $
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
/** @ingroup ientlppdu 
 * NTLP PDUs
 */

#ifndef _NTLP__NTLPPDU_H_
#define _NTLP__NTLPPDU_H_

#include "protlib_types.h"
#include "ntlp_global_constants.h"
#include "ntlp_ie.h"
#include "ntlp_object.h"
#include "address.h"

#include "mri.h"
#include "sessionid.h"
#include "resp_cookie.h"
#include "query_cookie.h"
#include "nli.h"
#include "ntlp_errorobject.h"
#include "stackprop.h"
#include "stackconf.h"
#include "nattraversal.h"
#include "nslpdata.h"
#include "helloid.h"

namespace ntlp {

using namespace protlib;
    
    
/** @addtogroup ientlppdu NTLP PDUs
 * @ingroup ie
 * @{
 */
    
/** NTLP protocol data unit
 * Base class for NTLP PDU, representing common NTLP header and NTLP Objects. 
 */
class ntlp_pdu : public NTLP_IE {
/***** inherited from IE *****/
public:
  ntlp_pdu* new_instance() const = 0;
  ntlp_pdu* copy() const = 0;
  virtual ntlp_pdu* deserialize(NetMsg& msg, coding_t coding, IEErrorList& errorlist, uint32& bread, bool skip = true);
  virtual void serialize(NetMsg& msg, coding_t coding, uint32& wbytes) const;
  virtual bool check() const;
  virtual size_t get_serialized_size(coding_t coding) const;
  virtual void clear_pointers();
  virtual bool operator==(const IE& n) const;
/***** new members *****/
public:
  /// max PDU size
  static const uint32 max_size;
  /// common header length
  static const uint32 common_header_length;
  /// number of type values
  static const uint32 num_of_types;
  
  /// type used in tlp list to differ ntlp & nslp object types 
  static const uint16 tlplist_nslp_object_type; 
  static const uint16 tlplist_ntlp_object_type;

protected:
  /// constructor
  ntlp_pdu(bool known, uint8 t = 0, uint32 numobj = 0);
  /// copy constructor
  ntlp_pdu(const ntlp_pdu& n);
  /// assignment
  ntlp_pdu& operator=(const ntlp_pdu& n);
  /// check two objects for equality and handle NULL pointers.
  static bool compare_objects(const ntlp_object* o1, const ntlp_object* o2);
public:

  enum pdu_flags_t {
    S_Flag = 1 << 7,
    R_Flag = 1 << 6,
    E_Flag = 1 << 5
  };


  /// destructor
  virtual ~ntlp_pdu();
  /// decode header for msg content length (excluding common header)
  static bool decode_common_header_ntlpv1_clen(NetMsg& m, uint32& clen_bytes);
  /// decode header
  static void decode_common_header_ntlpv1(NetMsg& msg, uint8& ver, uint8& hops, uint16& clen_bytes, uint16& nslpid, uint8& type, uint8& flags, bool& c_flag, IEErrorList& errorlist);

  static uint16 get_tlplist_nslp_object_type(const uint8* buf) {
	  return ntlp_pdu::tlplist_nslp_object_type;
  }

  // function pointer for session authorization object integrity protection mechanisms
  static bool (*check_hmac )(NetMsg& msg);
  static bool (*serialize_hmac)(NetMsg& msg);
  
  /// get NTLP message type
  uint8 get_type() const;
  // @{
  /// get/set basic header and integrity
  //localmsgreference* get_orig_ref() const;
  //localmsgreference* set_orig_ref(localmsgreference* r);
  //integrity* get_integrity() const;
  //integrity* set_integrity(integrity* r);
    
  // @}

  // functions to return TLV objects which are present in every PDU
  // if there are no such objects, dynamic_cast will simply fail
  inline
  mri* get_mri() const {
    mri* mymr = NULL;
    mymr = dynamic_cast<mri*>(arr.get(mri_ptr));
#ifdef DEBUG_HARD
    if (!mymr) DLog("ntlp_pdu", "Access to MRI, but none is contained");
#endif
    return mymr;
  }
  
  inline
  errorobject* get_errorobject() const {
    errorobject* errobj = NULL;
    errobj = dynamic_cast<errorobject*>(arr.get(errorobject_ptr));
#ifdef DEBUG_HARD
    if (!errobj) DLog("ntlp_pdu", "Access to Errorobject, but none is contained");
#endif
    return errobj;
  }

  inline
  nattraversal* get_nattraversal() const {
    nattraversal* mynat = NULL;
    mynat = dynamic_cast<nattraversal*>(arr.get(nattrav_ptr));
#ifdef DEBUG_HARD
    if (!mynat) DLog("ntlp_pdu", "Access to NAT traversal object, but none is contained");
#endif
    return mynat;
  }
  
  inline
  helloid* get_helloid() const {
    helloid* myhid = NULL;
    myhid = dynamic_cast<helloid*>(arr.get(helloid_ptr));
#ifdef DEBUG_HARD
    if (!myhid) DLog("ntlp_pdu", "Access to HelloID object, but none is contained");
#endif
    return myhid;
  }
	

  inline
  void add_nat(const nattraversal* nattrav) {
    if (nattrav) arr.insert(nattrav_ptr, nattrav->copy());
  }
  
  inline
  void add_helloid(const helloid* hid) {
    if (hid) arr.insert(helloid_ptr, hid->copy());
  }
  
  inline
  void add_nli(const nli* nl) {
    nli_ptr = arr.size();
    if (nl) arr.insert(nli_ptr, nl->copy());
  }
  
  
  
  inline
  sessionid* get_sessionid() const {
    sessionid* mysid = NULL;
    if (sessionid_ptr) mysid =dynamic_cast<sessionid*>(arr.get(sessionid_ptr));
#ifdef DEBUG_HARD
    if (!mysid) DLog("ntlp_pdu", "Access to SessionID, but none is contained");
#endif
    return mysid;
  }
  
  inline
  nli* get_nli() const {
    nli* mynl = NULL;
    if (nli_ptr) mynl=dynamic_cast<nli*>(arr.get(nli_ptr));
#ifdef DEBUG_HARD
    if (!mynl) DLog("ntlp_pdu", "Access to NLI, but none is contained");
#endif
    return mynl;
  }
	
	
  inline
  respcookie* get_respcookie() const {
    respcookie* mycook = NULL;
    if (respcookie_ptr) mycook= dynamic_cast<respcookie*>(arr.get(respcookie_ptr));
#ifdef DEBUG_HARD
    if (!mycook) DLog("ntlp_pdu", "Access to Responder Cookie, but none is contained");
#endif
    return mycook;
  }
  
  inline
  querycookie* get_querycookie() const {
    querycookie* mycook = NULL;
    if (querycookie_ptr) mycook = dynamic_cast<querycookie*>(arr.get(querycookie_ptr));
#ifdef DEBUG_HARD
    if (!mycook) DLog("ntlp_pdu", "Access to Query Cookie, but none is contained");
#endif
    return mycook;
  }
  
  
  inline
  stackprop* get_stackprop() const {
    stackprop* tmp = NULL;
    if (stackprop_ptr) tmp =dynamic_cast<stackprop*>(arr.get(stackprop_ptr)); 
#ifdef DEBUG_HARD
    if (!tmp) DLog("ntlp_pdu", "Access to Stack Proposal, but none is contained");
#endif
    return tmp;
  }
  
  inline
  stack_conf_data* get_stackconf() const {
    stack_conf_data* tmp = NULL;
    if (stackconf_ptr) tmp = dynamic_cast<stack_conf_data*>(arr.get(stackconf_ptr));
#ifdef DEBUG_HARD
    if (!tmp) DLog("ntlp_pdu", "Access to Stack Configuration, but none is contained");
#endif
    return tmp;
  }
  
  inline
  nslpdata* get_nslpdata() const {
    nslpdata* tmp = NULL;
    if (nslpdata_ptr) tmp = dynamic_cast<nslpdata*>(arr.get(nslpdata_ptr));
#ifdef DEBUG_HARD
    if (!tmp) DLog("ntlp_pdu", "Access to NSLP Data, but none is contained");
#endif
    return tmp;
  }
  
  
  // get/set NTLP common header
  inline
  const uint8 get_version() const {
    return ntlp_version;
  }
	
  inline
  void set_version(uint8 v) {
    ntlp_version=v;
  }
	
  inline
  const uint8 get_hops() const {
    return ntlp_hops;
  }
  
  inline
  void set_hops(uint8 h) {
    ntlp_hops=h;
  }
  
  inline
  void dec_hops() {
    if (ntlp_hops>=1) ntlp_hops = ntlp_hops-1;
  }

  inline
  const uint16 get_length() const {
    return length;
  }
  
  
  inline
  void set_length(uint16 l) {
    length=l;
  }
  
  inline
  const uint16 get_nslpid() const {
    return nslpid;
  }
  
  inline
  void set_nslpid(uint16 nslp_id) {
    nslpid= nslp_id;
  }

  inline
  const uint8 get_flags() const {
    return flags;
  }

  inline
  void set_flags(uint8 f) {
    flags=f;
  }
  

  inline
  void set_R() {
    flags= flags | R_Flag;
  }

  inline
  void unset_R() {
    flags= flags^R_Flag;
  }

  inline
  const bool get_R() const {
    return flags & R_Flag;
  }

  inline
  void set_E() {
    flags = flags | E_Flag;
  }
  
  inline
  void unset_E() {
    flags = flags ^ E_Flag;
  }

  inline
  const bool get_E() const {
    return flags & E_Flag;
  }

  inline
  void set_S() {
    flags = flags | S_Flag;
  }

  inline
  void unset_S() {
    flags = flags ^ S_Flag;
  }
  
  inline
  const bool get_S() const {
    return flags & S_Flag;
  }

  inline
  const bool get_C() const {
	  return c_flag;
  }

  inline
  void set_C() {
	  c_flag = true;
  }

  inline
  void unset_C() {
	  c_flag= false;
  }


protected:


  /// NTLP object array
  class objectarray {
  private:
    typedef vector<ntlp_object*> objarr_t;
  public:
    static const uint32 unset;
    typedef objarr_t::iterator iterator_t;
    typedef objarr_t::const_iterator const_iterator_t;
  public:
    objectarray(uint32 size = 5);
    objectarray(const objectarray& oa);
    ~objectarray();
    objectarray& operator=(const objectarray& oa);
    bool set(uint32 pos, ntlp_object* o);
    ntlp_object* get(uint32 pos) const;
    uint32 append(ntlp_object* o);
    bool insert(uint32 pos, ntlp_object* obj);
    void clear(bool destroy = true);
    void clear(uint32 pos, bool del = true);
    bool reserve(uint32 n, bool init = true);
  private:
    objarr_t arr;
  public:
    inline iterator_t begin() { return arr.begin(); }
    inline const_iterator_t begin() const { return arr.begin(); }
    inline iterator_t end() { return arr.end(); }
    inline const_iterator_t end() const { return arr.end(); }
    inline uint32 size() const { return arr.size(); }
    inline bool is_empty() const { return arr.empty(); }
  } arr; // end class objectarray
  
  
  // @{
  /// basic header
  uint8 ntlp_version;
  uint8 ntlp_hops;
  uint16 length;
  uint16 nslpid;
  uint8 type;
  uint8 flags;
  bool  c_flag; // this is special, because it's not in the same byte as the other flags

  
  uint32 nattrav_ptr;
  uint32 mri_ptr;
  uint32 sessionid_ptr;
  uint32 nli_ptr;
  uint32 querycookie_ptr;
  uint32 stackprop_ptr;
  uint32 stackconf_ptr;
  uint32 nslpdata_ptr;
  uint32 respcookie_ptr;
  uint32 errorobject_ptr;
  uint32 helloid_ptr;
  
  // @}
  void print_object(uint32 pos, ostream& os, uint32 level, const uint32 indent, bool& o, const char* name = NULL) const;
  /***** for deserialization *****/
  enum deser_error_t { deser_ok, type_error, subtype_error };
  /** Check and set type and subtype and prepare PDU for deserialization. */
  virtual deser_error_t accept_type_and_subtype(uint8 t) = 0;
  /** Check the given object and append it to the object array. */
  virtual bool accept_object(known_ntlp_object* o, uint32& designated_pos, IEErrorList& errorlist) = 0;
  /** Finish deserialization, set all fields and generate errors.
   * @returns true if PDU state OK, false otherwise.
   */
  virtual bool deser_end(IEErrorList& errorlist) = 0;
}; // end clas ntlp_pdu

/// Known NTLP PDUs
class known_ntlp_pdu : public ntlp_pdu {
/***** inherited from IE *****/
public:
	known_ntlp_pdu* new_instance() const = 0;
	known_ntlp_pdu* copy() const = 0;
protected:
	virtual void register_ie(IEManager* iem) const;
/***** new members *****/
public:
	/// message type
	/** All NTLP message types have to be listed here and their values should
	 * correspond with those sent over the wire. Otherwise they have to be
	 * mapped when (de)serializing.
	 * Type values must not exceed 8 bits in NTLP_V1.
	 */
	enum type_t {
		Query             =   0, 
		Response          =   1, 
		Confirm           =   2, 
		Data              =   3, 
		Error             =   4, 
		Hello             =   5, 
		MaxTypeNonValid   =   6,
		reserved          = 255
	}; // end type_t		
	static bool is_known(uint8 t);


	
protected:
    /// constructor
    known_ntlp_pdu(type_t t, uint32 numobj = 0);
    /// copy constructor
    known_ntlp_pdu(const known_ntlp_pdu& n);



public:
    /// Is this a query?
    bool is_query() const;
    /// Is this a response?
    bool is_response() const;
    bool is_confirm() const;
    bool is_error() const;
    bool is_data() const;
    bool is_hello() const;
        /// has the PDU a related response?
    bool has_response() const;
    /// Is this of other type - neither request nor response?
    bool is_other() const;
    /// Is this a response to another PDU?
    bool is_response_to(const known_ntlp_pdu& req) const;

    /// replace DATA payload
    bool set_data(nslpdata* data);

}; // end class known_ntlp_pdu


/** Decode common header for getting msg content length (bytes following the common header) 
 *  without doing any sanity checks. It is assumed that the common header starts at the
 *  current position of the buffer. Does not modify position pointer in m. 
 *  @return true if NTLP header seems to be ok, false if not a NTLP header
**/
inline
bool
ntlp_pdu::decode_common_header_ntlpv1_clen(NetMsg& m, uint32& clen_bytes) 
{
  // save starting position (we may have already skipped some bytes, e.g. magic number)
  uint32 savepos= m.get_pos();
  // get protocol version
  uint8 version = m.decode8();
  if (version == ntlp_version_default) 
  {
    // skip hop count
    m.decode8();
    // length counted in number of 32 bit words
    clen_bytes=m.decode16()*4;
    // go back to position where we came from
    m.set_pos(savepos);
    
    return true;
  }
  else 
  {
    clen_bytes=0;
    // go back to position where we came from
    m.set_pos(savepos);
    
    return false;
  }
} // end decode_common_header_ntlpv1_clen

/***** class known_ntlp_pdu *****/

/***** inherited from IE *****/

/** Register this ntlp_pdu with current type and subtype. */
inline
void known_ntlp_pdu::register_ie(IEManager* iem) const 
{
    iem->register_ie(category,type,0,this); // TODO: REMOVE SUBTYPE IN IEManager ?!?!?!?!?
} // end register_ie

/***** new members *****/

inline
ntlp_pdu::~ntlp_pdu() 
{ 
  arr.clear(true); 
}

/** set PDU type and subtype.
 * @param t message type
 * @param numobj number of objects to reserve for subclass
 */
inline
known_ntlp_pdu::known_ntlp_pdu(type_t t, uint32 numobj) 
  : ntlp_pdu(true,t,numobj)    
{
} // end constructor known_ntlp_pdu

inline
known_ntlp_pdu::known_ntlp_pdu(const known_ntlp_pdu& n) : ntlp_pdu(n) 
{
} // end copy constructor


/** @return type of the NTLP PDU. */
inline
uint8 
ntlp_pdu::get_type() const { return type; }


//inline
//localmsgreference* 
//ntlp_pdu::get_orig_ref() const {
//	return dynamic_cast<localmsgreference*>(arr.get(origref));
//} // end get_orig_ref


/** Return true if this is a REAL request that requires a response. */
inline
bool 
known_ntlp_pdu::is_query() const 
{
  return ((type==Query));
} // end is_query


/** Return true if this is a response. */
inline
bool 
known_ntlp_pdu::is_response() const 
{
  return ((type==Response));
} // end is_response

/** Return true if this is a confirm. */
inline
bool 
known_ntlp_pdu::is_confirm() const 
{
  return ((type==Confirm));
} // end is_confirm

/** Return true if this is an error. */
inline
bool 
known_ntlp_pdu::is_error() const 
{
  return ((type==Error));
} // end is_error


/** Return true if this is a response. */
inline
bool 
known_ntlp_pdu::is_data() const 
{
  return ((type==Data));
} // end is_data

/** Return true if this is a MA-Hello PDU. */
inline
bool 
known_ntlp_pdu::is_hello() const 
{
  return ((type==Hello));
} // end is_hello




/** Return true if this is neither a request nor a response.
 * This PDU does not need a response or is not a response to a request.
 * This is true e.g. for info or error PDUs.
 * @note RAbtReq is NOT a request since it doe NOT need a response.
 */
inline
bool 
known_ntlp_pdu::is_other() const 
{
  return ((type==Confirm) || (type==Data) || (type==Error)
	  || (type==Hello));


} // end is_other


inline
bool 
known_ntlp_pdu::has_response() const
{
  switch(type)
  {
    case Query  :
      return true;
    break;

    default:
      return false;
  } // end switch
} // end is_response_to

inline
bool 
known_ntlp_pdu::is_response_to(const known_ntlp_pdu& req) const 
{
    return ( (type==Response) && (req.type==Query) );
	   
} // end is_response_to


//@}

} // end namespace protlib

#endif // _NTLP__NTLPPDU_H_
