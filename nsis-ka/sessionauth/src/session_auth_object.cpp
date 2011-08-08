#include "session_auth_object.h"
#include "hmac_keyregistry.h"
#include "ntlp_pdu.h"
#include "nslp_pdu.h"
#include "benchmark_journal.h"
#include "session_auth_api.h"
#include <algorithm>

extern "C"
{
// hashing functions from OpenSSL
#include <openssl/hmac.h>
}

//ToDo: hmac error derived from protlib::IEError 

namespace nslp_auth {

using namespace protlib::log;
using namespace qos_nslp;
using namespace ntlp;

// #define PRINT_HMAC_BUF_LEN

#ifdef BENCHMARK
  extern benchmark_journal journal;
#endif

/**
 * Length of a header in bytes.
 */
const uint16 session_auth_object::HEADER_LENGTH = 4;
const uint16 session_auth_object::NSLP_TYPE = (uint16) 

known_nslp_object::SESSION_AUTH_OBJECT;
const size_t session_auth_object::HMAC_size = EVP_MD_size(EVP_sha1());

const uint16 session_auth_object::TRANS_ID_AUTH_HMAC_SHA1_96= 2;


/**
 * Extract the Length.
 *
 * Extract the Flags from a header. The header is expected to be
 * in host byte order already.
 *
 */
uint16 session_auth_object::extract_length(uint32 header_raw) throw (){
  	return (uint16)(header_raw & 0xFFF);
}


inline bool is_lower_than( auth_attr *a, auth_attr *b){
  return ( a->get_xtype() < b->get_xtype() ? true : 
         ( a->get_xtype() == b->get_xtype() && a->get_subtype() < b->get_subtype() ? true : false ));
}

bool session_auth_object::is_mandatory() const {
	return action==known_nslp_object::mandatory;
}

bool session_auth_object::is_ignore() const {
	return action==known_nslp_object::ignore;
}

bool session_auth_object::is_forward() const {
	return action==known_nslp_object::forward;
}

bool session_auth_object::is_refresh() const {
	return action==known_nslp_object::refresh;
}

void session_auth_object::set_mandatory() {
	action=known_nslp_object::mandatory;
}

void session_auth_object::set_ignore() {
	action=known_nslp_object::ignore;
}

void session_auth_object::set_forward() {
	action=known_nslp_object::forward;
}

void session_auth_object::set_refresh() {
	action=known_nslp_object::refresh;
}

// Auth Session Object Body length in bytes
size_t session_auth_object::get_serialized_size(coding_t coding) const {
	
	size_t size= HEADER_LENGTH;
	vector<auth_attr*>::const_iterator iter;
	for(iter=vec_attr.begin();iter!=vec_attr.end(); iter++){
			size+= (*iter)->get_serialized_size(coding);
	} // end for
	//DLog("session_auth_object", "get_serialized_size()=" << size);

	return size;
}

/**
 * Add an attribute to this object.
 *
 * If there was already a attribut registered by that ID, the old parameter
 * is deleted and the new one added instead.
 *
 * Note: All attributes are deleted by session_auths's destructor.
 *
 * @param attr the attribute to add
 */
void session_auth_object::insert_attr(auth_attr *attr) {
	vec_attr.push_back(attr);
}


/**
 * Remove a parameter.
 *
 * Remove the attribute with ID from this session auth object. If there is no
 * parameter with that ID, NULL is returned.
 *
 * The parameter itself is *not* deleted.
 *
 * @param id the parameter ID (12 bit)
 * @return the parameter with that ID or NULL if there is none
 */
auth_attr* session_auth_object::remove_attr(uint8 xtype, uint8 subtype) {
	vector<auth_attr*>::iterator iter;
	auth_attr* ret = NULL;
	for(iter=vec_attr.begin();iter!=vec_attr.end(); iter++){
	  if( (*iter)->get_xtype()==xtype && (*iter)->get_subtype()==subtype )
	  {
	    ret=*iter;
	    vec_attr.erase(iter);
	    break;
	  }
	} // end for
	return ret;
}


/**
 * Check whether session auth object is HMAC signed
 * @return true if HMAC signed object
 */
bool 
session_auth_object::is_hmac_signed() const {
	vector<auth_attr*>::const_iterator iter;

	for (iter= vec_attr.begin(); iter!= vec_attr.end(); iter++) {

		if ((*iter)-> get_xtype() == AUTH_ENT_ID && (*iter)-> get_subtype() == HMAC_SIGNED ) 
			return true;
  	}

	return false;
}


/**
 * Check whether session auth object is HMAC signed and contains required objects
 * @return true if HMAC signed object
 */
bool 
session_auth_object::check_hmac_cond() const {
	vector<auth_attr*>::const_iterator iter;

	bool data=false, obj_list=false, hmac_signed=false;

	for (iter= vec_attr.begin(); iter!= vec_attr.end(); iter++) {

		if ((*iter)-> get_xtype() == AUTH_ENT_ID && (*iter)-> get_subtype() == HMAC_SIGNED ) 
			hmac_signed=true;
		else 
		if ( (*iter)-> get_xtype() == AUTH_NSLP_OBJECT_LIST )
			obj_list = true;
		else 
		if ( (*iter)-> get_xtype() == AUTHENTICATION_DATA )
			data = true;
  	}

  	// signature given 
	if ( (hmac_signed && !data) || (obj_list && (!data || !hmac_signed)) ) {
		ERRCLog("auth_attr", "check_hmac_cond(): HMAC conditions failed (need AUTHENTICATION_DATA,NSLP_OBJECT_LIST)");
		return false;
	}
	return true;
}

/**
 * Parse a auth_attr header.
 * @param msg the NetMsg to read from
 * @param coding encoding to use (not checked!)
 * @param body_length the length of the body in bytes (according to the header)
 * @param errorlist errors
 * @param bytes_read the number of bytes read
 * @param skip skip the header in case of parse errors
 * @return true on success, false if there's a parse error
 */
bool session_auth_object::deserialize_body(NetMsg &msg, coding_t coding, uint16 body_length, IEErrorList &err, uint32 &bytes_read, bool skip){

	uint32 start_pos=msg.get_pos();

	AUTH_ATTR_IEManager *ieman = AUTH_ATTR_IEManager::instance();
//	ieman->register_known_ies();

	// Error: Message is shorter than the length field makes us believe.
	if ( msg.get_bytes_left() < body_length ) {
		catch_bad_alloc( err.put(
			new IEMsgTooShort(coding, category, start_pos)) );

		msg.set_pos(start_pos);
		return NULL;
	}

	// have to clean up first
	vector<auth_attr*>::const_iterator iter;
	for(iter= vec_attr.begin(); iter != vec_attr.end(); iter++) 
		delete *iter;

	vec_attr.clear();
    
	uint32 attr_bytes_read = 0;
	while (bytes_read < body_length) {
		attr_bytes_read = 0;
		IE *tmp_ie = ieman->deserialize_auth_attr(msg, coding, err, attr_bytes_read, skip);
		bytes_read += attr_bytes_read;
		if (tmp_ie) 
			vec_attr.push_back(dynamic_cast<auth_attr*>(tmp_ie));
		else 
			return false;
	}
	
	return true;
}

bool session_auth_object::deserialize_header(NetMsg &msg, coding_t coding,uint16 &body_length, IEErrorList &errorlist, uint32 &bytes_read, bool skip) {

	uint32 start_pos = msg.get_pos();

	uint32 header_raw;
	try {
		header_raw = msg.decode32();
	}
	catch ( NetMsgError ) {
		catch_bad_alloc( errorlist.put(new IEMsgTooShort(coding, get_category(), start_pos)) );
		return false;
	}

 	// The header's value is in number of 32 bit words
	body_length = (4*extract_length(header_raw));
	
	// Type 
	type = (known_nslp_object::type_t) ((header_raw >> 16)&0xFFF);
	
	// Flags
	action = (known_nslp_object::action_type_t) ((header_raw >> 30) & 0x03);
	
	// Error: Message is shorter than the length field makes us believe.
	if ( msg.get_bytes_left() < body_length ) {
		catch_bad_alloc( errorlist.put(new IEMsgTooShort(coding, category, start_pos)) );
 		msg.set_pos(start_pos);
		return false;
	}

	bytes_read = msg.get_pos() - start_pos;
	return true;
}



IE *
session_auth_object::deserialize(NetMsg &msg, coding_t coding, IEErrorList &err, uint32 &bytes_read, bool skip) {

	bytes_read = 0;
	uint32 start_pos = msg.get_pos();

	// check if coding is supported
	uint32 tmp;
	if ( ! check_deser_args(coding, err, tmp) )
		return NULL;

	// Parse header
	uint16 body_length = 0;
	//MP(benchmark_journal::PRE_DESERIALIZE_SESSIONAUTH_HEADER);
	if ( ! deserialize_header(msg, coding, body_length, err, bytes_read, skip) )
		return NULL;
	//MP(benchmark_journal::POST_DESERIALIZE_SESSIONAUTH_HEADER);

	// Parse body
	uint32 body_bytes_read = 0;
	//MP(benchmark_journal::PRE_DESERIALIZE_SESSIONAUTH_BODY);
	if ( ! (deserialize_body(msg, coding, body_length, err, body_bytes_read, skip)) )
		return NULL;
	//MP(benchmark_journal::POST_DESERIALIZE_SESSIONAUTH_BODY);
	bytes_read += body_bytes_read;

	
	// Check if deserialize_body() returned the correct length.
	if ( bytes_read != msg.get_pos() - start_pos )
		Log(ERROR_LOG, LOG_CRIT, "session_auth_object",	"deserialize(): byte count mismatch");

	return this;	// success
}


/**
 * Write a Aut_Attr header.
 *
 * @param msg the NetMsg to write to
 * @param coding encoding to use (not checked!)
 * @param bytes_written the number of bytes written
 * @param body_length the length of the body in bytes(!)
 *
 *    0                   1                   2                   3
 *    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 *   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *   |A|B|r|r|         Type          |r|r|r|r|        Length         |
 *   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *   +                                                               +
 *   //         Session Authorization Attribute List                //
 *   +                                                               +
 *   +---------------------------------------------------------------+
 */

void 
session_auth_object::serialize_header(NetMsg &msg, coding_t coding, uint32 &bytes_written, uint16 body_length) const {
	
	// RFC5981: The Length field is given in units of 32-bit words and
	// measures the length of the Value component of the TLV object (i.e.,
	// it does not include the standard header).
	uint16 length = body_length / 4 ;
	
	uint32 header = ((uint32)action<<30) | ((uint32)type<<16) | (uint32)length;

	msg.encode32(header);

	bytes_written = 4;
}


void 
session_auth_object::serialize(NetMsg &msg, coding_t coding, uint32 &bytes_written) const throw (IEError) {
	//cerr << "session_auth_object::serialize() ** begin **" << endl;

	bytes_written = 0;

	// Check if the object is in a valid state. Throw an exception if not.
	uint32 tmp;
	check_ser_args(coding, tmp);

	// Sort the attributelist, because easier to handle
	sort(vec_attr.begin(),vec_attr.end(), is_lower_than);


	// Check hmac conditions
	if ( is_hmac_signed() && check_hmac_cond()==false ) throw IEHmacError();
	/*
	 * Write header
	 */
	uint16 body_length = get_serialized_size(coding) - HEADER_LENGTH;

	//cerr << "session_auth_object::serialize() serialized header" << endl;
	try {
		uint32 header_bytes_written= 0;
		serialize_header(msg, coding, header_bytes_written, body_length);

		bytes_written += header_bytes_written;
	}
	catch (NetMsgError) {
		throw IEMsgTooShort(coding, get_category(), msg.get_pos());
	}


	/*
	 * Write body
	 */
	uint32 body_bytes_written= 0;
	try {
		serialize_body(msg, coding, body_bytes_written);

		bytes_written += body_bytes_written;
	}
	catch (NetMsgError) {
		throw IEError(IEError::ERROR_MSG_TOO_SHORT);
	}
	//cerr << "session_auth_object::serialize() ** end **  expected body len=" << body_length << " written body len=" << body_bytes_written << endl;

}

void 
session_auth_object::serialize_body(NetMsg &msg, coding_t coding, uint32 &bytes_written) const {

  vector<auth_attr*>::const_iterator iter;
  bytes_written= 0;
  
  uint32 attr_bytes_written= 0;
  
  for(iter= vec_attr.begin(); iter != vec_attr.end(); iter++) {
    attr_bytes_written = 0;
    if( (*iter)->get_xtype() == AUTHENTICATION_DATA ) {
    	auth_attr_data* authdata=dynamic_cast<auth_attr_data*>(*iter);
    	// if hmac is not set, insert padding 
    	if(!authdata->get_hmac()){
    		uint8* buf = new uint8[HMAC_size];
    		memset(buf,0,HMAC_size);
    		authdata->set_hmac(buf,HMAC_size);
		}
	}
    //cerr << "session_auth_object::serialize_body() - serializing attribute " << (*iter)->get_ie_name();

    (*iter)->serialize(msg,coding,attr_bytes_written);
    bytes_written += attr_bytes_written;

    //cerr << " expected length=" << (*iter)->get_serialized_size(coding) << " written=" << attr_bytes_written << " total=" << bytes_written << endl;
  }
  
}

session_auth_object *session_auth_object::new_instance() const {
	session_auth_object *qp = NULL;
	catch_bad_alloc(qp = new session_auth_object());
	return qp;
}


session_auth_object *session_auth_object::copy() const {
	session_auth_object *qp = NULL;
	catch_bad_alloc(qp = new session_auth_object(*this));
	return qp;
}

auth_attr* session_auth_object::get_attr(uint8 xtype, uint8 subtype) const {
	vector<auth_attr*>::const_iterator iter;
	auth_attr* ret = NULL;
	for(iter=vec_attr.begin();iter!=vec_attr.end(); iter++){
	  if( (*iter)->get_xtype()==xtype && (*iter)->get_subtype()==subtype )
	  {
	    ret=*iter;
	    break;
	  }
	} // end for
	return ret;
}

bool session_auth_object::equals_body(const session_auth_object &obj) const {
	if(obj.get_vec_length()!= vec_attr.size()) return false;
	vector<auth_attr*>::const_iterator iter;
    for(iter= vec_attr.begin(); iter != vec_attr.end(); iter++) {
		auth_attr *attr = obj.get_attr((*iter)->get_xtype(),(*iter)->get_subtype());
		if(attr== NULL || (*attr) != (*(*iter))) return false;
	} // end for
	return true;
}

bool session_auth_object::check() const {
	if( action < 4  && (type==known_nslp_object::SESSION_AUTH_OBJECT)) return check_body();
	else return false;
}

bool session_auth_object::check_body() const {

	// no attributes at all is not ok
	if (vec_attr.empty())
		return false;
	// check each attribute
	vector<auth_attr*>::const_iterator iter;
	for(iter= vec_attr.begin(); iter != vec_attr.end(); iter++) {
		if(!(*iter)->check())
		{
			ERRLog("session_auth_object", "check() failed for attribute " << (*iter)->get_ie_name());
			return false;
		}
	}
	return true;
}

bool session_auth_object::operator==(const IE &ie) const {
	const session_auth_object *obj = dynamic_cast<const session_auth_object*>(&ie);
	if(obj && action == obj->get_action() && type == obj->get_type()) return equals_body(*obj);
	else return false;
}

session_auth_object::~session_auth_object() {
	vector<auth_attr*>::const_iterator iter;
    for(iter= vec_attr.begin(); iter != vec_attr.end(); iter++) 
    	delete *iter;
    vec_attr.clear();
}

ostream &session_auth_object::print_attributes(ostream &os) const {
	os << "attributes={" << endl;

	vector<auth_attr*>::const_iterator iter;
    for(iter= vec_attr.begin(); iter != vec_attr.end(); iter++) 
		(*iter)->print(os, 1, 2) << endl;

	os << setw(0) << "" << "}]";

	return os;
}

ostream &session_auth_object::print(ostream &os, uint32 level, const uint32 indent, const char *name) const {
	os << setw(level*indent) << "";

	if ( name )
	  os << name << " = ";

	os << "[" << get_ie_name() << ": ";

	os << "action= ";
	switch(get_action()){
		case 0 :	os<<"mandatory";
					break; 
		case 1 :	os<<"ignore";
					break; 
		case 2 :	os<<"forward";
					break; 
		case 3 :	os<<"refresh";
					break; 
		default:	os<<"unknown (might be a bug)";
	} 
	os << ", object_type = 0x"<< std::hex << type << std::dec;

	os << ", attributes={" << setfill(' ') << endl;

	vector<auth_attr*>::const_iterator iter;
	for(iter= vec_attr.begin(); iter != vec_attr.end(); iter++) 
		(*iter)->print(os, level+1, indent) << endl;

	os << setw(level*indent) << "" << "}]";

	return os;
}

session_auth_object::session_auth_object(const session_auth_object &other)
 : known_nslp_object((known_nslp_object::type_t)other.get_type(), other.get_action()), ie_name("sessionauth object"), vec_attr(0)
{
	for(vector<auth_attr*>::const_iterator iter = other.get_vec_attr().begin();iter!=other.get_vec_attr().end();iter++)
		vec_attr.push_back((*iter)->copy());
}

bool session_auth_object::check_hmac(NetMsg &msg, coding_t coding, bool fill_data) {
	uint32 saved_pos = msg.get_pos();
	
	// fill TLP List first
	if (fill_data) { 

		uint32 pdu_len_bytes = 0;
		if(!ntlp::ntlp_pdu::decode_common_header_ntlpv1_clen(msg,pdu_len_bytes)) {
			ERRLog("session_auth_obj","Wrongly formed NTLP header (might be wrong version)");
			return false;
		}

		pdu_len_bytes += saved_pos+ntlp::ntlp_pdu::common_header_length;
		msg.fill_tlp_list(saved_pos+ntlp::ntlp_pdu::common_header_length, // start position
		                  pdu_len_bytes, // end position
		                  ntlp::ntlp_object::getLengthFromTLVHeader,
				  get_tlplist_ntlp_object_type,
		                  ntlp::ntlp_object::getTypeFromTLVHeader);
		
		TLP_list* tlp = msg.get_TLP_list();
		if (tlp) {

		   	const TLP_list::position_list_t* list = tlp->get_list(ntlp_pdu::tlplist_ntlp_object_type,known_ntlp_object::Nslpdata);
			if (list) {
				uint32 nslpdata_start_pos = *(list->begin());
				uint32 nslpdata_end_pos = nslpdata_start_pos + ntlp::ntlp_object::getLengthFromTLVHeader( msg.get_buffer() + nslpdata_start_pos);
				msg.fill_tlp_list(nslpdata_start_pos + ntlp_object::header_length + nslp_pdu::common_header_length,
						  nslpdata_end_pos,
						  ntlp::ntlp_object::getLengthFromTLVHeader,
						  get_tlplist_nslp_object_type,
						  ntlp::ntlp_object::getTypeFromTLVHeader,
						  false);
			}
			//DLog("session_auth_obj", "filled in TLP List:" << *tlp);
		} // endif tlp
	} // endif fill TLP list

	// check for presence of Session Authorization object
	const TLP_list::position_list_t *auth_list = msg.get_TLP_list()->get_list(ntlp_pdu::tlplist_nslp_object_type,nslp_auth::session_auth_object::NSLP_TYPE);
	if ( auth_list ) {
		DLog("session_auth_obj", "Checking for integrity protected objects");
		MP(benchmark_journal::PRE_HMAC_VERIFY);

		TLP_list::position_list_t::const_iterator auth_it = auth_list->begin();	
		while(auth_it != auth_list->end()) {
			uint8 *msg_digest = NULL;
			auth_attr_data* auth_data = NULL;
			if(!calc_HMAC(msg,msg_digest,auth_data,*auth_it,coding)) 
			{
				if (msg_digest == NULL && auth_data == NULL)
				{
					// ignore this session auth object since it is not of type HMAC signed
					auth_it++;
					continue;
				}

				DLog("session_auth_obj", "error occured during HMAC calculation");

				return false;
			}

			// compare hmacs
			const uint8* other_mac = auth_data->get_hmac();
	  		if( memcmp(msg_digest, other_mac, session_auth_object::HMAC_size) != 0 ) {
	  			msg.set_pos(saved_pos);
				delete msg_digest;			
				delete auth_data;

				DLog("session_auth_obj", "HMAC comparison failed");

	  			return false;
			}
			else
			  DLog("session_auth_obj", color[green] << "HMAC succesfully verified" << color[off]);

			delete msg_digest;			
			delete auth_data;
			auth_it++;
		} // end while AuthObj_type in linked list

		MP(benchmark_journal::POST_HMAC_VERIFY);

	} // list not empty

	// return to buffer position
	msg.set_pos(saved_pos);

	return true;
} 

// auth_data and msg_digest will be allocated, but not destroyed in this function
bool session_auth_object::calc_HMAC(NetMsg &msg, uint8*& msg_digest, auth_attr_data*& auth_data, uint32 auth_pos, coding_t coding) {
	//DLog("session_auth_obj","calc_HMAC() start - msglen:" << msg.get_size() << ", authpos=" << auth_pos);

	delete msg_digest;
	msg_digest = NULL;
	delete auth_data;
	auth_data = NULL;
	TLP_list* tlp = msg.get_TLP_list();
	
	auth_attr_hmac_trans_id* transId= NULL;
	auth_nslp_object_list* obj_list = NULL;
	msg.set_pos(auth_pos+2);
	uint32 auth_obj_len=msg.decode16()& 0xFFF; // in words
	uint32 obj_end_pos = auth_pos + 4*auth_obj_len + session_auth_object::HEADER_LENGTH; // in bytes

	while( msg.get_pos() < obj_end_pos) {

		uint16 tmp_len= msg.decode16(); // does not contain padding
		uint16 tmp_attr_len= round_up4(tmp_len);
		uint8 attr_xtype,attr_subtype;
		attr_xtype = msg.decode8();
		attr_subtype = msg.decode8();
		if( attr_xtype==AUTH_ENT_ID  && attr_subtype==subtype_auth_ent_hmac_signed ){
			msg.set_pos_r(-4);
			transId = new auth_attr_hmac_trans_id();
			uint32 attr_bytes_read = 0;
			IEErrorList err;
			transId->deserialize(msg, coding, err, attr_bytes_read, false);
			msg.set_pos_r(-attr_bytes_read+4);

		}
		if( attr_xtype==AUTH_NSLP_OBJECT_LIST && attr_subtype==0 ){
			msg.set_pos_r(-4);
			obj_list = new auth_nslp_object_list();
			uint32 attr_bytes_read = 0;
			IEErrorList err;
			obj_list->deserialize(msg, coding, err, attr_bytes_read, false);
			msg.set_pos_r(-attr_bytes_read+4);

		}
		if( attr_xtype==AUTHENTICATION_DATA && attr_subtype==0 ){
			msg.set_pos_r(-4);
			auth_data = new auth_attr_data();
			uint32 attr_bytes_read = 0;
			IEErrorList err;
			auth_data->deserialize(msg, coding, err, attr_bytes_read, false);
			msg.set_pos_r(-attr_bytes_read+4);
		}
		msg.set_pos_r(tmp_attr_len-4);
	}

	if (!transId) {
		DLog("ntlp_pdu::auth_hmac_check::check_hmac","not an HMAC signed session authorization object - ignoring");
		delete obj_list;
		delete auth_data;
		auth_data=NULL;
		return false;
	}

		
	// check conditions
	if( !(transId && auth_data) || (obj_list && !auth_data) ) {
		ERRCLog("ntlp_pdu::auth_hmac_check::check_hmac", "hmac conditions failed");
		delete transId;
		delete obj_list;
		delete auth_data;
		auth_data=NULL;
		return false;
	}
	
	// copy obj_types (pos and len)
	vector<pair<uint32,uint32> > hmac_obj_types(0);
	const TLP_list::position_list_t *obj_pos_list;
	TLP_list::position_list_t::const_iterator obj_entry;
	if (obj_list) {	
		vector<uint16>::const_iterator iter;
		for(iter=obj_list->get_list().begin();iter!=obj_list->get_list().end();iter++) {
			if(!(obj_pos_list=tlp->get_list(ntlp_pdu::tlplist_nslp_object_type,*iter))) {
				ERRLog("ntlp_pdu::auth_hmac_check::check_hmac","Objtype not found in PDU, but in auth_nslp_obj_list");
			} else {
				if(((*iter)!=session_auth_object::NSLP_TYPE) && ((*iter)!= ntlp::known_ntlp_object::SessionID) && ((*iter)!=ntlp::known_ntlp_object::MRI)) {
					obj_entry = obj_pos_list->begin();
					while(obj_entry!=obj_pos_list->end()){
						uint32 obj_length = ntlp::ntlp_object::getLengthFromTLVHeader(msg.get_buffer()+*obj_entry);
						hmac_obj_types.push_back(make_pair(*obj_entry,obj_length));
						obj_entry++;
					}
				}
			}
		}
	} // else there is no obj_list
			
	// even if there is no obj_list
	// have to insert MRI, SessionId, AuthObj
	// to include to hmac
	if (!(obj_pos_list=tlp->get_list(ntlp_pdu::tlplist_ntlp_object_type,ntlp::known_ntlp_object::MRI))){
		ERRLog("session_auth_object::check_hmac","MRI Object not found in PDU");
	} else {
		obj_entry = obj_pos_list->begin();
		while(obj_entry!=obj_pos_list->end()){
			uint32 obj_length = ntlp::ntlp_object::getLengthFromTLVHeader(msg.get_buffer()+*obj_entry);
			hmac_obj_types.push_back(make_pair(*obj_entry,obj_length));
			obj_entry++;
		}
	}
	if (!(obj_pos_list=tlp->get_list(ntlp_pdu::tlplist_ntlp_object_type,ntlp::known_ntlp_object::SessionID))){
		ERRLog("session_auth_object::check_hmac","SessionID Object not found in PDU");
	} else {
		obj_entry = obj_pos_list->begin();
		while(obj_entry!=obj_pos_list->end()){
			uint32 obj_length = ntlp::ntlp_object::getLengthFromTLVHeader(msg.get_buffer()+*obj_entry);
			hmac_obj_types.push_back(make_pair(*obj_entry,obj_length));
			obj_entry++;
		}
	}
	if (!(obj_pos_list=tlp->get_list(ntlp_pdu::tlplist_nslp_object_type,NSLP_TYPE))){
		ERRLog("session_auth_object::check_hmac","SessionAuth Object not found in PDU");	
	} else {
		obj_entry = obj_pos_list->begin();
		while(obj_entry!=obj_pos_list->end()){
			uint32 obj_length = ntlp::ntlp_object::getLengthFromTLVHeader(msg.get_buffer()+*obj_entry);
			hmac_obj_types.push_back(make_pair(*obj_entry, obj_length-session_auth_object::HMAC_size));
			obj_entry++;
		}
	}

	// sort obj_list (in order of msg_pos) 
	// operator<(pair,pair) is in utility defined (uses first element)
	sort(hmac_obj_types.begin(),hmac_obj_types.end());
			
	// copy to buffer
	uint8 *hmac_buffer  = new uint8[msg.get_size()];
	msg.copy_to(hmac_buffer,4,2); // copy NSLPID
  	uint32 hmac_bufferlen = 2;
  	uint32 last_pos = 0; // to find duplicated obj_types
  	vector<pair<uint32,uint32> >::iterator iter;
	for(iter=hmac_obj_types.begin();iter!=hmac_obj_types.end();iter++) {
		uint32 pos = iter->first;
		uint32 len = iter->second;
		if(pos != last_pos) {
			msg.copy_to(hmac_buffer+hmac_bufferlen,pos,len);
			hmac_bufferlen+=len;
		} // else this obj_type is already in the hmac_buffer 
		last_pos = pos;
	}
#ifdef PRINT_HMAC_BUF_LEN
    cout<<"HMAC buffer length (only used space) : "<<hmac_bufferlen<<endl;
#endif
			
	// calc hmac
  	const signkey_context* signkey_info= sign_key_registry.getKeyContext(auth_data->get_key_id());
  	if (signkey_info)
  	{
	        DLog("session_auth_obj", "got key ID 0x" << hex << auth_data->get_key_id() << dec);
		msg_digest= new unsigned char[session_auth_object::HMAC_size];
		memset(msg_digest,0,session_auth_object::HMAC_size);
		unsigned int mdlen= 0;
		// TODO: translate transform ID to SSL algo
		if ( transId->get_id() != TRANS_ID_AUTH_HMAC_SHA1_96 )
		  ERRLog("session_auth_obj", "unsupported Integrity Algorithm Transform ID, using SHA-1 anyway");
		// do the HMAC calculation
		HMAC(EVP_sha1(), signkey_info->getKey(), signkey_info->getKeyLen(), hmac_buffer, hmac_bufferlen, msg_digest, &mdlen);
  	} 
	else {
  		WLog("session_auth_obj","no key found for the corresponding keyId");
  		return false;
	}

	delete obj_list;
	delete transId;
	delete hmac_buffer;
	return true;	
}

/**
 * @para msg - Message buffer, this should point to the NTLP common header
 *
 **/
bool 
session_auth_object::serialize_hmac(NetMsg &msg, coding_t coding, bool fill_data) {
	uint32 saved_pos = msg.get_pos();
	
	if (fill_data) {
		uint32 pdu_len_bytes = 0;
		if(!ntlp::ntlp_pdu::decode_common_header_ntlpv1_clen(msg,pdu_len_bytes)) {
			ERRLog("session_auth_obj","Wrongly formed header (might be wrong version)");
			return false;
		}
		pdu_len_bytes += ntlp::ntlp_pdu::common_header_length;
		msg.fill_tlp_list(ntlp::ntlp_pdu::common_header_length,
	                      pdu_len_bytes,
	                      ntlp::ntlp_object::getLengthFromTLVHeader,
	                      get_tlplist_ntlp_object_type,
	                      ntlp::ntlp_object::getTypeFromTLVHeader);
	    TLP_list* tlp = msg.get_TLP_list();
	    if (tlp) {
	    	const TLP_list::position_list_t* list = tlp->get_list(ntlp_pdu::tlplist_ntlp_object_type,known_ntlp_object::Nslpdata);
		if(list) {
	    		uint32 nslpdata_start_pos = *(list->begin());
	    		uint32 nslpdata_end_pos = nslpdata_start_pos + ntlp::ntlp_object::getLengthFromTLVHeader( msg.get_buffer() + nslpdata_start_pos);
				msg.fill_tlp_list(nslpdata_start_pos + ntlp_object::header_length + nslp_pdu::common_header_length,
				                  nslpdata_end_pos,
				                  ntlp::ntlp_object::getLengthFromTLVHeader,
	                      		  get_tlplist_nslp_object_type,
	                      		  ntlp::ntlp_object::getTypeFromTLVHeader,
	                      		  false);	    		
		}
		//DLog("session_auth_obj", "filled in TLP List:" << *tlp);
	    }
	}
	
//	std::cout<<*msg.get_TLP_list();
	
	const TLP_list::position_list_t *sauth_objects = NULL;
	if ( (sauth_objects= msg.get_TLP_list()->get_list(ntlp_pdu::tlplist_nslp_object_type,nslp_auth::session_auth_object::NSLP_TYPE)) != NULL )
	{
		MP(benchmark_journal::PRE_HMAC_CREATE);

		TLP_list::position_list_t::const_iterator it = sauth_objects->begin();
		while (it != sauth_objects->end()) {
			uint8 *msg_digest = NULL;
			auth_attr_data* auth_data = NULL;
			if(!calc_HMAC(msg,msg_digest,auth_data,*it,coding))
			{
				if (msg_digest == NULL && auth_data == NULL)
				{
					// ignore this session auth object since it is not of type HMAC signed
					it++;
					continue;
				}

				// HMAC calculation failed
				DLog("session_auth_obj", "in serialize_hmac() - error occured during HMAC calculation");

				return false;
			}
			
			uint32 obj_length = ntlp::ntlp_object::getLengthFromTLVHeader(msg.get_buffer()+*it);
			uint32 HMAC_pos = *it + obj_length - nslp_auth::session_auth_object::HMAC_size;
			msg.copy_from(msg_digest, HMAC_pos , nslp_auth::session_auth_object::HMAC_size);

			DLog("session_auth_obj", "serialize_hmac() - calculated HMAC successfully");
			
			delete msg_digest;			
			delete auth_data;
			it++;
		} // end while AuthObj_type in linked list
	    
	    MP(benchmark_journal::POST_HMAC_CREATE);
	    
	} // else there is no Auth_Obj
	msg.set_pos(saved_pos);	
	return true;
}

uint16 session_auth_object::get_tlplist_nslp_object_type(const uint8* buf) {
	return ntlp_pdu::tlplist_nslp_object_type;
}

uint16 session_auth_object::get_tlplist_ntlp_object_type(const uint8* buf) {
	return ntlp_pdu::tlplist_ntlp_object_type;
} 	

} // end namespace
