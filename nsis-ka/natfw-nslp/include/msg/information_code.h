/*
 * The Information Code Object.
 *
 * $Id: information_code.h 2895 2008-02-21 00:42:23Z bless $
 * $HeadURL: https://svn.tm.kit.edu/nsis/natfw-nslp/trunk/include/msg/information_code.h $
 */
#ifndef NATFW_MSG__INFORMATION_CODE_H
#define NATFW_MSG__INFORMATION_CODE_H

#include "natfw_object.h"


namespace natfw {
  namespace msg {

     using namespace protlib;


/**
 * The Information Code Object, which is used in response messages.
 */
class information_code : public natfw_object {

  public:
	static const uint16 OBJECT_TYPE = 0x00F4;

	enum severity_class_t {
		sc_informational		= 1,
		sc_success			= 2,
		sc_protocol_error		= 3,
		sc_transient_failure		= 4,
		sc_permanent_failure		= 5,
		sc_signaling_session_failures	= 6
	};

	enum informational_code_t {
		ic_route_change			= 1,
		ic_reauthentication_required	= 2,
		ic_node_going_down		= 3
	};

	enum success_code_t {
		suc_successfully_processed	= 1
	};

	enum protocol_error_code_t {
		// TODO: add constants
		perr_unknown_object_field_value	= 8
	};

	enum transient_failure_code_t {
		tfail_resources_unavailable	= 1
	};

	enum permanent_failure_code_t {
		fail_authentication_failed	= 1,
		fail_authorization_failed	= 2,
		fail_transport_security		= 3,
		fail_internal_error		= 4,
		fail_no_nat_here		= 5,
		fail_no_edge_device_here	= 6,
		fail_nr_not_reached		= 7,
		fail_tracing_not_allowed	= 8
	};

	enum signaling_session_failures_code_t {
		sigfail_terminated_async		= 1,
		sigfail_lifetime_too_big		= 2,
		sigfail_no_reservation_found		= 3,
		sigfail_policy_rule_denied		= 4,
		sigfail_unknown_policy_rule_action	= 5,
		sigfail_rule_action_not_applicable	= 6,
		sigfail_dtinfo_object_required		= 7,
		sigfail_sub_ports_not_permitted		= 8,
		sigfail_ip_protocol_unsupported		= 9,
		sigfail_plain_ip_not_permitted		= 10,
		sigfail_icmp_type_not_permitted		= 11,
		sigfail_source_ip_range_too_large	= 12,
		sigfail_dest_ip_range_too_large		= 13,
		sigfail_source_port_range_too_large	= 14,
		sigfail_dest_port_range_too_large	= 15
	};

	explicit information_code();
	explicit information_code(uint8 severity, uint8 response_code = 0,
		uint16 object_type = 0, treatment_t t = tr_mandatory);

	virtual ~information_code();

	virtual information_code *new_instance() const;
	virtual information_code *copy() const;

	virtual size_t get_serialized_size(coding_t coding) const;
	virtual bool check_body() const;
	virtual bool equals_body(const natfw_object &other) const;
	virtual const char *get_ie_name() const;
	virtual ostream &print_attributes(ostream &os) const;


	virtual bool deserialize_body(NetMsg &msg, uint16 body_length,
			IEErrorList &err, bool skip);

	virtual void serialize_body(NetMsg &msg) const;


	/*
	 * New methods:
	 */
	uint8 get_severity_class() const;
	void set_severity_class(uint8 severity);

	uint8 get_response_code() const;
	void set_response_code(uint8 code);

	uint16 get_response_object_type() const;
	void set_response_object_type(uint16 object_type);

	bool is_success() const;

  private:
	// Disallow assignment for now.
	information_code &operator=(const information_code &other);

	static const char *const ie_name;

	uint8 severity_class; // 4 bits used
	uint8 response_code;
	uint16 response_object_type; // 12 bits used
};

  } // namespace msg
} // namespace natfw

#endif // NATFW_MSG__INFORMATION_CODE_H
