/*
 * ie_store.h - Store IEs by ID
 *
 * $Id: ie_store.h 4593 2009-10-20 14:30:51Z roehricht $
 * $HeadURL: https://svn.tm.kit.edu/nsis/qspec/trunk/include/ie_store.h $
 */
#ifndef QSPEC__IE_STORE_H
#define QSPEC__IE_STORE_H

#include <map>

#include "ie.h"


namespace qspec {
  using namespace protlib;


/**
 * Stores (ID, IE) mappings.
 *
 * This is a helper class intented for internal use. It mainly specializes
 * std::map and takes care of memory management issues.
 */
class ie_store {
	
  public:
	ie_store();
	ie_store(const ie_store &other);
	~ie_store();

	size_t size() const throw();
	IE *get(uint32 id) const throw();
	void set(uint32 id, IE *ie) throw();
	IE *remove(uint32 id) throw();

	bool operator==(const ie_store &other) const throw();


	typedef std::map<uint32, IE *>::const_iterator const_iterator;

	const_iterator begin() const throw() { return entries.begin(); }
	const_iterator end() const throw() { return entries.end(); }

  private:
	/**
	 * Maps IDs to IEs.
	 */
	std::map<uint32, IE *> entries;

	/**
	 * Shortcut.
	 */
	typedef const_iterator c_iter;
};


} // namespace qspec

#endif // QSPEC__IE_STORE_H
