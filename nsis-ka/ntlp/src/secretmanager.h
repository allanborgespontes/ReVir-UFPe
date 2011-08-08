/// ----------------------------------------*- mode: C++; -*--
/// @file secretmanager.h
/// Secret managing entity (manages changing/roll-over) of secrets
/// ----------------------------------------------------------
/// $Id: secretmanager.h 6282 2011-06-17 13:31:57Z bless $
/// $HeadURL: https://svn.tm.kit.edu/nsis/ntlp/trunk/src/secretmanager.h $
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
/*

  Stores local secret and provides manipulation functions.

*/
#ifndef _NTLP__SECRETMANAGER_H_
#define _NTLP__SECRETMANAGER_H_


#include "ntlp_object.h"
#include "address.h"
#include "protlib_types.h"

#include "sessionid.h"
#include "signalingmodule_ntlp.h"
#include "mri.h"
#include "ntlp_pdu.h"
#include "nli.h"
#include "hashmap"
#include "query_cookie.h"
#include "resp_cookie.h"


namespace ntlp {
    using namespace protlib;


 
   
    // the object encapsulating the secret data structure, provides lookup and manipulation
    class secretmanager {
	
	
    public:
	
	/// Constructor, initially fills secrets table and generation number
	secretmanager(uint32 tablesize, uint32 secretsize); 
	
	~secretmanager();
	
	/// Returns current generation number, needed on cookie generation
	inline uint32 get_generation_number() { return generation_number; };

	/// Returns secret size in Byte
	inline
	    uint32 get_secret_size() {
	    return secretsize/8;

	};


	/// Returns current secret
	inline
	    uchar* get_secret() {
	    return secrets.at(generation_number % tablesize);
	};
	
	/// Returns secret with generation number n
        /// @return either pointer to secret or NULL if generation number is not valid
	inline
	    uchar* get_secret(uint32 n) {
	  // generation number must fit
	  if (gen_numbers.at(n%tablesize) == n)
	    return secrets.at(n%tablesize);
	  else // generation number not valid
	    return NULL;
	};

	/// performs rapid rollover
	void forward();


    protected:
	
 	/// vector holding the secrets
	vector<uchar*> secrets;
	vector<uint32> gen_numbers;

	/// uint32 providing current Version
	uint32 generation_number;

	/// Table Size (Secret Count)
	uint32 tablesize;

	/// Table Size (Secret Count)
	uint32 secretsize;


    };    
    

    
    
    
    
}



#endif
