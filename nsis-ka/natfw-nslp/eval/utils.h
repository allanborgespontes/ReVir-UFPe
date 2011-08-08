/*
 * utils.h - Helper functions
 *
 * $Id: utils.h 2348 2006-11-20 12:39:09Z stud-matfried $
 * $HeadURL: https://svn.tm.kit.edu/nsis/natfw-nslp/trunk/eval/utils.h $
 *
 */
#include "network_message.h"
#include "apimessage.h"
#include "dispatcher.h"


namespace natfw {

template<class T> inline T min(T a, T b) {
	return ( a < b ) ? a : b;
}

protlib::NetMsg *build_create_msg(uint32 msn=0xdeadbeef, uint32 lifetime=30);
protlib::NetMsg *build_response_msg();
ntlp::APIMsg *build_api_create_msg(const appladdress &nr, bool final_hop);
ntlp::APIMsg *build_api_response_msg(const appladdress &nr, bool final_hop);
ntlp::APIMsg *build_api_teardown_msg(const appladdress &nr, bool final_hop);


class nop_dispatcher : public dispatcher {
  private:
	gistka_mapper mapper;

  public:
	nop_dispatcher(session_manager *m, nat_manager *n,
			policy_rule_installer *p, natfw_config *c)
		: dispatcher(m, n, p, c) { }

	void send_message(msg::ntlp_msg *msg) throw ();

	id_t start_timer(const session *s, int seconds) throw ();
};


}

// EOF
