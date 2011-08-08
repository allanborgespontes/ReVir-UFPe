/*
 * A NATFW NOTIFY Message.
 *
 * $Id: natfw_notify.h 2895 2008-02-21 00:42:23Z bless $
 * $HeadURL: https://svn.tm.kit.edu/nsis/natfw-nslp/trunk/include/msg/natfw_notify.h $
 */
#ifndef NATFW_MSG__NATFW_NOTIFY_H
#define NATFW_MSG__NATFW_NOTIFY_H

#include "ie.h"

#include "natfw_msg.h"


namespace natfw {
  namespace msg {


/**
 * A NATFW NOTIFY Message.
 *
 * This class implements the NATFW NOTIFY Message, which may be sent
 * asynchronously to report errors.
 */
class natfw_notify : public natfw_msg {

  public:
	static const uint16 MSG_TYPE = 4;

	explicit natfw_notify();
	explicit natfw_notify(const natfw_notify &other);
	virtual ~natfw_notify();

	/*
	 * Inherited methods:
	 */
	natfw_notify *new_instance() const;
	natfw_notify *copy() const;
	void register_ie(IEManager *iem) const;

	/*
	 * New methods:
	 */
	void set_information_code(uint8 severity, uint8 response_code,
		uint16 object_type = 0);
	uint8 get_severity_class() const;
	uint16 get_object_type() const;
};


  } // namespace msg
} // namespace natfw

#endif // NATFW_MSG__NATFW_NOTIFY_H
