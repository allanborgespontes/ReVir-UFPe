/// ----------------------------------------*- mode: C++; -*--
/// @file general_objects.h
/// SII-Handle object
/// ----------------------------------------------------------
/// $Id: general_objects.h 5249 2010-04-26 08:40:59Z roehricht $
/// $HeadURL: https://svn.tm.kit.edu/nsis/ntlp/trunk/src/general_objects.h $
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
/*********************************************************

holding objects which are not for serialization and are used
in API or internal processing of NTLP modules

**********************************************************/

#include "protlib_types.h"
#include "address.h"




namespace ntlp {


    using namespace protlib;
    using namespace protlib::log;


class sii_handle  
{
public:
        
    
inline 
uint32 get_handle()
    {
	return handle;
	
    };
 
inline 
void set_handle(uint32 hnd)
 {
     handle=hnd;
 };


inline
    sii_handle() 
    { 
	srand((unsigned)time(NULL));
	handle=rand();

//all done in initializer
    };

inline
    ~sii_handle()
    {
	//nothing to do
    };

 
 
 private:
 
 uint32 handle;
 

	
};
 



}
