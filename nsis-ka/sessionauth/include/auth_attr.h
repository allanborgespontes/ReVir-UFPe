#ifndef SESSION_AUTH_ATTR_H
#define SESSION_AUTH_ATTR_H

#include "ie.h"
#include <iomanip>
#include <iostream>

namespace nslp_auth {
  using namespace protlib;
  using namespace std;

enum category {
	cat_auth_obj			=	0,	
	cat_auth_attr			= 	1
};

enum xtype_t {
	AUTH_ENT_ID 		= 	1,
	SOURCE_ADDR		=	2,
	DEST_ADDR		=	3,
	START_TIME 		=	4,
	END_TIME		=	5,
	AUTH_NSLP_OBJECT_LIST	=	6,
	AUTHENTICATION_DATA	=	7
};

/*
 * AUTH Object Types
 * Attribute
 */
class auth_attr : public IE {
  public:
	static const uint16 HEADER_LENGTH;
	
	auth_attr();
	auth_attr(uint8 x_type, uint8 sub_type);
	
	virtual ~auth_attr() {};

	/*
	 * Inherited from IE
	 */
	virtual auth_attr *new_instance() const = 0;
	virtual auth_attr *copy() const = 0;

	virtual const char *get_ie_name() const = 0;

	virtual IE *deserialize(NetMsg &msg, coding_t coding, IEErrorList &err, uint32 &bytes_read, bool skip);

	virtual void serialize(NetMsg &msg, coding_t coding, uint32 &bytes_written) const throw (IEError);
	
	virtual bool operator==(const IE &ie) const;
	virtual bool check() const;
	virtual size_t get_serialized_size(coding_t coding) const = 0;
	virtual size_t get_serialized_size_nopadding(coding_t coding) const = 0;
	virtual bool supports_coding(coding_t c) const { return c == protocol_v1; }
	virtual void register_ie(IEManager *iem) const { iem->register_ie(cat_auth_attr, x_type, sub_type, this); }
	
	virtual bool equals_body(const auth_attr &attr) const = 0;
	virtual ostream &print(ostream &os, uint32 level, const uint32 indent, const char *name = NULL) const = 0;
 
 protected:
	virtual bool deserialize_header(NetMsg &msg, coding_t coding, uint16 &body_length, IEErrorList &err, uint32 &bytes_read, bool skip);

	virtual void serialize_header(NetMsg &msg, coding_t coding, uint32 &bytes_written, uint16 body_len) const;


	virtual bool check_body() const = 0;
	
	virtual bool deserialize_body(NetMsg &msg, coding_t coding, uint16 body_length, IEErrorList &err, uint32 &bytes_read, bool skip) = 0;

	virtual void serialize_body(NetMsg &msg, coding_t coding, uint32 &bytes_written) const = 0;

private:
	
	
/*
 * New methods
 */
	

 public:
	static uint8 extract_xtype(uint32 header_raw);
	static uint8 extract_subtype(uint32 header_raw);
	static uint16 extract_length(uint32 header_raw); 
	
	uint8 get_xtype() const { return x_type; }
	uint8 get_subtype() const { return sub_type; }
	
	void set_xtype(uint8 id) { x_type= id; }
	void set_subtype(uint8 id) { sub_type= id; }
	
 protected:
	uint8 x_type;
	uint8 sub_type;	

};

/*
* Constructor
*/
inline 
auth_attr::auth_attr(uint8 x_type, uint8 sub_type): IE(cat_auth_attr), x_type(x_type), sub_type(sub_type){
}

/**
 * Extract the x_type.
 *
 * Extract the xtype from a header. The header is expected to be
 * in host byte order already.
 *
 * @param header_raw 32 bits read from a NetMsg
 * @return the x_type
 */
inline uint8 auth_attr::extract_xtype(uint32 header_raw) {
	return (uint8)( (header_raw >> 8) & 0xFF );
}

inline uint8 auth_attr::extract_subtype(uint32 header_raw) {
	return (uint8)( (header_raw ) & 0xFF );
}

inline uint16 auth_attr::extract_length(uint32 header_raw) {
  	return (uint8)((header_raw >>16) & 0xFFFF);
}


} // end namespace

#endif //SESSION_AUTH_ATTR_H
