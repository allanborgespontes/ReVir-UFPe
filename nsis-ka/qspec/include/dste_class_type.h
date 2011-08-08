/*
 * dste_class_type.h - The QSPEC DSTE Class Type Parameter
 *
 * $Id: dste_class_type.h 4593 2009-10-20 14:30:51Z roehricht $
 * $HeadURL: https://svn.tm.kit.edu/nsis/qspec/trunk/include/dste_class_type.h $
 *
 */
#ifndef QSPEC__DSTE_CLASS_TYPE_H
#define QSPEC__DSTE_CLASS_TYPE_H

#include "qspec_param.h"


namespace qspec {
  using namespace protlib;


/**
 * The QSPEC DSTE Class Type Parameter.
 *
 * It contains an 8 bit DSTE Class Type value.
 */
class dste_class_type : public qspec_param {

  public:
	static const uint16 PARAM_ID;

	dste_class_type();
	explicit dste_class_type(uint8 value);
	explicit dste_class_type(uint16 param_id, uint8 value);

	virtual ~dste_class_type();

	virtual dste_class_type *new_instance() const;
	virtual dste_class_type *copy() const;

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
	dste_class_type &operator=(const dste_class_type &other);

	static const char *const ie_name;

	uint8 value;
};


} // namespace qspec

#endif // QSPEC__DSTE_CLASS_TYPE_H
