/*
 * Check building and linking with the NTLP implementation.
 *
 * $Id: ntlp_integration.cpp 3094 2008-07-02 13:58:37Z bless $
 * $HeadURL: https://svn.tm.kit.edu/nsis/natfw-nslp/trunk/test/ntlp_integration.cpp $
 */
#include <cppunit/TestCase.h>
#include <cppunit/extensions/HelperMacros.h>

#include "threadsafe_db.h" // from protlib

#include "apimessage.h" // from NTLP
#include "mri.h"	// from NTLP

#include "dispatcher.h"
#include "msg/natfw_ie.h"
#include "msg/natfw_msg.h"

using namespace natfw;
using namespace natfw::msg;


class NtlpIntegration : public CppUnit::TestCase {

	CPPUNIT_TEST_SUITE( NtlpIntegration );

	CPPUNIT_TEST( testLinking );
	CPPUNIT_TEST( testMessageWrapping );

	CPPUNIT_TEST_SUITE_END();

  public:
	void testLinking();
	void testMessageWrapping();

  private:
	ntlp::APIMsg *create_api_message() const;
};

CPPUNIT_TEST_SUITE_REGISTRATION( NtlpIntegration );


ntlp::APIMsg *NtlpIntegration::create_api_message() const {
	using ntlp::APIMsg;
	using ntlp::nslpdata;
	using ntlp::mri;
	using ntlp::mri_pathcoupled;
	using ntlp::tx_attr_t;

	natfw_create create;
	create.set_session_lifetime(30);
	create.set_extended_flow_info(extended_flow_info::ra_allow, 0);
	create.set_msg_sequence_number(47);

	NetMsg payload( create.get_serialized_size(IE::protocol_v1) );
	uint32 bytes_written;
	create.serialize(payload, IE::protocol_v1, bytes_written);

	APIMsg *msg = new APIMsg();
	nslpdata *data = new nslpdata(payload.get_buffer(), payload.get_size());
	uint32 nslpid = 2;
	ntlp::sessionid *sid = new ntlp::sessionid(1, 2, 3, 4);
	mri *m = new mri_pathcoupled(); // just for testing
	bool adj_check = false;
	uint32 sii = 0;
	tx_attr_t attr; // reliable and secure
	attr.reliable= true;
	attr.secure= false;

	uint16 ip_ttl = 100;
	uint16 ip_distance = 50;
	uint32 gist_hop_count = 10;

	msg->set_recvmessage(data, nslpid, sid, m, adj_check, sii, attr, 
		ip_ttl, ip_distance, gist_hop_count);

	return msg;
}


// use an object from the NTLP to see if linking works
void NtlpIntegration::testLinking() {
	protlib::tsdb::init();
	ntlp::APIMsg msg;
}


void NtlpIntegration::testMessageWrapping() {
	using namespace protlib;
	gistka_mapper m;

	NATFW_IEManager::register_known_ies();
	dispatcher d = dispatcher(NULL, NULL, NULL, NULL);

	ntlp::APIMsg *msg = create_api_message();
	event *e = m.map_to_event(msg);

	CPPUNIT_ASSERT( e != NULL );
	CPPUNIT_ASSERT( e->get_session_id() != NULL );

	uint128 sid_raw1( 1, 2, 3, 4 );
	uint128 sid_raw2 = e->get_session_id()->get_id();
	CPPUNIT_ASSERT( sid_raw1.w1 == sid_raw2.w1 );
	CPPUNIT_ASSERT( sid_raw1.w2 == sid_raw2.w2 );
	CPPUNIT_ASSERT( sid_raw1.w3 == sid_raw2.w3 );
	CPPUNIT_ASSERT( sid_raw1.w4 == sid_raw2.w4 );

	msg_event *me = dynamic_cast<msg_event *>(e);
	CPPUNIT_ASSERT( me != NULL );

	CPPUNIT_ASSERT( me->get_create() != NULL );

	delete msg;
}

// EOF
