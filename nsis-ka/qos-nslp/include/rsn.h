/// ----------------------------------------*- mode: C++; -*--
/// @file rsn.h
/// QoS NSLP Reservation Sequence Number
/// ----------------------------------------------------------
/// $Id: rsn.h 5253 2010-04-26 13:41:09Z roehricht $
/// $HeadURL: https://svn.tm.kit.edu/nsis/qos-nslp/trunk/include/rsn.h $
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
/** @ingroup iersn
 * @file
 * NSLP RSNObject
 */

#ifndef _NSLP__RSN_H_
#define _NSLP__RSN_H_

#include "protlib_types.h"
#include "nslp_object.h"

using namespace protlib;

namespace qos_nslp {

/** @addtogroup iersn Reservation Sequence Number
 * @ingroup ienslpobject
 * @{
 */

/// NSLP RSN - Reservation Sequence Number
class rsn : public known_nslp_object {
/***** inherited from IE ****/
public:
	virtual rsn* new_instance() const;
	virtual rsn* copy() const;
	virtual rsn* deserialize(NetMsg& msg, coding_t cod, IEErrorList& errorlist, uint32& bread, bool skip = true);
	virtual void serialize(NetMsg& msg, coding_t cod, uint32& wbytes) const;
	virtual bool check() const;
	virtual size_t get_serialized_size(coding_t coding) const;
	virtual bool operator==(const IE& ie) const;
	virtual const char* get_ie_name() const;
	virtual ostream& print(ostream& os, uint32 level, const uint32 indent, const char* name = NULL) const;
	virtual istream& input(istream& is, bool istty, uint32 level, const uint32 indent, const char* name = NULL);
/***** new members *****/
public:
	// @{
	/// constructors
	rsn();
	rsn(uint32 epoch_id, uint32 r);
	rsn(uint32 r);
	// @}
	/// copy constructor
	rsn(const rsn& n);
	/// assignment
	rsn& operator=(const rsn& n);
	/// set to default
	void set_default();
	/// get rsn no
	uint32 get() const { return reservation_seq_no; }

	/// Comparison
	bool operator==(const rsn& otherrsn) const { return epoch_id == otherrsn.epoch_id && reservation_seq_no == otherrsn.reservation_seq_no; }
	bool operator!=(const rsn& otherrsn) const { return epoch_id != otherrsn.epoch_id || reservation_seq_no != otherrsn.reservation_seq_no; }
	// note: epoch ID will not be considered for this comparison
	bool operator<(const rsn& otherrsn) const;
	bool operator>(const rsn& otherrsn) const;

	// @{
	/// set rsn
	void set(uint32 r) { reservation_seq_no= r; }

	/// increment operator
	void increment() { reservation_seq_no++; }

	/// increment operator (prefix)
	rsn& operator++() { reservation_seq_no++; return *this; }
	/// increment operator (postfix)
	rsn& operator++(int) { reservation_seq_no++; return *this; }

	//void generate_new();
	// @}
	// @{
	/// epoch_id
	void set_epoch_id(uint32 r) { epoch_id = r; }
	uint32 get_epoch_id() const { return epoch_id; }
	// @}
	// @{
	/// tests
	bool is_rsn() const { return type == RSN;}
	// @}
private:
	// The RSN object also contains an Epoch Identifier, which provides a
	// method for determining when a peer has restarted (e.g., due to node
	// reboot or software restart).
	uint32 epoch_id; 
	// Reservation Sequence Number
	uint32 reservation_seq_no;

	static const uint32 rsndefault;
	static const uint32 rsn_max_increment;

	static const char* const iename;
	static const uint16 contlen;
public:
	// this is set by the local system
	static uint32 local_epoch_id;

}; // end rsn


inline 
const char* rsn::get_ie_name() const { return iename; }


/** Default constructor for RSN object without any params. The value of the object will be set to the default.
  */
inline
rsn::rsn()
	: known_nslp_object(RSN, mandatory), epoch_id(local_epoch_id), reservation_seq_no(0) {
} // end constructor


/** Constructor for RSN object with given parameter.
  * @param epoch the epoch id
  * @param r the reservation sequence number for new RSN object.
  */
inline
rsn::rsn(uint32 epoch, uint32 r)
	: known_nslp_object(RSN, mandatory), epoch_id(epoch), reservation_seq_no(r) {
} // end constructor


/** Constructor for RSN object with given parameter.
  * @param r the reservation sequence number for new RSN object.
  */
inline
rsn::rsn(uint32 r)
	: known_nslp_object(RSN, mandatory), epoch_id(local_epoch_id), reservation_seq_no(r) {
} // end constructor


/** Comparison operator for RSNs
 * @note Epoch_ID is not considered
 **/
inline
bool 
rsn::operator<(const rsn& otherrsn) const
{ 
	return  ( (reservation_seq_no < otherrsn.reservation_seq_no) && (otherrsn.reservation_seq_no - reservation_seq_no) < (1UL << 31) )
		||
		( (reservation_seq_no > otherrsn.reservation_seq_no) && (reservation_seq_no - otherrsn.reservation_seq_no) > (1UL << 31) );
}


inline
bool 
rsn::operator>(const rsn& otherrsn) const
{ 
	return  ( (reservation_seq_no < otherrsn.reservation_seq_no) && (otherrsn.reservation_seq_no - reservation_seq_no) > (1UL << 31) )
		||
		( (reservation_seq_no > otherrsn.reservation_seq_no) && (reservation_seq_no - otherrsn.reservation_seq_no) < (1UL << 31) );
}

//@}

} // end namespace qos_nslp

#endif // _NSLP__RSN_H_
