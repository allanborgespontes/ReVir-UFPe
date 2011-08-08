/*
 * qspec_param_raw.h - A fallback parameter, accepting anything.
 *
 * $Id: qspec_param_raw.h 4593 2009-10-20 14:30:51Z roehricht $
 * $HeadURL: https://svn.tm.kit.edu/nsis/qspec/trunk/include/qspec_param_raw.h $
 *
 */
#ifndef QSPEC__QSPEC_PARAM_RAW_H
#define QSPEC__QSPEC_PARAM_RAW_H

#include "qspec_param.h"


namespace qspec {
  using namespace protlib;


/**
 * A fallback parameter.
 *
 * This parameter can be used for unknown QSPEC parameters. It parses the
 * header but treats the body as raw data. The parameter may be serialized,
 * but there is no way to modify the raw data it contains.
 *
 * When registered using register_ie(), it registers itself in category
 * cat_default_qspec_param, as a fallback if no matching parameter was found.
 */
class qspec_param_raw : public qspec_param {

  public:
	qspec_param_raw();
	virtual ~qspec_param_raw();

	virtual qspec_param_raw *new_instance() const;
	virtual qspec_param_raw *copy() const;

	virtual bool check_body() const;
	virtual bool equals_body(const qspec_param &other) const;
	virtual const char *get_ie_name() const;
	virtual ostream &print_attributes(ostream &os) const;
	virtual void register_ie(IEManager *iem) const;

	virtual size_t get_serialized_size(coding_t coding) const;

	virtual bool deserialize_header(NetMsg &msg, coding_t coding,
			uint16 &body_length, IEErrorList &errorlist,
			uint32 &bytes_read, bool skip);

	virtual bool deserialize_body(NetMsg &msg, coding_t coding,
			uint16 body_length, IEErrorList &err,
			uint32 &bytes_read, bool skip);

	virtual void serialize_body(NetMsg &msg, coding_t coding,
			uint32 &bytes_written) const;


	/*
	 * New methods
	 */
	virtual uchar *get_data() const;
	virtual size_t get_data_length() const;

  private:
	// Disallow assignment for now.
	qspec_param_raw &operator=(const qspec_param_raw &other);

	static const char *const ie_name;

	uchar *data;
	size_t data_length;
};


} // namespace qspec

#endif // QSPEC__QSPEC_PARAM_RAW_H
