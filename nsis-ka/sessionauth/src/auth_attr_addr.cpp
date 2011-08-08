#include "auth_attr_addr.h"
#include "logfile.h"



namespace nslp_auth {



auth_attr_addr::auth_attr_addr(uint8 xtype, uint8 subtype) : auth_attr(xtype, subtype), ie_name(), ip() {
	ie_name.clear();
	switch(xtype) {
		case AUTH_ENT_ID :	ie_name.append("AUTH_ENT_ID");
							break;
		case SOURCE_ADDR :	ie_name.append("SOURCE_ADDR");
							break;
		case DEST_ADDR : 	ie_name.append("DEST_ADDR");
							break;
		default: ie_name.append("UNKNOWN");
	}
	if(subtype == IPV4_ADDRESS) ie_name.append(" IPv4");
	else ie_name.append(" IPv6");
}

/*
*Destruktor
*/
auth_attr_addr::~auth_attr_addr(){
}

auth_attr_addr *auth_attr_addr::new_instance() const {
	auth_attr_addr *a = NULL;
	catch_bad_alloc( a = new auth_attr_addr() );
	return a;
}


auth_attr_addr *auth_attr_addr::copy() const {
	auth_attr_addr *a = NULL;
	catch_bad_alloc( a = new auth_attr_addr(*this) );
	return a;
}

bool auth_attr_addr::deserialize_body(NetMsg &msg, coding_t coding, uint16 body_length, IEErrorList &err, uint32 &bytes_read, bool skip) {

	uint32 start_pos = msg.get_pos();
	
	// Error: Message is shorter than the length field makes us believe.
	if ( msg.get_bytes_left() < body_length ) {
		catch_bad_alloc( err.put(new IEMsgTooShort(coding, category, start_pos)) );
 		msg.set_pos(start_pos);
		return false;
	}

	if(sub_type==IPV4_ADDRESS){
		struct in_addr tmp_ip;
		msg.decode(tmp_ip);
        ip.set_ip(tmp_ip);
		bytes_read = 4;
	}else{
		struct in6_addr tmp_ip;
		msg.decode(tmp_ip);
		ip.set_ip(tmp_ip);
		bytes_read = 16;
	}
	
	// Error: We expected the length field to be different.
	if ( (body_length + HEADER_LENGTH) !=(uint32) get_serialized_size_nopadding(coding) || body_length!= bytes_read) {

		catch_bad_alloc( err.put( new IEWrongLength(coding, get_category(), get_xtype(), get_subtype(), msg.get_pos())) );
		msg.set_pos(start_pos);
		return false;
	}
	
	return true;
}

void auth_attr_addr::serialize_body(NetMsg &msg, coding_t coding, uint32 &bytes_written) const {
 	if( ip.is_ipv4() ){
		struct in_addr tmp_ip;
		if(ip.get_ip(tmp_ip)){
			msg.encode(tmp_ip );		
			bytes_written = 4;
		}
	}else{
		struct in6_addr tmp_ip;
		if(ip.get_ip(tmp_ip)){
			msg.encode(tmp_ip);
			bytes_written = 16;
		}
	}
	
}

// Output for IPv4- or IPv6- Addresse
ostream &auth_attr_addr::print_attributes(ostream &os) const {
	if( ip.is_ipv4() ){
	  	return os << "ipv4 Address: " << ip.get_ip_str(); 
	}else{
		return os << "ipv6 Address: " << ip.get_ip_str(); 
	}
}

ostream &auth_attr_addr::print(ostream &os, uint32 level, const uint32 indent, const char *name ) const {
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


bool auth_attr_addr::equals_body(const auth_attr &attr) const {

	const auth_attr_addr *other = dynamic_cast<const auth_attr_addr*>(&attr);

	return other != NULL && ip == other->get_ip();
}

size_t auth_attr_addr::get_serialized_size(coding_t coding) const{
	if(ip.is_ipv4()) return 8;
	else return 20;
}

size_t auth_attr_addr::get_serialized_size_nopadding(coding_t coding) const{
	if(ip.is_ipv4()) return 8;
	else return 20;
}

const hostaddress& auth_attr_addr::get_ip() const {
  return ip;
}

void auth_attr_addr::set_ip(struct in_addr &ip_adr){
  ip.set_ip(ip_adr);
  sub_type=IPV4_ADDRESS;
  ie_name="IPv4 Address";
}

void auth_attr_addr::set_ip(struct in6_addr &ip_adr){
  ip.set_ip(ip_adr);
  sub_type=IPV6_ADDRESS;
  ie_name="IPv6 Address";
}

void auth_attr_addr::set_ip(const hostaddress &ip_adr){
  ip.set_ip(ip_adr);
  if(ip.is_ipv4()) {
    sub_type=IPV4_ADDRESS;
    ie_name="IPv4 Address";
  } else {
    sub_type=IPV6_ADDRESS;
    ie_name="IPv6 Address";
  }
}

bool auth_attr_addr::check_body() const{
	return !(ip.is_ip_unspec());
}

auth_attr_addr::auth_attr_addr(const auth_attr_addr& other)
 : auth_attr(other.get_xtype(), other.get_subtype()),ie_name(other.get_subtype() == IPV4_ADDRESS?"IPv4 Address":"IPv6 Address"),ip(other.get_ip())
{	
}

}
