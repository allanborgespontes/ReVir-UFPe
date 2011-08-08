/*
 * The ICMP Types Object.
 *
 * $Id: icmp_types.h 2895 2008-02-21 00:42:23Z bless $
 * $HeadURL: https://svn.tm.kit.edu/nsis/natfw-nslp/trunk/include/msg/icmp_types.h $
 */
#ifndef NATFW_MSG__ICMP_TYPES_H
#define NATFW_MSG__ICMP_TYPES_H

#include <list>

#include "natfw_object.h"


namespace natfw {
  namespace msg {

    using namespace protlib;


/**
 * The ICMP Types Object.
 */
class icmp_types : public natfw_object {

  public:
	static const uint16 OBJECT_TYPE = 0x00F9;

	explicit icmp_types();
	explicit icmp_types(const std::list<uint8> &types,
			treatment_t t = tr_optional);

	virtual ~icmp_types();

	virtual icmp_types *new_instance() const;
	virtual icmp_types *copy() const;

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
	const std::list<uint8> &get_icmp_types() const;

	typedef std::list<uint8>::const_iterator const_iterator;

  private:
	// Disallow assignment for now.
	icmp_types &operator=(const icmp_types &other);

	static const char *const ie_name;

	std::list<uint8> types;
};


  } // namespace msg
} // namespace natfw

#endif // NATFW_MSG__ICMP_TYPES_H
