/// ----------------------------------------*- mode: C++; -*--
/// @file flowinfo.h
/// Flowinfo Class to query MIP transforms and tunnel enter/exit
/// ----------------------------------------------------------
/// $Id: flowinfo.h 5249 2010-04-26 08:40:59Z roehricht $
/// $HeadURL: https://svn.tm.kit.edu/nsis/ntlp/trunk/include/flowinfo.h $
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
#ifndef FLOWINFO_H
#define FLOWINFO_H

#include <list>

// from protlib
#include "threads.h"
#include "address.h"

// from ntlp
#include "mri_pc.h"

#include "ntlp_global_constants.h"

namespace ntlp {
    using namespace protlib;
    using std::list;

struct Flowstatus {
public:
    enum flowstatus_t {
        fs_nothing,
        fs_normal,
        fs_tunnel,
        fs_home
    };

    Flowstatus(mri_pathcoupled &orig_mri, mri_pathcoupled *new_mri = 0,
        mri_pathcoupled *tunnel_id = 0, flowstatus_t type = fs_nothing,
        uint32 pp_overhead = 0);
    ~Flowstatus();

    mri_pathcoupled orig_mri;
    mri_pathcoupled *new_mri, *tunnel_id;
    flowstatus_t type;
    uint32 pp_overhead;
};

class MobilityMsg : public message {
public:
	MobilityMsg();
	/// destructor
	virtual ~MobilityMsg();

	void set_flowstatus(struct Flowstatus *fs) { msg_fs = fs; }
	struct Flowstatus *get_flowstatus() const { return msg_fs; }
private:
	struct Flowstatus *msg_fs;
};

struct FlowinfoParam : public ThreadParam {
    FlowinfoParam(bool debug = false);

    bool debug;
};

class Flowinfo : public Thread {
public:
    Flowinfo(const FlowinfoParam &p);
    ~Flowinfo();
    void main_loop(uint32 nr);

    Flowstatus *get_flowinfo(mri_pathcoupled &orig_mri);

private:
    void send_flow_update(void *arg);	// a-void header entanglement
    FlowinfoParam param;
    int sockfd;
    bool connected;
};

} // end namespace ntlp

#endif // FLOWINFO_H
