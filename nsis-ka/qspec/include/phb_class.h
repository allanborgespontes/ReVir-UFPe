/*
 * phb_class.h - The QSPEC PHB Class Parameter
 *
 * $Id: phb_class.h 4593 2009-10-20 14:30:51Z roehricht $
 * $HeadURL: https://svn.tm.kit.edu/nsis/qspec/trunk/include/phb_class.h $
 *
 */
#ifndef QSPEC__PHB_CLASS_H
#define QSPEC__PHB_CLASS_H

#include "qspec_param.h"


namespace qspec {
  using namespace protlib;


/**
 * The QSPEC PHB Class Parameter.
 *
 * It contains a 6 bit DiffServ Code Point (DSCP).
 */
class phb_class : public qspec_param {

  public:
	static const uint16 PARAM_ID;

	phb_class();
	explicit phb_class(uint8 dscp_value);
	explicit phb_class(uint16 phb_id);
	explicit phb_class(uint16 param_id, uint8 dscp_value);
	explicit phb_class(uint16 param_id, uint16 phb_id);

	virtual ~phb_class();

	virtual phb_class *new_instance() const;
	virtual phb_class *copy() const;

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
	uint8 get_dscp_value() const;
	void set_dscp_value(uint8 val);

	bool is_dscp_set() const;
	void set_dscp_set(uint8 val);

	uint16 get_phb_id() const;
	void set_phb_id(uint16 phb_id);

	void set_phb_id_set(uint16 phb_id);
	bool is_phb_id_set() const;

  private:
	// Disallow assignment for now.
	phb_class &operator=(const phb_class &other);

	static const char *const ie_name;
	/**
	 *
	 *   0   1   2   3   4   5   6   7   8   9  10  11  12  13  14  15
	 * +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
	 * |         DSCP          | 0   0   0   0   0   0   0   0   X   0 |
	 * +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
	 * X = denotes a set of values if set
	 *
	 *   0   1   2   3   4   5   6   7   8   9  10  11  12  13  14  15
	 * +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
	 * |                      PHB id code              | 0   0   X   1 |
	 * +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
	 *
	 */

	uint16 phb_id_field; // the field according to RFC 3140
};


} // namespace qspec

#endif // QSPEC__PHB_CLASS_H
