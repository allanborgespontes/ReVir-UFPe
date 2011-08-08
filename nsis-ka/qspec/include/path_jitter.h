/*
 * path_jitter.h - The QSPEC Path Jitter Parameter
 *
 * $Id: path_jitter.h 4593 2009-10-20 14:30:51Z roehricht $
 * $HeadURL: https://svn.tm.kit.edu/nsis/qspec/trunk/include/path_jitter.h $
 *
 */
#ifndef QSPEC__PATH_JITTER_H
#define QSPEC__PATH_JITTER_H

#include "qspec_param.h"


namespace qspec {
  using namespace protlib;


/**
 * The QSPEC Path Jitter Parameter.
 */
class path_jitter : public qspec_param {

  public:
	static const uint16 PARAM_ID;

	path_jitter();
	explicit path_jitter(uint32 value1, uint32 value2, uint32 value3, uint32 value4= 0);
	explicit path_jitter(uint16 param_id,
			     uint32 value1, uint32 value2, uint32 value3, uint32 value4= 0);

	virtual ~path_jitter();

	virtual path_jitter *new_instance() const;
	virtual path_jitter *copy() const;

	virtual bool check_body() const;
	virtual bool equals_body(const qspec_param &other) const;
	virtual const char *get_ie_name() const;
	virtual ostream &print_attributes(ostream &os) const;

	virtual size_t get_serialized_size(coding_t coding) const;

	virtual bool deserialize_body(NetMsg &msg, coding_t coding,
			uint16 body_length, IEErrorList &err,
			uint32 &bytes_read, bool skip);

	virtual void serialize_body(NetMsg &msg, coding_t coding,
			uint32 &bytes_written) const;


	/*
	 * New methods
	 */
	uint32 get_value1() const;
	void set_value1(uint32 val);

	uint32 get_value2() const;
	void set_value2(uint32 val);

	uint32 get_value3() const;
	void set_value3(uint32 val);

	uint32 get_value4() const;
	void set_value4(uint32 val);

  private:
	// Disallow assignment for now.
	path_jitter &operator=(const path_jitter &other);

	static const char *const ie_name;

	/**
	 * The path jitter in microseconds.
	 */
	uint32 values[4];
};


} // namespace qspec

#endif // QSPEC__PATH_JITTER_H
