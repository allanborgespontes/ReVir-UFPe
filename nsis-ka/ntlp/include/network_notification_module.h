/// ----------------------------------------*- mode: C++; -*--
/// @file network_notification_module.h
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
#ifndef NETWORK_NOTIFICATION_MODULE_H
#define NETWORK_NOTIFICATION_MODULE_H

// from protlib
#include "threads.h"

#include "nwn_uds_msg.h"


namespace ntlp {
using namespace protlib;


struct NetworkNotificationModuleParam : public ThreadParam {
  NetworkNotificationModuleParam(string uds_socket);
  string socket;
};


class NetworkNotificationModule : public Thread {
public:
  NetworkNotificationModule(const NetworkNotificationModuleParam &p);
  ~NetworkNotificationModule();

  void main_loop(uint32 nr);

private:
  NetworkNotificationModuleParam param;
};


class HandoverMsg : public message {
public:
  // constructor
  HandoverMsg();

  // destructor
  virtual ~HandoverMsg();


  void set_uds_msg(const struct nwn_uds_msg message) { msg = message; }
  struct nwn_uds_msg get_uds_msg() const { return msg; }
private:
  struct nwn_uds_msg msg;
};


} // end namespace ntlp

#endif // NETWORK_NOTIFICATION_MODULE_H

// vi: expandtab softtabstop=2 tabstop=2 shiftwidth=2
