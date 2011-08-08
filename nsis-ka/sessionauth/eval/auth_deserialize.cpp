#include "auth_deserialize.h"
#include "hmac_keyregistry.h"
#include <iostream>
#include "benchmark_journal.h"

namespace nslp_auth {
  using namespace std;

	extern nslp_auth::benchmark_journal journal;

auth_deserialize::auth_deserialize(bool do_Hmac)
 : Benchmark(), auth_obj(createAuthObj()), msg(new protlib::NetMsg(1000)), do_hmac(do_Hmac)
{
	uint32 expected_size = auth_obj->get_serialized_size(protlib::IE::protocol_v1);
	msg->encode32(0x0100001A); // NSLP HEADER  = version | Gist Hops | Message Length 
	msg->encode32(0x12340000); // NSLPID & offcut of the NSLP HEADER
	msg->encode16(0x0008); // NSLPDATA_Object = Action & Type
	msg->encode16((expected_size/4)+1); // NSLPDATA_Object = RSV & length (in 32 bit)
	msg->encode32(0x01000000); // NSLP HEADER ?
	auth_obj->serialize(*msg,protlib::IE::protocol_v1,bytes_written);
	if(do_hmac) {
		nslp_auth::sign_key_registry.storeKey(1, "123", 3, 2);
		auth_session_object::serialize_hmac(*msg,protlib::IE::protocol_v1,true);
	}
	if(auth_obj->get_serialized_size(protlib::IE::protocol_v1) != bytes_written )
		cout<<"Serialize Error because of unexpected size "<<endl;
	delete auth_obj;
	register_auth_attr_ies();
	auth_obj = new auth_session_object();
}

auth_deserialize::~auth_deserialize(){
	delete auth_obj;
	delete msg;
}

void auth_deserialize::performTask(){
	msg->set_pos(16);
	uint32 bytes_read = 0;
	IEErrorList errlist;
	MP(benchmark_journal::PRE_DESERIALIZE_SESSIONAUTH);
	auth_obj->deserialize(*msg, IE::protocol_v1, errlist, bytes_read, false);
	MP(benchmark_journal::POST_DESERIALIZE_SESSIONAUTH);
	if(do_hmac) {
		
		if(!auth_session_object::check_hmac(*msg, IE::protocol_v1))
			cout<<"HMac failed"<<endl;
	}
	if( bytes_read != bytes_written )
		cout<<"Deserialize Error because of unexpected size "<<endl;
	const vector<auth_attr*>& vec = auth_obj->get_vec_attr();
	vector<auth_attr*>::const_iterator it = vec.begin();
	for(;it!=vec.end();it++) {
		delete auth_obj->remove_attr((*it)->get_xtype(),(*it)->get_subtype());
	}
}

}
