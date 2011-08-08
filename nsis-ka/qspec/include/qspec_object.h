/*
 * qspec_object.h - QSPEC Object Class (QoS Desired, QoS Reserved etc.)
 *
 * $Id: qspec_object.h 4593 2009-10-20 14:30:51Z roehricht $
 * $HeadURL: https://svn.tm.kit.edu/nsis/qspec/trunk/include/qspec_object.h $
 *
 */
#ifndef QSPEC__QSPEC_OBJECT_H
#define QSPEC__QSPEC_OBJECT_H

#include "ie.h"

#include "ie_store.h"
#include "qspec_param.h"


namespace qspec {
  using namespace protlib;


/**
 * QSPEC Object Types.
 *
 * All valid object types available in QSPEC-T.
 */
enum object_type_t {
	ot_qos_desired		= 0, // read-only
	ot_qos_available	= 1,
	ot_qos_reserved		= 2, 
	ot_minimum_qos		= 3 // read-only
};


/**
 * qspec_object represents a QSPEC object.
 *
 * A qspec_object is a container that wraps a set of qspec_parameter objects.
 * In QSPEC's terminology, QoS-Desired, QoS-Reserved etc. are called
 * objects.
 *
 * This class takes care of reading/writing the object header and all
 * parameters it contains.
 */
class qspec_object : public IE {

  public:
	static const uint16 HEADER_LENGTH;

	qspec_object();
	explicit qspec_object(uint16 obj_id);
	explicit qspec_object(const qspec_object &other);

	virtual ~qspec_object();

	virtual qspec_object *new_instance() const;
	virtual qspec_object *copy() const;

	virtual IE *deserialize(NetMsg &msg, coding_t coding,
			IEErrorList &errorlist, uint32 &bytes_read, bool skip);

	virtual void serialize(NetMsg &msg, coding_t coding,
			uint32 &bytes_written) const throw (IEError);

	virtual bool check() const;
	virtual bool supports_coding(coding_t c) const;
	virtual size_t get_serialized_size(coding_t coding) const;

	virtual bool operator==(const IE &ie) const;
	virtual const char *get_ie_name() const;
	virtual ostream &print(ostream &os, uint32 level, const uint32 indent,
			const char *name = NULL) const;

	virtual void register_ie(IEManager *iem) const;


	/*
	 * New methods
	 */
	virtual uint16 get_object_id() const;
	virtual void set_object_id(uint16 id);

	virtual bool is_read_only() const;
	virtual void set_read_only();

	virtual bool has_error() const;
	virtual void set_error(bool err);

	virtual qspec_param *get_parameter(uint16 id) const;
	virtual size_t get_num_parameters() const;
	virtual void set_parameter(qspec_param *param);
	virtual qspec_param *remove_parameter(uint16 id);

	static uint16 extract_object_type(uint32 header_raw) throw ();

  private:
	// Disallow assignment for now.
	qspec_object &operator=(const qspec_object &other);

	static const char *const ie_name;

	/**
	 * QSPEC object header fields.
	 */
	uint16 object_id; ///< object type
	bool read_only;   ///< read-only object (set depending on object type and message)
      	bool error;       ///< error occured at object level


	/**
	 * Map QSPEC parameter ID to qspec_param.
	 */
	ie_store parameters;
	typedef ie_store::const_iterator param_iter;
};


} // namespace qspec

#endif // QSPEC__QSPEC_OBJECT_H
