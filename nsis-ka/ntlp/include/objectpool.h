/// ----------------------------------------*- mode: C++; -*--
/// @file objectpool.h
/// Object pool interface, optimization for unused objects
/// ----------------------------------------------------------
/// $Id: objectpool.h 6282 2011-06-17 13:31:57Z bless $
/// $HeadURL: https://svn.tm.kit.edu/nsis/ntlp/trunk/include/objectpool.h $
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
 * @file
 * Object pool interface.
 *
 * The object pool is a storage for unused objects. Every pool object 
 * has a common base class that defines operators now and delete. These do not
 * allocate memory the normal way, but look if there are objects in the pool.
 *
 * The object pool can manage objects of different sizes in one pool.
 * For each object size a queue is allocated which
 * holds pointers to available memory blocks.
 *
 * This is an internal header file.
 * Please include poolbject.h instead.
 *
 * @note if _USE_FASTQUEUE is defined, FastQueue is use instead of std::list
 * for object queues.
 */

#ifndef OBJECT_POOL_H
#define OBJECT_POOL_H

// turn on FastQueue
// seems tobe slower tieh fastqueue than with deque.
//#define _USE_FASTQUEUE

#include <stdlib.h>
#include <pthread.h>
#include "hashmap"

#ifdef _USE_FASTQUEUE
#include "fqueue.h"
#else
#include <deque>
#endif

#include "protlib_types.h"

using namespace protlib;

namespace drm {

/** @addtogroup objectpool Object Pool
 * @{
 */

/// ObjectPool --- singleton
class objectpool {
	public:
		/// get an object from the pool
		static void* getobject(size_t size);
		/// put an object into the pool
		static void putobject(void* obj, size_t size);
		/// free pool memory
		static void freepool();
		/// destructor
		virtual ~objectpool();		
	private:
		/// Pool initialized?
		static bool is_initialized;
		/// private default constructor
		objectpool();
		/// pointer to the one and only object pool
		static objectpool* instance;
		/// block size
		static const int blocksize;
		/// pool hash_map and queue types
		/** These types are needed to traverse the hash_map and the 
		 * pool queues to free all objects.
		 */
#ifdef _USE_FASTQUEUE
		typedef FastQueue poolqueue_t;
		typedef void* poolqueue_it_t;
#else
		typedef deque<void*> poolqueue_t;
		typedef poolqueue_t::iterator poolqueue_it_t;
#endif
		typedef hashmap_t<size_t,poolqueue_t*> poolhashmap_t;
		typedef poolhashmap_t::iterator poolhashmap_it_t;
		/// the pool hash_map
		poolhashmap_t* phm;
		/// calculate size of memory block
		static size_t getmemblocksize(size_t size);
		/// pool mutex
		static pthread_mutex_t poolmutex;
};

//@}

} // end namespace drm

#endif
