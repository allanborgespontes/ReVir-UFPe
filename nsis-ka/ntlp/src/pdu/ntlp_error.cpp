/// ----------------------------------------*- mode: C++; -*--
/// @file ntlp_error.cpp
/// GIST Error PDU
/// ----------------------------------------------------------
/// $Id: ntlp_error.cpp 5249 2010-04-26 08:40:59Z roehricht $
/// $HeadURL: https://svn.tm.kit.edu/nsis/ntlp/trunk/src/pdu/ntlp_error.cpp $
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
#include <iomanip>
#include "ntlp_error.h"
#include "ntlp_pdu.h"
#include "ntlp_object.h"


namespace ntlp {
    using namespace protlib;


/***** class error *****/

const char* const error::iename = "error";

const uint32 error::numobj = 4;

/***** inherited from IE *****/

error* error::new_instance() const {
	error* n = NULL;
	catch_bad_alloc(n = new error());
	return n;
} // end new_instance

error* error::copy() const {
	error* n = NULL;
	catch_bad_alloc(n =  new error(*this));
	return n;
} // end copy 

bool error::check() const {
	if (known_ntlp_pdu::check()) return (
		// check object presence
		true
	); else return false;
} // end check

bool error::operator==(const IE& ie) const {
	const error* n = dynamic_cast<const error*>(&ie);
	if (n && known_ntlp_pdu::operator==(*n)) {
		return true;
	} else return false;
} // end operator==

const char* error::get_ie_name() const { return iename; }

ostream& error::print(ostream& os, uint32 level, const uint32 indent, const char* name) const {
	bool objects = false;
	os<<setw(level*indent)<<"";
	if (name && (*name!=0)) os<<name<<" ";
	os<<"<"<<get_ie_name()<<">:";
	level++;
	if (get_nli()) print_object(nli_ptr,os,level,indent,objects,"NLI");
	print_object(errorobject_ptr,os,level,indent,objects,"Error Data");
	if (!objects) os <<" <empty>";
	return os;
} // end print

istream& error::input(istream& is, bool istty, uint32 level, const uint32 indent, const char* name) {
    


    ntlp_object* tmp;
	string s;
	
	if (istty) {
		cout<<setw(level*indent)<<"";
		if (name && (*name!=0)) cout<<name<<" ";
		cout<<"<"<<get_ie_name()<<">:"<<endl;
	} // end if istty
	level++;
			
	if (istty) cout<<setw(level*indent)<<""<<"NLI: ";
	
	arr.clear(nli_ptr,true);
	(tmp = new nli)->input(is,istty,level,indent);
	arr.insert(nli_ptr,tmp);
		
	arr.clear(errorobject_ptr,true);
	(tmp = new errorobject)->input(is,istty,level,indent,"Error object:");
	arr.insert(errorobject_ptr,tmp);
	
	// end catch_bad_alloc
	return is;

	

	return is;
} // end input

void error::clear_pointers() {
	known_ntlp_pdu::clear_pointers();
	arr.clear(nli_ptr, false);
	arr.clear(errorobject_ptr, false);

} // end clear_pointers

/***** inherited from gist_pdu *****/

ntlp_pdu::deser_error_t error::accept_type_and_subtype(uint8 t) {
    return deser_ok;
} // end accept_type_and_subtype

bool error::accept_object(known_ntlp_object* o, uint32& position, IEErrorList& errorlist) {
    if (state==deser_done) return false;
    
    if (o->is_nli()) {
	state=wait_errorobject;
	// duplicate object error
	if (nli_ptr) errorlist.put(new GIST_DuplicateObject(protocol_v1, Error, known_ntlp_object::NLI));
	nli_ptr=arr.size();
	position=nli_ptr;
	return true;
    }

    if (o->is_nattrav()) {
	state=initial;
	nattrav_ptr=0;
	position=nattrav_ptr;
	return true;
    }


    if (o->is_errorobject())  {
	state=deser_done;
	// duplicate object error
	if (errorobject_ptr) errorlist.put(new GIST_DuplicateObject(protocol_v1, Error, known_ntlp_object::ErrorObject));
	errorobject_ptr=arr.size();
	position=errorobject_ptr;
	return true;
    }
    if ((state==wait_errorobject) & (!(o->is_errorobject()))) return false;


	// When this is reached, something went wrong.
	return false;
} // end accept_object

bool error::deser_end(IEErrorList& errorlist) {
	switch (state) {
	    
	    case wait_nli:
		errorlist.put(new GIST_MissingObject(protocol_v1, Error, known_ntlp_object::NLI)); return false;
	    case deser_done: return true;
	    case initial:
	    case wait_errorobject:
		errorlist.put(new GIST_MissingObject(protocol_v1, Error, known_ntlp_object::ErrorObject)); return false;


	    default:
			errorlist.put(new IEError(IEError::ERROR_INVALID_STATE));
			return false;
	} // end switch state
} // end deser_end

/***** new members *****/

error::error() : known_ntlp_pdu(Error,numobj), 
		 state(initial) 
{
} // end constructor

errorobject* error::get_errorobject() {
  return dynamic_cast<errorobject*>(arr.get(errorobject_ptr));

}


error::error(nli* nl, errorobject* errordata) : known_ntlp_pdu(Error,numobj),
						state(initial)
{
  nattrav_ptr= 0;
  nli_ptr= 1;
  errorobject_ptr= 2;

  if(nl) { 
    arr.set(nli_ptr,nl); 
  } else arr.set(nli_ptr,NULL);
  arr.insert(errorobject_ptr,errordata);
} // end constructor

error::error(const error& n) : known_ntlp_pdu(n),
			       state(initial)
{
} // end copy constructor

error& error::operator=(const error& n) {
	known_ntlp_pdu::operator=(n);
	
	return *this;
} // end operator=
}
