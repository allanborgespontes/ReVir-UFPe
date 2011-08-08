/*
 * path_per.h - The QSPEC Path PER Parameter
 *
 * $Id: path_per.h 4593 2009-10-20 14:30:51Z roehricht $
 * $HeadURL: https://svn.tm.kit.edu/nsis/qspec/trunk/include/path_per.h $
 *
 */
#ifndef QSPEC__PATH_PER_H
#define QSPEC__PATH_PER_H

#include "qspec_param.h"


namespace qspec {
  using namespace protlib;


/**
 * The QSPEC Path Packet Error Rate (Path PER) Parameter.
 */
class path_per : public qspec_param {

  public:
	static const uint16 PARAM_ID;

	path_per();
	explicit path_per(float value);
	explicit path_per(uint16 param_id, float value);

	virtual ~path_per();

	virtual path_per *new_instance() const;
	virtual path_per *copy() const;

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
	float get_value() const;
	void set_value(float val);

  private:
	// Disallow assignment for now.
	path_per &operator=(const path_per &other);

	static const char *const ie_name;

	float value;
};


} // namespace qspec

#endif // QSPEC__PATH_PER_H
