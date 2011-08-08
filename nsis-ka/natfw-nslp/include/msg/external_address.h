/*
 * The External Address Object.
 *
 * $Id: external_address.h 2895 2008-02-21 00:42:23Z bless $
 * $HeadURL: https://svn.tm.kit.edu/nsis/natfw-nslp/trunk/include/msg/external_address.h $
 */
#ifndef NATFW_MSG__EXTERNAL_ADDRESS_H
#define NATFW_MSG__EXTERNAL_ADDRESS_H

#include "address.h"	// from protlib, for hostaddress

#include "natfw_object.h"


namespace natfw {
 namespace msg {

    using namespace protlib;


/**
 * The External Address Object.
 *
 * This may optionally be part of a RESPONSE message.
 */
class external_address : public natfw_object {

  public:
	static const uint16 OBJECT_TYPE = 0x00F2;

	explicit external_address();
	explicit external_address(hostaddress addr, uint16 port,
			treatment_t t = tr_optional);

	virtual ~external_address();

	virtual external_address *new_instance() const;
	virtual external_address *copy() const;

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
	hostaddress get_address() const;
	void set_address(hostaddress addr);

	uint16 get_port() const;
	void set_port(uint16 val);

  private:
	// Disallow assignment for now.
	external_address &operator=(const external_address &other);

	static const char *const ie_name;

	hostaddress address;
	uint16 port;
};


 } // namespace msg
} // namespace natfw

#endif // NATFW_MSG__EXTERNAL_ADDRESS_H
