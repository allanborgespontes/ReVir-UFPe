#include "auth_attr.h"
#include "logfile.h"
#include "auth_attr_string.h"
#include "ie.h"
#include <string.h>

namespace nslp_auth {
using namespace protlib::log;




auth_attr_string::auth_attr_string(uint8 xtype, uint8 subtype) : auth_attr(xtype, subtype), c_u(NULL), clen(0) {
	switch(subtype) {
		case FQDN:		ie_name = "FQDN";
			break;
		case ASCII_DN:		ie_name = "ASCII_DN";
			break;
		case UNICODE_DN:	ie_name = "UNICODE_DN";
			break;
		case URI:      		ie_name = "URI";
			break;
		case KRB_PRINCIPAL:	ie_name = "KRB_PRINCIPAL";
			break;
		case X509_V3_CERT:	ie_name = "X509_V3_CERT";
			break;
		case PGP_CERT:		ie_name = "PGP_CERT";
			break;
		default:	       	ie_name = NULL;
	}
}

/*
* Destructor
*/
auth_attr_string::~auth_attr_string(){
	delete c_u;
}

/*
* New Instance
*/
auth_attr_string *auth_attr_string::new_instance() const {
	auth_attr_string *a = NULL;
	catch_bad_alloc( a = new auth_attr_string() );
	return a;
}


auth_attr_string *auth_attr_string::copy() const {
	auth_attr_string *a = NULL;
	catch_bad_alloc( a = new auth_attr_string(*this) );
	return a;
}

size_t auth_attr_string::get_serialized_size_nopadding(coding_t coding) const{
	return 4+clen; // header with content, excluding padding
}

size_t auth_attr_string::get_serialized_size(coding_t coding) const{
	return 4+round_up4(clen); // header plus padded content
}

bool auth_attr_string::deserialize_body(NetMsg &msg, coding_t coding, uint16 body_length, IEErrorList &err, uint32 &bytes_read, bool skip) {

	bytes_read = 0;
	uint32 start_pos = msg.get_pos();

	// Error: Message is shorter than the length field makes us believe.
	if ( msg.get_bytes_left() < body_length ) {
		catch_bad_alloc( err.put(new IEMsgTooShort(coding, category, start_pos)) );
 		msg.set_pos(start_pos);
		return false;
	}

	// length of uchar* (uchar is 1 Byte) is body_length (in Byte)
	uint32 len = body_length;
	delete c_u;
	c_u = new uchar[len];
	*c_u= 0;
	msg.decode(c_u , len);
	clen=len;
	bytes_read = len;
	
	// Error: We expected the length field to be different.
	if ((body_length + HEADER_LENGTH) !=(uint16) get_serialized_size_nopadding(coding) || body_length!= bytes_read) {
		catch_bad_alloc( err.put( new IEWrongLength(coding, get_category(), get_xtype(), get_subtype(), msg.get_pos())) );
		msg.set_pos(start_pos);
		return false;
	}
	
	// jump over padding
	if( bytes_read%4 ) {
		msg.set_pos_r(4 - (bytes_read % 4));
		bytes_read += (4 - (bytes_read % 4));
	}

	return true;
}

void auth_attr_string::serialize_body(NetMsg &msg, coding_t coding, uint32 &bytes_written) const {
	msg.encode(c_u , clen);
	bytes_written = clen;

	// handle padding
	if ( bytes_written % 4 )
	{
		msg.padding(4 - ( bytes_written % 4), true);
		bytes_written += 4 - ( bytes_written % 4);
	}
}


const uchar* auth_attr_string::get_string() const{
	return c_u;
}

// Datatype utf-8 ASCII???
void auth_attr_string::set_string(uchar *c, uint32 len) {
	delete c_u;
	c_u = new uchar[len];
	c_u = (uchar*)memcpy(c_u, c, (size_t) len);
	clen = len;
}


void 
auth_attr_string::set_string(const std::string& entity_id_str) {
  delete c_u; c_u=NULL ;
  uint32 len= entity_id_str.length();
  if (len)
  {
    c_u = new uchar[len];
    c_u = (uchar*)memcpy(c_u, entity_id_str.c_str(), (size_t) len);
  }
  clen = len;
}

uint32 auth_attr_string::get_length() const{
	return clen;
}

bool auth_attr_string::equals_body(const auth_attr &attr) const {
  const auth_attr_string *other = dynamic_cast<const auth_attr_string*>(&attr);

  if ( other == NULL || clen != other->get_length() )
      return false;

   return (c_u == NULL && other->get_string() == NULL) || (c_u != NULL && other->get_string() != NULL && memcmp(c_u, other->get_string(), clen) == 0);

}

ostream &auth_attr_string::print_attributes(ostream &os) const {
	os << "string : "<<c_u;
	return os; 
}

ostream &auth_attr_string::print(ostream &os, uint32 level, const uint32 indent, const char *name ) const {
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

bool auth_attr_string::check() const {
	if( x_type != AUTH_ENT_ID || ie_name == NULL ) return false;
	else return check_body();
}

bool auth_attr_string::check_body() const {
	if( c_u == NULL || clen == 0 ) return false;
	else return true;
}

auth_attr_string::auth_attr_string(const auth_attr_string& other)
 : auth_attr(other.get_xtype(), other.get_subtype()), ie_name (other.get_ie_name()), clen(other.get_length()) {
	c_u = new uchar[other.get_length()];
	memcpy(c_u,other.get_string(),other.get_length());
}

}

