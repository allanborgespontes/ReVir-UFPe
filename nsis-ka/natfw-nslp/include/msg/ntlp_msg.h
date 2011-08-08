/*
 * An NTLP Message.
 *
 * $Id: ntlp_msg.h 2438 2006-12-08 15:07:34Z stud-matfried $
 * $HeadURL: https://svn.tm.kit.edu/nsis/natfw-nslp/trunk/include/msg/ntlp_msg.h $
 */
#ifndef NATFW_MSG__NTLP_MSG_H
#define NATFW_MSG__NTLP_MSG_H

#include "mri.h"	// from NTLP

#include "natfw_msg.h"
#include "session_id.h"
#include "session.h"


namespace natfw {
  namespace msg {
     using namespace protlib;


/**
 * An NTLP Message.
 *
 * This class encapsulates all the data required for sending a message via
 * the NTLP. It consists of data for the NTLP header, such as addresses,
 * ports, and protocol IDs, and of a NATFW message. The NATFW message
 * represents the NTLP body.
 *
 * Note: We don't inherit from IE because this message will not be serialized
 * or deserialized (only the NATFW message will).
 */
class ntlp_msg {

  public:
	ntlp_msg(const session_id &id, natfw_msg *body,
			ntlp::mri *m, uint32 sii_handle);
	virtual ~ntlp_msg();

	inline session_id get_session_id() const { return sid; }
	inline natfw_msg *get_natfw_msg() const { return msg; }
	inline ntlp::mri *get_mri() const { return routing_info; }
	inline uint32 get_sii_handle() const { return sii_handle; }


	ntlp_msg *copy() const;
	ntlp_msg *copy_for_forwarding(ntlp::mri *new_mri=NULL) const;

	ntlp_msg *create_response(uint8 severity, uint8 response_code) const;
	ntlp_msg *create_error_response(const request_error &e) const;
	ntlp_msg *create_success_response(uint32 session_lifetime) const;

	inline natfw_create *get_natfw_create() const;
	inline natfw_ext *get_natfw_ext() const;
	inline natfw_response *get_natfw_response() const;

  private:
	ntlp_msg(const natfw_msg &other);

	session_id sid;

	ntlp::mri *routing_info;
	uint32 sii_handle;

	// This will be deserialized and used as the NTLP body.
	natfw_msg *msg;
};

std::ostream &operator<<(std::ostream &out, const ntlp_msg &s);

bool is_greater_than(uint32 s1, uint32 s2);


inline natfw_create *ntlp_msg::get_natfw_create() const {
	return dynamic_cast<natfw_create *>(get_natfw_msg());
}

inline natfw_ext *ntlp_msg::get_natfw_ext() const {
	return dynamic_cast<natfw_ext *>(get_natfw_msg());
}

inline natfw_response *ntlp_msg::get_natfw_response() const {
	return dynamic_cast<natfw_response *>(get_natfw_msg());
}


  } // namespace msg
} // namespace natfw

#endif // NATFW_MSG__NTLP_MSG_H
