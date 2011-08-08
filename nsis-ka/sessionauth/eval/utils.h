#include "auth_session_object.h"
#include "data.h"
#include "reservemsg.h"

namespace nslp_auth {

auth_session_object* createAuthObj(uint32 keyid);
auth_session_object* createAuthObj();
ntlp::data* create_NTLP_PDU(bool with_sessionauth, uint32 keyid, bool storekey, uint16 qspec_param_count);
ntlp::data* create_NTLP_PDU(bool with_sessionauth);
qos_nslp::reservereq* create_NSLP_PDU(bool with_sessionauth, uint32 keyid, bool storekey, uint16 qspec_param_count);
qos_nslp::reservereq* create_NSLP_PDU(bool with_sessionauth);
void register_NTLP_ies();
void register_auth_attr_ies();

}

