#include "auth_attr_data.h"
#include "session_auth_object.h"
#include "logfile.h"
#include <iomanip>
extern "C"
{
// hashing functions from OpenSSL
#include <openssl/hmac.h>
}


namespace nslp_auth {

const size_t auth_attr_data::HMAC_size = EVP_MD_size(EVP_sha1());


size_t auth_attr_data::get_serialized_size(coding_t coding) const {
	// attribute header + Key-ID 32bit + HMAC data
	return round_up4( 8 + hmac_len );
}

size_t auth_attr_data::get_serialized_size_nopadding(coding_t coding) const {
	// attribute header + Key-ID 32bit + HMAC data
	return ( 8 + hmac_len );
}

auth_attr_data *auth_attr_data::new_instance() const {
	auth_attr_data *a = NULL;
	catch_bad_alloc( a = new auth_attr_data() );
	return a;
}


auth_attr_data 
*auth_attr_data::copy() const {
	auth_attr_data *a = NULL;
	catch_bad_alloc( a = new auth_attr_data(*this) );
	return a;
}


bool 
auth_attr_data::deserialize_body(NetMsg &msg, coding_t coding, uint16 body_length, IEErrorList &err, uint32 &bytes_read, bool skip) {
	uint32 start_pos = msg.get_pos();

	// Error: Message is shorter than the length field makes us believe.
	if ( msg.get_bytes_left() < body_length ) {
		catch_bad_alloc( err.put(new IEMsgTooShort(coding, category, start_pos)) );
 		msg.set_pos(start_pos);
		return false;
	}
	
	key_id = msg.decode32();
	bytes_read += 4;
	hmac_len = body_length-4;

	// copy netmsg content to hmac buffer
	delete[] hmac;
	hmac = new uint8[hmac_len];
	//DLog("auth_attr_data","before copy_to(), pos=" << msg.get_pos() << ", start_pos=" << start_pos);
	msg.copy_to(hmac, msg.get_pos(), hmac_len);
	msg.set_pos_r(hmac_len);
	bytes_read+= hmac_len;
	
	// Error: We expected the length field to be different.
	if ( (body_length + HEADER_LENGTH) !=(uint32) get_serialized_size_nopadding(coding) || body_length != bytes_read) {
		//cerr << "*** body len: " << body_length << " expected: " << get_serialized_size_nopadding(coding) << " bytes_read:" << bytes_read << endl;
		catch_bad_alloc( err.put( new IEWrongLength(coding, get_category(), get_xtype(), get_subtype(), msg.get_pos())) );
		msg.set_pos(start_pos);
		return false;
	}
	
	// jump over padding
	if( bytes_read%4 ) {
		msg.set_pos_r(4 - (bytes_read % 4));
		bytes_read += (4 - (bytes_read % 4));
	}

	//cerr << "*** AUTHENTICATION_DATA *** body len: " << body_length << " expected: " << get_serialized_size_nopadding(coding)-HEADER_LENGTH << " hmac_len:" << hmac_len << " bytes_read:" << bytes_read << endl;

	return true;
}


void 
auth_attr_data::serialize_body(NetMsg &msg, coding_t coding, uint32 &bytes_written) const {
 	bytes_written=0;
 	msg.encode32(key_id);
	msg.copy_from(hmac, msg.get_pos(), hmac_len);
	msg.set_pos_r(hmac_len);
	bytes_written= 4 + hmac_len; // key_id + HMAC data
	// add padding if necessary
	if ( bytes_written % 4 )
	{
		msg.padding(4 - ( bytes_written % 4), true);
		bytes_written += 4 - ( bytes_written % 4);
	}
} 	


ostream &
auth_attr_data::print_attributes(ostream &os) const {

  ios_base::fmtflags flags = os.flags();	// save stream flags

  os << "key id: " << key_id << ", data:" << hex << noshowbase;
  for(uint16 i = 0; i <= hmac_len -1; i++){
    os << setw(2) << setfill('0') << static_cast<unsigned short>(hmac[i]);
  }
  os << setfill(' ');// << endl;   
  os.setf(flags);		// reset stream flags

  return os;
}


bool 
auth_attr_data::equals_body(const auth_attr &attr) const {

  const auth_attr_data *other = dynamic_cast<const auth_attr_data*>(&attr);

  if(other == NULL || key_id != other->get_key_id() || hmac_len != other->get_hmac_len() ) 
    return false;

  return memcmp(hmac,other->get_hmac(),hmac_len) == 0;
}

ostream &auth_attr_data::print(ostream &os, uint32 level, const uint32 indent, const char *name ) const {
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

bool auth_attr_data::check() const {
	return check_body();
}
	
bool auth_attr_data::check_body() const{
	return (hmac != NULL);
}

auth_attr_data::auth_attr_data(const auth_attr_data& other) : auth_attr(AUTHENTICATION_DATA, 0), ie_name("AUTHENTICATION DATA"), key_id(other.get_key_id()), hmac_len(other.get_hmac_len()) {
	hmac = new uint8[other.get_hmac_len()];
	memcpy(hmac,other.get_hmac(),other.get_hmac_len());
}


} // end namespace
