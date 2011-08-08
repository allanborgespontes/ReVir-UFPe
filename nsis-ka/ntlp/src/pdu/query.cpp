/// ----------------------------------------*- mode: C++; -*--
/// @file query.cpp
/// GIST Query PDU
/// ----------------------------------------------------------
/// $Id: query.cpp 5249 2010-04-26 08:40:59Z roehricht $
/// $HeadURL: https://svn.tm.kit.edu/nsis/ntlp/trunk/src/pdu/query.cpp $
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
#include <query.h>
#include <iomanip>
#include <ntlp_pdu.h>
#include <ntlp_object.h>


namespace ntlp {
    using namespace protlib;

/**
 * @defgroup querypdu GIST Query PDU
 *  @ingroup ientlppdu
 * @{
 */

/***** class query *****/

const char* const query::iename = "query";

const uint32 query::numobj = 8;

/***** inherited from IE *****/

query* query::new_instance() const {
	query* n = NULL;
	catch_bad_alloc(n = new query());
	return n;
} // end new_instance

query* query::copy() const {
	query* n = NULL;
	catch_bad_alloc(n =  new query(*this));
	return n;
} // end copy 

bool query::check() const {
	if (known_ntlp_pdu::check()) return (
		// check object presence
		true
	); else return false;
} // end check

bool query::operator==(const IE& ie) const {
	const query* n = dynamic_cast<const query*>(&ie);
	if (n && known_ntlp_pdu::operator==(*n)) {
		return true;
	} else return false;
} // end operator==

const char* query::get_ie_name() const { return iename; }

ostream& query::print(ostream& os, uint32 level, const uint32 indent, const char* name) const {
	bool objects = false;
	os<<setw(level*indent)<<"";
	if (name && (*name!=0)) os<<name<<" ";
	os<<"<"<<get_ie_name()<<">:";
	level++;
	os<<setw(level*indent)<<"";
	os<<"NSLP ID: " <<  nslpid << endl;
	if (get_nattraversal()) print_object(nattrav_ptr,os,level,indent,objects,"NAT traversal object");
	if (get_mri()) print_object(mri_ptr,os,level,indent,objects,"Message Routing Information");
	if (get_sessionid()) print_object(sessionid_ptr,os,level,indent,objects,"SessionID");
	if (get_nli()) print_object(nli_ptr,os,level,indent,objects,"NLI");
	if (get_querycookie()) print_object(querycookie_ptr,os,level,indent,objects,"Query Cookie");
	if (get_stackprop()) print_object(stackprop_ptr,os,level,indent,objects,"StackProposal");
	if (get_stackconf()) print_object(stackconf_ptr,os,level,indent,objects,"StackConfigurationData");
	if (get_nslpdata()) print_object(nslpdata_ptr,os,level,indent,objects,"NSLP Data");
	if (!objects) os <<" <empty>";
	return os;
} // end print

istream& query::input(istream& is, bool istty, uint32 level, const uint32 indent, const char* name) {
    


        ntlp_object* tmp;
	string s;
	
	if (istty) {
		cout<<setw(level*indent)<<"";
		if (name && (*name!=0)) cout<<name<<" ";
		cout<<"<"<<get_ie_name()<<">:"<<endl;
	} // end if istty
	level++;
	
	if (istty) cout<<setw(level*indent)<<""<<"MRI: ";
	
	cout << "NSLPID:";
	
	cin >> nslpid;

	arr.clear(mri_ptr,true);
	(tmp = new mri_pathcoupled)->input(is,istty,level,indent);
	arr.set(mri_ptr,tmp);
		
	arr.clear(sessionid_ptr,true);
	(tmp = new sessionid)->input(is,istty,level,indent);
	arr.set(sessionid_ptr,tmp);

	(tmp = new nli)->input(is,istty,level,indent);
	arr.set(nli_ptr,tmp);

	tmp = new querycookie;
	arr.set(querycookie_ptr, tmp);

	tmp = new stackprop;
	arr.set(stackprop_ptr, tmp);
	
	// add Forwards-TCP
	stackprofile* tmp2 = new stackprofile;
	tmp2->addprotocol(6);

	dynamic_cast<stackprop*>(tmp)->add_profile(*tmp2);

	tmp = new stack_conf_data;
	arr.set(stackconf_ptr, tmp);

	ma_protocol_option* tmp3 = new ma_protocol_option(6,0,false);
	tmp3->add16(3333);

	dynamic_cast<stack_conf_data*>(tmp)->add_protoption(*tmp3);


	(tmp = new nslpdata)->input(is,istty,level,indent);
	arr.set(nslpdata_ptr, tmp);
	
	// end catch_bad_alloc
	return is;



} // end input

void query::clear_pointers() {
	known_ntlp_pdu::clear_pointers();
	arr.clear(mri_ptr,false);
	arr.clear(sessionid_ptr,false);
	arr.clear(nli_ptr, false);
	arr.clear(querycookie_ptr, false);
	arr.clear(stackprop_ptr, false);
	arr.clear(stackconf_ptr, false);
	arr.clear(nslpdata_ptr, false);
} // end clear_pointers

/***** inherited from gist_pdu *****/

ntlp_pdu::deser_error_t query::accept_type_and_subtype(uint8 t) {
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

bool query::accept_object(known_ntlp_object* o, uint32& position, IEErrorList& errorlist) {
    if (state==deser_done) return false;
    if (o->is_mri()) {
	state=wait_sessionid;
	// duplicate object error
	if (mri_ptr) errorlist.put(new GIST_DuplicateObject(protocol_v1, Query, known_ntlp_object::MRI));
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
	if (sessionid_ptr) errorlist.put(new GIST_DuplicateObject(protocol_v1, Query, known_ntlp_object::SessionID));
	sessionid_ptr=arr.size();
	position=sessionid_ptr;
	return true;
    }
    if ((state==wait_sessionid) & (!(o->is_sessionid()))) return false;



    if ((state==wait_nli) & (o->is_nli())) { 
	state = wait_querycookie;
	// duplicate object error
	if (nli_ptr) errorlist.put(new GIST_DuplicateObject(protocol_v1, Query, known_ntlp_object::NLI));
	nli_ptr=arr.size();
	position=nli_ptr;
	return true;
    }
    if ((state==wait_nli) & (!(o->is_nli()))) return false;


    if ((state==wait_querycookie) & (o->is_querycookie())) { 
	state = wait_stackprop;
	// duplicate object error
	if (querycookie_ptr) errorlist.put(new GIST_DuplicateObject(protocol_v1, Query, known_ntlp_object::QueryCookie));
	querycookie_ptr=arr.size();
	position=querycookie_ptr;
	return true;
    }
    if ((state==wait_querycookie) & (!(o->is_querycookie()))) return false;
    

    if ((state==wait_stackprop) & (o->is_stackprop())) { 
	state = wait_stackconf;
	// duplicate object error
	if (stackprop_ptr) errorlist.put(new GIST_DuplicateObject(protocol_v1, Query, known_ntlp_object::StackProposal));
	stackprop_ptr=arr.size();
	position=stackprop_ptr;
	return true;
    }

    if ((state==wait_stackprop) & (o->is_nslpdata())) { 
	state = deser_done;
	// duplicate object error
	if (nslpdata_ptr) errorlist.put(new GIST_DuplicateObject(protocol_v1, Query, known_ntlp_object::Nslpdata));
	nslpdata_ptr=arr.size();
	position=nslpdata_ptr;
	return true;
    }

    if ((state==wait_stackprop) & (!(o->is_stackprop()))) return false;




    if ((state==wait_stackconf) & (o->is_stackconf())) { 
	state = wait_nslpdata;
	// duplicate object error
	if (stackconf_ptr) errorlist.put(new GIST_DuplicateObject(protocol_v1, Query, known_ntlp_object::StackConfiguration));
	stackconf_ptr=arr.size();
	position=stackconf_ptr;
	return true;
    }
    if ((state==wait_stackconf) & (!(o->is_stackconf()))) return false;

   if ((state==wait_nslpdata) & (o->is_nslpdata())) { 
	state = deser_done;
	// duplicate object error
	if (nslpdata_ptr) errorlist.put(new GIST_DuplicateObject(protocol_v1, Query, known_ntlp_object::Nslpdata));
	nslpdata_ptr=arr.size();
	position=nslpdata_ptr;
	return true;
    }
    if ((state==wait_nslpdata) & (!(o->is_nslpdata()))) return false;


	// When this is reached, something went wrong.
	return false;
} // end accept_object

bool query::deser_end(IEErrorList& errorlist) {
	switch (state) {
	    case initial:
	    case wait_mri:
		errorlist.put(new GIST_MissingObject(protocol_v1, Query, known_ntlp_object::MRI)); return false;
	    case wait_sessionid:
		errorlist.put(new GIST_MissingObject(protocol_v1, Query, known_ntlp_object::SessionID)); return false;
	    case wait_nli:
		errorlist.put(new GIST_MissingObject(protocol_v1, Query, known_ntlp_object::NLI)); return false;
	    case deser_done: return true;
		
	    case wait_stackprop:
		state=deser_done;
		return true;

	    case wait_stackconf:
		errorlist.put(new GIST_MissingObject(protocol_v1, Query, known_ntlp_object::StackConfiguration)); return false;

	    case wait_nslpdata:
		state=deser_done;
		return true;
	    default:
			errorlist.put(new IEError(IEError::ERROR_INVALID_STATE));
			return false;
	} // end switch state
} // end deser_end

/***** new members *****/

query::query() : known_ntlp_pdu(Query,7),
		 state(initial)
{
} // end constructor

query::query(mri* mr, sessionid* sid, nli* nl, querycookie* qucookie, stackprop* stckprop, stack_conf_data* stckconf, nslpdata* nslpdat) : known_ntlp_pdu(Query,7),
																     state(initial)
{
	nattrav_ptr=0;
	mri_ptr=1;
	sessionid_ptr=2;
	nli_ptr=3;
	querycookie_ptr=4;
	stackprop_ptr=5;
	stackconf_ptr=6;
	nslpdata_ptr=7;
	arr.set(mri_ptr,mr);
	arr.set(sessionid_ptr,sid);
	arr.set(nli_ptr,nl);
	if (qucookie) arr.set(querycookie_ptr,qucookie);
	if (stckprop) arr.set(stackprop_ptr,stckprop);
	if (stckconf) arr.set(stackconf_ptr,stckconf);
	if (nslpdat) arr.set(nslpdata_ptr,nslpdat);
} // end constructor

query::query(const query& n) : known_ntlp_pdu(n),
                               state(n.state)
{
} // end copy constructor

query& query::operator=(const query& n) {
	known_ntlp_pdu::operator=(n);
	
	return *this;
} // end operator=

// @}

}
