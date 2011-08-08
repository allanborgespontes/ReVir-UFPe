/*
 * The Nonce Object.
 *
 * $Id: nonce.h 2895 2008-02-21 00:42:23Z bless $
 * $HeadURL: https://svn.tm.kit.edu/nsis/natfw-nslp/trunk/include/msg/nonce.h $
 */
#ifndef NATFW_MSG__NONCE_H
#define NATFW_MSG__NONCE_H

#include "natfw_object.h"


namespace natfw {
 namespace msg {

    using namespace protlib;


/**
 * The Nonce Object.
 *
 * Instances of this class contain a 32 bit nonce.
 *
 * TODO: I think this doesn't have to be cryptographically strong, so probably
 *       a static counter for all nonce objects will be enough. This has to
 *       be thread-safe, so maybe a nonce factory is better?
 */
class nonce : public natfw_object {

  public:
	static const uint16 OBJECT_TYPE = 0x00F5;

	explicit nonce();
	explicit nonce(uint32 value, treatment_t t = tr_mandatory);

	virtual ~nonce();

	virtual nonce *new_instance() const;
	virtual nonce *copy() const;

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
	uint32 get_value() const;
	void set_value(uint32 val);

  private:
	// Disallow assignment for now.
	nonce &operator=(const nonce &other);

	static const char *const ie_name;

	uint32 value;
};


 } // namespace msg
} // namespace natfw

#endif // NATFW_MSG__NONCE_H
