/*
 * excess_treatment.h - The Excess Treatment Parameter
 *
 * $Id: excess_treatment.h 4593 2009-10-20 14:30:51Z roehricht $
 * $HeadURL: https://svn.tm.kit.edu/nsis/qspec/trunk/include/excess_treatment.h $
 *
 */
#ifndef QSPEC__EXCESS_TREATMENT_H
#define QSPEC__EXCESS_TREATMENT_H

#include "qspec_param.h"


namespace qspec {
  using namespace protlib;


/**
 * All valid QSPEC Excess Treatment Parameter values.
 */
enum excess_treatment_t {
	et_drop		= 0,
	et_shape	= 1,
	et_remark	= 2,
	et_no_metering	= 3
};


/**
 * The QSPEC Excess Treatment Parameter.
 */
class excess_treatment : public qspec_param {

  public:
	static const uint16 PARAM_ID;

	excess_treatment();
	explicit excess_treatment(excess_treatment_t value, uint8 remark= 0);
	explicit excess_treatment(uint16 param_id, excess_treatment_t value, uint8 remark= 0);

	virtual ~excess_treatment();

	virtual excess_treatment *new_instance() const;
	virtual excess_treatment *copy() const;

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
	excess_treatment_t get_value() const;
	void set_value(excess_treatment_t val);

	uint8 get_remark_value() const;
	void set_remark_value(uint8 val);

  private:
	// Disallow assignment for now.
	excess_treatment &operator=(const excess_treatment &other);

	static const char *const ie_name;

	excess_treatment_t value; // action
	uint8 remark_to_value; // 6 bits DSCP value
};


} // namespace qspec

#endif // QSPEC__EXCESS_TREATMENT_H
