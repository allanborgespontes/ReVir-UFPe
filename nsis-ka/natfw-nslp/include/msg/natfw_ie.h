/*
 * The NATFW IE Manager.
 *
 * $Id: natfw_ie.h 1718 2006-05-04 08:07:01Z stud-matfried $
 * $HeadURL: https://svn.tm.kit.edu/nsis/natfw-nslp/trunk/include/msg/natfw_ie.h $
 */
#ifndef NATFW_MSG__IE_H
#define NATFW_MSG__IE_H

#include "ie.h"

namespace natfw {
 namespace msg {
     using namespace protlib;


/**
 * Categories for NATFW classes.
 */
enum category_t {
	cat_natfw_msg			= 0,
	cat_natfw_object		= 1,

	cat_default_natfw_msg		= 2,
	cat_default_natfw_object	= 3
};


/**
 * An Interface for reading/writing NATFW Messages.
 *
 * The NATFW_IEManager is a Singleton which provides methods to read/write
 * NATFW Messages from/to NetMsg objects. Those methods are called
 * deserialize() and serialize(), respectively.
 *
 * To deserialize() a NATFW Message, each IE to be used during the process
 * has to be registered with NATFW_IEManager using register_ie(). Registered
 * IEs will be freed automatically as soon as either clear() is called or
 * NATFW_IEManager itself is destroyed.
 *
 * The only way to get an NATFW_IEManager object is through the static
 * instance() method.
 */
class NATFW_IEManager : public IEManager {

  public:
	static NATFW_IEManager *instance();
	static void clear();

	static void register_known_ies();

	virtual IE *deserialize(NetMsg &msg, uint16 category,
			IE::coding_t coding, IEErrorList &errorlist,
			uint32 &bytes_read, bool skip);

  protected:
	// protected constructor to prevent instantiation
	NATFW_IEManager();

	virtual IE *lookup_ie(uint16 category, uint16 type, uint16 subtype);

  private:
	static NATFW_IEManager *natfw_inst;

	IE *deserialize_msg(NetMsg &msg,
		IE::coding_t coding, IEErrorList &errorlist,
		uint32 &bytes_read, bool skip);

	IE *deserialize_object(NetMsg &msg,
		IE::coding_t coding, IEErrorList &errorlist,
		uint32 &bytes_read, bool skip);
};


 } // namespace msg
} // namespace natfw

#endif // NATFW_MSG__IE_H
