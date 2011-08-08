#ifndef AUTH_ATTR_DATA_H
#define AUTH_ATTR_DATA_H

#include "auth_attr.h"
#include <string.h>


namespace nslp_auth {
	using namespace protlib;



class auth_attr_data : public auth_attr {

public:
	inline auth_attr_data();
	auth_attr_data(uint32 keyid);
	explicit auth_attr_data(const auth_attr_data& other);
	
	virtual ~auth_attr_data() { delete[] hmac; }
	virtual auth_attr_data *new_instance() const;
	virtual auth_attr_data *copy() const;

	virtual bool check() const;
	virtual bool check_body() const;
	virtual bool equals_body(const auth_attr &other) const;
	virtual const char *get_ie_name() const { return ie_name; }
	virtual ostream &print_attributes(ostream &os) const;
	virtual ostream &print(ostream &os, uint32 level, const uint32 indent, const char *name) const;
	virtual size_t get_serialized_size(coding_t coding) const;
	virtual size_t get_serialized_size_nopadding(coding_t coding) const;

	virtual bool deserialize_body(NetMsg &msg, coding_t coding, uint16 body_length, IEErrorList &err, uint32 &bytes_read, bool skip);

	virtual void serialize_body(NetMsg &msg, coding_t coding, uint32 &bytes_written) const;


private:
	static const size_t HMAC_size;
	const char * ie_name;
	uint32 key_id;
	// point to HMAC DATA (Message digest or Message Integrity Check)
	uint8* hmac;
	// length of HMAC data in number of bytes
	uint16 hmac_len;
public:
	/*
	 * New methods
	 */
	uint32 get_key_id() const { return key_id; }
	const uint8* get_hmac() const { return hmac; }
	uint16 get_hmac_len() const { return hmac_len; }
	void set_key_id(uint32 new_key_id) { key_id = new_key_id; }
	void set_hmac(uint8* h, uint16 length) {  delete[] hmac; hmac_len= length; hmac = h;   }
	void set_hmac(const uint8* hmac_src, uint16 length) {  delete[] hmac; hmac_len= length; hmac= new uchar[hmac_len]; memcpy(hmac, hmac_src, hmac_len); }
	// these are just alias names for transparent auth data
	void set_auth_data(uint8* data, uint16 length) { set_hmac(data,length); }
	void set_auth_data(const uint8* data, uint16 length) { set_hmac(data,length); }
	uint16 get_auth_data_len() { return get_hmac_len(); }
	const uint8* get_auth_data() const { return get_hmac(); }
};

/**
 * Constructors
 **/
inline
auth_attr_data::auth_attr_data() : auth_attr(AUTHENTICATION_DATA, 0), ie_name("AUTHENTICATION_DATA"), key_id(0), hmac(NULL), hmac_len(HMAC_size) {
	hmac= new uchar[hmac_len]; memset(hmac,0,hmac_len);
}

inline
auth_attr_data::auth_attr_data(uint32 keyid) : auth_attr(AUTHENTICATION_DATA, 0), ie_name("AUTHENTICATION_DATA"), key_id(keyid), hmac(NULL), hmac_len(HMAC_size) {
	hmac= new uchar[hmac_len]; memset(hmac,0,hmac_len);
}

} // end namespace auth

#endif
