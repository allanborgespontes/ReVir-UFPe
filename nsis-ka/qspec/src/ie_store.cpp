/*
 * ie_store.h - Store IEs by ID
 *
 * $Id: ie_store.cpp 4593 2009-10-20 14:30:51Z roehricht $
 * $HeadURL: https://svn.tm.kit.edu/nsis/qspec/trunk/src/ie_store.cpp $
 *
 */
#include "logfile.h"

#include "ie_store.h"


using namespace qspec;
using namespace protlib::log;


/**
 * Standard constructor.
 *
 * Creates an empty ie_store.
 */
ie_store::ie_store() : entries() {
	// nothing to do
}


/**
 * Copy constructor.
 *
 * Makes a deep copy of the object passed as an argument.
 *
 * @param other the object to copy
 */
ie_store::ie_store(const ie_store &other) {

	// copy all entries
	for (c_iter i = other.entries.begin(); i != other.entries.end(); i++) {
		const uint32 id = i->first;
		const IE *ie = i->second;

		if ( ie )
			set(id, ie->copy());
		else
			Log(ERROR_LOG, LOG_CRIT, "ie_store",
				"copy constructor: the other IE is NULL");
	}
}


/**
 * Destructor.
 *
 * Deletes all entries this ie_store contains.
 */
ie_store::~ie_store() {
	for ( c_iter i = entries.begin(); i != entries.end(); i++ )
		delete i->second;
}


bool ie_store::operator==(const ie_store &other) const throw() {

	// The number of entries has to be equal, otherwise the check
	// after this one doesn't work.
	if ( size() != other.size() )
		return false;

	// All entries have to be identical.
	for ( c_iter i = entries.begin(); i != entries.end(); i++ ) {
		const uint32 id = i->first;
		const IE *obj = i->second;

		IE *other_obj = other.get(id);

		// Note: other_obj can't be NULL, set() takes care of that.
		if ( other_obj == NULL || *obj != *other_obj )
			return false;
	}

	return true;	// no difference found
}


/**
 * Return the number of contained entries.
 *
 * @return the number of entries this ie_store contains
 */
size_t ie_store::size() const throw() {
	return entries.size();
}


/**
 * Returns the entry registered for a given ID.
 *
 * The object is not removed.
 *
 * @return the entry or NULL, if none is registered for that ID
 */
IE *ie_store::get(uint32 id) const throw() {

	/*
	 * Note: This is a true "const" implementation. Accessing the map
	 *       via entries[id] would modify it, even if no entry is found!
	 */
	c_iter i = entries.find(id);

	if ( i != entries.end() )
		return i->second;
	else
		return NULL;
}


/**
 * Add an entry.
 *
 * If there is already an entry registered with the same ID, then the old
 * one is deleted. Note that ie_store's destructor will delete all registered
 * entries. Because of this, IE objects may only be added once. Otherwise
 * the destructor would try to delete them twice.
 *
 * Adding a NULL ie is not allowed. In this case, no operation is performed.
 *
 * @param id the ID used as the key
 * @param ie the IE to add (NULL is not allowed)
 */
void ie_store::set(uint32 id, IE *ie) throw () {

	if ( ie == NULL )
		return;

	IE *old = entries[id];

	if ( old )
		delete old;

	entries[id] = ie;
}


/**
 * Remove an entry. 
 *
 * Remove the entry with the given ID from this ie_store. If there is no
 * entry with that ID, NULL is returned.
 *
 * The returned IE itself is *not* deleted.
 *
 * @param id an entry's ID
 * @return the entry with that ID or NULL if there is none
 */
IE *ie_store::remove(uint32 id) throw () {
	IE *ie = entries[id];

	// Note: entries[id] created an entry, even if none existed before!
	entries.erase(id);

	return ie; // either the IE or NULL
}


// EOF
