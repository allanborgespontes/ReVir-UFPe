/// ----------------------------------------*- mode: C++; -*--
/// @file tp_queryencap.h
/// GIST Query encapsulation
/// ----------------------------------------------------------
/// $Id: tp_queryencap_nfq.h 5874 2011-02-15 10:54:31Z bless $
/// $HeadURL: https://svn.tm.kit.edu/nsis/protlib/trunk/include/tp_queryencap_nfq.h $
// ===========================================================
//                      
// Copyright (C) 2005-2007, all rights reserved by
// - Institute of Telematics, Universitaet Karlsruhe (TH)
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
/** @ingroup tpqueryencap
 * @ file
 * TP over Query Encapsulation
 */

#ifndef TP_QE_H
#define TP_QE_H

#include <ext/hash_map>

#include "tp.h"
#include "threads.h"
#include "threadsafe_db.h"
#include "connectionmap.h"
#include "assocdata.h"
#include "tp_over_udp.h"
#include "linux/netfilter.h"

namespace protlib 
{
/** this struct conatains parameters that determine 
  * the behavior of listener and receiver threads in TPqueryEncap
  * @param common_header_length - size of the common header, packets must have at least this size
  * @param getmsglength         - pointer to function that knows how to extract the length from the common header
  * @param port                 - port number for master listener thread (server port)
  * @param raovec               - vector of router alert option values that should be intercepted
  * @param tpoverudp            - pointer to UDP send/recv module (required to get source port for sending packets)
  * @param strict_rao           - if set an RAO is strictly required for interception, otherwise the magic number suffices
  * @param magic_number         - GIST magic number (first 32 bit of UDP payload) in host byte order, only checked if != 0
  * @param sleep                - time (in ms) that listener and receiver wait at a poll() call
  * @param source               - source module address
  * @param dest                 - destination module, where internal message are sent
  */
struct TPqueryEncapParam : public ThreadParam 
{
  /// constructor
    TPqueryEncapParam(
      unsigned short common_header_length,
      bool (*const getmsglength) (NetMsg& m, uint32& clen_words),
      port_t p,
      vector<uint32>& raovec,
      const TPoverUDP* tpoverudp,
      const bool &strict_rao,
      uint32 GIST_magic_num= 0,
      uint32 sleep = ThreadParam::default_sleep_time,
      bool debug_pdu = false,
      message::qaddr_t source = message::qaddr_tp_queryencap,
      message::qaddr_t dest = message::qaddr_signaling,
      bool sendaborts = false,
      uint8 tos = 0x10) :
      ThreadParam(sleep,"TPqueryEncap",1,1),
      port(p),
      debug_pdu(debug_pdu),
      source(source),
      dest(dest),
      common_header_length(common_header_length),
      strict_rao(strict_rao),
      magic_number(GIST_magic_num),
      getmsglength(getmsglength),
      terminate(false),
      ip_tos(tos),
      raovec(raovec),
      tpoverudp(tpoverudp)
        {};

    /// port to bind master listener thread to
    const port_t port;
    bool debug_pdu;
    /// message source
    const message::qaddr_t source;
    const message::qaddr_t dest;
    /// what is the length of the common header
    const unsigned short common_header_length;
    /// RAO required for interception? (read-only)
    const bool &strict_rao;
    /// GIST magic number that should be verified (must be given in host order)
    const uint32 magic_number;

    /// function pointer to a function that figures out the msg length in number of 4 byte words
    /// it returns false if error occured (e.g., malformed header), result is returned in variable clen_words
    bool (*const getmsglength) (NetMsg& m, uint32& clen_words);
    
    /// should master thread terminate?
    const bool terminate;
    const uint8 ip_tos;
    vector<uint32>& raovec;

    /// function pointer to function returning a pointer to an already established socket
    /// that we use as sending socket in udp send
    const TPoverUDP* tpoverudp;
}; // end TPqueryEncapParam
    
    
/// TP over UDP
/** This class implements the TP interface using UDP. */
class TPqueryEncap : public TP, public Thread 
{
/***** inherited from TP *****/
public:
  virtual void terminate(const address& addr) {};
  virtual void send(protlib::NetMsg* msg_to_send, const protlib::address& destaddr, bool use_existing_conn, const  protlib::address *local_addr);
  /***** inherited from Thread *****/
public:
  /// main loop
  virtual void main_loop(uint32 nr);
  
/***** other members *****/
public:
  /// constructor
  // NOTE: We use the protocol identifier 255 for internal usage in the framework for this messages!
  TPqueryEncap(const TPqueryEncapParam& p);

  /// virtual destructor
  virtual ~TPqueryEncap();
  
  typedef
  struct receiver_thread_arg
  {
    const AssocData* peer_assoc;
    bool sig_terminate;
    bool terminated;
  public:
    receiver_thread_arg(const AssocData* peer_assoc) : 
      peer_assoc(peer_assoc), sig_terminate(false), terminated(true) {};
  } receiver_thread_arg_t;
  
  class receiver_thread_start_arg_t
  {
  public:
    TPqueryEncap* instance;
    receiver_thread_arg_t* rtargp;
    
    receiver_thread_start_arg_t(TPqueryEncap* instance, receiver_thread_arg_t* rtargp) :
      instance(instance), rtargp(rtargp) {};
  };

  
private:

  /// send a message to the network via UDP
  void udpsend(NetMsg* msg, appladdress* addr, const hostaddress *own_addr);
  
  /// a static starter method to invoke the IPv6 interceptor
  static void* ipv6_catcher_thread_starter(void *argp);

  /// IPv6 catcher thread procedure
  void ipv6_catcher_thread();

  /// a static starter method to invoke the IPv4 interceptor
  static void* ipv4_catcher_thread_starter(void *argp);

  /// IPv4 catcher thread procedure
  void ipv4_catcher_thread();

  /// callback functions for netfilter
  static int callback_rcv_v4(struct nfq_q_handle *qh, struct nfgenmsg *nfmsg, struct nfq_data *tb, void *callback_data);
  static int callback_rcv_v6(struct nfq_q_handle *qh, struct nfgenmsg *nfmsg, struct nfq_data *tb, void *callback_data);
  
  /// terminates all active receiver or sender threads
  void terminate_all_threads();

  void setup_socket_ipv4(int sockfd, int hop_limit, int rao);
  void setup_socket_ipv6(int sockfd, int hop_limit, int rao);
  
  /// ConnectionMap instance for keeping track of all existing connections
  ConnectionMap connmap;
  
  /// parameters for main TPqueryEncap thread
  const TPqueryEncapParam tpparam;
  
  /// did we already abort at thread shutdown
  bool already_aborted;
  /// message queue
  FastQueue* msgqueue;

    int master_listener_socket;
  
  bool debug_pdu;
}; // end class TPqueryEncap

/** A simple internal message for selfmessages
 * please note that carried items may get deleted after use of this message 
 * the message destructor does not delete any item automatically
 */
class TPqueryEncapMsg : public message 
{
 public:
  // message type start/stop thread, send data
  enum msg_t { start, 
	       stop,
	       send_data
  };

 private:
  const AssocData* peer_assoc;
  const TPqueryEncapMsg::msg_t type;
  NetMsg* netmsg;
  appladdress* addr;

public:
  TPqueryEncapMsg(const AssocData* peer_assoc, message::qaddr_t source= qaddr_unknown, TPqueryEncapMsg::msg_t type= stop) : 
    message(type_transport, source), peer_assoc(peer_assoc), type(type), netmsg(0), addr(0)  {}

  TPqueryEncapMsg(NetMsg* netmsg, appladdress* addr, message::qaddr_t source= qaddr_unknown) : 
    message(type_transport, source), peer_assoc(0), type(send_data), netmsg(netmsg), addr(addr) {}

  const AssocData* get_peer_assoc() const { return peer_assoc; }
  TPqueryEncapMsg::msg_t get_msgtype() const { return type; }
  NetMsg* get_netmsg() const { return netmsg; }
  appladdress* get_appladdr() const { return addr; } 
};

} // end namespace protlib

#endif
