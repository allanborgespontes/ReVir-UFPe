/// ----------------------------------------*- mode: C++; -*--
/// @file ntlp_error.h
/// GIST Error PDU
/// ----------------------------------------------------------
/// $Id: ntlp_error.h 5249 2010-04-26 08:40:59Z roehricht $
/// $HeadURL: https://svn.tm.kit.edu/nsis/ntlp/trunk/src/pdu/ntlp_error.h $
// ===========================================================
//                      
// Copyright (C) 2005-2010, all rights reserved by
// - Institute of Telematics, Karlsruhe Institute of Technology
//
// More information and contact:
// https://projekte.tm.uka.de/trac/NSIS
//                      
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; version 2 of the License
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
//
// ===========================================================
#ifndef NTLP_ERROR_H
#define NTLP_ERROR_H

#include <ntlp_pdu.h>
#include <ntlp_object.h>
#include <mri_pc.h>
#include <stackprop.h>
#include <stackconf.h>
#include <nli.h>
#include <ntlp_errorobject.h>


namespace ntlp {
    using namespace protlib;

/// NTLP Request
class error : public known_ntlp_pdu {
public:
/***** inherited from IE *****/
	virtual error* new_instance() const;
	virtual error* copy() const;
	virtual bool check() const;
	virtual bool operator==(const IE& ie) const;
	virtual const char* get_ie_name() const;
	virtual ostream& print(ostream& os, uint32 level, const uint32 indent, const char* name = NULL) const;
	virtual istream& input(istream& is, bool istty, uint32 level, const uint32 indent, const char* name = NULL);
	virtual void clear_pointers();
/***** inherited from ntlp_pdu *****/
protected:
	virtual deser_error_t accept_type_and_subtype(uint8 t);
	virtual bool accept_object(known_ntlp_object* o, uint32& position, IEErrorList& errorlist);
	virtual bool deser_end(IEErrorList& errorlist);
/***** new members *****/
public:
	// @{
	/// constructor
	error();
      	error(nli* nl, errorobject* errordata);
	// @}
	errorobject* get_errorobject();
	/// copy constructor
	error(const error& n);
	/// assignment
	error& operator=(const error& n);
	// @{
       	// @}
private:	
	/// PDU fields
	static const char* const iename;
	/// default number of objects
	static const uint32 numobj;
	/// deserialization state
	enum deser_state_t {
	    initial,
	    wait_nli,
	    deser_done,
	    wait_errorobject
	} state;
}; // end class error

}

#endif
