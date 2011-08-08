#include "nslp_serialize.h"
#include <iostream>
#include "../include/benchmark_journal.h"


namespace nslp_auth {
  using namespace std;

extern class benchmark_journal journal;

nslp_serialize::nslp_serialize() : expected_size(0), reserve_pdu(create_NSLP_PDU(true)), msg(new protlib::NetMsg(1000)) {
	expected_size = reserve_pdu->get_serialized_size(protlib::IE::protocol_v1);
}

nslp_serialize::~nslp_serialize() {
	delete reserve_pdu;
	delete msg;
}

void nslp_serialize::performTask() {
	msg->set_pos(0);
	uint32 bytes_written = 0;
	//MP(benchmark_journal::PRE_SERIALIZE_NSLP);
	reserve_pdu->serialize(*msg,protlib::IE::protocol_v1,bytes_written);
	//MP(benchmark_journal::POST_SERIALIZE_NSLP);
	if( bytes_written != expected_size )
		cout<<"Error: unexpected pdu size"<<endl;
}

}
