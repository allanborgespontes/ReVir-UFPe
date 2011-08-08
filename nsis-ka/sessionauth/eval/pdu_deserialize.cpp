#include "pdu_deserialize.h"
#include "ntlp_pdu.h"
#include <iostream>

#include "benchmark_journal.h"

namespace nslp_auth {
  using namespace std;

extern class benchmark_journal journal;

pdu_deserialize::pdu_deserialize(bool with_hmac)
 : bytes_written(0), msg(new protlib::NetMsg(1000)) {
 	register_NTLP_ies();
 	ntlp::data* data_pdu = create_NTLP_PDU(with_hmac);
	uint32 expected_size = data_pdu->get_serialized_size(protlib::IE::protocol_v1);
	data_pdu->serialize(*msg,protlib::IE::protocol_v1,bytes_written);
	if( bytes_written != expected_size )
		cout<<"Error: unexpected pdu size"<<endl;
	delete data_pdu;
}

pdu_deserialize::~pdu_deserialize() {
	delete msg;
}

void pdu_deserialize::performTask() {
	msg->set_pos(0);
	uint32 bytes_read = 0;
	IEErrorList err;
	MP(benchmark_journal::PRE_DESERIALIZE_NTLP);
	IE* ie = ntlp::NTLP_IEManager::instance()->deserialize(*msg, ntlp::NTLP_IE::cat_known_pdu, protlib::IE::protocol_v1, err, bytes_read, false);
	MP(benchmark_journal::POST_DESERIALIZE_NTLP);

	if( !err.is_empty()  )
		cout<<"Error: deserialize error occured"<<endl;
	if( bytes_read != bytes_written )
		cout<<"Error: unexpected deserialized size : "<<bytes_read<<endl;
	delete ie;
}

}
