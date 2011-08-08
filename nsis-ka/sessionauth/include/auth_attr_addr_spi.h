#ifndef AUTH_ATTR_ADDR_SPI_H
#define AUTH_ATTR_ADDR_SPI_H

#include "auth_attr.h"
#include "auth_attr_addr.h"


namespace nslp_auth{
	using namespace protlib;


class auth_attr_addr_spi : public auth_attr{

public:
	inline auth_attr_addr_spi();
	auth_attr_addr_spi(uint8 xtype, uint8 subtype);
	
	virtual ~auth_attr_addr_spi();
	virtual auth_attr_addr_spi *new_instance() const;
	virtual auth_attr_addr_spi *copy() const;

	virtual bool check() const { return check_body(); }
	virtual bool check_body() const;
	virtual bool equals_body(const auth_attr &other) const;
	virtual const char *get_ie_name() const { return ie_name; }
	virtual ostream &print_attributes(ostream &os) const;
	virtual ostream &print(ostream &os, uint32 level, const uint32 indent, const char *name) const;

	virtual size_t get_serialized_size(coding_t coding) const;
	virtual size_t get_serialized_size_nopadding(coding_t coding) const;
	virtual bool deserialize_body(NetMsg &msg, coding_t coding, uint16 body_length, IEErrorList &err, uint32 &bytes_read, bool skip);

	virtual void serialize_body(NetMsg &msg, coding_t coding, uint32 &bytes_written) const;

private:
// Disallow assignment for now.
	static const char *const ie_name;
	uint32 spi;
public:
	/*
	 * New methods
	 */
	uint32 get_spi() const;
	void set_spi(uint32 spi);
};

/*
*Constuructor
*/
inline
auth_attr_addr_spi::auth_attr_addr_spi() : auth_attr(SOURCE_ADDR, SPI), spi(0){
}


}//end namespace auth
#endif
