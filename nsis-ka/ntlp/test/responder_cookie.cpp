// ----------------------------------------*- mode: C++; -*--
// responder_cookie.cpp Unit test for the responder cookie
// ----------------------------------------------------------
// $Id:$
// $HeadURL:$
// ==========================================================
//                      
// Copyright (C) 2009, all rights reserved by
// - Institute of Telematics, University of Karlsruhe (TH)
//
// More information and contact:
// https://projekte.tm.uka.de/trac/NSIS
//                      
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; version 2 of the License
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
//
// ==========================================================
#include "test_suite.h"

#include "ntlp_statemodule.h"

#include "mri_pc.h"
#include "routingtable.h"

#include "query.h"
#include "response.h"
#include "gist_conf.h"

#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/evp.h>


using namespace ntlp;
using namespace protlib;

namespace ntlp {
// configuration class
gistconf gconf;
}

SSL_CTX *ssl_ctx = NULL;

netaddress loc_if4("127.0.0.1");
netaddress loc_if6("::1");

// PI
peer_identity* pi1=new peer_identity();
peer_identity* pi2=new peer_identity();

// initial fill of RAOVec. We push "0" onto it
vector<uint32> raovec;

secretmanager secrets(2, 128);

// init routingtable
routingtable rt;

nli localnliv4(200, 30000, pi1, loc_if4);
nli localnliv6(200, 30000, pi1, loc_if6);

// NSLPtable is used by both Signaling Module AND StateModule
NSLPtable nslptable;

const char *
SSLerrmessage(void)
{
  unsigned long	errcode;
  const char	*errreason;
  static char	errbuf[32];
    
  errcode = ERR_get_error();
  if (errcode == 0)
    return "No SSL error reported";
  errreason = ERR_reason_error_string(errcode);
  
  if (errreason != NULL)
    return errreason;
  snprintf(errbuf, sizeof(errbuf), "SSL error code %lu", errcode);
  return errbuf;
}


class resp_cookie_test : public CppUnit::TestFixture {

  CPPUNIT_TEST_SUITE( resp_cookie_test );

	//CPPUNIT_TEST( test_secret_manager );
  CPPUNIT_TEST( test_resp_cookie );
  // Add more tests here.

  CPPUNIT_TEST_SUITE_END();

public:
	void setUp() {
		DLog("test","Start Setup()");
		gconf.repository_init(); 
		gconf.setRepository();
		// initialize OpenSSL stuff 
		SSL_library_init();
		OpenSSL_add_ssl_algorithms();
		SSL_load_error_strings();
		
		ssl_ctx=SSL_CTX_new(TLSv1_client_method());
		
		if (!ssl_ctx)
		{
			cerr << "could not create SSL context: " << SSLerrmessage() << endl;
		}
		
		addresses= new AddressList();
		smpar= new StatemoduleParam(rt,
				       localnliv4,
				       *addresses,
				       NULL, 
				       secrets, 
				       nslptable, 
				       raovec,
				       10);
		
		statemod= new Statemodule(*smpar);
		raovec.push_back(0);
		register_msgs();
		DLog("test","End Setup()");
	}

	void tearDown()
		{
			delete addresses;
			delete statemod;
			delete smpar;
		}

private:
	void register_msgs() {
		// register the necessary NTLP instances
		NTLP_IE *tmp;
		tmp = new mri_pathcoupled;
		NTLP_IEManager::instance()->register_ie(tmp);
		tmp = new nattraversal;
		NTLP_IEManager::instance()->register_ie(tmp);
		tmp = new ntlp::sessionid;
		NTLP_IEManager::instance()->register_ie(tmp);
		tmp = new nslpdata;
		NTLP_IEManager::instance()->register_ie(tmp);
		tmp = new nli;
		NTLP_IEManager::instance()->register_ie(tmp);
		tmp = new querycookie;
		NTLP_IEManager::instance()->register_ie(tmp);
		tmp = new stack_conf_data;
		NTLP_IEManager::instance()->register_ie(tmp);
		tmp = new stackprop;
		NTLP_IEManager::instance()->register_ie(tmp);
		tmp = new response;
		NTLP_IEManager::instance()->register_ie(tmp);
		tmp = new respcookie;
		NTLP_IEManager::instance()->register_ie(tmp); 
		tmp = new errorobject;
		NTLP_IEManager::instance()->register_ie(tmp);
	}

	void test_secret_manager() {

		uint32 current_generation_number= secrets.get_generation_number();

		uchar* current_secret1= secrets.get_secret();
		uchar* current_secret2= secrets.get_secret(current_generation_number);
		
		CPPUNIT_ASSERT (current_secret1 == current_secret2 );

		uint32 old_generation_number= current_generation_number;

		// new generation number and key
		secrets.forward();

		// old secret should be still available!
		uchar* previous_secret= secrets.get_secret(old_generation_number);
		CPPUNIT_ASSERT (previous_secret == current_secret2 );
		
		// new secret should also work
		current_generation_number= secrets.get_generation_number();
		current_secret1= secrets.get_secret();
		current_secret2= secrets.get_secret(current_generation_number);

		CPPUNIT_ASSERT (current_secret1 == current_secret2 );
	}

	void test_resp_cookie() {
		test_secret_manager();

		// test if we have a saved nat traversal object (NTO)
		NetMsg opaque_natinfobuf(localnliv4.get_serialized_size(IE::protocol_v1));
		uint32 written= 0;
		localnliv4.serialize(opaque_natinfobuf, IE::protocol_v1, written);
		natinfo nat_info;
		nat_info.set(opaque_natinfobuf);

		std::vector<natinfo> txlated_information;
		txlated_information.push_back(nat_info);
		std::vector<uint16> translated_objects;
		hostaddress source_b("141.3.71.3");
		hostaddress dest_b("141.3.71.4");
		mri_pathcoupled mri2(source_b, 32, dest_b, 20, true);

		nattraversal nto(&mri2, txlated_information, translated_objects);
		
		nto.push_natinfo(nat_info);
		nto.add_translation(known_ntlp_object::NLI);
		
		NetMsg* ntobuf_p= NULL;
		// put NTO as transparent data into hash if present
		uint32 ntobuf_size= nto.get_serialized_size(IE::protocol_v1);
		ntobuf_p= new NetMsg(ntobuf_size);
		// should be serialized including the common object header
		written= 0;
		nto.serialize(*ntobuf_p, IE::protocol_v1, written);
		// object should be completely written into buffer
		CPPUNIT_ASSERT(written == ntobuf_p->get_size());

		test_resp_cookie_arg(NULL);
		test_resp_cookie_arg(ntobuf_p);

		// secret key rollover
		secrets.forward();

		test_resp_cookie_key_rollover(NULL);


		test_resp_cookie_key_rollover(ntobuf_p);

	}
	
private:
	void test_resp_cookie_arg(NetMsg* ntobuf_p) {
		hostaddress source("10.0.0.1");
		hostaddress dest("10.0.0.2");

		mri_pathcoupled mri1(source, 32, dest, 20, true);
		sessionid sid;
		sid.generate_random();

		// routingkey(mri* mr= NULL, sessionid* sid= NULL, uint32 nslpid= 0) :
		routingkey r_key(mri1.copy(), sid.copy(), 0);

		int16 if_index=2;
		respcookie* rc= statemod->create_resp_cookie(&localnliv4, &r_key, 0, if_index, ntobuf_p);
		//respcookie* rc= statemod->create_resp_cookie(&localnliv4, &r_key, 0, if_index, NULL);

		NetMsg* rcookiebuf= new NetMsg(rc->get_serialized_size(IE::protocol_v1));
		uint32 written= 0;
		rc->serialize(*rcookiebuf, IE::protocol_v1, written);
		// object should be completely written into buffer
		CPPUNIT_ASSERT(written == rcookiebuf->get_size());
		rcookiebuf->set_pos(0);

		uint32 bytes_read= 0;
		IEErrorList errorlist;
		respcookie* rcvd_rspcookie= new respcookie;
		rcvd_rspcookie->deserialize(*rcookiebuf, IE::protocol_v1, errorlist, bytes_read, false);
		//DLog("create_resp_cookie:", "bytes_read: " << bytes_read << " size: " << rcookiebuf->get_size() );
		CPPUNIT_ASSERT ( bytes_read == rcookiebuf->get_size() );
		CPPUNIT_ASSERT( *rcvd_rspcookie == *rc );
		
		//Bool Statemodule::evaluate_resp_cookie(const nli* querier_nli, const routingkey* r_key, const respcookie* responder_cookie)
		
		CPPUNIT_ASSERT (statemod->evaluate_resp_cookie(&localnliv4, &r_key, rcvd_rspcookie) == true);

		respcookie* bad_rcvd_rspcookie= rcvd_rspcookie->copy();
		// manipulate cookie - generation number
		uchar* cbuf= bad_rcvd_rspcookie->get_buffer();
		*(cbuf+1)= ~ *(cbuf+1);
		CPPUNIT_ASSERT (statemod->evaluate_resp_cookie(&localnliv4, &r_key, bad_rcvd_rspcookie) == false);
		*(cbuf+1)= ~ *(cbuf+1);
		// manipulate cookie - if_index
		*(cbuf+5)= ~ *(cbuf+5);
		CPPUNIT_ASSERT (statemod->evaluate_resp_cookie(&localnliv4, &r_key, bad_rcvd_rspcookie) == false);
		*(cbuf+5)= ~ *(cbuf+5);

		// manipulate cookie - td length
		*(cbuf+7)= ~ *(cbuf+7);
		CPPUNIT_ASSERT (statemod->evaluate_resp_cookie(&localnliv4, &r_key, bad_rcvd_rspcookie) == false);
		*(cbuf+7)= ~ *(cbuf+7);

		// manipulate cookie - td 
		*(cbuf+9)= ~ *(cbuf+9);
		CPPUNIT_ASSERT (statemod->evaluate_resp_cookie(&localnliv4, &r_key, bad_rcvd_rspcookie) == false);
		*(cbuf+9)= ~ *(cbuf+9);

		// manipulate cookie - hmac
		*(cbuf+8+bad_rcvd_rspcookie->get_transparent_data_len())= ~*(cbuf+8+bad_rcvd_rspcookie->get_transparent_data_len()); 
		CPPUNIT_ASSERT (statemod->evaluate_resp_cookie(&localnliv4, &r_key, bad_rcvd_rspcookie) == false);
		*(cbuf+8+bad_rcvd_rspcookie->get_transparent_data_len())= ~*(cbuf+8+bad_rcvd_rspcookie->get_transparent_data_len()); 

		delete bad_rcvd_rspcookie;
		delete rcvd_rspcookie;
	}

	void test_resp_cookie_key_rollover(NetMsg* ntobuf_p) {
		hostaddress source("10.0.0.1");
		hostaddress dest("10.0.0.2");

		mri_pathcoupled mri1(source, 32, dest, 20, true);
		sessionid sid;
		sid.generate_random();

		// routingkey(mri* mr= NULL, sessionid* sid= NULL, uint32 nslpid= 0) :
		routingkey r_key(mri1.copy(), sid.copy(), 0);

		int16 if_index=2;
		uint32 generationnumber= secrets.get_generation_number();
		respcookie* rc= statemod->create_resp_cookie(&localnliv4, &r_key, generationnumber, if_index, ntobuf_p);
		//respcookie* rc= statemod->create_resp_cookie(&localnliv4, &r_key, 0, if_index, NULL);
		// now roll over
		secrets.forward();

		NetMsg* rcookiebuf= new NetMsg(rc->get_serialized_size(IE::protocol_v1));
		uint32 written= 0;
		rc->serialize(*rcookiebuf, IE::protocol_v1, written);
		// object should be completely written into buffer
		CPPUNIT_ASSERT(written == rcookiebuf->get_size());
		rcookiebuf->set_pos(0);

		uint32 bytes_read= 0;
		IEErrorList errorlist;
		respcookie* rcvd_rspcookie= new respcookie;
		rcvd_rspcookie->deserialize(*rcookiebuf, IE::protocol_v1, errorlist, bytes_read, false);
		//DLog("create_resp_cookie:", "bytes_read: " << bytes_read << " size: " << rcookiebuf->get_size() );
		CPPUNIT_ASSERT ( bytes_read == rcookiebuf->get_size() );
		CPPUNIT_ASSERT( *rcvd_rspcookie == *rc );
		
		//Bool Statemodule::evaluate_resp_cookie(const nli* querier_nli, const routingkey* r_key, const respcookie* responder_cookie)
		DLog("evaluate_resp_cookie","generation number= 0x" << hex << rcvd_rspcookie->get_generationnumber() << ", current gen number=" << secrets.get_generation_number());
		CPPUNIT_ASSERT (statemod->evaluate_resp_cookie(&localnliv4, &r_key, rcvd_rspcookie) == true);

		respcookie* bad_rcvd_rspcookie= rcvd_rspcookie->copy();
		// manipulate cookie - generation number
		uchar* cbuf= bad_rcvd_rspcookie->get_buffer();
		*(cbuf+1)= ~ *(cbuf+1);
		CPPUNIT_ASSERT (statemod->evaluate_resp_cookie(&localnliv4, &r_key, bad_rcvd_rspcookie) == false);
		*(cbuf+1)= ~ *(cbuf+1);
		// manipulate cookie - if_index
		*(cbuf+5)= ~ *(cbuf+5);
		CPPUNIT_ASSERT (statemod->evaluate_resp_cookie(&localnliv4, &r_key, bad_rcvd_rspcookie) == false);
		*(cbuf+5)= ~ *(cbuf+5);

		// manipulate cookie - td length
		*(cbuf+7)= ~ *(cbuf+7);
		CPPUNIT_ASSERT (statemod->evaluate_resp_cookie(&localnliv4, &r_key, bad_rcvd_rspcookie) == false);
		*(cbuf+7)= ~ *(cbuf+7);

		// manipulate cookie - td 
		*(cbuf+9)= ~ *(cbuf+9);
		CPPUNIT_ASSERT (statemod->evaluate_resp_cookie(&localnliv4, &r_key, bad_rcvd_rspcookie) == false);
		*(cbuf+9)= ~ *(cbuf+9);

		// manipulate cookie - hmac
		*(cbuf+8+bad_rcvd_rspcookie->get_transparent_data_len())= ~*(cbuf+8+bad_rcvd_rspcookie->get_transparent_data_len()); 
		CPPUNIT_ASSERT (statemod->evaluate_resp_cookie(&localnliv4, &r_key, bad_rcvd_rspcookie) == false);
		*(cbuf+8+bad_rcvd_rspcookie->get_transparent_data_len())= ~*(cbuf+8+bad_rcvd_rspcookie->get_transparent_data_len()); 

		delete bad_rcvd_rspcookie;
		delete rcvd_rspcookie;
	}

	StatemoduleParam* smpar;
	Statemodule* statemod;
        AddressList* addresses;
};

CPPUNIT_TEST_SUITE_REGISTRATION( resp_cookie_test );
