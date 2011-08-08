/*
 * A NATFW RESPONSE Message.
 *
 * $Id: natfw_response.h 2895 2008-02-21 00:42:23Z bless $
 * $HeadURL: https://svn.tm.kit.edu/nsis/natfw-nslp/trunk/include/msg/natfw_response.h $
 */
#ifndef NATFW_MSG__NATFW_RESPONSE_H
#define NATFW_MSG__NATFW_RESPONSE_H

#include "ie.h"

#include "natfw_msg.h"


namespace natfw {
  namespace msg {


class natfw_create; // forward declaration
class natfw_ext; // forward declaration

/**
 * A NATFW RESPONSE Message.
 *
 * This class implements the NATFW RESPONSE Message.
 */
class natfw_response : public natfw_msg {

  public:
	static const uint16 MSG_TYPE = 3;

	natfw_response();
	natfw_response(const natfw_response &other);
	natfw_response(natfw_create *c, uint8 severity, uint8 response_code);
	natfw_response(natfw_ext *ext, uint8 severity, uint8 response_code);
	virtual ~natfw_response();

	/*
	 * Inherited methods:
	 */
	natfw_response *new_instance() const;
	natfw_response *copy() const;
	void register_ie(IEManager *iem) const;

	/*
	 * New methods:
	 */
	void set_session_lifetime(uint32 seconds);
	uint32 get_session_lifetime() const;

	void set_msg_sequence_number(uint32 msn);

	void set_information_code(uint8 severity, uint8 response_code,
		uint16 object_type = 0);
	uint8 get_severity_class() const;
	uint16 get_response_object_type() const;

	void set_external_address(hostaddress addr, uint16 port);
	hostaddress get_external_address() const;
	uint16 get_external_port() const;

	bool is_success() const;

	bool is_response_to(const natfw_create *msg) const;
	bool is_response_to(const natfw_ext *msg) const;
};


  } // namespace msg
} // namespace natfw

#endif // NATFW_MSG__NATFW_RESPONSE_H
