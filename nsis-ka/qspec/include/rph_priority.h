/*
 * rph_priority.h - The QSPEC RPH Priority Parameter
 *
 * $Id: rph_priority.h 4593 2009-10-20 14:30:51Z roehricht $
 * $HeadURL: https://svn.tm.kit.edu/nsis/qspec/trunk/include/rph_priority.h $
 *
 */
#ifndef QSPEC__RPH_PRIORITY_H
#define QSPEC__RPH_PRIORITY_H

#include "qspec_param.h"


namespace qspec {
  using namespace protlib;


/**
 * Symbolic names for the RPN Namespace field.
 */
enum rpn_namespace_t {
	rn_dsn			= 0,
	rn_drsn			= 1,
	rn_q735			= 2,
	rn_ets			= 3,
	rn_wps			= 4,
	rn_not_populated	= 5
};


/**
 * Symbolic names for the RPN priority field if the RPN Namespace field
 * is set to rn_dsn.
 */
enum dsn_priority_t {
	dsn_flash_override	= 0,
	dsn_flash		= 1,
	dsn_immediate		= 2,
	dsn_priority		= 3,
	dsn_routine		= 4
};


/**
 * Symbolic names for the RPN priority field if the RPN Namespace field
 * is set to rn_drsn.
 */
enum drsn_priority_t {
	drsn_flash_override_override	= 0,
	drsn_flash_override		= 1,
	drsn_flash			= 2,
	drsn_immediate			= 3,
	drsn_priority			= 4,
	drsn_routine			= 5
};


// Note: No enums for q735, ets and wps required.


/**
 * The QSPEC RPH Priority Parameter
 */
class rph_priority : public qspec_param {

  public:
	static const uint16 PARAM_ID;

	rph_priority();
	explicit rph_priority(uint16 rpn_namespace, uint8 rpn_priority);
	explicit rph_priority(uint16 param_id,
			uint16 rpn_namespace, uint8 rpn_priority);

	virtual ~rph_priority();

	virtual rph_priority *new_instance() const;
	virtual rph_priority *copy() const;

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
	uint16 get_rpn_namespace() const;
	void set_rpn_namespace(uint16 val);

	uint8 get_rpn_priority() const;
	void set_rpn_priority(uint8 val);

  private:
	// Disallow assignment for now.
	rph_priority &operator=(const rph_priority &other);

	static const char *const ie_name;

	uint16 rpn_namespace;
	uint8 rpn_priority;
};


} // namespace qspec

#endif // QSPEC__RPH_PRIORITY_H
