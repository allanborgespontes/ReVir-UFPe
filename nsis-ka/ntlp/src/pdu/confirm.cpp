/// ----------------------------------------*- mode: C++; -*--
/// @file confirm.cpp
/// GIST Confirm PDU
/// ----------------------------------------------------------
/// $Id: confirm.cpp 5249 2010-04-26 08:40:59Z roehricht $
/// $HeadURL: https://svn.tm.kit.edu/nsis/ntlp/trunk/src/pdu/confirm.cpp $
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
#include <confirm.h>
#include <iomanip>
#include <ntlp_pdu.h>
#include <ntlp_object.h>
#include <ntlp_statemodule.h>


namespace ntlp {
    using namespace protlib;

/**
 * @defgroup confirmpdu GIST Confirm PDU
 *  @ingroup ientlppdu
 * @{
 */

/***** class confirm *****/

const char* const confirm::iename = "confirm";

const uint32 confirm::numobj = 4;

/***** inherited from IE *****/

confirm* confirm::new_instance() const {
	confirm* n = NULL;
	catch_bad_alloc(n = new confirm());
	return n;
} // end new_instance

confirm* confirm::copy() const {
	confirm* n = NULL;
	catch_bad_alloc(n =  new confirm(*this));
	return n;
} // end copy 

bool confirm::check() const {
	if (known_ntlp_pdu::check()) return (
		// check object presence
		true
	); else return false;
} // end check

bool confirm::operator==(const IE& ie) const {
	const confirm* n = dynamic_cast<const confirm*>(&ie);
	if (n && known_ntlp_pdu::operator==(*n)) {
		return true;
	} else return false;
} // end operator==

const char* confirm::get_ie_name() const { return iename; }

ostream& confirm::print(ostream& os, uint32 level, const uint32 indent, const char* name) const {
	bool objects = false;
	os<<setw(level*indent)<<"";
	if (name && (*name!=0)) os<<name<<" ";
	os<<"<"<<get_ie_name()<<">:";
	level++;
	if (get_mri()) print_object(mri_ptr,os,level,indent,objects,"Message Routing Information");
	if (get_sessionid()) print_object(sessionid_ptr,os,level,indent,objects,"SessionID");
	if (get_nli()) print_object(nli_ptr,os,level,indent,objects,"NLI");
	if (get_respcookie()) print_object(respcookie_ptr,os,level,indent,objects,"Responder Cookie");
	if (get_stackprop()) print_object(stackprop_ptr,os,level,indent,objects,"StackProposal");
	if (get_stackconf()) print_object(stackconf_ptr,os,level,indent,objects,"StackConfigurationData");
	if (get_nslpdata()) print_object(nslpdata_ptr,os,level,indent,objects,"NSLP Data");
	if (!objects) os <<" <empty>";
	return os;
} // end print

istream& confirm::input(istream& is, bool istty, uint32 level, const uint32 indent, const char* name) {
    

    mri* mr;
    sessionid* sessid;
        ntlp_object* tmp;
	string s;
	
	if (istty) {
		cout<<setw(level*indent)<<"";
		if (name && (*name!=0)) cout<<name<<" ";
		cout<<"<"<<get_ie_name()<<">:"<<endl;
	} // end if istty
	level++;
	
	cout << "NSLPID:";
	
	is >> nslpid;
	

	if (istty) cout<<setw(level*indent)<<""<<"MRI: "<< endl;
	
	arr.clear(mri_ptr,true);
	(mr = new mri_pathcoupled)->input(is,istty,level,indent);
	arr.set(mri_ptr,mr);
		
	arr.clear(sessionid_ptr,true);
	(sessid = new sessionid)->input(is,istty,level,indent,"Confirm ");
	arr.set(sessionid_ptr,sessid);

	arr.clear(nli_ptr,true);
	(tmp = new nli)->input(is,istty,level,indent);
	arr.set(nli_ptr,tmp);

	arr.clear(respcookie_ptr,true);
	
	routingkey* r_key=new routingkey;
	r_key->mr=mr;
	r_key->sid=sessid;

	tmp = new respcookie;
	arr.set(respcookie_ptr,tmp);

	arr.clear(stackprop_ptr,true);
	tmp = new stackprop;
	arr.set(stackprop_ptr,tmp);

	arr.clear(nslpdata_ptr,true);
	tmp = new nslpdata;
	arr.set(nslpdata_ptr,tmp);




	
	// end catch_bad_alloc
	return is;



} // end input

void confirm::clear_pointers() {
	known_ntlp_pdu::clear_pointers();
	arr.clear(mri_ptr,false);
	arr.clear(sessionid_ptr,false);
	arr.clear(nli_ptr, false);
	arr.clear(respcookie_ptr, false);
	arr.clear(stackprop_ptr, false);
	arr.clear(stackconf_ptr, false);
	arr.clear(nslpdata_ptr, false);
} // end clear_pointers

/***** inherited from gist_pdu *****/

ntlp_pdu::deser_error_t confirm::accept_type_and_subtype(uint8 t) {
    return deser_ok;
} // end accept_type_and_subtype

bool confirm::accept_object(known_ntlp_object* o, uint32& position, IEErrorList& errorlist) {
    if (state==deser_done) return false;
    if (o->is_mri()) {
	state=wait_sessionid;
	// duplicate object error
	if (mri_ptr) errorlist.put(new GIST_DuplicateObject(protocol_v1, Confirm, known_ntlp_object::MRI));
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


    if ((state==wait_sessionid) && (o->is_sessionid()))  {
	state=wait_nli;
	// duplicate object error
	if (sessionid_ptr) errorlist.put(new GIST_DuplicateObject(protocol_v1, Confirm, known_ntlp_object::SessionID));
	sessionid_ptr=arr.size();
	position=sessionid_ptr;
	return true;
    }
    if ((state==wait_sessionid) && (!(o->is_sessionid()))) return false;



    if ((state==wait_nli) && (o->is_nli())) { 
	state = wait_respcookie;
	// duplicate object error
	if (nli_ptr) errorlist.put(new GIST_DuplicateObject(protocol_v1, Confirm, known_ntlp_object::NLI));
	nli_ptr=arr.size();
	position=nli_ptr;
	return true;
    }
    if ((state==wait_nli) && (!(o->is_nli()))) return false;


    if ((state==wait_respcookie) && (o->is_respcookie())) { 
	state = wait_stackprop;
	// duplicate object error
	if (respcookie_ptr) errorlist.put(new GIST_DuplicateObject(protocol_v1, Confirm, known_ntlp_object::RespCookie));
	respcookie_ptr=arr.size();
	position=respcookie_ptr;
	return true;
    }

    if ( ((state==wait_stackprop) || (state==wait_respcookie))  && (o->is_stackprop())) { 
	state = wait_stackconf;
	// duplicate object error
	if (stackprop_ptr) errorlist.put(new GIST_DuplicateObject(protocol_v1, Confirm, known_ntlp_object::StackProposal));
	stackprop_ptr=arr.size();
	position = stackprop_ptr;
	return true;
    }


    if ((state==wait_stackprop) && (o->is_nslpdata())) { 
	state = deser_done;
	// duplicate object error
	if (nslpdata_ptr) errorlist.put(new GIST_DuplicateObject(protocol_v1, Confirm, known_ntlp_object::Nslpdata));
	nslpdata_ptr=arr.size();
	position = nslpdata_ptr;
	return true;
    }

    if ((state==wait_stackconf) && (o->is_stackconf())) { 
	state = wait_nslpdata;
	// duplicate object error
	if (stackconf_ptr) errorlist.put(new GIST_DuplicateObject(protocol_v1, Confirm, known_ntlp_object::StackConfiguration));
	stackconf_ptr=arr.size();
	position = stackconf_ptr;
	return true;
    }

    if ((state==wait_stackprop) && (o->is_stackconf())) { 
	state = deser_done;
	// missing object error
	// stack conf cannot be sent without a stack proposal
	errorlist.put(new GIST_MissingObject(protocol_v1, Confirm, known_ntlp_object::StackProposal));
	return true;
    }

    if ((state==wait_stackprop) && (!(o->is_stackprop()))) return false;

   if ((state==wait_nslpdata) && (o->is_nslpdata())) { 
	state = deser_done;
	// duplicate object error
	if (nslpdata_ptr) errorlist.put(new GIST_DuplicateObject(protocol_v1, Confirm, known_ntlp_object::Nslpdata));
	nslpdata_ptr=arr.size();
	position = nslpdata_ptr;
	return true;
    }
    if ((state==wait_nslpdata) && (!(o->is_nslpdata()))) return false;


	// When this is reached, something went wrong.
	return false;
} // end accept_object

bool confirm::deser_end(IEErrorList& errorlist) {
	switch (state) {
	    case initial:
	    case wait_mri:
		errorlist.put(new GIST_MissingObject(protocol_v1, Response, known_ntlp_object::MRI)); return false;
	    case wait_sessionid:
		errorlist.put(new GIST_MissingObject(protocol_v1, Response, known_ntlp_object::SessionID)); return false;
	    case wait_nli:
		errorlist.put(new GIST_MissingObject(protocol_v1, Response, known_ntlp_object::NLI)); return false;
	    case deser_done: return true;
		
	    case wait_respcookie:
		state = deser_done;
		return true;

	    case wait_stackprop:
		state = deser_done;
		return true;
		
	    case wait_nslpdata:
		state = deser_done;
		return true;

	    default:
			errorlist.put(new IEError(IEError::ERROR_INVALID_STATE));
			return false;
	} // end switch state
} // end deser_end

/***** new members *****/

confirm::confirm() : known_ntlp_pdu(Confirm,numobj), 
		     state(initial) 
{

} // end constructor

confirm::confirm(mri* mr, sessionid* sid, nli* nl, respcookie* resp_cookie, stackprop* stckprop, stack_conf_data* stckconf, nslpdata* nslpdat) : known_ntlp_pdu(Confirm,numobj),
																	   state(initial) 
{
	nattrav_ptr=0;
	mri_ptr=1;
	sessionid_ptr=2;
	nli_ptr=3;
	respcookie_ptr=4;
	stackprop_ptr=5;
	stackconf_ptr=6;
	nslpdata_ptr=7;

	arr.set(mri_ptr,mr);
	arr.set(sessionid_ptr,sid);
	arr.insert(nli_ptr,nl);
	arr.insert(respcookie_ptr, resp_cookie);
	arr.insert(stackprop_ptr,stckprop);
	arr.insert(stackconf_ptr, stckconf);
	arr.insert(nslpdata_ptr,nslpdat);
	state=initial;
} // end constructor

confirm::confirm(const confirm& n) : known_ntlp_pdu(n),
    state(n.state)
{
} // end copy constructor

confirm& confirm::operator=(const confirm& n) {
	known_ntlp_pdu::operator=(n);
	
	return *this;
} // end operator=

// @}

} // end namespace
