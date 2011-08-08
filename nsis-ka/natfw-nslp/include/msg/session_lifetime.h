/*
 * The Session Lifetime Object.
 *
 * $Id: session_lifetime.h 2895 2008-02-21 00:42:23Z bless $
 * $HeadURL: https://svn.tm.kit.edu/nsis/natfw-nslp/trunk/include/msg/session_lifetime.h $
 */
#ifndef NATFW_MSG__SESSION_LIFETIME_H
#define NATFW_MSG__SESSION_LIFETIME_H

#include "natfw_object.h"


namespace natfw {
 namespace msg {

    using namespace protlib;


/**
 * The Session Lifetime Object.
 */
class session_lifetime : public natfw_object {

  public:
	static const uint16 OBJECT_TYPE = 0x00F1;

	explicit session_lifetime();
	explicit session_lifetime(uint32 seconds, treatment_t t = tr_mandatory);

	virtual ~session_lifetime();

	virtual session_lifetime *new_instance() const;
	virtual session_lifetime *copy() const;

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
	session_lifetime &operator=(const session_lifetime &other);

	static const char *const ie_name;

	uint32 value;
};


 } // namespace msg
} // namespace natfw

#endif // NATFW_MSG__SESSION_LIFETIME_H
