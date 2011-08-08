#include "auth_attr_addr_spi.h"
#include "logfile.h"



namespace nslp_auth {


const char *const auth_attr_addr_spi::ie_name = "SPI ADDRESS";


auth_attr_addr_spi::auth_attr_addr_spi(uint8 xtype, uint8 subtype) : auth_attr(xtype, subtype),spi(0){	
}

/*
*Destruktor
*/
auth_attr_addr_spi::~auth_attr_addr_spi(){
}

auth_attr_addr_spi *auth_attr_addr_spi::new_instance() const {
	auth_attr_addr_spi *a = NULL;
	catch_bad_alloc( a = new auth_attr_addr_spi() );
	return a;
}


auth_attr_addr_spi *auth_attr_addr_spi::copy() const {
	auth_attr_addr_spi *a = NULL;
	catch_bad_alloc( a = new auth_attr_addr_spi(*this) );
	return a;
}

bool auth_attr_addr_spi::deserialize_body(NetMsg &msg, coding_t coding, uint16 body_length, IEErrorList &err, uint32 &bytes_read, bool skip) {
	uint32 start_pos = msg.get_pos();
	bytes_read = 0;
	
	// Error: Message is shorter than the length field makes us believe.
	if ( msg.get_bytes_left() < body_length ) {
		catch_bad_alloc( err.put(new IEMsgTooShort(coding, category, start_pos)) );
 		msg.set_pos(start_pos);
		return false;
	}
	
	spi = msg.decode32();
	bytes_read = 4;
	
	// Error: We expected the length field to be different.
	if ((body_length + HEADER_LENGTH) !=(uint32) get_serialized_size_nopadding(coding) || body_length != bytes_read) {
		catch_bad_alloc( err.put( new IEWrongLength(coding, get_category(), get_xtype(), get_subtype(), msg.get_pos())) );
		msg.set_pos(start_pos);
		return false;
	}
	
	return true;	
}

void auth_attr_addr_spi::serialize_body(NetMsg &msg, coding_t coding, uint32 &bytes_written) const {
	msg.encode32(spi);
	bytes_written = 4;
}

size_t auth_attr_addr_spi::get_serialized_size(coding_t coding) const{
	return 8;
}

size_t auth_attr_addr_spi::get_serialized_size_nopadding(coding_t coding) const{
	return 8;
}

bool auth_attr_addr_spi::check_body() const{
	if(spi!=0) return true;
	return false;	
}

bool auth_attr_addr_spi::equals_body(const auth_attr &attr) const {

	const auth_attr_addr_spi * other = dynamic_cast<const auth_attr_addr_spi*>(&attr);

	return other!=NULL && spi == other->get_spi();
}

uint32 auth_attr_addr_spi::get_spi() const {
	return spi;
}

void auth_attr_addr_spi::set_spi(uint32 spi_){
 	spi=spi_;
}

ostream &auth_attr_addr_spi::print_attributes(ostream &os) const {
	return os << "Address: " << spi; 
}

ostream &auth_attr_addr_spi::print(ostream &os, uint32 level, const uint32 indent, const char *name ) const {
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

}

