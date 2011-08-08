/*
 * preemption_priority.h - The QSPEC Preemption/Defending Priority Parameters
 *
 * $Id: preemption_priority.h 4593 2009-10-20 14:30:51Z roehricht $
 * $HeadURL: https://svn.tm.kit.edu/nsis/qspec/trunk/include/preemption_priority.h $
 *
 */
#ifndef QSPEC__PREEMPTION_PRIORITY_H
#define QSPEC__PREEMPTION_PRIORITY_H

#include "qspec_param.h"


namespace qspec {
  using namespace protlib;


/**
 * The QSPEC Preemption/Defending Priority Parameters
 */
class preemption_priority : public qspec_param {

  public:
	static const uint16 PARAM_ID;

	preemption_priority();
	explicit preemption_priority(uint16 preemption_priority,
			uint16 defending_priority);
	explicit preemption_priority(uint16 param_id,
			uint16 preemption_priority, uint16 defending_priority);

	virtual ~preemption_priority();

	virtual preemption_priority *new_instance() const;
	virtual preemption_priority *copy() const;

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
	uint16 get_preemption_priority() const;
	void set_preemption_priority(uint16 val);

	uint16 get_defending_priority() const;
	void set_defending_priority(uint16 val);

  private:
	// Disallow assignment for now.
	preemption_priority &operator=(const preemption_priority &other);

	static const char *const ie_name;

	uint16 preemption_priority_value;
	uint16 defending_priority_value;
};


} // namespace qspec

#endif // QSPEC__PREEMPTION_PRIORITY_H
