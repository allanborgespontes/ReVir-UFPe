/// ----------------------------------------*- mode: C++; -*--
/// @file nwnclient.cpp
/// client application that injects network events into GIST
/// via UNIX domain socket IPC interface
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

#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <linux/if_ether.h>
#include <arpa/inet.h>
#include <linux/if.h>

#include <errno.h>
#include <string.h>

#include <iostream>
#include <cstdlib>

#include "nwn_uds_msg.h"

using namespace std;
using namespace ntlp;


const string sockname("/tmp/gist_nwn");


void usage(const char *progname) {
  fprintf(stderr, "Usage:\n%s -aho <iface> <l2_ap_n> -4|-6 <l3_ar_n> <l3_mn>\n%s -ho <iface> <l2_ap_n> -4|-6 <l3_ar_n> <l3_mn>\n\n", progname, progname);
  exit(1);
}

void invalid_af(const char *str) {
  fprintf(stderr, "inet_pton(): Invalid address family: %s\n", str);
  exit(1);
}

void invalid_address(const char *str) {
  fprintf(stderr, "inet_pton(): Invalid address format: %s\n", str);
  exit(1);
}

void parse_mac_address(const char *str, uint8_t *mac) {
  uint32_t a, b, c, d, e, f;

  int ret = sscanf(str, "%x:%x:%x:%x:%x:%x", &a, &b, &c, &d, &e, &f);
  if (ret != 6) {
    fprintf(stderr, "Invalid address format: %s\n", str);
    exit(1);
  }

  mac[0] = a;
  mac[1] = b;
  mac[2] = c;
  mac[3] = d;
  mac[4] = e;
  mac[5] = f;
}


int main(int argc, char *argv[])
{
  int sockfd;
  struct sockaddr_un raddr;
  uint32_t register msg_type;
  uint8_t l2_ap_n[ETH_ALEN];
  uint32_t l3_af;


  // parsing command line arguments
  if (argc != 7) {
    usage(argv[0]);
  }

  if (strcmp(argv[1], "-aho") == 0) {
    msg_type = NWN_MSG_TYPE_AHO;
  }
  else if (strcmp(argv[1], "-ho") == 0) {
    msg_type = NWN_MSG_TYPE_HO;
  }
  else {
    usage(argv[0]);
  }

  parse_mac_address(argv[3], l2_ap_n);

  if (strcmp(argv[4], "-4") == 0) {
    l3_af = AF_INET;
  }
  else if (strcmp(argv[4], "-6") == 0) {
    l3_af = AF_INET6;
  }
  else {
    usage(argv[0]);
  }

  struct nwn_uds_msg msg;
  msg.nwn_msg_type = msg_type;
  if (msg_type == NWN_MSG_TYPE_AHO) {
    memcpy(msg.aho_msg.iface, argv[2], IFNAMSIZ);
    memcpy(msg.aho_msg.l2_ap_n, l2_ap_n, ETH_ALEN);

    msg.aho_msg.l3_af = l3_af;

    if(l3_af == AF_INET) {
      register int ret = inet_pton(AF_INET, argv[5], &msg.aho_msg.l3_ar_n_ipv4);
      if (ret < 0) {
        invalid_af("AF_INET");
      }
      else if (ret == 0) {
        invalid_address(argv[5]);
      }

      ret = inet_pton(AF_INET, argv[6], &msg.aho_msg.l3_mn_ipv4);
      if (ret < 0) {
        invalid_af("AF_INET");
      }
      else if (ret == 0) {
        invalid_address(argv[6]);
      }
    }
    else if(l3_af == AF_INET6) {
      register int ret = inet_pton(AF_INET6, argv[5], &msg.aho_msg.l3_ar_n_ipv6);
      if (ret < 0) {
        invalid_af("AF_INET6");
      }
      else if (ret == 0) {
        invalid_address(argv[5]);
      }

      ret = inet_pton(AF_INET6, argv[6], &msg.aho_msg.l3_mn_ipv6);
      if (ret < 0) {
        invalid_af("AF_INET6");
      }
      else if (ret == 0) {
        invalid_address(argv[6]);
      }
    }
  }
  else if (msg_type == NWN_MSG_TYPE_HO) {
    memcpy(msg.ho_msg.iface, argv[2], IFNAMSIZ);
    memcpy(msg.ho_msg.l2_ap_n, l2_ap_n, ETH_ALEN);

    msg.ho_msg.l3_af = l3_af;

    if(l3_af == AF_INET) {
      register int ret = inet_pton(AF_INET, argv[5], &msg.ho_msg.l3_ar_n_ipv4);
      if (ret < 0) {
        invalid_af("AF_INET");
      }
      else if (ret == 0) {
        invalid_address(argv[5]);
      }

      ret = inet_pton(AF_INET, argv[6], &msg.ho_msg.l3_mn_ipv4);
      if (ret < 0) {
        invalid_af("AF_INET");
      }
      else if (ret == 0) {
        invalid_address(argv[6]);
      }
    }
    else if(l3_af == AF_INET6) {
      register int ret = inet_pton(AF_INET6, argv[5], &msg.ho_msg.l3_ar_n_ipv6);
      if (ret < 0) {
        invalid_af("AF_INET6");
      }
      else if (ret == 0) {
        invalid_address(argv[5]);
      }

      ret = inet_pton(AF_INET6, argv[6], &msg.ho_msg.l3_mn_ipv6);
      if (ret < 0) {
        invalid_af("AF_INET6");
      }
      else if (ret == 0) {
        invalid_address(argv[6]);
      }
    }
  }


  // connecting to GIST
  if ((sockfd = socket(PF_UNIX, SOCK_SEQPACKET, 0)) == -1) {
    cout << "Could not create socket: " << strerror(errno) << endl;

    exit(1);
  }


  raddr.sun_family = AF_UNIX;
  strcpy(raddr.sun_path, sockname.c_str());
  if (connect(sockfd, (struct sockaddr *)&raddr, sizeof(raddr)) == -1) {
    cout << "Could not connect to socket " << sockname << ": "
      << strerror(errno) << endl;

    exit(1);
  }

  if(send(sockfd, &msg, sizeof(msg), 0) < 0) {
    cout << "Could not send message: " << strerror(errno) << endl;

    exit(1);
  }


	return 0;
}

// vi: expandtab softtabstop=2 tabstop=2 shiftwidth=2
