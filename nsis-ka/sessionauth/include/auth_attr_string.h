#ifndef AUTH_ATTR_STRING_H
#define AUTH_ATTR_STRING_H

#include "auth_attr.h"
#include <string>



namespace nslp_auth{
	using namespace protlib;
	using namespace std;

enum subtype_ent_id_t {
  	FQDN	 	= 3,
 	ASCII_DN 	= 4,
	UNICODE_DN	= 5,
	URI		= 6,
	KRB_PRINCIPAL	= 7,
	X509_V3_CERT	= 8,
	PGP_CERT	= 9
};

class auth_attr_string : public auth_attr {

public:
	auth_attr_string();
	auth_attr_string(uint8 xtype, uint8 subtype);
	explicit auth_attr_string(const auth_attr_string& other);
	virtual ~auth_attr_string();
	virtual auth_attr_string *new_instance() const;
	virtual auth_attr_string *copy() const;

	virtual bool check() const;
	virtual bool check_body() const;
	virtual bool equals_body(const auth_attr &attr) const;
	virtual const char *get_ie_name() const { return ie_name; }
	virtual ostream &print_attributes(ostream &os) const;
 	virtual ostream &print(ostream &os, uint32 level, const uint32 indent, const char *name) const;
	
 	virtual size_t get_serialized_size(coding_t coding) const;
 	virtual size_t get_serialized_size_nopadding(coding_t coding) const;

	virtual bool deserialize_body(NetMsg &msg, coding_t coding, uint16 body_length, IEErrorList &err, uint32 &bytes_read, bool skip);

	virtual void serialize_body(NetMsg &msg, coding_t coding, uint32 &bytes_written) const;

private:
// Disallow assignment for now.
	const char * ie_name;
	uchar * c_u;
	uint32 clen;
public:
	/*
	 * New methods
	 */
	const uchar* get_string() const;
	void set_string(uchar *c, uint32 len);
	void set_string(const std::string& entity_id_str);
	uint32 get_length() const;
};

inline 
/*
*Constructor
*/
auth_attr_string::auth_attr_string() : auth_attr(AUTH_ENT_ID , FQDN), ie_name ("FQDN"), c_u(NULL), clen(0) {
}

}//end namespace auth
#endif
