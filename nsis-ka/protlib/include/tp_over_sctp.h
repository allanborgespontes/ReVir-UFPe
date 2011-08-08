/// ----------------------------------------*- mode: C++; -*--
/// @file tp_over_sctp.h
/// SCTP transport protocol module
/// ----------------------------------------------------------
/// $Id: tp_over_sctp.h 6282 2011-06-17 13:31:57Z bless $
/// $HeadURL: https://svn.tm.kit.edu/nsis/protlib/trunk/include/tp_over_sctp.h $
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
// tp_over_sctp.h
// ----------------------------------------------------------
// $Id: tp_over_sctp.h 6282 2011-06-17 13:31:57Z bless $
// $HeadURL: https://svn.tm.kit.edu/nsis/protlib/trunk/include/tp_over_sctp.h $
// ==========================================================
// Implementation of a SCTP-based transport module
// Created by Max Laier (from tcp template)
// Institute of Telematics, University of Karlsruhe (TH)
// ==========================================================

/** @ingroup tpsctp
 * TP over SCTP
 */

#ifndef TP_OVER_SCTP_H_
#define TP_OVER_SCTP_H_

#include "hashmap"

#include "tp.h"
#include "threads.h"
#include "threadsafe_db.h"
#include "connectionmap.h"
#include "assocdata.h"

namespace protlib
{

struct TPoverSCTPParam:public ThreadParam 
{
	// Constructor
	TPoverSCTPParam(unsigned short common_header_length,
	    bool (*const getmsglength) (NetMsg &m, uint32 &len_bytes),
	    port_t p, const char *threadname = "TPoverSCTP",
	    uint32 sleep = ThreadParam::default_sleep_time,
	    bool debug_pdu = false,
	    message::qaddr_t source = message::qaddr_tp_over_sctp,
	    message::qaddr_t dest = message::qaddr_signaling,
	    // XXX: what should init_rto and max_rto be?
	    bool sendaborts = false, uint32 init_rto = 1, uint32 max_rto = 2):
	// DEFAULTS
	    ThreadParam(sleep, threadname, 1, 1), port(p),
	    debug_pdu(debug_pdu), source(source), dest(dest),
	    common_header_length(common_header_length),
	    getmsglength(getmsglength), terminate(false), init_rto(init_rto),
	    max_rto(max_rto) {};

	// port to bind master listener thread to
	const port_t port;
	// enable debugging
	bool debug_pdu;

	// message endpoints
	const message::qaddr_t source;
	const message::qaddr_t dest;

	const unsigned short common_header_length;

	bool (*const getmsglength) (NetMsg &m, uint32 &clen_words);

	// should master thread terminate?
	const bool terminate;
	const uint32 init_rto;
	const uint32 max_rto; 
};

// TP over SCTP
// This class implements the TP interface using SCTP.
class TPoverSCTP:public TP, public Thread
{
	// inherited from TP
        public:
	        virtual void send(NetMsg *msg, const address &addr, bool use_existing_connection, const address *local_addr);
	        virtual void terminate(const address &addr);

	// inherited from Thread
	public:
		virtual void main_loop(uint32 nr);

	// other members
	public:
		// Constructor
		TPoverSCTP(const TPoverSCTPParam &p):
		// DEFAULTS
		    TP(tsdb::get_sctp_id(), "sctp", p.name,
		        p.common_header_length, p.getmsglength),
		    Thread(p), tpparam(p), already_aborted(false),
		    debug_pdu(p.debug_pdu), accept_thread_running(false)
		{
			init = true;
		}

		// Destructor
		virtual ~TPoverSCTP();

		typedef struct receiver_thread_arg
		{
				const AssocData *peer_assoc;
				bool sig_terminate;
				bool terminated;
			public:
				receiver_thread_arg(const AssocData *peer_assoc) : 
				    peer_assoc(peer_assoc), sig_terminate(false),
				    terminated(true) {};
		} receiver_thread_arg_t;

		class receiver_thread_start_arg_t
		{
			public:
				TPoverSCTP *instance;
				receiver_thread_arg_t *rtargp;

				receiver_thread_start_arg_t(TPoverSCTP *instance,
				    receiver_thread_arg_t *rtargp) :
				    instance(instance), rtargp(rtargp) {};
		};

		class sender_thread_start_arg_t
		{
			public:
				TPoverSCTP *instance;
				FastQueue *sender_thread_queue;
				sender_thread_start_arg_t(TPoverSCTP *instance, FastQueue *sq) :
				    instance(instance), sender_thread_queue(sq) {};
  		};

	private:
		// Accept thread
		// Store accept thread ID and status
		pthread_t accept_thread_ID;
		// Accept thread loop + static wrapper
		void accept_thread();
		static void *accept_thread_starter(void *argp);

		// Receiver threads
		// Map thread <-> arguments for signalling
		typedef hashmap_t<pthread_t, receiver_thread_arg_t*> recv_thread_argmap_t;
		recv_thread_argmap_t recv_thread_argmap;

		// Low level thread loop function + static wrapper
		void receiver_thread(void *argp);
		static void *receiver_thread_starter(void *argp);

		// High level interface for receiver thread creation/destruction
		void create_new_receiver_thread(AssocData *peer_assoc);
		void stop_receiver_thread(AssocData *peer_assoc);
		void cleanup_receiver_thread(AssocData *peer_assoc);

		// Sender threads
		// Map sender thread queue <-> address
		typedef hashmap_t<appladdress, FastQueue*> sender_thread_queuemap_t;
		sender_thread_queuemap_t senderthread_queuemap;

		// Low level thread loop function + static wrapper
		void sender_thread(void *argp);
		static void *sender_thread_starter(void *argp);

		// High level interface for sender thread creation/destruction
		void create_new_sender_thread(FastQueue *senderqueue);
		void terminate_sender_thread(const AssocData *assoc);

		// Cleanup all remaining threads
		void terminate_all_threads();

		// Stores connections (address, socket, thread)
		ConnectionMap connmap;
		// Return Association from connmap or initiate new one
		AssocData *get_connection_to(const appladdress& addr);

		// Initialized from Constructor (DEFAULTS)
		// Thread parameters (from caller)
		const TPoverSCTPParam tpparam;
		// Did we already abort (false)
		bool already_aborted;
		// Print debugging (tpparam.debug_pdu, from caller) 
  		bool debug_pdu;
  		// Did we start an accept thread, yet? (false)		
		bool accept_thread_running;
};

/** A simple internal message for selfmessages
 * please note that carried items may get deleted after use of this message 
 * the message destructor does not delete any item automatically
 */
class TPoverSCTPMsg:public message 
{
	public:
		// message type start/stop thread, send data
		enum msg_t { start, stop, send_data };
	private:
	const AssocData *peer_assoc;
	const TPoverSCTPMsg::msg_t type;
	NetMsg *netmsg;
	appladdress *addr;
	public:
		TPoverSCTPMsg(const AssocData *peer_assoc,
		    message::qaddr_t source = qaddr_unknown,
		    TPoverSCTPMsg::msg_t type = stop) : 
		    message(type_transport, source), peer_assoc(peer_assoc),
		    type(type), netmsg(0), addr(0)  {}
		TPoverSCTPMsg(NetMsg *netmsg, appladdress *addr,
		    message::qaddr_t source= qaddr_unknown) : 
		    message(type_transport, source), peer_assoc(0), type(send_data),
		    netmsg(netmsg), addr(addr) {}

		const AssocData *get_peer_assoc() const { return peer_assoc; }
		TPoverSCTPMsg::msg_t get_msgtype() const { return type; }
		NetMsg *get_netmsg() const { return netmsg; }
		appladdress *get_appladdr() const { return addr; } 
};


}

#endif /*TP_OVER_SCTP_H_ */
