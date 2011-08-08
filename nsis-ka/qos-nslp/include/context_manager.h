// ===========================================================
//                      
// Copyright (C) 2008-2010, all rights reserved by
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
#ifndef _CONTEXT_MANAGER__H
#define _CONTEXT_MANAGER__H

#include "nslp_session_context_map.h"
#include "qos_nslp_aho_contextmap.h"

namespace qos_nslp {
  using namespace std;
  using namespace protlib;

class Context_Manager
{
public:

#ifdef USE_AHO
	Context_Manager():scm_vec(0), acm_vec(0){};
#else
	Context_Manager():scm_vec(0){};
#endif
	~Context_Manager(){ clear(); delete context_manager_p;};

	
	static Context_Manager* instance();
	void clear();

        NSLP_Session_Context_Map* get_scm();
#ifdef USE_AHO
        AhoContextMap* get_acm();
#endif
        void add_context_map();

private:
	std::vector< NSLP_Session_Context_Map* > scm_vec;
#ifdef USE_AHO
	std::vector< AhoContextMap* > acm_vec;
#endif

	// pointer to singleton
	static Context_Manager* context_manager_p;
}; // end class context manager


} // end namespace
#endif

