/// ----------------------------------------*- mode: C++; -*--
/// @file nslp_session_context_map.cpp
/// QoS NSLP Session Context Map
/// ----------------------------------------------------------
/// $Id: nslp_session_context_map.cpp 5253 2010-04-26 13:41:09Z roehricht $
/// $HeadURL: https://svn.tm.kit.edu/nsis/qos-nslp/trunk/src/nslp_session_context_map.cpp $
// ===========================================================
//                      
// Copyright (C) 2005-2010, all rights reserved by
// - Institute of Telematics, Karlsruhe Institute of Technology
//
// More information and contact:
// https://projekte.tm.uka.de/trac/NSIS
//                      
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; version 2 of the License
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
// 02110-1301 USA.
//
// ===========================================================

#include "nslp_session_context_map.h"

namespace qos_nslp {


NSLP_Session_Context_Map::NSLP_Session_Context_Map() {
    pthread_mutexattr_init(&mutex_attr);
#ifdef _DEBUG
    pthread_mutexattr_settype(&mutex_attr, PTHREAD_MUTEX_ERRORCHECK);
#else
    pthread_mutexattr_settype(&mutex_attr, PTHREAD_MUTEX_NORMAL);
#endif
    
    pthread_mutex_init(&mutex, &mutex_attr);  
}


NSLP_Session_Context_Map::~NSLP_Session_Context_Map() {
    pthread_mutex_destroy(&mutex);
    pthread_mutexattr_destroy(&mutex_attr);
}


/**
 * finds context for given sessionid
 */
NSLP_Session_Context* NSLP_Session_Context_Map::find_session_context(const sessionid &sid) {
    lock();
    
    session_context_map_const_it_t it = session_context_map.find(sid.get_hash());
    NSLP_Session_Context* context = (it != session_context_map.end()) ? it->second : NULL;
    
    unlock();
    
    return context;
}


/**
 * finds rii_hashmap for given sessionid
 */
NSLP_Session_Context_Map::riimap_t &NSLP_Session_Context_Map::get_rii_hashmap(const sessionid &sid) {
    lock();

    riimap_t *rii_hashmap;

    session_rii_map_const_it_t it;
    if ((it = session_to_rii_hashmap.find(sid.get_hash())) != session_to_rii_hashmap.end()) {
        rii_hashmap = it->second;
    }
    else {
        rii_hashmap = new NSLP_Session_Context_Map::riimap_t();

        pair<simple_sid, riimap_t *> tmppair;

        tmppair.first = sid.get_hash();
        tmppair.second = rii_hashmap;
        
        session_to_rii_hashmap.insert(tmppair);
    }

    assert(rii_hashmap != NULL);
    
    unlock();
    
    return *rii_hashmap;
}


void NSLP_Session_Context_Map::insert_mri2context_map(const sessionid &sid, const ntlp::mri_pathcoupled &lmri) {
    lock();

    mri2context_map.insert(pair<const mri_pathcoupled *, simple_sid>(lmri.copy(), sid.get_hash()));
    
    unlock();
}


NSLP_Session_Context* NSLP_Session_Context_Map::find_first(const mri_pathcoupled &mri, search_context &sc) {
    NSLP_Session_Context *context = NULL;
    
    lock();
    
    if (sc.sidlist)
        sc.sidlist->clear();
    else
        sc.sidlist = new list<simple_sid>;
    
    mri2context_map_it_t lb, ub, it;
    lb = mri2context_map.lower_bound(&mri);
    ub = mri2context_map.upper_bound(&mri);
    for (it = lb; it != ub && it != mri2context_map.end(); it++)
        sc.sidlist->push_front((*it).second);
    
    sc.slit = sc.sidlist->begin();
    while (sc.slit != sc.sidlist->end() && context == NULL) {
        session_context_map_const_it_t hit = session_context_map.find((*sc.slit));
        context = (hit != session_context_map.end()) ? hit->second : NULL;
        sc.slit++;
    }
    
    unlock();
    
    return context;
}


NSLP_Session_Context* NSLP_Session_Context_Map::find_next(search_context &sc) {
    NSLP_Session_Context *context = NULL;
    
    if (sc.sidlist == NULL)
        return NULL;

    lock();
    
    while (sc.slit != sc.sidlist->end() && context == NULL) {
        session_context_map_const_it_t hit = session_context_map.find((*sc.slit));
        context = (hit != session_context_map.end()) ? hit->second : NULL;
        sc.slit++;
    }
    
    unlock();
    
    return context;
}


/**
 * inserts context for given sessionid
 */
bool NSLP_Session_Context_Map::insert_session_context(const sessionid &sid, NSLP_Session_Context *context) {
    pair<uint64, NSLP_Session_Context *> tmppair;
    pair<session_context_map_it_t, bool> tmpiterator;

    tmppair.first = sid.get_hash();
    tmppair.second = context;
    
    lock();
    
    tmpiterator = session_context_map.insert(tmppair);
    
    unlock();

    /*
    lock();
    context_hashmap_it_t hit= context_hashmap.insert(make_pair(r_hash,context));
    context_hashmap[r_hash] = context;
    unlock();
    
    // hit.first now points to the newly inserted element
    // return success or failure
    return hit.second;
    */
    
    // hit.first now points to the newly inserted element
    // return success or failure

    return tmpiterator.second;
}


void NSLP_Session_Context_Map::erase(const sessionid &sid) {
    lock();
    
    session_context_map.erase(sid.get_hash());

    mri2context_map_it_t it = mri2context_map.begin();
    while (it != mri2context_map.end()) {
        if ((*it).second == sid.get_hash())
            mri2context_map.erase(it);
        
        it++;
    }
    
    unlock();
}


/*
 * Traverses all contexts of the NSLP_Session_Context_Map executing func on them. The parameters
 * param1 and param2 are passed unchanged to the function "func".
 * Because of locking reasons func should run as short as possible.
 */
void
NSLP_Session_Context_Map::traverse_contexts(state_manager *sm, proc_func_t func, void *param1, void *param2)
{
    lock();
    
    session_context_map_const_it_t it=session_context_map.begin();
    while(it != session_context_map.end()) {
        // execute function "func"
        (sm->*func)(it->second, param1, param2);
        
        it++;
    }
    
    unlock();
}

} // end namespace qos_nslp

// vi: expandtab softtabstop=4 tabstop=4 shiftwidth=4
