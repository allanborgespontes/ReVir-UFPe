/// ----------------------------------------*- mode: C++; -*--
/// @file nslp_session_context_map.h
/// QoS NSLP Session Context Map
/// ----------------------------------------------------------
/// $Id: nslp_session_context_map.h 6282 2011-06-17 13:31:57Z bless $
/// $HeadURL: https://svn.tm.kit.edu/nsis/qos-nslp/trunk/include/nslp_session_context_map.h $
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
#ifndef NSLP_SESSION_CONTEXT_MAP_H
#define NSLP_SESSION_CONTEXT_MAP_H


#include "nslp_session_context.h"
#include "sessionid.h"
#include "rii.h"
#include "mri_pc.h"
#include "QoS_StateModule.h"


using namespace protlib;


namespace qos_nslp {


class NSLP_Session_Context_Map {
public:
    // rii_hashmap maps RII numbers to pointers of RII objects
    typedef hashmap_t<uint32, rii *> riimap_t;
    typedef riimap_t::iterator rii_hashmap_it_t;


    class search_context {
    public:
        search_context() { sidlist = NULL; };
        ~search_context() { delete sidlist; };

        list<simple_sid> *sidlist;
        list<simple_sid>::iterator slit;
    };


    // constructor
    NSLP_Session_Context_Map();

    // destructor
    ~NSLP_Session_Context_Map();


    NSLP_Session_Context *find_session_context(const sessionid &sid);
    NSLP_Session_Context_Map::riimap_t &get_rii_hashmap(const sessionid &sid);

    void insert_mri2context_map(const sessionid &sid, const ntlp::mri_pathcoupled &lmri);
    NSLP_Session_Context* find_first(const mri_pathcoupled &mri, search_context &sc);
    NSLP_Session_Context* find_next(search_context &sc);

    bool insert_session_context(const sessionid &sid, NSLP_Session_Context *context);
    void erase(const sessionid &sid);

    typedef void (state_manager::*proc_func_t)(NSLP_Session_Context *, void *, void *);
    void traverse_contexts(state_manager *sm, proc_func_t func, void *param1, void *param2);

private:
    // session_context_map stores all NSLP_Session_Contexts (session states)
    typedef hashmap_t<simple_sid, NSLP_Session_Context *> session_context_map_t;
    typedef session_context_map_t::iterator session_context_map_it_t;
    typedef session_context_map_t::const_iterator session_context_map_const_it_t;
    session_context_map_t session_context_map;

    // context multimap that stores flows with a logical mri associated
    typedef multimap<const ntlp::mri_pathcoupled *, simple_sid, ntlp::mri_pathcoupled::ltmri> mri2contextmap_t;
    typedef mri2contextmap_t::iterator mri2context_map_it_t;
    typedef mri2contextmap_t::const_iterator mri2context_map_const_it_t;
    mri2contextmap_t mri2context_map;

    // map which maps a SID to a pointers of a rii_hashmap
    typedef hashmap_t<simple_sid, riimap_t *> session_rii_map_t;
    typedef session_rii_map_t::iterator session_rii_map_it_t;
    typedef session_rii_map_t::const_iterator session_rii_map_const_it_t;
    session_rii_map_t session_to_rii_hashmap;


    // This mutex is used to lock the context map 
    pthread_mutex_t mutex;

    // mutex attribute for setting mutex kind
    pthread_mutexattr_t mutex_attr;


    // lock mutex
    void lock();

    // unlock mutex
    void unlock();
}; // end class NSLP_Session_Context_Map


inline void NSLP_Session_Context_Map::lock() {
    if (pthread_mutex_lock(&mutex)) {
        ERRLog("NSLP_Session_Context_Map", "Error while locking mutex");
    }
}


inline void NSLP_Session_Context_Map::unlock() {
    pthread_mutex_unlock(&mutex);
}

} // end namespace qos_nslp

#endif // NSLP_SESSION_CONTEXT_MAP_H

// vi: expandtab softtabstop=4 tabstop=4 shiftwidth=4
