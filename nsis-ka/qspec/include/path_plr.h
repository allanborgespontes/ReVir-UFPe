/*
 * path_plr.h - The QSPEC Path PLR Parameter
 *
 * $Id: path_plr.h 4593 2009-10-20 14:30:51Z roehricht $
 * $HeadURL: https://svn.tm.kit.edu/nsis/qspec/trunk/include/path_plr.h $
 *
 */
#ifndef QSPEC__PATH_PLR_H
#define QSPEC__PATH_PLR_H

#include "qspec_param.h"


namespace qspec {
  using namespace protlib;


/**
 * The QSPEC Path Packet Loss Ratio (Path PLR) Parameter.
 */
class path_plr : public qspec_param {

  public:
	static const uint16 PARAM_ID;

	path_plr();
	explicit path_plr(float value);
	explicit path_plr(uint16 param_id, float value);

	virtual ~path_plr();

	virtual path_plr *new_instance() const;
	virtual path_plr *copy() const;

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
	path_plr &operator=(const path_plr &other);

	static const char *const ie_name;

	float value;
};


} // namespace qspec

#endif // QSPEC__PATH_PLR_H
