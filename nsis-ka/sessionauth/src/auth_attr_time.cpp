#include "logfile.h"

#include "auth_attr.h"
#include "auth_attr_time.h"
#include "ie.h"


namespace nslp_auth {

using namespace protlib::log;


auth_attr_time::auth_attr_time(uint8 xtype, uint8 subtype, time_t time) :
 auth_attr(xtype, subtype), timestamp(time), ie_name
(xtype==START_TIME?"NTP_TIMESTAMP_FOR_START":"NTP_TIMESTAMP_FOR_END"){
}

/*
*Destruktor
*/
auth_attr_time::~auth_attr_time(){
}

//create a new instance
auth_attr_time *auth_attr_time::new_instance() const {
	auth_attr_time *a = NULL;
	catch_bad_alloc( a = new auth_attr_time() );
	return a;
}

bool auth_attr_time::deserialize_body(NetMsg &msg, coding_t coding, uint16 body_length, IEErrorList &err, uint32 &bytes_read, bool skip) {

	bytes_read = 0;	
	uint32 start_pos = msg.get_pos();

	// Error: Message is shorter than the length field makes us believe.
	if ( msg.get_bytes_left() < body_length ) {
		catch_bad_alloc( err.put(new IEMsgTooShort(coding, category, start_pos)) );
 		msg.set_pos(start_pos);
		return false;
	}

	timestamp = (time_t) msg.decode64();
	bytes_read = 8;

	// Error: We expected the length field to be different.
	if ( (body_length + HEADER_LENGTH) !=(uint32) get_serialized_size_nopadding(coding) || body_length != bytes_read) {
		catch_bad_alloc( err.put( new IEWrongLength(coding, get_category(), get_xtype(), get_subtype(), msg.get_pos())) );
		msg.set_pos(start_pos);
		return false;
	}
	
	return true;
}

size_t auth_attr_time::get_serialized_size(coding_t coding) const{
	return 12;
}

size_t auth_attr_time::get_serialized_size_nopadding(coding_t coding) const{
	return 12;
}

void auth_attr_time::serialize_body(NetMsg &msg, coding_t coding, uint32 &bytes_written) const {
		msg.encode64((uint64)timestamp);
		bytes_written = 8;
}
		
void auth_attr_time::set_value(time_t val){
	timestamp=val;
}	

bool auth_attr_time::equals_body(const auth_attr &attr) const {
  const auth_attr_time *other = dynamic_cast<const auth_attr_time*>(&attr);

  if( other == NULL )  return false;

  return timestamp == other->get_value();
	
}

ostream &auth_attr_time::print_attributes(ostream &os) const {
	os << "time: "<<timestamp;
	return os; 
}

ostream &auth_attr_time::print(ostream &os, uint32 level, const uint32 indent, const char *name ) const {
	os << setw(level*indent) << "";

	if ( name )
	  os << name <<  " = ";

	os << "[" << ie_name << ": xtype = "<<(int)x_type<<", subtype = "<<(int)sub_type<<", attribute_body : { \n";
	os << setw((level+1)*indent) << "";

	print_attributes(os);
	
	os <<"\n"<<setw(level*indent) << "";	 
	os << "}]";

	return os;	
}

bool auth_attr_time::check() const {
	return check_body();
}

bool auth_attr_time::check_body() const {
	if ( timestamp >= time(NULL) ) return true;
	else return false;
}

auth_attr_time *auth_attr_time::copy() const {
	auth_attr_time *qp = NULL;
	catch_bad_alloc(qp = new auth_attr_time(*this));
	return qp;
}

auth_attr_time::auth_attr_time(const auth_attr_time& other) : auth_attr(other.get_xtype(), other.get_subtype()), timestamp(other.get_value()),
ie_name(other.get_xtype()==START_TIME?"NTP_TIMESTAMP_FOR_START":"NTP_TIMESTAMP_FOR_END"){
}

}

