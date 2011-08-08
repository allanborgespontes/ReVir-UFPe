/*
 * slack_term.h - The QSPEC Slack Term Parameter
 *
 * $Id: slack_term.h 4593 2009-10-20 14:30:51Z roehricht $
 * $HeadURL: https://svn.tm.kit.edu/nsis/qspec/trunk/include/slack_term.h $
 *
 */
#ifndef QSPEC__SLACK_TERM_H
#define QSPEC__SLACK_TERM_H

#include "qspec_param.h"


namespace qspec {
  using namespace protlib;


/**
 * The QSPEC Slack Term Parameter.
 */
class slack_term : public qspec_param {

  public:
	static const uint16 PARAM_ID;

	slack_term();
	explicit slack_term(uint32 value);
	explicit slack_term(uint16 param_id, uint32 value);

	virtual ~slack_term();

	virtual slack_term *new_instance() const;
	virtual slack_term *copy() const;

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
	uint32 get_value() const;
	void set_value(uint32 val);

  private:
	// Disallow assignment for now.
	slack_term &operator=(const slack_term &other);

	static const char *const ie_name;

	uint32 value;
};


} // namespace qspec

#endif // QSPEC__SLACK_TERM_H
