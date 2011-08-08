/*
 * The Data Terminal Information Object.
 *
 * $Id: data_terminal_info.h 2895 2008-02-21 00:42:23Z bless $
 * $HeadURL: https://svn.tm.kit.edu/nsis/natfw-nslp/trunk/include/msg/data_terminal_info.h $
 */
#ifndef NATFW_MSG__DATA_TERMINAL_INFO_H
#define NATFW_MSG__DATA_TERMINAL_INFO_H

#include "address.h"	// from protlib, for hostaddress

#include "natfw_object.h"


namespace natfw {
  namespace msg {

    using namespace protlib;


/**
 * The Data Terminal Information Object.
 *
 * The content of this object depends on the flags. If a flag is not set, the
 * corresponding attribute is undefined and may not be used.
 * 
 * Only the sender address and the address prefix are always present.
 */
class data_terminal_info : public natfw_object {

  public:
	static const uint16 OBJECT_TYPE = 0x00F7;

	explicit data_terminal_info();
	explicit data_terminal_info(hostaddress address, uint8 prefix = 32,
			treatment_t t = tr_optional);

	virtual ~data_terminal_info();

	virtual data_terminal_info *new_instance() const;
	virtual data_terminal_info *copy() const;

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

	uint8 get_prefix() const;
	void set_prefix(uint8 length);

	bool has_protocol() const;
	uint8 get_protocol() const;
	void set_protocol(uint8 value);

	bool has_port_numbers() const;
	uint16 get_source_port() const;
	void set_source_port(uint16 port);
	uint16 get_destination_port() const;
	void set_destination_port(uint16 port);

	bool has_ipsec_spi() const;
	uint32 get_ipsec_spi() const;
	void set_ipsec_spi(uint32 value);

  private:
	static const char *const ie_name;

	// always present
	hostaddress sender_address;
	uint8 sender_prefix;

	// the following are all optional
	bool protocol_present;
	uint8 protocol;

	bool port_numbers_present;
	uint16 dest_port, src_port;

	bool ipsec_spi_present;
	uint32 ipsec_spi;
};


  } // namespace msg
} // namespace natfw

#endif // NATFW_MSG__DATA_TERMINAL_INFO_H
