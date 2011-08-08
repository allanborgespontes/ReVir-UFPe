/// ----------------------------------------*- mode: C++; -*--
/// @file network_notification_module.cpp
/// NetworkNotification class which implements a thread that
/// waits for network events and injects them into GIST
/// ----------------------------------------------------------
/// $Id$
/// $HeadURL$
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
#include <arpa/inet.h>

#include <errno.h>
#include <string.h>
#include <iomanip>

#include "network_notification_module.h"
#include "nwn_uds_msg.h"
#include "messages.h"


namespace ntlp {
using namespace protlib;


NetworkNotificationModuleParam::NetworkNotificationModuleParam(string uds_socket)
  : ThreadParam(ThreadParam::default_sleep_time, "NetworkNotificationModule"),
  socket(uds_socket)
{
}



NetworkNotificationModule::NetworkNotificationModule(const NetworkNotificationModuleParam &p)
  : Thread(p),
  param(p)
{
}


NetworkNotificationModule::~NetworkNotificationModule()
{
}


void NetworkNotificationModule::main_loop(uint32 nr)
{
  int accept_fd;
  struct sockaddr_un saddr;

  int retry = 0;


  while (get_state()==STATE_RUN) {
    sleep(retry);
    retry += 1;

    unlink(param.socket.c_str());
    if ((accept_fd = socket(PF_UNIX, SOCK_SEQPACKET, 0)) == -1) {
      DLog(param.name, "Could not create socket: " << strerror(errno));

      continue;
    }

    memset(&saddr, 0, sizeof(saddr));
    saddr.sun_family = AF_UNIX;
    strcpy(saddr.sun_path, param.socket.c_str());
    if (bind(accept_fd, (struct sockaddr *)&saddr, sizeof(saddr)) == -1) {
      DLog(param.name, "Could not bind socket: " << strerror(errno));

      close(accept_fd);
      continue;
    }

    if (listen(accept_fd, 1) == -1) {
      DLog(param.name, "Listen at socket " << param.socket <<
          " failed, error: " << strerror(errno));

      close(accept_fd);
      continue;
    }

    Log(INFO_LOG, LOG_NORMAL, param.name, color[green] << "Listening at "
        << param.socket << color[off]);


    retry = 0;

    while (get_state()==STATE_RUN) {
      struct pollfd fds;

      fds.fd = accept_fd;
      fds.events = POLLIN;
      fds.revents = 0;

      int ret = poll(&fds, 1, param.sleep_time * 500);
      if (ret == 0) {
        continue;
      }
      else if (ret == -1) {
        if (errno == EINTR) {
          continue;
        }

        DLog(param.name, "Error while polling: " << strerror(errno));

        close(accept_fd);
        break;
      }


      int client_fd;
      if ((client_fd = accept(accept_fd, NULL, 0)) == -1) {
        DLog(param.name, "Accept failed, error: " << strerror(errno));

        continue;
      }

      DLog(param.name, "Accept successful!");

      struct nwn_uds_msg buffer;
      int len = recv(client_fd, &buffer, sizeof(struct nwn_uds_msg), 0);
      if (len < 0) {
        if (errno == EAGAIN || errno == EINTR) {
          close(client_fd);
          continue;
        }

        DLog(param.name, "Error while receiving: " << strerror(errno));

        close(client_fd);
        close(accept_fd);
        break;
      }

      if (len != sizeof(struct nwn_uds_msg)) {
        DLog(param.name, "Got message of wrong size: " << len);

        close(client_fd);
        continue;
      }

      // A lot of debugging messages
      const uint32_t ADDRSTRLEN = INET_ADDRSTRLEN >= INET6_ADDRSTRLEN ? INET_ADDRSTRLEN : INET6_ADDRSTRLEN;
      char ip_addr_ar_n[ADDRSTRLEN];
      char ip_addr_mn[ADDRSTRLEN];
      uint8_t *mac;

      switch (buffer.nwn_msg_type) {
        case NWN_MSG_TYPE_AHO:

          Log(DEBUG_LOG, LOG_NORMAL, param.name, "Received Message of type: NWN_MSG_TYPE_AHO");

          Log(DEBUG_LOG, LOG_NORMAL, param.name, "AHO-iface: " << buffer.aho_msg.iface);

          mac = buffer.aho_msg.l2_ap_n;
          // typecast to uint32_t is needed because uint8_t is internally of type unsigned char!
          Log(DEBUG_LOG, LOG_NORMAL, param.name, "AHO-l2_ap_n: " << setfill('0') << hex
              << setw(2) << (uint32_t)mac[0] << ":"
              << setw(2) << (uint32_t)mac[1] << ":"
              << setw(2) << (uint32_t)mac[2] << ":"
              << setw(2) << (uint32_t)mac[3] << ":"
              << setw(2) << (uint32_t)mac[4] << ":"
              << setw(2) << (uint32_t)mac[5]);

          if (buffer.aho_msg.l3_af == AF_INET) {
            inet_ntop(AF_INET, &buffer.aho_msg.l3_ar_n_ipv4, ip_addr_ar_n, ADDRSTRLEN);
            inet_ntop(AF_INET, &buffer.aho_msg.l3_mn_ipv4, ip_addr_mn, ADDRSTRLEN);
          }
          else if(buffer.aho_msg.l3_af == AF_INET6) {
            inet_ntop(AF_INET6, &buffer.aho_msg.l3_ar_n_ipv6, ip_addr_ar_n, ADDRSTRLEN);
            inet_ntop(AF_INET6, &buffer.aho_msg.l3_mn_ipv6, ip_addr_mn, ADDRSTRLEN);
          }
          Log(DEBUG_LOG, LOG_NORMAL, param.name, "AHO-l3_ar_n: " << ip_addr_ar_n);
          Log(DEBUG_LOG, LOG_NORMAL, param.name, "AHO-l3_mn: " << ip_addr_mn);

          break;


        case NWN_MSG_TYPE_HO:

          Log(DEBUG_LOG, LOG_NORMAL, param.name, "Received Message of type: NWN_MSG_TYPE_HO");

          Log(DEBUG_LOG, LOG_NORMAL, param.name, "HO-iface: " << buffer.ho_msg.iface);

          mac = buffer.ho_msg.l2_ap_n;
          // typecast to uint32_t is needed because uint8_t is internally of type unsigned char!
          Log(DEBUG_LOG, LOG_NORMAL, param.name, "HO-l2_ap_n: " << setfill('0') << hex
              << setw(2) << (uint32_t)mac[0] << ":"
              << setw(2) << (uint32_t)mac[1] << ":"
              << setw(2) << (uint32_t)mac[2] << ":"
              << setw(2) << (uint32_t)mac[3] << ":"
              << setw(2) << (uint32_t)mac[4] << ":"
              << setw(2) << (uint32_t)mac[5]);

          if (buffer.ho_msg.l3_af == AF_INET) {
            inet_ntop(AF_INET, &buffer.ho_msg.l3_ar_n_ipv4, ip_addr_ar_n, ADDRSTRLEN);
            inet_ntop(AF_INET, &buffer.ho_msg.l3_mn_ipv4, ip_addr_mn, ADDRSTRLEN);
          }
          else if(buffer.ho_msg.l3_af == AF_INET6) {
            inet_ntop(AF_INET6, &buffer.ho_msg.l3_ar_n_ipv6, ip_addr_ar_n, ADDRSTRLEN);
            inet_ntop(AF_INET6, &buffer.ho_msg.l3_mn_ipv6, ip_addr_mn, ADDRSTRLEN);
          }
          Log(DEBUG_LOG, LOG_NORMAL, param.name, "AHO-l3_ar_n: " << ip_addr_ar_n);
          Log(DEBUG_LOG, LOG_NORMAL, param.name, "AHO-l3_mn: " << ip_addr_mn);

          break;
      }

      // passing message to Statemodule
      HandoverMsg *msg = new HandoverMsg();
      msg->set_uds_msg(buffer);
      msg->send_or_delete(message::qaddr_handover, message::qaddr_coordination);


      DLog(param.name, "Closing client socket.");

      close(client_fd);
    } // inner working loop
  } // outer working loop


  DLog(param.name, "Closing accepting socket " << param.socket);

  close(accept_fd);
  unlink(param.socket.c_str());
}


// constructor
HandoverMsg::HandoverMsg() : message(type_handover)
{
}

// destructor
HandoverMsg::~HandoverMsg()
{
}


} // end namespace ntlp

// vi: expandtab softtabstop=2 tabstop=2 shiftwidth=2
