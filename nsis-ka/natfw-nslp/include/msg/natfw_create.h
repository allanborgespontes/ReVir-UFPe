/*
 * A NATFW CREATE Message.
 *
 * $Id: natfw_create.h 2896 2008-02-21 00:54:43Z bless $
 * $HeadURL: https://svn.tm.kit.edu/nsis/natfw-nslp/trunk/include/msg/natfw_create.h $
 */
#ifndef NATFW_MSG__NATFW_CREATE_H
#define NATFW_MSG__NATFW_CREATE_H

#include "ie.h"

#include "natfw_msg.h"


namespace natfw {
  namespace msg {


/**
 * A NATFW CREATE Message.
 *
 * This class implements a NATFW CREATE Message.
 */
class natfw_create : public natfw_msg {

  public:
	static const uint16 MSG_TYPE = 1;

	explicit natfw_create(bool proxy_mode=false);
	explicit natfw_create(const natfw_create &other);
	virtual ~natfw_create();

	/*
	 * Inherited methods:
	 */
	natfw_create *new_instance() const;
	natfw_create *copy() const;
	void register_ie(IEManager *iem) const;

	/*
	 * New methods:
	 */
	void set_session_lifetime(uint32 seconds);
	uint32 get_session_lifetime() const;

	void set_extended_flow_info(extended_flow_info::rule_action_t action,
		uint16 sub_ports = 0);
	extended_flow_info::rule_action_t get_rule_action() const;
	uint16 get_subsequent_ports() const;

	void set_msg_sequence_number(uint32 msn);
	
	void set_nonce(uint32 value); // optional; mandatory for proxy mode
	void set_nonce();
	uint32 get_nonce() const;

	void set_icmp_types(const std::list<uint8> &types); // optional
};


  } // namespace msg
} // namespace natfw

#endif // NATFW_MSG__NATFW_CREATE_H
