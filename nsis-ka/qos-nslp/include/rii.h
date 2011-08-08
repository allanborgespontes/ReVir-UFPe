/// ----------------------------------------*- mode: C++; -*--
/// @file rii.h
/// QoS NSLP Request Identification Information Object
/// ----------------------------------------------------------
/// $Id: rii.h 5253 2010-04-26 13:41:09Z roehricht $
/// $HeadURL: https://svn.tm.kit.edu/nsis/qos-nslp/trunk/include/rii.h $
// ===========================================================
//                      
// Copyright (C) 2005-2010, all rights reserved by
// - Institute of Telematics, Karlsruhe Institute of Technology
//
// More information and contact:
// https://projekte.tm.uka.de/trac/NSIS
//                      
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; version 2 of the License
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
// 02110-1301 USA.
//
// ===========================================================
/** @ingroup ierii
 * @file
 * NSLP RIIObject
 */

#ifndef _NSLP__RII_H_
#define _NSLP__RII_H_

#include "protlib_types.h"
#include "nslp_object.h"
#include "messages.h"
#include "qspec.h"

using namespace protlib;

namespace qos_nslp {

/** @ingroup ierii Request Identification Number
 * @ingroup ienslpobject
 * @{
 */

/// NSLP RII
class rii : public known_nslp_object {
/***** inherited from IE ****/
public:
	virtual rii* new_instance() const;
	virtual rii* copy() const;
	virtual rii* deserialize(NetMsg& msg, coding_t cod, IEErrorList& errorlist, uint32& bread, bool skip = true);
	virtual void serialize(NetMsg& msg, coding_t cod, uint32& wbytes) const;
	virtual bool check() const;
	virtual size_t get_serialized_size(coding_t coding) const;
	virtual bool operator==(const IE& ie) const;
	virtual const char* get_ie_name() const;
	virtual ostream& print(ostream& os, uint32 level, const uint32 indent, const char* name = NULL) const;
	virtual istream& input(istream& is, bool istty, uint32 level, const uint32 indent, const char* name = NULL);
/***** new members *****/
public:
	static const uint32 riidefault;
	// @{
	/// constructor
	rii();
	rii(uint32 r);
	// @}
	/// copy constructor
	rii(const rii& n);
	/// assignment
	rii& operator=(const rii& n);
	/// set to default
	//void set_default();
	/// get rii
	void get(uint32& r) const;
	// @{
	/// set rii
	void set(uint32 r);
	void generate_new();
	void set_retry_counter(uint32 r);
	void get_retry_counter(uint32& r) const;
	void increment_retry_counter();
	void set_rii_timer_id(id_t t);
	void get_rii_timer_id(id_t& t);
	void set_own(bool own);
	bool get_own();
	void set_is_reserve(bool r);
	bool get_is_reserve();
	// @}
	// @{
	/// set up qspec
	void set_qspec(const qspec_object* set_q) {
	  if (set_q) {
		  delete q;
		  q = set_q->copy();
	  }
	}

	const qspec_object* get_qspec() const {
		return  q;
	}
	// @}
	// @{
	/// tests
	bool is_rii() const;
	// @}
private:
	uint32 riiref;
	uint32 retry_counter;
	id_t rii_timer_id;
	bool is_own;
	bool is_reserve;
	qspec_object* q;
	static const char* const iename;
	static const uint16 contlen;
}; // end rii



/** Get function for RII object
  * @param r into this variable the value of RII object will be copied
  */
inline
void 
rii::get(uint32& r) const {
	r = riiref;
} // end get

/** Set function for RII object
  * @param r the value of the RII object will be set to the value of this variable
  */
inline
void 
rii::set(uint32 r) {
	riiref = r;
} // end set

/** Set retry counter: indicates how many times this RII object has been already sent
  * @param r this value indicates how many times this RII object has been already sent
  */
inline
void 
rii::set_retry_counter(uint32 r)
{
    retry_counter = r;
}

/** Get retry counter: indicates how many times this RII object has been already sent
  * @param r this value indicates how many times this RII object has been already sent
  */
inline
void 
rii::get_retry_counter(uint32& r) const
{
    r = retry_counter;
}

/** Increment retry counter: increments the value that indicates  how many times this RII object has been already sent
  */
inline
void
rii::increment_retry_counter()
{
    retry_counter++;
}

/** Set timer id of the current timer for this RII object
  * @param t id of the timer that has been started for the current RII object
  */
inline
void 
rii::set_rii_timer_id(id_t t)
{
    rii_timer_id = t;
}
	
/** Get timer id of timer for this RII object
  * @param t id of the timer that has been started for the current RII object will be saved in this variable
  */
inline
void 
rii::get_rii_timer_id(id_t& t)
{
    t = rii_timer_id;
}

/** Generate function for RII object
  * This function is used to generate a new random value for RII object.
  */
inline
void 
rii::generate_new() {
    riiref = rand();
}


/** This function sets OWN bit of this RII object.
  * @param own this bit indicates whether the current node is the generator of this RII object.
  */
inline
void 
rii::set_own(bool own)
{
    is_own = own;
}

	
/** This function gets OWN bit of this RII object.
  * OWN bit indicates whether the current node is the generator of this RII object.
  */
inline
bool 
rii::get_own()
{
  return is_own;
}

/** This function sets the value that indicates whether the current RII is for a RESERVE or a QUERY message.
  * @param r the value of the local variable will be set to this value.
  */
inline
void 
rii::set_is_reserve(bool r)
{
  is_reserve = r;
}
	
/** This function gets the value that indicates whether the current RII is for a RESERVE or a QUERY message.
  * @return r the value that indicates whether the current RII is for a RESERVE or a QUERY message.
  */
inline
bool 
rii::get_is_reserve()
{
  return is_reserve;
}

/** Check function for RII object
  * This function returns true if the current object is of type RII.
  */
inline
bool 
rii::is_rii() const {
	return (type==RII);
} // end is_rii

//@}

} // end namespace qos_nslp

#endif // _NSLP__RII_H_
