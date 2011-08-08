/// ----------------------------------------*- mode: C++; -*--
/// @file hello.cpp
/// Hello PDU
/// ----------------------------------------------------------
/// $Id: hello.cpp 5249 2010-04-26 08:40:59Z roehricht $
/// $HeadURL: https://svn.tm.kit.edu/nsis/ntlp/trunk/src/pdu/hello.cpp $
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
#include <hello.h>
#include <iomanip>
#include <ntlp_pdu.h>
#include <ntlp_object.h>


namespace ntlp {
    using namespace protlib;
/**
 * @defgroup hellopdu GIST Hello PDU
 *  @ingroup ientlppdu
 * @{
 */


/***** class hello *****/

const char* const hello::iename = "hello";

const uint32 hello::numobj = 1;

/***** inherited from IE *****/

hello* hello::new_instance() const {
	hello* n = NULL;
	catch_bad_alloc(n = new hello());
	return n;
} // end new_instance

hello* hello::copy() const {
	hello* n = NULL;
	catch_bad_alloc(n =  new hello(*this));
	return n;
} // end copy 

bool hello::check() const {
	if (known_ntlp_pdu::check()) {
		if (!get_helloid())
			return false;
		return true;
	} else
		return false;
} // end check

bool hello::operator==(const IE& ie) const {
	const hello* n = dynamic_cast<const hello*>(&ie);
	if (n && known_ntlp_pdu::operator==(*n)) {
		return n->get_helloid() == get_helloid();
	} else
		return false;
} // end operator==

const char* hello::get_ie_name() const { return iename; }

ostream& hello::print(ostream& os, uint32 level, const uint32 indent, const char* name) const {
	bool objects = false;
	os<<setw(level*indent)<<"";
	if (name && (*name!=0)) os<<name<<" ";
	os<<"<"<<get_ie_name()<<">:";
	level++;
	os<<setw(level*indent)<<"";
	if (get_helloid()) print_object(helloid_ptr,os,level,indent,objects,"HelloID");
	if (!objects) cout<<" <empty>";
	return os;
} // end print

istream& hello::input(istream& is, bool istty, uint32 level, const uint32 indent, const char* name) {
    



	// end catch_bad_alloc
	return is;



} // end input

void hello::clear_pointers() {
	known_ntlp_pdu::clear_pointers();
	arr.clear(helloid_ptr,false);

} // end clear_pointers

/***** inherited from gist_pdu *****/

ntlp_pdu::deser_error_t hello::accept_type_and_subtype(uint8 t) {
    return deser_ok;
} // end accept_type_and_subtype


bool hello::accept_object(known_ntlp_object* o, uint32& position, IEErrorList& errorlist) {
	if (state == deser_done)
		return false;

	if (o->is_helloid()) {
		state = deser_done;
		position = helloid_ptr = arr.size();
		return true;
	}

	return false;
} // end accept_object


bool hello::deser_end(IEErrorList& errorlist) {

    return (state == deser_done);

} // end deser_end

/***** new members *****/

hello::hello() : known_ntlp_pdu(Hello,numobj), state(initial) {
	helloid_ptr = 1;
} // end constructor

hello::hello(helloid *id) : known_ntlp_pdu(Hello,numobj), state(initial) {
	helloid_ptr = 1;
	if (id)
		arr.set(helloid_ptr, id);
}

hello::hello(const hello& n) : known_ntlp_pdu(n), state(n.state) {
	helloid *myhid;
	helloid_ptr = 1;
	if ((myhid = n.get_helloid())) {
		myhid = new helloid(*myhid);
		arr.set(helloid_ptr, myhid);
	}
} // end copy constructor

hello& hello::operator=(const hello& n) {
	known_ntlp_pdu::operator=(n);
	
	return *this;
} // end operator=

// @}

}
