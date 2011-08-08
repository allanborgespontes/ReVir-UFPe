#include "hmac_keyregistry.h"
#include "session_auth_object.h"

namespace nslp_auth {

// global repository holding signing keys
hmac_keyregistry sign_key_registry;

const uint16 hmac_keyregistry::default_hash_algo= session_auth_object::TRANS_ID_AUTH_HMAC_SHA1_96;

/**
 * this function stores or updates an existing key
 *
 */
void
hmac_keyregistry::storeKey(key_id_t key_id, const void* key, uint16 keylen, uint16 algo)
{
  // if key already found, delete the key
  deleteKey(key_id);
  // store key under key id
  uchar* keydata= new uchar[keylen];
  memcpy(keydata, key, keylen);

  signkey_context* keycontext_p= new signkey_context(keydata, keylen, algo);
  key_registry[key_id]= keycontext_p;
}


const signkey_context*
hmac_keyregistry::getKeyContext(key_id_t key_id) const
{
  keyregistry_t::const_iterator hm_iter;

  hm_iter= key_registry.find(key_id);
  if ( hm_iter != key_registry.end() )
  {
    return (*hm_iter).second;
  }
  else // not found
    return NULL;
}


const uchar* 
hmac_keyregistry::getKey(key_id_t key_id) const
{
  keyregistry_t::const_iterator hm_iter;

  hm_iter= key_registry.find(key_id);
  if ( hm_iter != key_registry.end() && ((*hm_iter).second) != NULL )
  {    
    return ((*hm_iter).second)->getKey();
  }
  else // not found
    return NULL;
}


void
hmac_keyregistry::deleteKey(key_id_t key_id)
{
  keyregistry_t::iterator hm_iter;

  hm_iter= key_registry.find(key_id);
  if ( hm_iter != key_registry.end() )
  {
    delete (*hm_iter).second;
    (*hm_iter).second= NULL;
  }  
}

} // end namespace
