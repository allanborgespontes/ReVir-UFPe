/*
 * qspec_ie.h - The QSPEC IE Manager
 *
 * $Id: qspec_ie.h 4593 2009-10-20 14:30:51Z roehricht $
 * $HeadURL: https://svn.tm.kit.edu/nsis/qspec/trunk/include/qspec_ie.h $
 */
#ifndef QSPEC__IE_H
#define QSPEC__IE_H

#include "ie.h"

namespace qspec {
  using namespace protlib;


/**
 * Categories for QSPEC classes.
 */
enum category_t {
	cat_qspec_pdu			= 0,
	cat_qspec_object		= 1,
	cat_qspec_param			= 2,

	cat_default_qspec_pdu		= 3,
	cat_default_qspec_object	= 4,
	cat_default_qspec_param		= 5
};


/**
 * An Interface for reading/writing QSPECs.
 *
 * The QSPEC_IEManager is a Singleton that provides methods to read/write
 * QSPECs from/to NetMsg objects. Those methods are called deserialize()
 * and serialize(), respectively.
 *
 * To deserialize() a QSPEC, each IE to be used during the process (except
 * for qspec_object) has to be registered with QSPEC_IEManager using
 * register_ie(). Registered IEs will be freed automatically as soon as
 * either clear() is called or QSPEC_IEManager itself is destroyed.
 *
 * The only way to get an QSPEC_IEManager object is through the static
 * instance() method.
 *
 * Note that this singleton is able to coexist with other IEManager
 * subclasses.
 */
class QSPEC_IEManager : public IEManager {

  public:
	static QSPEC_IEManager *instance();
	static void clear();

	static void register_known_ies();

	virtual IE *deserialize(NetMsg &msg, uint16 category,
			IE::coding_t coding, IEErrorList &errorlist,
			uint32 &bytes_read, bool skip);

  protected:
	// protected constructor to prevent instantiation
	QSPEC_IEManager();

	virtual IE *lookup_ie(uint16 category, uint16 type, uint16 subtype);

  private:
	static QSPEC_IEManager *qspec_inst;

	IE *deserialize_qspec_pdu(NetMsg &msg,
		IE::coding_t coding, IEErrorList &errorlist,
		uint32 &bytes_read, bool skip);

	IE *deserialize_qspec_object(NetMsg &msg,
		IE::coding_t coding, IEErrorList &errorlist,
		uint32 &bytes_read, bool skip);

	IE *deserialize_qspec_param(NetMsg &msg,
		IE::coding_t coding, IEErrorList &errorlist,
		uint32 &bytes_read, bool skip);
};


} // end namespace qspec

#endif // QSPEC__IE_H
