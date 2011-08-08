// ===========================================================
//                      
// Copyright (C) 2005-2011, all rights reserved by
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
/** @file
 * The context manager singleton separates different context maps
 * in a vector for using QoS_NSLP in simulations.
 */

#include "context_manager.h"

#ifdef NSIS_OMNETPP_SIM
#include <module_manager.h>
#endif

namespace qos_nslp {
  using namespace std;
  using namespace protlib;

Context_Manager* Context_Manager::context_manager_p = NULL;

void Context_Manager::clear()
{
    for(unsigned int i = 0; i < scm_vec.size(); i++) {
        delete scm_vec[i];
    }
#ifdef USE_AHO
    for(unsigned int j = 0; j < acm_vec.size(); j++) {
        delete acm_vec[j];
    }
#endif
}

Context_Manager* Context_Manager::instance()
{
    if (context_manager_p)
        return context_manager_p;

    context_manager_p = new Context_Manager();
    return context_manager_p;
}

void Context_Manager::add_context_map()
{
    unsigned int nsisid = 1;

    //get NSIS ID if it's an omnet++ simulation
#ifdef NSIS_OMNETPP_SIM
    nsisid = ModuleManager::getCurrentNsisId();
#endif

    /* resize vector if too small
     * fill new vector entries with new context maps
     * supports non continuous initialization
    */
    if (scm_vec.size() < (nsisid + 1))
        scm_vec.resize(nsisid + 1 , new NSLP_Session_Context_Map());
#ifdef USE_AHO
    if (acm_vec.size() < (nsisid + 1))
        acm_vec.resize(nsisid + 1 , new AhoContextMap());
#endif

}

NSLP_Session_Context_Map* Context_Manager::get_scm()
{
    unsigned int nsisid = 1;

    //get NSIS ID if it's an omnet++ simulation
#ifdef NSIS_OMNETPP_SIM
    nsisid = ModuleManager::getCurrentNsisId();
#endif

    if (scm_vec.size() < (nsisid + 1)) {
        std::cout << "Context_Manager: Map not added yet, adding it now." << std::endl;
        add_context_map();
    }

    return scm_vec[nsisid];
}

#ifdef USE_AHO
AhoContextMap* Context_Manager::get_acm()
{
    unsigned int nsisid = 1;

    //get NSIS ID if it's an omnet++ simulation
#ifdef NSIS_OMNETPP_SIM
    nsisid = ModuleManager::getCurrentNsisId();
#endif

    if (acm_vec.size() < (nsisid + 1)) {
        std::cout << "Context_Manager: Map not added yet, adding it now." << std::endl;
        add_context_map();
    }

    return acm_vec[nsisid];
}
#endif

} // end namespace
