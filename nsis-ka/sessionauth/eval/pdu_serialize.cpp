#include "pdu_serialize.h"
#include <iostream>
#include "../include/benchmark_journal.h"


namespace nslp_auth {
  using namespace std;

extern class benchmark_journal journal;

pdu_serialize::pdu_serialize(bool auth_session_object) 
	: expected_size(0), data_pdu(create_NTLP_PDU(auth_session_object)), msg(new protlib::NetMsg(1000)) 
{
	expected_size = data_pdu->get_serialized_size(protlib::IE::protocol_v1);
}

pdu_serialize::~pdu_serialize() {
	delete data_pdu;
	delete msg;
}

void pdu_serialize::performTask() {
	msg->set_pos(0);
	uint32 bytes_written = 0;
	data_pdu->serialize(*msg,protlib::IE::protocol_v1,bytes_written);
	if( bytes_written != expected_size )
		cout<<"Error: unexpected pdu size"<<endl;
}

}
