#ifndef HMAC_KEYREGISTRY_H
#define HMAC_KEYREGISTRY_H

#include "protlib_types.h"
#include <string.h>

#include "hashmap"

namespace nslp_auth {

  using namespace protlib;

typedef uint32 key_id_t;

class signkey_context {
 
 public:
 signkey_context() : sign_key(0), sign_keylen(0), hash_algo(0) {};
  
 signkey_context(uchar *sign_key, uint16 sign_keylen, uint16 hash_algo) : sign_key(sign_key), sign_keylen(sign_keylen), hash_algo(hash_algo) {};
  ~signkey_context() { delete sign_key; }

 const uchar* getKey() const { return sign_key; }
 uint16 getKeyLen() const { return sign_keylen; }

 private:
    uchar *sign_key;
    uint16 sign_keylen;
    uint16 hash_algo;
};

class hmac_keyregistry 
{
 public:

  typedef hashmap_t<key_id_t, signkey_context*> keyregistry_t;

  void storeKey(key_id_t key_id, const void* key, uint16 keylen, uint16 mdalgo= default_hash_algo);
  const uchar* getKey(key_id_t key_id) const;
  const signkey_context* getKeyContext(key_id_t key_id) const;
  void deleteKey(key_id_t key_id);

 private:
  keyregistry_t key_registry;

  static const uint16 default_hash_algo;

};

// this is a singleton
extern hmac_keyregistry sign_key_registry;

}

#endif // guardian HMAC_KEYREGISTRY_H
