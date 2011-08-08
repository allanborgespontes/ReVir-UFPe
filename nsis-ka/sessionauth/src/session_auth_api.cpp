#include "session_auth_api.h"
#include "session_auth_object.h"


bool 
nslp_auth::check_hmac(NetMsg &msg, IE::coding_t coding, bool fill_data)
{
  	return nslp_auth::session_auth_object::check_hmac(msg, coding, fill_data);
}


bool 
nslp_auth::serialize_hmac(NetMsg &msg, IE::coding_t coding, bool fill_data)
{
  	return nslp_auth::session_auth_object::serialize_hmac(msg, coding, fill_data);
}

uint16 
nslp_auth::get_tlplist_nslp_object_type(const uint8* buf)
{
	return nslp_auth::session_auth_object::get_tlplist_nslp_object_type(buf);
}

uint16 
nslp_auth::get_tlplist_ntlp_object_type(const uint8* buf)
{
	return nslp_auth::session_auth_object::get_tlplist_ntlp_object_type(buf);
}
