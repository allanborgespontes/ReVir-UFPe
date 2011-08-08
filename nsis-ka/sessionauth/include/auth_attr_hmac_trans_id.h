#ifndef AUTH_ATTR_HMAC_TRANS_ID_H
#define AUTH_ATTR_HMAC_TRANS_ID_H

#include "auth_attr.h"



namespace nslp_auth{
	using namespace protlib;


enum subtype_auth_ent_id_t {
  	HMAC_SIGNED			=	10	
};

/*
 * Transform Type 3 - Integrity Algorithm Transform IDs
 *
 * http://www.iana.org/assignments/ikev2-parameters
 *
 * 0             NONE                                [RFC5996]
 * 1             AUTH_HMAC_MD5_96                    [RFC2403][RFC5996]
 * 2             AUTH_HMAC_SHA1_96                   [RFC2404][RFC5996]
 * 3             AUTH_DES_MAC                        [RFC5996]
 * 4             AUTH_KPDK_MD5                       [RFC5996]
 * 5             AUTH_AES_XCBC_96                    [RFC3566][RFC5996]
 * 6             AUTH_HMAC_MD5_128                   [RFC4595]
 * 7             AUTH_HMAC_SHA1_160                  [RFC4595]
 * 8             AUTH_AES_CMAC_96                    [RFC4494]
 * 9             AUTH_AES_128_GMAC                   [RFC4543]
 * 10            AUTH_AES_192_GMAC                   [RFC4543]
 * 11            AUTH_AES_256_GMAC                   [RFC4543]
 * 12            AUTH_HMAC_SHA2_256_128              [RFC4868]
 * 13            AUTH_HMAC_SHA2_384_192              [RFC4868]
 * 14            AUTH_HMAC_SHA2_512_256              [RFC4868]
 *
 */
class auth_attr_hmac_trans_id : public auth_attr {

public:
	auth_attr_hmac_trans_id();
	auth_attr_hmac_trans_id(uint16 id);
	
	virtual ~auth_attr_hmac_trans_id();
	virtual auth_attr_hmac_trans_id *new_instance() const;
	virtual auth_attr_hmac_trans_id *copy() const;

	virtual bool check() const;
	virtual bool check_body() const;
	virtual bool equals_body(const auth_attr &other) const;
	virtual const char *get_ie_name() const { return ie_name; }
	virtual ostream &print_attributes(ostream &os) const;
	virtual ostream &print(ostream &os, uint32 level, const uint32 indent, const char *name) const;

	virtual size_t get_serialized_size(coding_t coding) const;
	virtual size_t get_serialized_size_nopadding(coding_t coding) const;
	virtual bool deserialize_body(NetMsg &msg, coding_t coding, uint16 body_length, IEErrorList &err, uint32 &bytes_read, bool skip);

	virtual void serialize_body(NetMsg &msg, coding_t coding, uint32 &bytes_written) const;

private:
	const char * ie_name;
	uint16 transId;

public:
	/*
	 * New methods
	 */
	void set_id(uint16 id);
	uint16 get_id() const;

};


/*
 *Constructor
 */
inline
auth_attr_hmac_trans_id::auth_attr_hmac_trans_id() : auth_attr(AUTH_ENT_ID, HMAC_SIGNED),ie_name("HMAC transform ID"), transId(0){
}

} //end namespace auth
#endif
