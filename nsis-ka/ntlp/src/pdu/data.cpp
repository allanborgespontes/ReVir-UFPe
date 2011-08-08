/// ----------------------------------------*- mode: C++; -*--
/// @file data.cpp
/// GIST Data PDU
/// ----------------------------------------------------------
/// $Id: data.cpp 5249 2010-04-26 08:40:59Z roehricht $
/// $HeadURL: https://svn.tm.kit.edu/nsis/ntlp/trunk/src/pdu/data.cpp $
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
#include <data.h>
#include <iomanip>
#include <ntlp_pdu.h>
#include <ntlp_object.h>


namespace ntlp {
    using namespace protlib;

/**
 * @defgroup datapdu GIST Data PDU
 *  @ingroup ientlppdu
 * @{
 */

/***** class data *****/

const char* const data::iename = "data";

const uint32 data::numobj = 4;

/***** inherited from IE *****/

data* data::new_instance() const {
	data* n = NULL;
	catch_bad_alloc(n = new data());
	return n;
} // end new_instance

data* data::copy() const {
	data* n = NULL;
	catch_bad_alloc(n =  new data(*this));
	return n;
} // end copy 

bool data::check() const {
	if (known_ntlp_pdu::check()) return (
		// check object presence
		true
	); else return false;
} // end check

bool data::operator==(const IE& ie) const {
	const data* n = dynamic_cast<const data*>(&ie);
	if (n && known_ntlp_pdu::operator==(*n)) {
		return true;
	} else return false;
} // end operator==

const char* data::get_ie_name() const { return iename; }

ostream& data::print(ostream& os, uint32 level, const uint32 indent, const char* name) const {
	bool objects = false;
	os<<setw(level*indent)<<"";
	if (name && (*name!=0)) os<<name<<" ";
	os<<"<"<<get_ie_name()<<">:";
	level++;
	if (get_mri()) print_object(mri_ptr,os,level,indent,objects,"Message Routing Information");
	if (get_sessionid()) print_object(sessionid_ptr,os,level,indent,objects,"SessionID");
	if (get_nli()) print_object(nli_ptr,os,level,indent,objects,"NLI");
	if (get_nslpdata()) print_object(nslpdata_ptr,os,level,indent,objects,"NSLP Data");
	if (!objects) cout<<" <empty>";
	return os;
} // end print

istream& data::input(istream& is, bool istty, uint32 level, const uint32 indent, const char* name) {
    


        ntlp_object* tmp;
	string s;
	
	if (istty) {
		cout<<setw(level*indent)<<"";
		if (name && (*name!=0)) cout<<name<<" ";
		cout<<"<"<<get_ie_name()<<">:"<<endl;
	} // end if istty
	level++;
	
	if (istty) cout<<setw(level*indent)<<""<<"MRI: ";
	
	arr.clear(mri_ptr,true);
	(tmp = new mri_pathcoupled)->input(is,istty,level,indent);
	arr.set(mri_ptr,tmp);
		
	arr.clear(sessionid_ptr,true);
	(tmp = new sessionid)->input(is,istty,level,indent,"inquiry reference");
	arr.set(sessionid_ptr,tmp);
	
	// end catch_bad_alloc
	return is;



} // end input

void data::clear_pointers() {
	known_ntlp_pdu::clear_pointers();
	arr.clear(mri_ptr,false);
	arr.clear(sessionid_ptr,false);
	arr.clear(nli_ptr, false);
	arr.clear(nslpdata_ptr, false);
} // end clear_pointers

/***** inherited from gist_pdu *****/

ntlp_pdu::deser_error_t data::accept_type_and_subtype(uint8 t) {
    return deser_ok;
} // end accept_type_and_subtype

bool data::accept_object(known_ntlp_object* o, uint32& position, IEErrorList& errorlist) {
    if (state==deser_done) return false;
    if (o->is_mri()) {
	state=wait_sessionid;
	// duplicate object error
	if (mri_ptr) errorlist.put(new GIST_DuplicateObject(protocol_v1, Data, known_ntlp_object::MRI));
	mri_ptr=arr.size();
	position=mri_ptr;
	return true;
    }

    if (o->is_nattrav()) {
	state=initial;
	nattrav_ptr=0;
	position=nattrav_ptr;
	return true;
    }



    if ((state==wait_sessionid) & (o->is_sessionid()))  {
	state=wait_nli;
	// duplicate object error
	if (sessionid_ptr) errorlist.put(new GIST_DuplicateObject(protocol_v1, Data, known_ntlp_object::SessionID));
	sessionid_ptr=arr.size();
	position=sessionid_ptr;
	return true;
    }
    if ((state==wait_sessionid) & (!(o->is_sessionid()))) return false;



    if ((state==wait_nli) & (o->is_nli())) { 
	state = wait_nslpdata;
	// duplicate object error
	if (nli_ptr) errorlist.put(new GIST_DuplicateObject(protocol_v1, Data, known_ntlp_object::NLI));
	nli_ptr=arr.size();
	position=nli_ptr;
	return true;
    }
    if ((state==wait_nli) & (!(o->is_nli()) && !(o->is_nslpdata()))) return false;


    if ((state==wait_nli) & (o->is_nslpdata())) { 
	state = deser_done;
	// duplicate object error
	if (nslpdata_ptr) errorlist.put(new GIST_DuplicateObject(protocol_v1, Data, known_ntlp_object::Nslpdata));
	nslpdata_ptr=arr.size();
	position=nslpdata_ptr;
	return true;
    }





   if ((state==wait_nslpdata) & (o->is_nslpdata())) { 
	state = deser_done;
	// duplicate object error
	if (nslpdata_ptr) errorlist.put(new GIST_DuplicateObject(protocol_v1, Data, known_ntlp_object::Nslpdata));
	nslpdata_ptr=arr.size();
	position=nslpdata_ptr;
	return true;
    }
    if ((state==wait_nslpdata) & (!(o->is_nslpdata()))) return false;


	// When this is reached, something is wrong.
	return false;
} // end accept_object

bool data::deser_end(IEErrorList& errorlist) {
	switch (state) {
	    case initial:
	    case wait_mri:
		errorlist.put(new GIST_MissingObject(protocol_v1, Data, known_ntlp_object::MRI)); return false;
	    case wait_sessionid:
		errorlist.put(new GIST_MissingObject(protocol_v1, Data, known_ntlp_object::SessionID)); return false;
	    case wait_nli:
		errorlist.put(new GIST_MissingObject(protocol_v1, Data, known_ntlp_object::NLI)); return false;
	    case deser_done: return true;
		
	    case wait_nslpdata:
		errorlist.put(new GIST_MissingObject(protocol_v1, Data, known_ntlp_object::Nslpdata)); return false;

	    default:
			errorlist.put(new IEError(IEError::ERROR_INVALID_STATE));
			return false;
	} // end switch state
} // end deser_end

/***** new members *****/

data::data() : known_ntlp_pdu(Data,numobj),
	       state(initial)
{
} // end constructor

data::data(mri* mr, sessionid* sid, nli* nl, nslpdata* nslpdat) : known_ntlp_pdu(Data,numobj),
								  state(initial)
{
	nattrav_ptr=0;
	mri_ptr=1;
	sessionid_ptr=2;
	nli_ptr=3;
	nslpdata_ptr=4;
	arr.insert(mri_ptr,mr);
	arr.insert(sessionid_ptr,sid);
	arr.insert(nli_ptr,nl);
	arr.insert(nslpdata_ptr,nslpdat);
} // end constructor

data::data(const data& n) : known_ntlp_pdu(n),
			    state(n.state)
{
} // end copy constructor

data& data::operator=(const data& n) {
	known_ntlp_pdu::operator=(n);
	
	return *this;
} // end operator=

// @}

} // end namespace
