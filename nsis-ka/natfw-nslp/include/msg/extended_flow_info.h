/*
 * The Extended Flow Information Object.
 *
 * $Id: extended_flow_info.h 2895 2008-02-21 00:42:23Z bless $
 * $HeadURL: https://svn.tm.kit.edu/nsis/natfw-nslp/trunk/include/msg/extended_flow_info.h $
 */
#ifndef NATFW_MSG__EXTENDED_FLOW_INFO_H
#define NATFW_MSG__EXTENDED_FLOW_INFO_H

#include "natfw_object.h"


namespace natfw {
 namespace msg {

     using namespace protlib;


/**
 * The Extended Flow Information Object.
 */
class extended_flow_info : public natfw_object {

  public:
	static const uint16 OBJECT_TYPE = 0x00F3;

	enum rule_action_t {
		ra_allow	= 1,
		ra_deny		= 2
	};

	explicit extended_flow_info();
	explicit extended_flow_info(uint16 rule_action, uint16 sub_ports = 0,
		treatment_t t = tr_mandatory);

	virtual ~extended_flow_info();

	virtual extended_flow_info *new_instance() const;
	virtual extended_flow_info *copy() const;

	virtual size_t get_serialized_size(coding_t coding) const;
	virtual bool check_body() const;
	virtual bool equals_body(const natfw_object &other) const;
	virtual const char *get_ie_name() const;
	virtual ostream &print_attributes(ostream &os) const;


	virtual bool deserialize_body(NetMsg &msg, uint16 body_length,
			IEErrorList &err, bool skip);

	virtual void serialize_body(NetMsg &msg) const;


	/*
	 * New methods
	 */
	uint16 get_rule_action() const;
	void set_rule_action(uint16 action);

	uint16 get_sub_ports() const;
	void set_sub_ports(uint16 num_ports);

  private:
	// Disallow assignment for now.
	extended_flow_info &operator=(const extended_flow_info &other);

	static const char *const ie_name;

	uint16 rule_action;
	uint16 sub_ports;
};

 } // namespace msg
} // namespace natfw

#endif // NATFW_MSG__EXTENDED_FLOW_INFO_H
