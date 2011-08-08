/// ----------------------------------------*- mode: C++; -*--
/// @file poolobject.h
/// base class for all objects that shall be managed by the objectpool
/// ----------------------------------------------------------
/// $Id: poolobject.h 5249 2010-04-26 08:40:59Z roehricht $
/// $HeadURL: https://svn.tm.kit.edu/nsis/ntlp/trunk/include/poolobject.h $
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
/** @ingroup objectpool
 *
 * This is the base class for all objects that shall be managed by the 
 * object pool.
 * Those classes simply must inherit from poolobject.
 * Use new and delete to get and free objects.
 *
 * You can turn off object pool support without cnanging the source code
 * by defining _NO_OBJECTPOOL.
 */

#ifndef POOL_OBJECT_H
#define POOL_OBJECT_H

#ifndef _NO_POOL_OBJECT
#include "objectpool.h"
#endif

using namespace protlib;

namespace drm {

/** @addtogroup objectpool Object Pool
 * @{
 */

/// base class for pool objects
/** All classes managed by the object pool must inherit new and delete 
 * from this class. 
 *
 * The operators are defined inline in this header file, because they 
 * just call static objectpool methods. 
 */
class poolobject {
#ifndef _NO_OBJECTPOOL
	public:
	inline static void* operator new(size_t size) {
		return objectpool::getobject(size);
	} // end operator new
	inline static void operator delete(void* obj, size_t size) {
		return objectpool::putobject(obj,size);
	} // end operator new
#endif
};

//@}

} // end namespace drm

#endif	
