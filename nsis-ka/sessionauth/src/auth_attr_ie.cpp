#include "logfile.h"

#include "auth_attr_ie.h"
#include "auth_attr.h"
#include "benchmark_journal.h"


namespace nslp_auth {
using namespace protlib::log;

#ifdef BENCHMARK
  extern benchmark_journal journal;
#endif

/**
 * This is where our single AUTH_ATTR_IEManager is stored.
 */
AUTH_ATTR_IEManager *AUTH_ATTR_IEManager::auth_inst = NULL;


/**
 * Constructor for child classes.
 *
 * This constructor has been made protected to only allow instantiation
 * via the static instance() method.
 */
AUTH_ATTR_IEManager::AUTH_ATTR_IEManager() : IEManager() {
	// nothing to do
}


/**
 * Singleton static factory method. Returns the same AUTH_ATTR_IEManager object,
 * no matter how often it is called.
 *
 * Note: Calling clear() causes the object to be deleted and the next call
 * to instance() create a new AUTH_ATTR_IEManager object.
 *
 * @return always the same AUTH_ATTR_IEManager object
 */
AUTH_ATTR_IEManager *AUTH_ATTR_IEManager::instance() {

	// Instance already exists, so return it.
	if ( auth_inst )
		return auth_inst;

	// We don't have an instance yet. Create it.
	try {
		auth_inst = new AUTH_ATTR_IEManager();
		Log(DEBUG_LOG, LOG_NORMAL, "AUTH_ATTR_IEManager",
			"Created AUTH_ATTR_IEManager singleton: " << auth_inst);
	}
	catch ( bad_alloc ) {
		Log(ERROR_LOG, LOG_CRIT, "AUTH_ATTR_IEManager",
				"Cannot create AUTH_ATTR_IEManager singleton");
		throw IEError(IEError::ERROR_NO_IEMANAGER);
	}

	return auth_inst;
}


/**
 * Delete the singleton instance.
 *
 * After calling this, all pointers to or into the object are invalid.
 * The instance() method has to be called before using the AUTH_ATTR_IEManager
 * next time.
 */
void AUTH_ATTR_IEManager::clear() {
	delete auth_inst;
	auth_inst = 0;
	Log(DEBUG_LOG, LOG_NORMAL, "AUTH_ATTR_IEManager",
		"Deleted AUTH_ATTR_IEManager singleton");
}


/**
 * Register all known IEs.
 *
 * This method clears the registry and then registers all IEs known to this
 * implementation. It solely exists for convenience.
 */
void AUTH_ATTR_IEManager::register_known_ies() {
	clear();

	// auth_ent_id
	instance()->register_ie(new auth_attr_addr(AUTH_ENT_ID,subtype_auth_ent_ipv4_address));
	instance()->register_ie(new auth_attr_addr(AUTH_ENT_ID,subtype_auth_ent_ipv6_address));
	instance()->register_ie(new auth_attr_string(AUTH_ENT_ID,subtype_auth_ent_ascii_dn_x500));
	instance()->register_ie(new auth_attr_string(AUTH_ENT_ID,subtype_auth_ent_unicode_dn_x500));
	instance()->register_ie(new auth_attr_string(AUTH_ENT_ID,subtype_auth_ent_fqdn));
	instance()->register_ie(new auth_attr_string(AUTH_ENT_ID,subtype_auth_ent_uri));
	instance()->register_ie(new auth_attr_string(AUTH_ENT_ID,subtype_auth_ent_krb_principal));
	instance()->register_ie(new auth_attr_string(AUTH_ENT_ID,subtype_auth_ent_x509_v3_cert));
	instance()->register_ie(new auth_attr_string(AUTH_ENT_ID,subtype_auth_ent_pgp_cert));
	instance()->register_ie(new auth_attr_hmac_trans_id());
	// source_addr
	instance()->register_ie(new auth_attr_addr(SOURCE_ADDR,subtype_source_addr_ipv4_address));
 	instance()->register_ie(new auth_attr_addr(SOURCE_ADDR,subtype_source_addr_ipv6_address));
 	instance()->register_ie(new auth_attr_portlist(SOURCE_ADDR,subtype_source_addr_udp_portlist));
 	instance()->register_ie(new auth_attr_portlist(SOURCE_ADDR,subtype_source_addr_tcp_portlist));
 	instance()->register_ie(new auth_attr_addr_spi(SOURCE_ADDR,subtype_source_addr_spi));
 	// dest_addr
	instance()->register_ie(new auth_attr_addr(DEST_ADDR,subtype_source_addr_ipv4_address));
 	instance()->register_ie(new auth_attr_addr(DEST_ADDR,subtype_source_addr_ipv6_address));
 	instance()->register_ie(new auth_attr_portlist(DEST_ADDR,subtype_source_addr_udp_portlist));
 	instance()->register_ie(new auth_attr_portlist(DEST_ADDR,subtype_source_addr_tcp_portlist));
 	instance()->register_ie(new auth_attr_addr_spi(DEST_ADDR,subtype_source_addr_spi));
    // start_time
 	instance()->register_ie(new auth_attr_time(START_TIME,subtype_start_time_ntp_timestamp));
    // end_time
 	instance()->register_ie(new auth_attr_time(END_TIME,subtype_end_time_ntp_timestamp));
	// nslp_object_list
	instance()->register_ie(new auth_nslp_object_list());
	// authentification_data
	instance()->register_ie(new auth_attr_data());
}


/**
 * Parse a AUTH parameter.
 *
 * Helper method for deserialize(). Parameters work like in deserialize().
 *
 * @param msg a buffer containing the serialized PDU
 * @param coding the protocol version used in msg
 * @param errorlist returns the exceptions caught while parsing the message
 * @param bytes_read returns the number of bytes read from msg
 * @param skip if true, try to ignore errors and continue reading
 * @return the newly created IE, or NULL on error
 */
IE *AUTH_ATTR_IEManager::deserialize_auth_attr(NetMsg &msg,
		IE::coding_t coding, IEErrorList &errorlist,
		uint32 &bytes_read, bool skip) {

	/*
	 * Peek ahead to find out the parameter id.
	 */
	uint8 xtype,subtype;
	try {
		uint32 header_raw = msg.decode32(false); // don't move position
		xtype = auth_attr::extract_xtype(header_raw);
		subtype = auth_attr::extract_subtype(header_raw);
	}
	catch ( NetMsgError ) {
		errorlist.put(new IEMsgTooShort(
				coding, cat_auth_attr, msg.get_pos()) );
		return NULL; // fatal error
	}


	IE *ie = new_instance(cat_auth_attr, xtype, subtype);
	
	if ( ie == NULL ) {
		Log(ERROR_LOG, LOG_CRIT, "AUTH_ATTR_IEManager",
		    "unregistered (xtype,subtype)-tuple xtype: " << (uint16) xtype << " with subtype:" << (uint16) subtype );
		catch_bad_alloc( errorlist.put(new IEWrongType(
			coding, cat_auth_attr, msg.get_pos())) );
		return NULL;
	}
	
#ifdef BENCHMARK
	uint32 attr_type_add = 0;
	switch(xtype) {
		case AUTH_NSLP_OBJECT_LIST :
			attr_type_add = 2;
			break;
	}
	//MP((benchmark_journal::measuring_point_id_t)(benchmark_journal::PRE_DESERIALIZE_SESSIONAUTH_ATTR_UNKNOWN + attr_type_add));
#endif
	IE *ret = ie->deserialize(msg, coding, errorlist, bytes_read, skip);
#ifdef BENCHMARK	
	//MP((benchmark_journal::measuring_point_id_t)(benchmark_journal::POST_DESERIALIZE_SESSIONAUTH_ATTR_UNKNOWN + attr_type_add));
#endif

	if ( ret == NULL )
		delete ie;

	return ret; // either an initialized IE or NULL on error
}

}
// EOF
