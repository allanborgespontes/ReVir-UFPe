/*
 * y1541_qos_class.h - The QSPEC Y.1541 QoS Class Parameter
 *
 * $Id: y1541_qos_class.h 4593 2009-10-20 14:30:51Z roehricht $
 * $HeadURL: https://svn.tm.kit.edu/nsis/qspec/trunk/include/y1541_qos_class.h $
 *
 */
#ifndef QSPEC__Y1541_QOS_CLASS_H
#define QSPEC__Y1541_QOS_CLASS_H

#include "qspec_param.h"


namespace qspec {
  using namespace protlib;


/**
 * The QSPEC Y.1541 QoS Class Parameter.
 *
 * It contains an 8 bit Y.1541 QoS Class value.
 */
class y1541_qos_class : public qspec_param {

  public:
	static const uint16 PARAM_ID;

	y1541_qos_class();
	explicit y1541_qos_class(uint8 value);
	explicit y1541_qos_class(uint16 param_id, uint8 value);

	virtual ~y1541_qos_class();

	virtual y1541_qos_class *new_instance() const;
	virtual y1541_qos_class *copy() const;

	virtual bool check_body() const;
	virtual bool equals_body(const qspec_param &other) const;
	virtual const char *get_ie_name() const;
	virtual ostream &print_attributes(ostream &os) const;


	virtual bool deserialize_body(NetMsg &msg, coding_t coding,
			uint16 body_length, IEErrorList &err,
			uint32 &bytes_read, bool skip);

	virtual void serialize_body(NetMsg &msg, coding_t coding,
			uint32 &bytes_written) const;


	/*
	 * New methods
	 */
	uint8 get_value() const;
	void set_value(uint8 val);

  private:
	// Disallow assignment for now.
	y1541_qos_class &operator=(const y1541_qos_class &other);

	static const char *const ie_name;

	uint8 value;
};


} // namespace qspec

#endif // QSPEC__Y1541_QOS_CLASS_H
