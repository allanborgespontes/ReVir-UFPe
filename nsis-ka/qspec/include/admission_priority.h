/*
 * admission_priority.h - The Admission Priority Parameter
 *
 * $Id: admission_priority.h 4593 2009-10-20 14:30:51Z roehricht $
 * $HeadURL: https://svn.tm.kit.edu/nsis/qspec/trunk/include/admission_priority.h $
 *
 */
#ifndef QSPEC__ADMISSION_PRIORITY_H
#define QSPEC__ADMISSION_PRIORITY_H

#include "qspec_param.h"


namespace qspec {
  using namespace protlib;


/**
 * All valid QSPEC Admission Priority Parameter values.
 */
enum admission_priority_t {
	ap_best_effort_priority_flow	= 0,
	ap_normal_priority_flow		= 1,
	ap_high_priority_flow		= 2,
	ap_other_admission_priority     = 0xFF
};


/**
 * The QSPEC Admission Priority Parameter.
 */
class admission_priority : public qspec_param {

  public:
	static const uint16 PARAM_ID;

	admission_priority();
	explicit admission_priority(admission_priority_t value);
	explicit admission_priority(uint16 param_id,
			admission_priority_t value);

	virtual ~admission_priority();

	virtual admission_priority *new_instance() const;
	virtual admission_priority *copy() const;

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
	admission_priority_t get_y2171value() const;
	void set_y2171value(admission_priority_t val);

	uint8 get_value() const;
	void set_value(uint8 val);

  private:
	// Disallow assignment for now.
	admission_priority &operator=(const admission_priority &other);

	static const char *const ie_name;

	admission_priority_t y2171_ap_value;
	uint8 ap_value;
};


} // namespace qspec

#endif // QSPEC__ADMISSION_PRIORITY_H
