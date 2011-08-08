/*
 * Interface for NATFW objects.
 *
 * $Id: natfw_object.h 2896 2008-02-21 00:54:43Z bless $
 * $HeadURL: https://svn.tm.kit.edu/nsis/natfw-nslp/trunk/include/msg/natfw_object.h $
 *
 */
#ifndef NATFW_MSG__NATFW_OBJECT_H
#define NATFW_MSG__NATFW_OBJECT_H

#include "ie.h"


namespace natfw {
  namespace msg {
     using namespace protlib;


/**
 * The representation of a NATFW object.
 *
 * This class provides methods to serialize/deserialize NATFW objects
 * as well as access to various header fields. natfw_object is abstract;
 * all concrete parameters have to be subclasses.
 */
class natfw_object : public IE {

  public:
	enum treatment_t {
		tr_mandatory	= 0,
		tr_optional	= 1,
		tr_forward	= 2,
		tr_reserved	= 3
	};

	virtual ~natfw_object();

	/*
	 * Inherited from IE
	 */
	virtual natfw_object *new_instance() const = 0;
	virtual natfw_object *copy() const = 0;

	virtual const char *get_ie_name() const = 0;
	virtual size_t get_serialized_size(coding_t coding) const = 0;

	virtual IE *deserialize(NetMsg &msg, coding_t coding, IEErrorList &err,
			uint32 &bytes_read, bool skip);

	virtual void serialize(NetMsg &msg, coding_t coding,
			uint32 &bytes_written) const throw (IEError);

	virtual bool operator==(const IE &ie) const;
	virtual bool check() const;
	virtual bool supports_coding(coding_t c) const;
	virtual void register_ie(IEManager *iem) const;


	/*
	 * New methods
	 */
	treatment_t get_treatment() const;
	void set_treatment(treatment_t t);

	uint16 get_object_type() const;
	void set_object_type(uint16 obj_type);

	static uint16 extract_object_type(uint32 header_raw) throw ();

  protected:
	/**
	 * Length of a NATFW Object header in bytes.
	 */
	static const uint16 HEADER_LENGTH = 4;

	/**
	 * The encoding used (necessary for the IE interfaces).
	 */
	static const coding_t CODING = protocol_v1;


	natfw_object();
	natfw_object(uint16 object_type, treatment_t tr);

	virtual bool deserialize_header(NetMsg &msg, uint16 &body_length,
			IEErrorList &err, bool skip);

	virtual void serialize_header(NetMsg &msg, uint16 body_len) const;

	virtual ostream &print_attributes(ostream &os) const;
	virtual ostream &print(ostream &os, uint32 level, const uint32 indent,
			const char *name = NULL) const;

	virtual bool check_body() const = 0;
	virtual bool equals_body(const natfw_object &obj) const = 0;

	virtual bool deserialize_body(NetMsg &msg, uint16 body_length,
			IEErrorList &err, bool skip) = 0;

	virtual void serialize_body(NetMsg &msg) const = 0;

  private:
	uint16 object_type;
	treatment_t treatment;
};


  } // namespace msg
} // namespace natfw


/*
 * Include all known objects for convenience.
 */
#include "session_lifetime.h"
#include "external_address.h"
#include "extended_flow_info.h"
#include "information_code.h"
#include "nonce.h"
#include "msg_sequence_number.h"
#include "data_terminal_info.h"
#include "icmp_types.h"


#endif // NATFW_MSG__NATFW_OBJECT_H
