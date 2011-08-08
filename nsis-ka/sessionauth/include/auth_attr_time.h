#ifndef AUTH_ATTR_TIME_H
#define AUTH_ATTR_TIME_H


#include <time.h>
#include "protlib_types.h"
#include "auth_attr.h"


namespace nslp_auth{
	using namespace protlib;


enum subtype_time_t{

	 NTP_TIMESTAMP	= 	1
};

class auth_attr_time : public auth_attr {

public:
	inline auth_attr_time();
	inline auth_attr_time(uint8 xtype, uint8 subtype);
	auth_attr_time(uint8 xtype, uint8 subtype, time_t time);
	explicit auth_attr_time(const auth_attr_time& other);

	virtual ~auth_attr_time();
	virtual auth_attr_time *new_instance() const;
	virtual auth_attr_time *copy() const;

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
	time_t timestamp;
 	const char * ie_name; 
public:
	//New Methods:
	void set_value(time_t val);
	time_t get_value() const { return timestamp; }

};
/*
*Consturuktor
*/
inline
auth_attr_time::auth_attr_time():auth_attr(START_TIME, NTP_TIMESTAMP), timestamp(time(NULL)), ie_name("NTP_TIMESTAMP_FOR_START"){
}

inline
auth_attr_time::auth_attr_time(uint8 xtype, uint8 subtype):auth_attr(xtype, subtype), timestamp(time(NULL)), ie_name
(xtype==START_TIME?"NTP_TIMESTAMP_FOR_START":"NTP_TIMESTAMP_FOR_END"){
}

}//end namespace auth
#endif
