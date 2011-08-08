/// ----------------------------------------*- mode: C++; -*--
/// @file flowinfo.cpp
/// Flowinfo Class to query MIP transforms and tunnel enter/exit
/// ----------------------------------------------------------
/// $Id: flowinfo.cpp 5249 2010-04-26 08:40:59Z roehricht $
/// $HeadURL: https://svn.tm.kit.edu/nsis/ntlp/trunk/src/flowinfo.cpp $
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

#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <unistd.h>
#include <iostream>
#include <string>
#include <list>
#include <map>
#include <sstream>
#include <netdb.h>
#include <errno.h>

#include <uds.h>

#include "flowinfo.h"
#include "messages.h"

namespace ntlp {
  using namespace protlib;

Flowinfo *global_flowinfo_p = NULL;

struct _lt_ips {
    bool operator()(const struct mip6_uds_request *a,
      const struct mip6_uds_request *b) const {

            unsigned char *ptr = (unsigned char *)a;
            size_t i;
            for (i = 0; i < sizeof(struct mip6_uds_request); i++) {
                printf("%02x:", *ptr);
                ptr++;
            }
            printf(" ?= \n");
            ptr = (unsigned char *)b;
            for (i = 0; i < sizeof(struct mip6_uds_request); i++) {
                printf("%02x:", *ptr);
                ptr++;
            }
            bool res = memcmp(a, b, sizeof(struct mip6_uds_request)) < 0;
            printf(" %s\n", res?"YES":"NO");
        return (res);

//        return ((memcmp(a, b, sizeof(struct mip6_uds_request)) < 0));
    }
};

struct _cond_reply {
    pthread_cond_t cond;
    struct mip6_uds_msg buf;
    int status;
#define	REPLY_STATUS_WAITING	0
#define	REPLY_STATUS_GOOD	1
#define	REPLY_STATUS_DEAD	2
};

static struct mip6_uds_msg global_buf;
static pthread_mutex_t _reply_map_mtx;
static pthread_cond_t _global_cond;
static bool busy;

Flowstatus::Flowstatus(mri_pathcoupled &orig_mri,
    mri_pathcoupled *new_mri, mri_pathcoupled *tunnel_id,
    flowstatus_t type, uint32 pp_overhead)
  : orig_mri(orig_mri),
    new_mri(new_mri),
    tunnel_id(tunnel_id),
    type(type),
    pp_overhead(pp_overhead)
{
    // new_mri should always be "the right thing"[tm]
    if (type == fs_nothing)
        new_mri = tunnel_id = &orig_mri;
    if (type == fs_normal)
        tunnel_id = &orig_mri;
    if (type == fs_home)
        tunnel_id = &orig_mri;
}

Flowstatus::~Flowstatus()
{
    if (type == fs_tunnel) {
        delete new_mri;
        delete tunnel_id;
    }
    if (type == fs_normal || type == fs_home)
        delete new_mri;
}

MobilityMsg::MobilityMsg() :
	message(type_mobility),
	msg_fs(0)
{
}

MobilityMsg::~MobilityMsg()
{
    if (msg_fs)
        delete msg_fs;
}

FlowinfoParam::FlowinfoParam(bool debug)
  : ThreadParam(ThreadParam::default_sleep_time, "Flowinfo"),
  debug(debug)
{
}

Flowinfo::Flowinfo(const FlowinfoParam &p)
  : Thread(p),
    param(p)
{
    pthread_mutex_init(&_reply_map_mtx, NULL);
    pthread_cond_init(&_global_cond, NULL);
    busy = false;
    global_flowinfo_p = this;
    sockfd = -1;
    connected = false;
}

Flowinfo::~Flowinfo()
{
    global_flowinfo_p = NULL;
}

void
Flowinfo::send_flow_update(void *arg)
{
    struct mip6_uds_msg *buf = (struct mip6_uds_msg *)arg;

    MobilityMsg *msg = new MobilityMsg();
    Flowstatus *fs;
    mri_pathcoupled *orig_mri = new mri_pathcoupled();
    mri_pathcoupled *new_mri;

    hostaddress ha;
    ha.set_ip(buf->mum_reply.orig_src);
    orig_mri->set_sourceaddress(ha);
    ha.set_ip(buf->mum_event.orig_dst);
    orig_mri->set_destaddress(ha);

    new_mri = orig_mri->copy();
    ha.set_ip(buf->mum_reply.new_src);
    if (buf->mum_event.flags & (MUM_REPLY_FLAG_SRC | MUM_REPLY_FLAG_TUNNEL |
      MUM_REPLY_FLAG_HOMEREG))
        new_mri->set_sourceaddress(ha);
    ha.set_ip(buf->mum_event.new_dst);
    if (buf->mum_event.flags & (MUM_REPLY_FLAG_DST | MUM_REPLY_FLAG_TUNNEL |
      MUM_REPLY_FLAG_HOMEREG))
        new_mri->set_destaddress(ha);

    if (buf->mum_event.flags & MUM_REPLY_FLAG_TUNNEL) {
        mri_pathcoupled *tunnel_info = new mri_pathcoupled();
        fs = new Flowstatus(*orig_mri, new_mri, tunnel_info,
            Flowstatus::fs_tunnel, 40);  // IP6inIP6-Tunnel
    } if (!(buf->mum_event.flags & MUM_REPLY_FLAG_HOMEREG)) {
        int overhead = 0;
        if (buf->mum_event.flags & MUM_REPLY_FLAG_SRC)
            overhead += 24;
        if (buf->mum_event.flags & MUM_REPLY_FLAG_DST)
            overhead += 24;
        // A bit more if we have both, but that's good enough for the moment.
        fs = new Flowstatus(*orig_mri, new_mri, 0, Flowstatus::fs_normal,
            overhead);
    } else {
        fs = new Flowstatus(*orig_mri, new_mri, 0, Flowstatus::fs_home, 0);
    }

    msg->set_flowstatus(fs);

    msg->send_or_delete(message::qaddr_mobility, message::qaddr_coordination);
}

void
Flowinfo::main_loop(uint32 nr)
{
    int retry = 0;

    while (get_state()==STATE_RUN) {

    struct sockaddr_un raddr = {0};

    sleep(retry);
    retry += 1;

    if ((sockfd = socket(PF_UNIX, SOCK_SEQPACKET, 0)) == -1) {
        DLog(param.name,
             "Could not create socket!" << strerror(errno));
        continue;
    }

    raddr.sun_family = AF_UNIX;
    strcpy(raddr.sun_path, MUM_SOCK_NAME);
    if (connect(sockfd, (struct sockaddr *)&raddr, sizeof(raddr)) == -1) {
        DLog(param.name,
             "Could not connect socket!" << strerror(errno));
        continue;
    }

    connected = true;

    struct pollfd fds = {0};
    struct mip6_uds_msg buf;
    int l, error = 0;

    Log(DEBUG_LOG, LOG_NORMAL, "Flowinfo", "main: ready");

    retry = 0;

    while (get_state()==STATE_RUN) {
        fds.fd = sockfd;
        fds.events = POLLIN;
        fds.revents = 0;

        error = poll(&fds, 1, param.sleep_time);
        if (error == 0)
            continue;
        if (error == -1) {
            if (errno == EINTR)
                continue;
            DLog(param.name,
                "error while polling: " << strerror(errno));
            break;
        }

        Log(DEBUG_LOG, LOG_NORMAL, "Flowinfo", "main: recv'ing");
        l = recv(sockfd, &buf, sizeof(buf), 0);
        if (l == 0) {
            DLog(param.name,
                "mip6d has shut down, trying to reconnect");
            break;
        }
        if (l < 0) {
            if (errno == EAGAIN || errno == EINTR)
                continue;
            DLog(param.name,
                "error while recv'ing: " << strerror(errno));
        }
        if (l != sizeof(buf)) {
            DLog(param.name,
                "got message of wrong size: " << l);
            continue;
        }
        switch (buf.mum_type) {
        case MUM_TYPE_REPLY: {
            Log(DEBUG_LOG, LOG_NORMAL, "Flowinfo", "main: got REPLY");
            pthread_mutex_lock(&_reply_map_mtx);
            global_buf = buf;
            pthread_mutex_unlock(&_reply_map_mtx);
            pthread_cond_broadcast(&_global_cond);
            break;
        }
        case MUM_TYPE_EVENT: {
            Log(DEBUG_LOG, LOG_NORMAL, "Flowinfo", "main: got EVENT");
            send_flow_update(&buf);
            break;
        }
        case MUM_TYPE_ERROR:
        default:
            break;
        }
    } // inner working loop

    connected = false;

    } // outer working loop
}

Flowstatus *
Flowinfo::get_flowinfo(mri_pathcoupled &orig_mri)
{
    mri_pathcoupled *new_mri = orig_mri.copy();
    ssize_t count;
    int error;

    if (!connected)
        return new Flowstatus(orig_mri, new_mri, 0, Flowstatus::fs_nothing, 0);

    Log(DEBUG_LOG, LOG_NORMAL, "Flowinfo", "get_flowinfo: start");

    pthread_mutex_lock(&_reply_map_mtx);
    while (busy)
        pthread_cond_wait(&_global_cond, &_reply_map_mtx);

    busy = true;

    global_buf.mum_type = MUM_TYPE_REQUEST;
    if (orig_mri.get_downstream()) {
        orig_mri.get_sourceaddress().get_ip(global_buf.mum_request.src_addr);
        orig_mri.get_destaddress().get_ip(global_buf.mum_request.dst_addr);
    } else {
        orig_mri.get_sourceaddress().get_ip(global_buf.mum_request.dst_addr);
        orig_mri.get_destaddress().get_ip(global_buf.mum_request.src_addr);
    }

    count = send(sockfd, &global_buf, sizeof(global_buf), 0);
    if (count != sizeof(global_buf)) {
        Log(DEBUG_LOG, LOG_NORMAL, "Flowinfo", "get_flowinfo: send error");
        busy = false;
        pthread_mutex_unlock(&_reply_map_mtx);
        pthread_cond_broadcast(&_global_cond);
        return new Flowstatus(orig_mri, new_mri, 0, Flowstatus::fs_nothing, 0);
    }

    if ((error = pthread_cond_wait(&_global_cond, &_reply_map_mtx)) != 0) {
        Log(DEBUG_LOG, LOG_NORMAL, "Flowinfo", "get_flowinfo: wait error" << strerror(error));
        busy = false;
        pthread_mutex_unlock(&_reply_map_mtx);
        pthread_cond_broadcast(&_global_cond);
        return new Flowstatus(orig_mri, new_mri, 0, Flowstatus::fs_nothing, 0);
    }

    Log(DEBUG_LOG, LOG_NORMAL, "Flowinfo", "get_flowinfo: success");

    hostaddress ha;
    ha.set_ip(global_buf.mum_reply.new_src);
    if (global_buf.mum_reply.flags & (MUM_REPLY_FLAG_SRC | MUM_REPLY_FLAG_TUNNEL |
      MUM_REPLY_FLAG_HOMEREG))
        new_mri->set_sourceaddress(ha);
    ha.set_ip(global_buf.mum_reply.new_dst);
    if (global_buf.mum_reply.flags & (MUM_REPLY_FLAG_DST | MUM_REPLY_FLAG_TUNNEL |
      MUM_REPLY_FLAG_HOMEREG))
        new_mri->set_destaddress(ha);

    if (global_buf.mum_reply.flags & MUM_REPLY_FLAG_TUNNEL) {
        mri_pathcoupled *tunnel_info = new mri_pathcoupled();
        busy = false;
        pthread_mutex_unlock(&_reply_map_mtx);
        pthread_cond_broadcast(&_global_cond);
        return new Flowstatus(orig_mri, new_mri, tunnel_info,
            Flowstatus::fs_tunnel, 40);  // IP6inIP6-Tunnel
    } else {
        int overhead = 0;
        if (global_buf.mum_reply.flags & (MUM_REPLY_FLAG_SRC | MUM_REPLY_FLAG_HOMEREG))
            overhead += 24;
        if (global_buf.mum_reply.flags & MUM_REPLY_FLAG_DST)
            overhead += 24;
        // A bit more if we have both, but that's good enough for the moment.
        busy = false;
        pthread_mutex_unlock(&_reply_map_mtx);
        pthread_cond_broadcast(&_global_cond);
        if (overhead != 0)
	    return new Flowstatus(orig_mri, new_mri, 0, Flowstatus::fs_normal,
                 overhead);
	else
	    return new Flowstatus(orig_mri, new_mri, 0, Flowstatus::fs_nothing,
	         0);
    }
}

} // end namespace ntlp
