/// ----------------------------------------*- mode: C++; -*--
/// @file response.cpp
/// GIST Response
/// ----------------------------------------------------------
/// $Id: response.cpp 5249 2010-04-26 08:40:59Z roehricht $
/// $HeadURL: https://svn.tm.kit.edu/nsis/ntlp/trunk/src/pdu/response.cpp $
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
#include <response.h>
#include <iomanip>
#include <ntlp_pdu.h>
#include <ntlp_object.h>


namespace ntlp {
    using namespace protlib;

/**
 * @defgroup respondepdu GIST Response PDU
 *  @ingroup ientlppdu
 * @{
 */


/***** class response *****/

const char* const response::iename = "response";

const uint32 response::numobj = 7;

/***** inherited from IE *****/

response* response::new_instance() const {
	response* n = NULL;
	catch_bad_alloc(n = new response());
	return n;
} // end new_instance

response* response::copy() const {
	response* n = NULL;
	catch_bad_alloc(n =  new response(*this));
	return n;
} // end copy 

bool response::check() const {
	if (known_ntlp_pdu::check()) return (
		// check object presence
		true
	); else return false;
} // end check

bool response::operator==(const IE& ie) const {
	const response* n = dynamic_cast<const response*>(&ie);
	if (n && known_ntlp_pdu::operator==(*n)) {
		return true;
	} else return false;
} // end operator==

const char* response::get_ie_name() const { return iename; }

ostream& response::print(ostream& os, uint32 level, const uint32 indent, const char* name) const {
	bool objects = false;
	os<<setw(level*indent)<<"";
	if (name && (*name!=0)) os<<name<<" ";
	os<<"<"<<get_ie_name()<<">:";
	level++;
	os<<setw(level*indent)<<"";
	os<<"NSLP ID: " << nslpid << endl;
	if (get_nattraversal()) print_object(nattrav_ptr,os,level,indent,objects,"NAT traversal object");
	if (get_mri()) print_object(mri_ptr,os,level,indent,objects,"Message Routing Information");
	if (get_sessionid()) print_object(sessionid_ptr,os,level,indent,objects,"SessionID");
	if (get_nli()) print_object(nli_ptr,os,level,indent,objects,"NLI");
	if (get_querycookie()) print_object(querycookie_ptr,os,level,indent,objects,"Query Cookie");
	if (get_respcookie()) print_object(respcookie_ptr,os,level,indent,objects,"Responder Cookie");
	if (get_stackprop()) print_object(stackprop_ptr,os,level,indent,objects,"StackProposal");
	if (get_stackconf()) print_object(stackconf_ptr,os,level,indent,objects,"StackConfigurationData");
	if (get_nslpdata()) print_object(nslpdata_ptr,os,level,indent,objects,"NSLP Data");
	if (!objects) os <<" <empty>";
	return os;
} // end print

istream& response::input(istream& is, bool istty, uint32 level, const uint32 indent, const char* name) {
    


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
	(tmp = new sessionid)->input(is,istty,level,indent);
	arr.set(sessionid_ptr,tmp);

	arr.clear(nli_ptr,true);
	tmp = new nli;
	arr.set(nli_ptr,tmp);

	arr.clear(querycookie_ptr,true);
	tmp = new querycookie;
	arr.set(querycookie_ptr,tmp);

	arr.clear(respcookie_ptr,true);
	tmp = new respcookie;
	arr.set(respcookie_ptr,tmp);

	arr.clear(stackprop_ptr,true);
	tmp = new stackprop;
	arr.set(stackprop_ptr,tmp);

	arr.clear(stackconf_ptr,true);
	tmp = new stack_conf_data;
	arr.set(stackconf_ptr,tmp);

	arr.clear(nslpdata_ptr,true);
	tmp = new nslpdata;
	arr.set(nslpdata_ptr,tmp);






	
	// end catch_bad_alloc
	return is;



} // end input

void response::clear_pointers() {
	known_ntlp_pdu::clear_pointers();
	arr.clear(nattrav_ptr,false);
	arr.clear(mri_ptr,false);
	arr.clear(sessionid_ptr,false);
	arr.clear(nli_ptr, false);
	arr.clear(querycookie_ptr,false);
	arr.clear(respcookie_ptr,false);
	arr.clear(stackprop_ptr, false);
	arr.clear(stackconf_ptr, false);
	arr.clear(nslpdata_ptr, false);
} // end clear_pointers

/***** inherited from gist_pdu *****/

ntlp_pdu::deser_error_t response::accept_type_and_subtype(uint8 t) {
	// prepare for deserialization
	//origref = msgref = msgintegrity = objectarray::unset;
	//state = wait_origref;
	// chech type and subtype
	//if (t==type) {
	//	if (st==subtype) return deser_ok;
	//	else return subtype_error;
	//} else return type_error;
    return deser_ok;
} // end accept_type_and_subtype

bool response::accept_object(known_ntlp_object* o, uint32& position, IEErrorList& errorlist) {
    if (state==deser_done) return false;
    if (o->is_mri()) {
	state=wait_sessionid;
	// duplicate object error
	if (mri_ptr) errorlist.put(new GIST_DuplicateObject(protocol_v1, Response, known_ntlp_object::MRI));
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
	if (sessionid_ptr) errorlist.put(new GIST_DuplicateObject(protocol_v1, Response, known_ntlp_object::SessionID));
	sessionid_ptr=arr.size();
	position=sessionid_ptr;
	return true;
    }
    if ((state==wait_sessionid) && (!(o->is_sessionid()))) return false;


    if ((state==wait_nli) && (o->is_nli())) { 
	state = wait_querycookie;
	// duplicate object error
	if (nli_ptr) errorlist.put(new GIST_DuplicateObject(protocol_v1, Response, known_ntlp_object::NLI));
	nli_ptr=arr.size();
	position=nli_ptr;
	return true;
    }
    if ((state==wait_nli) && (!(o->is_nli()))) return false;

    if ((state==wait_querycookie) && (o->is_querycookie())) { 
	state = wait_respondercookie;
	// duplicate object error
	if (querycookie_ptr) errorlist.put(new GIST_DuplicateObject(protocol_v1, Response, known_ntlp_object::QueryCookie));
	querycookie_ptr=arr.size();
	position=querycookie_ptr;
	return true;
    }
    if ((state==wait_querycookie) && (!(o->is_querycookie()))) return false;


    if ((state==wait_respondercookie) && (o->is_respcookie())) { 
	state = wait_stackprop;
	// duplicate object error
	if (respcookie_ptr) errorlist.put(new GIST_DuplicateObject(protocol_v1, Response, known_ntlp_object::RespCookie));
	respcookie_ptr=arr.size();
	position=respcookie_ptr;
	return true;
    }
    if ((state==wait_respondercookie) && (!(o->is_respcookie()))) return false;


    if ((state==wait_stackprop) && (o->is_stackprop())) { 
	state = wait_stackconf;	
	// duplicate object error
	if (stackprop_ptr) errorlist.put(new GIST_DuplicateObject(protocol_v1, Response, known_ntlp_object::StackProposal));
	stackprop_ptr=arr.size();
	position=stackprop_ptr;
	return true;
    }

    if ((state==wait_stackprop) && (o->is_nslpdata())) { 
	state = deser_done;
	// duplicate object error
	if (nslpdata_ptr) errorlist.put(new GIST_DuplicateObject(protocol_v1, Response, known_ntlp_object::Nslpdata));
	nslpdata_ptr=arr.size();
	position=nslpdata_ptr;
	return true;
    }

    if ((state==wait_stackprop) && (!(o->is_stackprop()))) return false;




    if ((state==wait_stackconf) && (o->is_stackconf())) { 
	state = wait_nslpdata;
	// duplicate object error
	if (stackconf_ptr) errorlist.put(new GIST_DuplicateObject(protocol_v1, Response, known_ntlp_object::StackConfiguration));
	stackconf_ptr=arr.size();
	position=stackconf_ptr;
	return true;
    }
    if ((state==wait_stackconf) && (!(o->is_stackconf()))) return false;

   if ((state==wait_nslpdata) && (o->is_nslpdata())) { 
	state = deser_done;
	// duplicate object error
	if (nslpdata_ptr) errorlist.put(new GIST_DuplicateObject(protocol_v1, Response, known_ntlp_object::Nslpdata));
	nslpdata_ptr=arr.size();
	position=nslpdata_ptr;
	return true;
    }
    if ((state==wait_nslpdata) && (!(o->is_nslpdata()))) return false;


	// When this is reached, something went wrong.
	return false;
} // end accept_object

bool response::deser_end(IEErrorList& errorlist) {
	switch (state) {
	    case initial:
	    case wait_mri:
		errorlist.put(new GIST_MissingObject(protocol_v1, Response, known_ntlp_object::MRI)); return false;
	    case wait_sessionid:
		errorlist.put(new GIST_MissingObject(protocol_v1, Response, known_ntlp_object::SessionID)); return false;
	    case wait_nli:
		errorlist.put(new GIST_MissingObject(protocol_v1, Response, known_ntlp_object::NLI)); return false;
	    case deser_done: return true;
		
	    case wait_respondercookie:
	    case wait_stackprop:
		state=deser_done;
		return true;

	    case wait_stackconf:
		errorlist.put(new GIST_MissingObject(protocol_v1, Response, known_ntlp_object::StackConfiguration)); return false;
		//nslpdata is OPTIONAL
	    case wait_nslpdata:
		state=deser_done;
		return true;
	    default:
			errorlist.put(new IEError(IEError::ERROR_INVALID_STATE));
			return false;
	} // end switch state
} // end deser_end

/***** new members *****/

response::response() : known_ntlp_pdu(Response,numobj),
		       state(initial)
{
} // end constructor

response::response(mri* mr, sessionid* sid, nli* nl, querycookie* qu_cookie, respcookie* rsp_cookie, stackprop* stckprop, stack_conf_data* stckconf, nslpdata* nslpdat) : known_ntlp_pdu(Response,numobj),
																				    state(initial)

{
	nattrav_ptr=0;
	mri_ptr=1;
	sessionid_ptr=2;
	nli_ptr=3;
	querycookie_ptr=4;
	respcookie_ptr=5;
	stackprop_ptr=6;
	stackconf_ptr=7;
	nslpdata_ptr=8;
	arr.set(mri_ptr,mr);
	arr.set(sessionid_ptr,sid);
	arr.insert(nli_ptr,nl);
	arr.insert(querycookie_ptr, qu_cookie);
	arr.insert(respcookie_ptr, rsp_cookie);
	arr.insert(stackprop_ptr,stckprop);
	arr.insert(stackconf_ptr,stckconf);
	arr.insert(nslpdata_ptr,nslpdat);
} // end constructor

response::response(const response& n) : known_ntlp_pdu(n),
					state(n.state)
{
} // end copy constructor

response& response::operator=(const response& n) {
	known_ntlp_pdu::operator=(n);
	
	return *this;
} // end operator=


// @}

}
