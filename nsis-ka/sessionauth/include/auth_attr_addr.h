#ifndef AUTH_ATTR_ADDR_H
#define AUTH_ATTR_ADDR_H

#include "auth_attr.h"
#include <address.h>


namespace nslp_auth{
	using namespace protlib;


enum subtype_adr_t {
  	IPV4_ADDRESS 		=	1,
 	IPV6_ADDRESS 		=   2,
	UDP_PORT_LIST		=	3,
	TCP_PORT_LIST		=	4,
	SPI					=	5
};

class auth_attr_addr : public auth_attr {

public:
	auth_attr_addr();
	auth_attr_addr(uint8 xtype, uint8 subtype);
	explicit auth_attr_addr(const auth_attr_addr& other);
	
	virtual ~auth_attr_addr();
	virtual auth_attr_addr *new_instance() const;
	virtual auth_attr_addr *copy() const;

	virtual bool check_body() const;
	virtual bool equals_body(const auth_attr &other) const;
	virtual const char *get_ie_name() const { return ie_name.c_str(); }
	virtual ostream &print_attributes(ostream &os) const;
	virtual ostream &print(ostream &os, uint32 level, const uint32 indent, const char *name = NULL) const;
	
	virtual size_t get_serialized_size(coding_t coding) const;
	virtual size_t get_serialized_size_nopadding(coding_t coding) const;

	virtual bool deserialize_body(NetMsg &msg, coding_t coding, uint16 body_length, IEErrorList &err, uint32 &bytes_read, bool skip);

	virtual void serialize_body(NetMsg &msg, coding_t coding, uint32 &bytes_written) const;

private:
	string ie_name;
	hostaddress ip;

public:
	/*
	 * New methods
	 */
	const hostaddress& get_ip() const;
	void set_ip(struct in_addr& ip);
	void set_ip(struct in6_addr& ip);
	void set_ip(const hostaddress& ip);

};


/*
 * Constructor
 */
inline
auth_attr_addr::auth_attr_addr() : auth_attr(SOURCE_ADDR, IPV4_ADDRESS),ie_name("SOURCE_ADDR IPv4"),ip() {
}

} //end namespace auth
#endif
