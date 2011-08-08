/*
 * A NATFW EXT Message.
 *
 * $Id: natfw_ext.h 2896 2008-02-21 00:54:43Z bless $
 * $HeadURL: https://svn.tm.kit.edu/nsis/natfw-nslp/trunk/include/msg/natfw_ext.h $
 */
#ifndef NATFW_MSG__NATFW_EXT_H
#define NATFW_MSG__NATFW_EXT_H

#include "ie.h"

#include "natfw_msg.h"


namespace natfw {
  namespace msg {


/**
 * A NATFW EXT Message.
 *
 * This class implements a NATFW EXT Message.
 */
class natfw_ext : public natfw_msg {

  public:
	static const uint16 MSG_TYPE = 2;

	explicit natfw_ext(bool proxy_mode=false);
	explicit natfw_ext(const natfw_ext &other);
	virtual ~natfw_ext();

	/*
	 * Inherited methods:
	 */
	natfw_ext *new_instance() const;
	natfw_ext *copy() const;
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


	void set_data_terminal_info(hostaddress address, uint8 prefix);

	void set_data_terminal_info(uint8 protocol,
		uint16 dr_port, uint16 ds_port);

	/*
	void set_data_terminal_info(hostaddress address, uint8 prefix,
		uint8 protocol);
	void set_data_terminal_info(hostaddress address, uint8 prefix,
		uint8 protocol, uint16 dest_port, uint16 src_port);
	void set_data_terminal_info(hostaddress address, uint8 prefix,
		uint8 protocol, uint32 ipsec_spi);
	*/

	hostaddress get_ds_address() const;
	uint16 get_ds_port() const;
	uint16 get_dr_port() const;
	uint16 get_protocol() const;
};


  } // namespace msg
} // namespace natfw

#endif // NATFW_MSG__NATFW_EXT_H
