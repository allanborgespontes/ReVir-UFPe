/// ----------------------------------------*- mode: C++; -*--
/// @file tp_over_uds.h
/// Transport over Unix Domain Sockets
/// ----------------------------------------------------------
/// $Id: tp_over_uds.h 6282 2011-06-17 13:31:57Z bless $
/// $HeadURL: https://svn.tm.kit.edu/nsis/protlib/trunk/include/tp_over_uds.h $
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
// ----------------------------------------*- mode: C++; -*--
// tp_over_uds.h
// Implementation of a transport module for Unix domain sockets
// ----------------------------------------------------------
// $Id: tp_over_uds.h 6282 2011-06-17 13:31:57Z bless $
// $HeadURL: https://svn.tm.kit.edu/nsis/protlib/trunk/include/tp_over_uds.h $
// ==========================================================
/** @ingroup transport
 * @file tp_over_uds.h
 * TP over UDS
 */

#ifndef TP_OVER_UDS_H
#define TP_OVER_UDS_H

#include "hashmap"

#include "tp.h"
#include "threads.h"
#include "threadsafe_db.h"
#include "connectionmap_uds.h"
#include "assocdata_uds.h"

namespace protlib 
{
/** this struct conatains parameters that determine 
  * the behavior of listener and receiver threads in TPoverUDS
  * @param port - port number for master listener thread (server port)
  * @param sleep - time (in ms) that listener and receiver wait at a poll() call
  * @param d - destination module, where internal message are sent
  */
struct TPoverUDSParam : public ThreadParam 
{
  /// constructor
  TPoverUDSParam(
         unsigned short common_header_length,
	 bool (*const getmsglength) (NetMsg& m, uint32& clen_bytes),
	 string udssocket,
	 bool server,
	 uint32 sleep = ThreadParam::default_sleep_time,
	 bool debug_pdu = false,
	 message::qaddr_t source = message::qaddr_transport,
	 message::qaddr_t dest = message::qaddr_signaling,
         bool sendaborts = false,
         uint8 tos = 0x10) :
      ThreadParam(sleep,"TPoverUDS", 1,1),
      udssocket(udssocket),
      debug_pdu(debug_pdu),
      source(source),
      dest(dest),
      common_header_length(common_header_length),
      getmsglength(getmsglength),
      terminate(false),
      ip_tos(tos),
      server(server)
        {};

    /// port to bind master listener thread to
    const string udssocket;
    bool debug_pdu;
    /// message source
    const message::qaddr_t source;
    const message::qaddr_t dest;
    /// what is the length of the common header
    const unsigned short common_header_length;
    
    /// function pointer to a function that figures out the msg length in number of 4 byte words
    /// it returns false if error occured (e.g., malformed header), result is returned in variable clen_words
    bool (*const getmsglength) (NetMsg& m, uint32& clen_words);
    
    /// should master thread terminate?
    const bool terminate;
    const uint8 ip_tos;
    bool server;
}; // end TPoverUDPParam


/// TP over TCP
/** This class implements the TP interface using TCP. */
class TPoverUDS : public TP, public Thread 
{
/***** inherited from TP *****/
public:
  /// sends a network message, spawns receiver thread if necessary
  virtual void send(NetMsg* msg, const address& addr, bool use_existing_connection, const address *local_addr);
  virtual void terminate(const address& addr);
  
  /***** inherited from Thread *****/
public:
  /// main loop
  virtual void main_loop(uint32 nr);
  
/***** other members *****/
public:
  /// constructor
  TPoverUDS(const TPoverUDSParam& p) :
    TP(253,"uds",p.name,p.common_header_length,p.getmsglength),
    Thread(p), tpparam(p), already_aborted(false), msgqueue(NULL), debug_pdu(p.debug_pdu)
  { 
    // perform some initializing actions
    // currently not required (SCTP had to init its library)
    init= true; ///< init done;
  }
  /// virtual destructor
  virtual ~TPoverUDS();
  
  typedef
  struct receiver_thread_arg
  {
    const AssocDataUDS* peer_assoc;
    bool sig_terminate;
    bool terminated;
  public:
    receiver_thread_arg(const AssocDataUDS* peer_assoc) : 
      peer_assoc(peer_assoc), sig_terminate(false), terminated(true) {};
  } receiver_thread_arg_t;
  
  class receiver_thread_start_arg_t
  {
  public:
    TPoverUDS* instance;
    receiver_thread_arg_t* rtargp;
    
    receiver_thread_start_arg_t(TPoverUDS* instance, receiver_thread_arg_t* rtargp) :
      instance(instance), rtargp(rtargp) {};
  };

  class sender_thread_start_arg_t
  {
  public:
    TPoverUDS* instance;
    FastQueue* sender_thread_queue;
    
    sender_thread_start_arg_t(TPoverUDS* instance, FastQueue* sq) :
      instance(instance), sender_thread_queue(sq) {};
  };
  
private:
  /// returns already existing connection or establishes a new one
  AssocDataUDS* get_connection_to(udsaddress& addr);

  /// receiver thread for a specific socket
  void sender_thread(void *argp);
  
  /// receiver thread for a specific socket
  void receiver_thread(void *argp);

  /// send a message to the local process via UNIX domain sockets
  void udssend(NetMsg* msg, udsaddress* addr);
  
  /// sender thread starter for a specific socket
  static void* sender_thread_starter(void *argp);

  /// receiver thread starter for a specific socket
  static void* receiver_thread_starter(void *argp);
  
  /// a static starter method to invoke the actual main listener
  static void* master_listener_thread_starter(void *argp);
  
  /// main listener thread procedure
  void master_listener_thread();
  
  // create and start new sender thread
  void create_new_sender_thread(FastQueue* senderqueue);

  // create and start new receiver thread
  void create_new_receiver_thread(AssocDataUDS* peer_assoc);
  
  /// terminates particular thread
  void stop_receiver_thread(AssocDataUDS* peer_assoc);

  /// cleans up thread management structures
  void cleanup_receiver_thread(AssocDataUDS* peer_assoc);

  /// terminates a sender thread
  void terminate_sender_thread(const AssocDataUDS* assoc);
  
  /// terminates all active receiver or sender threads
  void terminate_all_threads();
  
  /// ConnectionMap instance for keeping track of all existing connections
  ConnectionMapUDS connmap;
  
  /// store per receiver thread arguments, e.g. for signaling termination
  typedef hashmap_t<pthread_t, receiver_thread_arg_t*> recv_thread_argmap_t;
  recv_thread_argmap_t  recv_thread_argmap;

  /// store sender thread related information
  typedef hashmap_t<udsaddress, FastQueue*> sender_thread_queuemap_t;
  sender_thread_queuemap_t  senderthread_queuemap;
  
  /// parameters for main TPoverUDS thread
  const TPoverUDSParam tpparam;
  
  /// did we already abort at thread shutdown
  bool already_aborted;
  /// message queue
  FastQueue* msgqueue;
  
  bool debug_pdu;
}; // end class TPoverUDS

/** A simple internal message for selfmessages
 * please note that carried items may get deleted after use of this message 
 * the message destructor does not delete any item automatically
 */
class TPoverUDSMsg : public message 
{
 public:
  // message type start/stop thread, send data
  enum msg_t { start, 
	       stop,
	       send_data
  };

 private:
  const AssocDataUDS* peer_assoc;
  const TPoverUDSMsg::msg_t type;
  NetMsg* netmsg;
  udsaddress* addr;

public:
  TPoverUDSMsg(const AssocDataUDS* peer_assoc, message::qaddr_t source= qaddr_unknown, TPoverUDSMsg::msg_t type= stop) : 
    message(type_transport, source), peer_assoc(peer_assoc), type(type), netmsg(0), addr(0)  {}

  TPoverUDSMsg(NetMsg* netmsg, udsaddress* addr, message::qaddr_t source= qaddr_unknown) : 
    message(type_transport, source), peer_assoc(0), type(send_data), netmsg(netmsg), addr(addr) {}

  const AssocDataUDS* get_peer_assoc() const { return peer_assoc; }
  TPoverUDSMsg::msg_t get_msgtype() const { return type; }
  NetMsg* get_netmsg() const { return netmsg; }
  udsaddress* get_udsaddr() const { return addr; } 
};

} // end namespace protlib

#endif
