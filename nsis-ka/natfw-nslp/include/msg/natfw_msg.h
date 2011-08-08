/*
 * A NATFW Message.
 *
 * $Id: natfw_msg.h 2438 2006-12-08 15:07:34Z stud-matfried $
 * $HeadURL: https://svn.tm.kit.edu/nsis/natfw-nslp/trunk/include/msg/natfw_msg.h $
 *
 */
#ifndef NATFW_MSG__NATFW_MSG_H
#define NATFW_MSG__NATFW_MSG_H

#include "ie.h"

#include "ie_store.h"
#include "natfw_object.h"


namespace natfw {
  namespace msg {
     using namespace protlib;


/**
 * A NATFW Message.
 *
 * This class implements a NATFW Message. It can read and initialize itself
 * from or write itself into a NetMsg object using deserialize()/serialize(),
 * respectively.
 */
class natfw_msg : public IE {

  public:
	virtual ~natfw_msg();

	/*
	 * Inherited from IE:
	 */
	virtual natfw_msg *new_instance() const;
	virtual natfw_msg *copy() const;

	virtual natfw_msg *deserialize(NetMsg &msg, coding_t coding,
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
	 * New methods:
	 */
	virtual uint8 get_msg_type() const;
	virtual bool has_msg_sequence_number() const;
	virtual uint32 get_msg_sequence_number() const;

	// TODO: move to CREATE and EXT. not allowed for others
	virtual bool is_proxy_mode() const;
	virtual void set_proxy_mode(bool proxy_mode);

	static uint8 extract_msg_type(uint32 header_raw) throw ();

  protected:
	static const uint16 HEADER_LENGTH;

	// protected constructors to prevent instantiation
	explicit natfw_msg();
	explicit natfw_msg(uint8 msg_type, bool proxy_mode);
	explicit natfw_msg(const natfw_msg &other);

	virtual size_t get_num_objects() const;
	virtual natfw_object *get_object(uint16 object_type) const;
	virtual void set_object(natfw_object *obj);
	virtual natfw_object *remove_object(uint16 object_type);

	virtual void set_msg_type(uint8 mt);

  private:
	static const char *const ie_name;

	/**
	 * NATFW Message header fields.
	 */
	uint8 msg_type;
	bool proxy_mode_flag;

	/**
	 * Map NATFW Object Type to natfw_object.
	 */
	ie_store objects;
	typedef ie_store::const_iterator obj_iter;
};

  } // namespace msg
} // namespace natfw


/*
 * Include all known messages for convenience.
 */
#include "natfw_create.h"
#include "natfw_ext.h"
#include "natfw_response.h"
#include "natfw_notify.h"

#endif // NATFW_MSG__NATFW_MSG_H
