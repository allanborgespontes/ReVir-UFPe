#include "generic_attr_test.h"
#include "auth_attr_addr.h"
#include "auth_attr_addr_spi.h"
#include "auth_attr_data.h"
#include "auth_attr_hmac_trans_id.h"
#include "auth_attr_portlist.h"
#include "auth_attr_string.h"
#include "auth_attr_time.h"
#include "auth_nslp_object_list.h"
#include "hmac_keyregistry.h"

using namespace nslp_auth;

extern key_id_t pskey_id1;
extern key_id_t pskey_id2;


class AuthAttrAddrTest : public GenericAttributeTest {

	CPPUNIT_TEST_SUITE( AuthAttrAddrTest );
	AUTH_ATTR_DEFAULT_TESTS()
	CPPUNIT_TEST_SUITE_END();

  public:
  	virtual auth_attr *createInstance1() const {
		auth_attr_addr *a = new auth_attr_addr();
		hostaddress ip("192.168.0.1");
		a->set_ip(ip);		
		return a;
	}
	
	virtual auth_attr *createInstance2() const {
		auth_attr_addr *a = new auth_attr_addr(DEST_ADDR,IPV4_ADDRESS);
		hostaddress ip("192.168.2.1");
		a->set_ip(ip);		
		return a;
	}
	
	virtual void mutate1(auth_attr *attribute) const {
		hostaddress ip("192.168.1.1");
		auth_attr_addr *attr_addr = dynamic_cast<auth_attr_addr*>(attribute);
		CPPUNIT_ASSERT( attr_addr != NULL ); 
		attr_addr->set_ip(ip);
	}

};


class AuthAttrAddrSPITest : public GenericAttributeTest {

	CPPUNIT_TEST_SUITE( AuthAttrAddrSPITest );
	AUTH_ATTR_DEFAULT_TESTS()
	CPPUNIT_TEST_SUITE_END();

  public:
  	virtual auth_attr *createInstance1() const {
		auth_attr_addr_spi *a = new auth_attr_addr_spi();
		a->set_spi(1);		
		return a;
	}
	
	virtual auth_attr *createInstance2() const {
		auth_attr_addr_spi *a = new auth_attr_addr_spi();
		a->set_spi(2);		
		return a;
	}
	
	virtual void mutate1(auth_attr *attribute) const {
		auth_attr_addr_spi *attr_addr = dynamic_cast<auth_attr_addr_spi*>(attribute);
		CPPUNIT_ASSERT( attr_addr != NULL ); 
		attr_addr->set_spi(3);
	}

};


class AuthAttrDataTest : public GenericAttributeTest {

	CPPUNIT_TEST_SUITE( AuthAttrDataTest );
	AUTH_ATTR_DEFAULT_TESTS()
	CPPUNIT_TEST_SUITE_END();

  public:
  	virtual auth_attr *createInstance1() const {
		auth_attr_data *a = new auth_attr_data(pskey_id1);
		uint8* hmac = new uint8[3];
		for(unsigned int i = 0; i<3; i++) hmac[i]=i;
		a->set_hmac(hmac, 3);		
		return a;
	}
	
	virtual auth_attr *createInstance2() const {
		auth_attr_data *a = new auth_attr_data(pskey_id1);
		uint8* hmac = new uint8[session_auth_object::HMAC_size];
		for(unsigned int i = 0; i<session_auth_object::HMAC_size; i++) hmac[i]=0;
		a->set_hmac(hmac, session_auth_object::HMAC_size);		
		return a;
	}
	
	virtual void mutate1(auth_attr *attribute) const {
		auth_attr_data *attr_data = dynamic_cast<auth_attr_data*>(attribute);
		CPPUNIT_ASSERT( attr_data != NULL ); 
		attr_data->set_key_id(pskey_id2);
	}
};


class AuthAttrHmacTransIdTest : public GenericAttributeTest {

	CPPUNIT_TEST_SUITE( AuthAttrHmacTransIdTest );
	AUTH_ATTR_DEFAULT_TESTS()
	CPPUNIT_TEST_SUITE_END();

  public:
  	virtual auth_attr *createInstance1() const {
		auth_attr_hmac_trans_id *a = new auth_attr_hmac_trans_id();
		a->set_id(1);
		return a;
	}
	
	virtual auth_attr *createInstance2() const {
		auth_attr_hmac_trans_id *a = new auth_attr_hmac_trans_id();
		a->set_id(2);
		return a;
	}
	
	virtual void mutate1(auth_attr *attribute) const {
		auth_attr_hmac_trans_id *attr_hmac_id = dynamic_cast<auth_attr_hmac_trans_id*>(attribute);
		CPPUNIT_ASSERT( attr_hmac_id != NULL ); 
		attr_hmac_id->set_id(3);
	}

};


class AuthAttrPortlistTest : public GenericAttributeTest {

	CPPUNIT_TEST_SUITE( AuthAttrPortlistTest );
	AUTH_ATTR_DEFAULT_TESTS()
	CPPUNIT_TEST_SUITE_END();

  public:
  	virtual auth_attr *createInstance1() const {
		auth_attr_portlist *a = new auth_attr_portlist();
		a->insert(1);
		a->insert(2);
		a->insert(3);
		return a;
	}
	
	virtual auth_attr *createInstance2() const {
		auth_attr_portlist *a = new auth_attr_portlist();
		a->insert(4);
		a->insert(5);
		a->insert(6);
		return a;
	}
	
	virtual void mutate1(auth_attr *attribute) const {
		auth_attr_portlist *attr_portlist = dynamic_cast<auth_attr_portlist*>(attribute);
		CPPUNIT_ASSERT( attr_portlist != NULL ); 
		attr_portlist->insert(7);
	}

};


class AuthAttrStringTest : public GenericAttributeTest {

	CPPUNIT_TEST_SUITE( AuthAttrStringTest );
	AUTH_ATTR_DEFAULT_TESTS()
	CPPUNIT_TEST_SUITE_END();

  public:
  	virtual auth_attr *createInstance1() const {
		auth_attr_string *a = new auth_attr_string();
		uchar *uc = new uchar[2];
		uc[0]='a';
		uc[1]=0;
		a->set_string(uc,2);
		return a;
	}
	
	virtual auth_attr *createInstance2() const {
		auth_attr_string *a = new auth_attr_string();
		uchar *uc = new uchar[2];
		uc[0]='b';
		uc[1]=0;
		a->set_string(uc,2);
		return a;
	}
	
	virtual void mutate1(auth_attr *attribute) const {
		auth_attr_string *attr_string = dynamic_cast<auth_attr_string*>(attribute);
		CPPUNIT_ASSERT( attr_string != NULL ); 
		uchar *uc = new uchar[2];
		uc[0]='c';
		uc[1]=0;
		attr_string->set_string(uc,2);
	}

};


class AuthAttrTimeTest : public GenericAttributeTest {

	CPPUNIT_TEST_SUITE( AuthAttrTimeTest );
	AUTH_ATTR_DEFAULT_TESTS()
	CPPUNIT_TEST_SUITE_END();

  public:
  	virtual auth_attr *createInstance1() const {
		auth_attr_time *a = new auth_attr_time();
		return a;
	}
	
	virtual auth_attr *createInstance2() const {
		auth_attr_time *a = new auth_attr_time();
		a->set_value(time(NULL)+6000);
		return a;
	}
	
	virtual void mutate1(auth_attr *attribute) const {
		auth_attr_time *attr_time = dynamic_cast<auth_attr_time*>(attribute);
		CPPUNIT_ASSERT( attr_time != NULL ); 
		attr_time->set_value(time(NULL)+12000);
	}

};


class AuthNslpObjectListTest : public GenericAttributeTest {

	CPPUNIT_TEST_SUITE( AuthNslpObjectListTest );
	AUTH_ATTR_DEFAULT_TESTS()
	CPPUNIT_TEST_SUITE_END();

  public:
  	virtual auth_attr *createInstance1() const {
		auth_nslp_object_list *a = new auth_nslp_object_list();
		a->insert(1);
		return a;
	}
	
	virtual auth_attr *createInstance2() const {
		auth_nslp_object_list *a = new auth_nslp_object_list();
		a->insert(10);
		return a;
	}
	
	virtual void mutate1(auth_attr *attribute) const {
		auth_nslp_object_list *obj_list = dynamic_cast<auth_nslp_object_list*>(attribute);
		CPPUNIT_ASSERT( obj_list != NULL ); 
		obj_list->insert(4);
	}

};


CPPUNIT_TEST_SUITE_REGISTRATION( AuthAttrAddrTest );
CPPUNIT_TEST_SUITE_REGISTRATION( AuthAttrAddrSPITest );
CPPUNIT_TEST_SUITE_REGISTRATION( AuthAttrDataTest );
CPPUNIT_TEST_SUITE_REGISTRATION( AuthAttrHmacTransIdTest );
CPPUNIT_TEST_SUITE_REGISTRATION( AuthAttrPortlistTest );
CPPUNIT_TEST_SUITE_REGISTRATION( AuthAttrStringTest );
CPPUNIT_TEST_SUITE_REGISTRATION( AuthAttrTimeTest );
CPPUNIT_TEST_SUITE_REGISTRATION( AuthNslpObjectListTest );
