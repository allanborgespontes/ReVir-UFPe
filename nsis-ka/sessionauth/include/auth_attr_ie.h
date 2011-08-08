#ifndef AUTH__IE_H
#define AUTH__IE_H

#include "ie.h"
#include "auth_attr.h"
#include "auth_attr_addr.h"
#include "auth_attr_addr_spi.h"
#include "auth_attr_data.h"
#include "auth_attr_hmac_trans_id.h"
#include "auth_attr_portlist.h"
#include "auth_attr_string.h"
#include "auth_attr_time.h"
#include "auth_nslp_object_list.h"


namespace nslp_auth {
  using namespace protlib;


/**
 * Subtypes
 */
enum auth_ent_id_subtype_t {
	subtype_auth_ent_ipv4_address 	= 1,
	subtype_auth_ent_ipv6_address  	= 2,
	subtype_auth_ent_fqdn 		  	= 3,
	subtype_auth_ent_ascii_dn_x500	= 4,
	subtype_auth_ent_unicode_dn_x500= 5,
	subtype_auth_ent_uri			= 6,
	subtype_auth_ent_krb_principal  = 7,
 	subtype_auth_ent_x509_v3_cert  	= 8,
	subtype_auth_ent_pgp_cert	  	= 9,
	subtype_auth_ent_hmac_signed	= 10
};

enum source_addr_subtype_t {
	subtype_source_addr_ipv4_address 	= 1,
	subtype_source_addr_ipv6_address  	= 2,
	subtype_source_addr_udp_portlist   	= 3,
	subtype_source_addr_tcp_portlist	= 4,
	subtype_source_addr_spi				= 5
};

enum dest_addr_subtype_t {
	subtype_dest_addr_ipv4_address 		= 1,
	subtype_dest_addr_ipv6_address  	= 2,
	subtype_dest_addr_udp_portlist   	= 3,
	subtype_dest_addr_tcp_portlist		= 4,
	subtype_dest_addr_spi				= 5
};

enum start_time_subtype_t {
	subtype_start_time_ntp_timestamp 	= 1
};

enum end_time_subtype_t {
	subtype_end_time_ntp_timestamp 		= 1
};

/**
 * An Interface for reading/writing AUTHs.
 *
 * The AUTH_ATTR_IEManager is a Singleton that provides methods to read/write
 * AUTHs from/to NetMsg objects. Those methods are called deserialize()
 * and serialize(), respectively.
 *
 * To deserialize() a AUTH, each IE to be used during the process (except
 * for auth_object) has to be registered with AUTH_ATTR_IEManager using
 * register_ie(). Registered IEs will be freed automatically as soon as
 * either clear() is called or AUTH_ATTR_IEManager itself is destroyed.
 *
 * The only way to get an AUTH_ATTR_IEManager object is through the static
 * instance() method.
 *
 * Note that this singleton is able to coexist with other IEManager
 * subclasses.
 */
class AUTH_ATTR_IEManager : public IEManager {

  public:
	static AUTH_ATTR_IEManager *instance();
	static void clear();

	static void register_known_ies();

	IE *deserialize_auth_attr(NetMsg &msg,
		IE::coding_t coding, IEErrorList &errorlist,
		uint32 &bytes_read, bool skip);

  protected:
	// protected constructor to prevent instantiation
	AUTH_ATTR_IEManager();

	virtual IE *deserialize(NetMsg &msg, uint16 category,
			IE::coding_t coding, IEErrorList& errorlist,
			uint32 &bytes_read, bool skip = true) { return NULL; }
//	virtual IE *lookup_ie(uint16 category, uint16 type, uint16 subtype);

  private:
	static AUTH_ATTR_IEManager *auth_inst;


};


} // end namespace auth

#endif // AUTH__IE_H
