/*
 * qspec_param.h - Interface for QSPEC parameters
 *
 * $Id: qspec_param.h 4593 2009-10-20 14:30:51Z roehricht $
 * $HeadURL: https://svn.tm.kit.edu/nsis/qspec/trunk/include/qspec_param.h $
 *
 */
#ifndef QSPEC__QSPEC_PARAM_H
#define QSPEC__QSPEC_PARAM_H

#include "ie.h"


namespace qspec {
  using namespace protlib;


/**
 * Tool to convert float to uint32 values and vice versa.
 */
union converter_t {
	float val_float;
	uint32 val_uint;
};


void encode(NetMsg &msg, float f);
float decode_float(NetMsg &msg);

bool is_non_negative_finite(float f);


/**
 * qspec_param represents a QSPEC parameter.
 *
 * This class provides methods to serialize/deserialize QSPEC parameters
 * as well as access to various header fields. qspec_param is abstract;
 * all concrete parameters have to be subclasses.
 *
 * qspec_param provides default implementations that are appropriate for
 * most QSPEC parameters, but not for all. Parameters have to override
 * get_serialized_size(), for example, if they differ in size.
 */
class qspec_param : public IE {

  public:
	static const uint16 HEADER_LENGTH;
	static const uint16 DEFAULT_BODY_LENGTH;

	qspec_param();
	qspec_param(uint16 param_id);

	virtual ~qspec_param();

	/*
	 * Inherited from IE
	 */
	virtual qspec_param *new_instance() const = 0;
	virtual qspec_param *copy() const = 0;

	virtual const char *get_ie_name() const = 0;

	virtual IE *deserialize(NetMsg &msg, coding_t coding, IEErrorList &err,
			uint32 &bytes_read, bool skip);

	virtual void serialize(NetMsg &msg, coding_t coding,
			uint32 &bytes_written) const throw (IEError);

	virtual bool operator==(const IE &ie) const;
	virtual bool check() const;
	virtual size_t get_serialized_size(coding_t coding) const;
	virtual bool supports_coding(coding_t c) const;
	virtual void register_ie(IEManager *iem) const;


	/*
	 * New methods
	 */
	bool is_mandatory() const;
	void set_mandatory(bool m);

	bool has_error() const;
	void set_error(bool e);

	bool is_not_supported() const;
	void set_not_supported(bool n);

	uint16 get_parameter_id() const;
	void set_parameter_id(uint16 id);

	static uint16 extract_parameter_id(uint32 header_raw) throw ();

  protected:
	virtual bool deserialize_header(NetMsg &msg, coding_t coding,
			uint16 &body_length, IEErrorList &err,
			uint32 &bytes_read, bool skip);

	virtual void serialize_header(NetMsg &msg, coding_t coding,
			uint32 &bytes_written, uint16 body_len) const;

	virtual ostream &print_attributes(ostream &os) const;
	virtual ostream &print(ostream &os, uint32 level, const uint32 indent,
			const char *name = NULL) const;

	virtual bool check_body() const = 0;
	virtual bool equals_body(const qspec_param &param) const = 0;

	virtual bool deserialize_body(NetMsg &msg, coding_t coding,
			uint16 body_length, IEErrorList &err,
			uint32 &bytes_read, bool skip) = 0;

	virtual void serialize_body(NetMsg &msg, coding_t coding,
			uint32 &bytes_written) const = 0;

  private:
	uint16 parameter_id;
	bool mandatory;
	bool error;
	bool not_supported;
};


} // namespace qspec


/*
 * Include all known parameters for convenience.
 *
 */
#include "qspec_param_raw.h"
#include "t_mod.h"
#include "path_latency.h"
#include "path_jitter.h"
#include "path_plr.h"
#include "path_per.h"
#include "slack_term.h"
#include "preemption_priority.h"
#include "admission_priority.h"
#include "rph_priority.h"
#include "excess_treatment.h"
#include "phb_class.h"
#include "dste_class_type.h"
#include "y1541_qos_class.h"

#endif // QSPEC__QSPEC_PARAM_H
