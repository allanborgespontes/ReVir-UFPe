#include "auth_attr_hmac_trans_id.h"
#include "logfile.h"



namespace nslp_auth {



auth_attr_hmac_trans_id::auth_attr_hmac_trans_id(uint16 id) : auth_attr(AUTH_ENT_ID, HMAC_SIGNED),ie_name("HMAC transform ID"),transId(id) {
}

/*
* Destructor
*/
auth_attr_hmac_trans_id::~auth_attr_hmac_trans_id(){
}

auth_attr_hmac_trans_id *auth_attr_hmac_trans_id::new_instance() const {
	auth_attr_hmac_trans_id *a = NULL;
	catch_bad_alloc( a = new auth_attr_hmac_trans_id() );
	return a;
}

auth_attr_hmac_trans_id *auth_attr_hmac_trans_id::copy() const {
	auth_attr_hmac_trans_id *a = NULL;
	catch_bad_alloc( a = new auth_attr_hmac_trans_id(*this) );
	return a;
}

size_t auth_attr_hmac_trans_id::get_serialized_size(coding_t coding) const{
	return 8;
}

size_t auth_attr_hmac_trans_id::get_serialized_size_nopadding(coding_t coding) const{
	return 8;
}

bool auth_attr_hmac_trans_id::deserialize_body(NetMsg &msg, coding_t coding, uint16 body_length, IEErrorList &err, uint32 &bytes_read, bool skip) {

	uint32 start_pos = msg.get_pos();
	
// Error: Message is shorter than the length field makes us believe.
	if ( msg.get_bytes_left() < body_length ) {
		catch_bad_alloc( err.put(new IEMsgTooShort(coding, category, start_pos)) );
 		msg.set_pos(start_pos);
		return false;
	}
	
	// skip three reserved/padding bytes
	msg.set_pos_r(2);
	transId = msg.decode16();
	bytes_read+= 4;
	
	// Error: We expected the length field to be different.
	if ( round_up4(body_length + HEADER_LENGTH) !=(uint32) get_serialized_size(coding) || body_length!= bytes_read) {

		catch_bad_alloc( err.put( new IEWrongLength(coding, get_category(), get_xtype(), get_subtype(), msg.get_pos())) );
		msg.set_pos(start_pos);
		return false;
	}
	return true;
}

void auth_attr_hmac_trans_id::serialize_body(NetMsg &msg, coding_t coding, uint32 &bytes_written) const {
 	msg.padding(2);
 	msg.encode16(transId);
 	bytes_written = 4;
}

// Output for IPv4- or IPv6- Address
ostream &auth_attr_hmac_trans_id::print_attributes(ostream &os) const {
	  	return os << "HMAC transform ID:" << (unsigned int)transId; 
}

bool auth_attr_hmac_trans_id::equals_body(const auth_attr &attr) const {

	const auth_attr_hmac_trans_id *other = dynamic_cast<const auth_attr_hmac_trans_id*>(&attr);

	return other != NULL && transId == other->get_id();
}


void auth_attr_hmac_trans_id::set_id(uint16 id){
 	transId=id; 
}

uint16 auth_attr_hmac_trans_id::get_id() const{
  return transId;
}

ostream &auth_attr_hmac_trans_id::print(ostream &os, uint32 level, const uint32 indent, const char *name ) const {
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

bool auth_attr_hmac_trans_id::check() const {
	return check_body();
}

bool auth_attr_hmac_trans_id::check_body() const {
	return true;
}

}

