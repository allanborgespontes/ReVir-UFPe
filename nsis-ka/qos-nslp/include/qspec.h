/// ----------------------------------------*- mode: C++; -*--
/// @file qspec.h
/// QSPEC object
/// ----------------------------------------------------------
/// $Id: qspec.h 5271 2010-05-10 14:23:40Z bless $
/// $HeadURL: https://svn.tm.kit.edu/nsis/qos-nslp/trunk/include/qspec.h $
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
/** @ingroup ieqspec
 * @file
 * NSLP QSPECObject
 */

#ifndef _NSLP__QSPEC_H_
#define _NSLP__QSPEC_H_

#include "protlib_types.h"
#include "nslp_object.h"

#include "qspec_ie.h"
#include "qspec_pdu.h"
#include "qspec_param.h"

using namespace protlib;

namespace qos_nslp {

/** @addtogroup ieqspec Quality Specification
 * @ingroup ienslpobject
 * @{
 */

/// NSLP QSPEC
class qspec_object : public known_nslp_object {
/***** inherited from IE ****/
public:
	virtual qspec_object* new_instance() const;
	virtual qspec_object* copy() const;
	virtual qspec_object* deserialize(NetMsg& msg, coding_t cod, IEErrorList& errorlist, uint32& bread, bool skip = true);
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
	/// constructor
	qspec_object();
	qspec_object(const qspec::qspec_pdu* q_pdu);
	// @}
	/// copy constructor
	qspec_object(const qspec_object& n);
	/// assignment
	qspec_object& operator=(const qspec_object& n);
	/// get qspec_object content (qspec template pdu)
	const qspec::qspec_pdu* get_qspec_data() const;
	/// set qspec_object content (qspec template pdu)
	void set_qspec_data(qspec::qspec_pdu* data);
	// @{
	/// tests
	bool is_qspec() const;
	// @}
	void set_contlen(uint32 len);

private:
	qspec::qspec_pdu* qspec_data;
	static const char* const iename;
	uint16 contlen;
}; // end qspec_object

//@}

} // end namespace qos_nslp

#endif // _NSLP__QSPEC_H_
