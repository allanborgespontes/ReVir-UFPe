#ifndef SESSION_AUTH_OBJECT_H
#define SESSION_AUTH_OBJECT_H

#include <vector>
#include <ios>
#include "auth_attr.h"
#include "logfile.h"
#include "ntlp_object.h"
#include "nslp_object.h"
#include "auth_attr.h"
#include "auth_attr_ie.h"



namespace nslp_auth {
using namespace protlib;
using namespace qos_nslp;
  
/*
 * AUTH Object Types
 * Attribute
 */
class session_auth_object : public known_nslp_object {
public:
	static const uint16 HEADER_LENGTH;
	static const uint16 NSLP_TYPE;
	static const size_t HMAC_size;
	static const uint16 TRANS_ID_AUTH_HMAC_SHA1_96;

	inline session_auth_object();
	inline session_auth_object(NSLP_IE::action_type_t action, known_nslp_object::type_t SESSION_AUTH_OBJECT);
	explicit session_auth_object(const session_auth_object &other);
	
	virtual ~session_auth_object();

	/*
	 * Inherited from IE
	 */
	virtual session_auth_object *new_instance() const;
	virtual session_auth_object *copy() const;

	virtual IE *deserialize(NetMsg &msg, coding_t coding, IEErrorList &err, uint32 &bytes_read, bool skip);

	virtual void serialize(NetMsg &msg, coding_t coding, uint32 &bytes_written) const throw (IEError);

	virtual bool supports_coding(coding_t c) const { return c == protocol_v1; }
	virtual void register_ie(IEManager *iem) const { iem->register_ie(NSLP_IE::cat_known_nslp_object,known_nslp_object::SESSION_AUTH_OBJECT,0,this); }

 	virtual bool operator==(const IE &ie) const;
	virtual const char *get_ie_name() const { return ie_name; }


	virtual bool deserialize_header(NetMsg &msg, coding_t coding, uint16 &body_length, IEErrorList &err, uint32 &bytes_read, bool skip);

	virtual void serialize_header(NetMsg &msg, coding_t coding, uint32 &bytes_written, uint16 body_len) const;

	virtual ostream &print_attributes(ostream &os) const;
	virtual ostream &print(ostream &os, uint32 level, const uint32 indent, const char *name = NULL) const;

	virtual bool check() const;
	virtual bool check_body() const;
	virtual bool equals_body( const session_auth_object &obj) const;

	virtual size_t get_serialized_size(coding_t coding) const;
	
	virtual bool deserialize_body(NetMsg &msg, coding_t coding, uint16 body_length, IEErrorList &err, uint32 &bytes_read, bool skip);

	virtual void serialize_body(NetMsg &msg, coding_t coding, uint32 &bytes_written) const;

	bool is_hmac_signed() const;
	bool check_hmac_cond() const;
	
	action_type_t get_action() const { return action;}

  private:
   	const char* ie_name;
	mutable vector<auth_attr*> vec_attr;

  public:	
   
	uint16 extract_length(uint32 header_raw) throw ();
	bool is_mandatory() const;
	bool is_ignore() const;
	bool is_forward() const;
	bool is_refresh() const;
	auth_attr* get_attr(uint8 xtype, uint8 subtype) const;
	inline size_t get_vec_length() const { return vec_attr.size(); }
	void insert_attr(auth_attr *attr); 
	auth_attr* remove_attr(uint8 xtype, uint8 subtype);	
	void set_mandatory();
	void set_ignore();
	void set_forward();
	void set_refresh();
	const vector<auth_attr*>& get_vec_attr() const { return vec_attr; }

// help ntlp to serialize and check hmac
  public:
  	static bool check_hmac(NetMsg &msg, coding_t coding, bool fill_data=true);
  	static bool serialize_hmac(NetMsg &msg, coding_t coding, bool fill_data=false);
  	static uint16 get_tlplist_nslp_object_type(const uint8* buf);
  	static uint16 get_tlplist_ntlp_object_type(const uint8* buf);  	
  private:
  	static bool calc_HMAC(NetMsg &msg, uint8*& msg_digest, nslp_auth::auth_attr_data*& auth_data, uint32 auth_pos, coding_t coding);


};


/*
 * Standard Constructor
 */
inline
session_auth_object::session_auth_object():
  known_nslp_object(known_nslp_object::SESSION_AUTH_OBJECT, NSLP_IE::forward), ie_name("session authentication object"), vec_attr(0)
{
}

inline
session_auth_object::session_auth_object(NSLP_IE::action_type_t action, known_nslp_object::type_t obj_type):
  known_nslp_object(obj_type, action), ie_name("session authentication object"), vec_attr(0)
{
}

} // end namespace

#endif //SESSION_AUTH_ATTR_H
