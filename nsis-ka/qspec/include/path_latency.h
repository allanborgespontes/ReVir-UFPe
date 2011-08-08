/*
 * path_latency.h - The QSPEC Path Latency Parameter
 *
 * $Id: path_latency.h 4593 2009-10-20 14:30:51Z roehricht $
 * $HeadURL: https://svn.tm.kit.edu/nsis/qspec/trunk/include/path_latency.h $
 *
 */
#ifndef QSPEC__PATH_LATENCY_H
#define QSPEC__PATH_LATENCY_H

#include "qspec_param.h"


namespace qspec {
  using namespace protlib;


/**
 * The QSPEC Path Latency Parameter.
 */
class path_latency : public qspec_param {

  public:
	static const uint16 PARAM_ID;

	path_latency();
	explicit path_latency(uint32 value);
	explicit path_latency(uint16 param_id, uint32 value);

	virtual ~path_latency();

	virtual path_latency *new_instance() const;
	virtual path_latency *copy() const;

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
	path_latency &operator=(const path_latency &other);

	static const char *const ie_name;

	/**
	 * The path latency in microseconds.
	 */
	uint32 value;
};


} // namespace qspec

#endif // QSPEC__PATH_LATENCY_H
