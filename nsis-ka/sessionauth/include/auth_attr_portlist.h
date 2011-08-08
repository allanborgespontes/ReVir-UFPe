#ifndef AUTH_ATTR_IP_PORTLIST_H
#define AUTH_ATTR_IP_PORTLIST_H

#include "auth_attr.h"
#include "auth_attr_addr.h"

#include <vector>
#include <iterator>
namespace nslp_auth{
	using namespace protlib;


class auth_attr_portlist : public auth_attr{
public:
	auth_attr_portlist();
	auth_attr_portlist(uint8 xtype,uint8 subtype);
private:
	virtual ~auth_attr_portlist();
	virtual auth_attr_portlist *new_instance() const;
	virtual auth_attr_portlist *copy() const;

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

// Disallow assignment for now.
	const char * ie_name;
	vector<port_t> list;
public:	
	const vector<port_t>& get_list() const;
	uint16 getListSize() const;
	void insert(uint16 p);
	bool isPortIn(uint16 p)const;

};
/*
*Constuructor
*/
inline
auth_attr_portlist:: auth_attr_portlist(): auth_attr(SOURCE_ADDR, UDP_PORT_LIST), ie_name("UDP_PORT_LIST"){
}
}//end namespace auth
#endif
