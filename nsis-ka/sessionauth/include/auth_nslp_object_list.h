#ifndef AUTH_NSLP_OBJECT_LIST_H
#define AUTH_NSLP_OBJECT_LIST_H


#include "auth_attr.h"
#include <vector>
#include <iterator>

namespace nslp_auth{
	using namespace protlib;

class auth_nslp_object_list : public auth_attr {

public:
	auth_nslp_object_list();
	inline auth_nslp_object_list(uint8 xtype, uint8 subtype);
//	auth_nslp_object_list(uint8 xtype, uint8 subtype);

	virtual ~auth_nslp_object_list();
	virtual auth_nslp_object_list *new_instance() const;
	virtual auth_nslp_object_list *copy() const;

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
	bool isObjTypeIn(uint16 p)const;

// New Methods:
	const vector<uint16>& get_list() const { return obj_type_list; }
	uint16 getListSize() const { return obj_type_list.size(); }
	void insert(uint16 p);
	void clear_list(){ obj_type_list.clear();}
private:
 	const char * ie_name;
 	vector<uint16> obj_type_list; 

};
/*
*Consturuktor
*/
inline
auth_nslp_object_list::auth_nslp_object_list():auth_attr(AUTH_NSLP_OBJECT_LIST, 0), ie_name("NSLP Object List") {
}

}//end namespace auth
#endif
