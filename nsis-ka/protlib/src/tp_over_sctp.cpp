/// ----------------------------------------*- mode: C++; -*--
/// @file tp_over_sctp.cpp
/// SCTP-based transport module (using SCTP socket interface)
/// ----------------------------------------------------------
/// $Id: tp_over_sctp.cpp 4210 2009-08-06 11:22:29Z bless $
/// $HeadURL: https://svn.tm.kit.edu/nsis/protlib/trunk/src/tp_over_sctp.cpp $
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
 
extern "C"
{
#include <sys/types.h> 
#include <sys/poll.h>
#include <sys/socket.h>

#include <netinet/in.h>
#include <netinet/sctp.h>

#include <arpa/inet.h>

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
}

#include <iostream>
#include <set>
#include <sstream>
#include <string>

#include "tp_over_sctp.h"
#include "threadsafe_db.h"
#include "cleanuphandler.h"
#include "setuid.h"
#include "queuemanager.h"
#include "logfile.h"

// XXX: Parameter?
const unsigned int max_listen_queue_size = 10;
static char in6_strbuf[INET6_ADDRSTRLEN];

namespace protlib {

using namespace log;

/** @defgroup tpsctp TP over SCTP
 * @ingroup network
 * @{
 */

// inherited from TP
void
TPoverSCTP::send(NetMsg *msg, const address &in_addr, bool use_existing_connection, const address *local_addr)
{
	if (msg == NULL) {
	  ERRCLog(tpparam.name,"send() - called without valid NetMsg buffer (NULL)");
		return;
	}

	appladdress *addr = NULL;
	addr = dynamic_cast<appladdress *>(in_addr.copy());

	if (addr == NULL)
	{
	  ERRCLog(tpparam.name,"send() - given destination address is not of expected type (appladdress): type=" << (int) in_addr.get_type());
	  return;
	}


	lock();
	sender_thread_queuemap_t::const_iterator it = senderthread_queuemap.find(*addr);
	FastQueue *destqueue = NULL;

	if (it == senderthread_queuemap.end()) { // no sender thread so far
	  // if we already have an existing connection it is save to create a sender thread
	  // since get_connection_to() will not be invoked, so an existing connection will
	  // be used
	  const AssocData* assoc = connmap.lookup(*addr);
	  
	  //DLog(tpparam.name,"send() - use_existing_connection:" << (use_existing_connection ? "true" : "false") << " assoc:" << assoc);

	  if (use_existing_connection==false || (assoc && assoc->shutdown==false && assoc->socketfd>0))
	      { // it is allowed to create a new connection for this thread

		// create a new queue for sender thread
		FastQueue *sender_thread_queue= new FastQueue;

		create_new_sender_thread(sender_thread_queue);
		// XXX: Handle failure?
		senderthread_queuemap.insert(pair<appladdress, FastQueue *>
		    (*addr, sender_thread_queue));
		destqueue = sender_thread_queue;
	      }
	} else { // we have a sender thread, use stored queue from map
		destqueue = it->second; 
	}
	unlock();

	// send a send_data message to it (if we have a destination queue)
	if (destqueue != NULL) {
		// both parameters will be freed after message was sent!
		TPoverSCTPMsg *internalmsg = new TPoverSCTPMsg(msg,
		    new appladdress(*addr));
		if (internalmsg != NULL) {
			// send the internal message to the sender thread queue
			internalmsg->send(tpparam.source, destqueue);
		}
	} else {
	  
	  if (!use_existing_connection)
	    WLog(tpparam.name,"send() - found entry for address, but no active sender thread available for peer addr:" << *addr << " - dropping data");
	  else
	    DLog(tpparam.name,"no active sender thread found for peer " << *addr << " - but policy forbids to set up a new connection, will drop data");
	  
	  // cannot send data, so we must drop it
	  delete msg;
	}

	if (addr)
		delete addr;
}

void
TPoverSCTP::terminate(const address &in_addr)
{
#ifndef _NO_LOGGING
	const char *const thisproc="terminate()   - ";
#endif

	appladdress *addr = NULL;
	addr = dynamic_cast<appladdress*>(in_addr.copy());

	if (!addr)
		return;

	AssocData *assoc = NULL;

	lock();
	assoc = connmap.lookup(*addr);
	if (assoc != NULL) {
		EVLog(tpparam.name, thisproc <<
		    "got request to shutdown connection for peer " << addr);

		if (!assoc->shutdown) {
			if (assoc->socketfd > 0) {
				// disallow sending
				if (shutdown(assoc->socketfd,SHUT_WR) == -1) {
					ERRLog(tpparam.name, thisproc <<
					    "shutdown (write) on socket for peer " << addr <<
					    " returned error:" << strerror(errno));
				} else {
					EVLog(tpparam.name, thisproc <<
					    "initiated closing of connection for peer " << addr <<
					    ". Shutdown (write) on socket "<< assoc->socketfd );	  
				}
			}
			assoc->shutdown = true;
		} else
			EVLog(tpparam.name, thisproc << "connection for peer " << addr <<
			    " is already in mode shutdown");
	} else
		WLog(tpparam.name, thisproc << "could not find a connection for peer " << addr);

	stop_receiver_thread(assoc);

	unlock();

	if (addr)
		delete addr;
}

// inherited from Thread
void
TPoverSCTP::main_loop(uint32 nr)
{
	// get internal queue for messages from receiver_thread
	FastQueue *fq = get_fqueue();

	if (!fq) {
		Log(ERROR_LOG,LOG_CRIT, tpparam.name, "Cannot find message queue");
		return;
	}

	// register queue for receiving internal messages from other modules
	QueueManager::instance()->register_queue(fq, tpparam.source);

	// start accept thread
	int pthread_status = pthread_create(&accept_thread_ID, 
	    NULL,	// NULL: default attributes: thread is joinable and has a 
	    		//       default, non-realtime scheduling policy
	    accept_thread_starter, this);
	if (pthread_status) {
		Log(ERROR_LOG, LOG_CRIT, tpparam.name,
		    "New master listener thread could not be created: " <<
		    strerror(pthread_status));
		// XXX: Do something about it?
	} else
		Log(DEBUG_LOG, LOG_NORMAL, tpparam.name,
		    "Master listener thread started");

	// define max latency for thread reaction on termination/stop signal
	timespec wait_interval = { 0, 250000000L }; // 250ms
	message *msg;
	TPoverSCTPMsg *local_msg;
	state_t currstate;

	// Main work loop
	while (((currstate = get_state()) != STATE_ABORT) &&
	    (currstate != STATE_STOP)) {
		// poll internal message queue (blocking)
		if ((msg = fq->dequeue_timedwait(wait_interval)) == NULL)
			continue;

		if ((local_msg = dynamic_cast<TPoverSCTPMsg *>(msg)) == NULL) {
			Log(ERROR_LOG, LOG_CRIT, tpparam.name, "Dynamic_cast failed - received unexpected and unknown internal message source "
			    << msg->get_source());
		}

		// Handle the message
		if (local_msg->get_msgtype() == TPoverSCTPMsg::stop) {
			// a receiver thread terminated and signaled for cleanup by master thread
			AssocData *assoc = const_cast<AssocData*>(local_msg->get_peer_assoc());
			Log(DEBUG_LOG, LOG_NORMAL, tpparam.name,
			    "Got cleanup request for thread <" << assoc->thread_ID <<'>');

			lock();
			cleanup_receiver_thread(assoc);
			unlock();
		} else if (local_msg->get_msgtype() == TPoverSCTPMsg::start) {
			// start a new receiver thread
			create_new_receiver_thread(const_cast<AssocData*>(local_msg->get_peer_assoc()));
		} else
			Log(ERROR_LOG,LOG_CRIT, tpparam.name, "unexpected internal message:" << local_msg->get_msgtype());

	}

	if (currstate == STATE_STOP)
		Log(INFO_LOG,LOG_NORMAL, tpparam.name,
		    "Asked to abort, stopping all receiver threads");

	// do not accept any more messages
	fq->shutdown();
	// terminate all receiver and sender threads that are still active 
	terminate_all_threads();
}

TPoverSCTP::~TPoverSCTP()
{
  init= false;

  Log(DEBUG_LOG,LOG_NORMAL, tpparam.name,  "Destructor called");

  QueueManager::instance()->unregister_queue(tpparam.source);
}

void
TPoverSCTP::accept_thread()
{
	int status;

	if (accept_thread_running) {
			Log(ERROR_LOG, LOG_CRIT, tpparam.name,
			    "Accept thread already running at TID " << accept_thread_ID);
			return;
	}

	// create a listening socket
	int accept_socket = socket(AF_INET6, SOCK_STREAM, IPPROTO_SCTP);
	if (accept_socket == -1) {
		Log(ERROR_LOG, LOG_CRIT, tpparam.name, "Could not create a new socket, error: " << strerror(errno));
		return;
	}

	// Disable Nagle Algorithm, set (SCTP_NODELAY)
	int nodelayflag = 1;
	status = setsockopt(accept_socket, SOL_SCTP, SCTP_NODELAY,
	    &nodelayflag, sizeof(nodelayflag));
	if (status) {
		Log(ERROR_LOG, LOG_NORMAL, tpparam.name, "Could not set socket option SCTP_NODELAY:" << strerror(errno));
	}

	// Reuse ports, even if they are busy
	int socketreuseflag = 1;
	status = setsockopt(accept_socket, SOL_SOCKET, SO_REUSEADDR,
	    (const char *) &socketreuseflag, sizeof(socketreuseflag));
	if (status) {
		Log(ERROR_LOG, LOG_NORMAL, tpparam.name, "Could not set socket option SO_REUSEADDR:" << strerror(errno));
	}

	// TODO: Do we need more socket options?  RTO, v4-mapped, ...

	struct sockaddr_in6 own_address = { 0 };
	own_address.sin6_family = AF_INET6;
	// accept incoming connections on all interfaces 
	own_address.sin6_addr = in6addr_any;
	// use port number in param structure
	own_address.sin6_port = htons(tpparam.port);

	// bind the newly created socket
	status = bind(accept_socket,
	    reinterpret_cast<struct sockaddr *>(&own_address), sizeof(own_address));
	if (status) {
		Log(ERROR_LOG, LOG_CRIT, tpparam.name, "Binding to " << 
		    inet_ntop(AF_INET6, &own_address.sin6_addr, in6_strbuf, INET6_ADDRSTRLEN) <<
		    " port " << tpparam.port << " failed, error: " << strerror(errno));
		return;
	}

	status = listen(accept_socket, max_listen_queue_size);
	if (status) {
		Log(ERROR_LOG, LOG_CRIT, tpparam.name, "Listen at socket " <<
		    accept_socket << " failed, error: " << strerror(errno));
		return;
	} else {
		Log(INFO_LOG, LOG_NORMAL, tpparam.name, color[green] <<
		    "Listening at port #" << tpparam.port << color[off]);
	}

	// activate O_NON_BLOCK for accept (accept does not block)
	fcntl(accept_socket,F_SETFL, O_NONBLOCK);

	// prepare poll parameter
	int poll_errno;
	const unsigned int number_poll_sockets = 1; 
	struct pollfd poll_fd = { 0 };
	poll_fd.fd = accept_socket;
	poll_fd.events = POLLIN | POLLPRI; 
	poll_fd.revents = 0;

	// prepare accept parameter
	struct sockaddr_in6 peer_address;
	socklen_t peer_address_len;
	int conn_socket;

	// check for thread terminate condition using get_state()
	state_t currstate;
	status = 0;

	// Register status in object
	// Do not to return from while loop, just break for cleanup
	accept_thread_running = true;
	// main accept loop, update currstate and check if we should go on
	while (((currstate = get_state()) != STATE_ABORT) &&
	    (currstate != STATE_STOP)) {
		// wait on number_poll_sockets (main drm socket) 
		// for the events specified above for sleep_time (in ms)
		status = poll(&poll_fd, number_poll_sockets, tpparam.sleep_time);
		poll_errno = errno;		// save errno or Log etc. might mess w/ it

		if (status == 0) {	// poll timed out
#ifdef DEBUG_HARD
			Log(DEBUG_LOG,LOG_UNIMP, tpparam.name,
			    "Listen Thread - Poll timed out after " <<
			    tpparam.sleep_time << " ms.");
#endif
			continue;
		}
		
		if (status == -1) {
			if (poll_errno != EINTR) {
				Log(ERROR_LOG, LOG_CRIT, tpparam.name,
				    "Poll status indicates error: " << strerror(poll_errno));
			} else {
				Log(EVENT_LOG, LOG_NORMAL, tpparam.name, "Poll status: " <<
				    strerror(poll_errno));
			}
			// XXX: continue; ?
		}

		// At this point we are sure that something interesting happend
		if (poll_fd.revents & POLLERR) {
			if (poll_errno != EINTR) {
				Log(ERROR_LOG,LOG_CRIT, tpparam.name, "Poll caused error " <<
				    strerror(poll_errno) << " - indicated by revents");
			} else {
				Log(EVENT_LOG,LOG_NORMAL, tpparam.name, "poll(): " <<
				    strerror(poll_errno));
			}
		}
		if (poll_fd.revents & POLLHUP) {
			Log(ERROR_LOG,LOG_CRIT, tpparam.name, "Poll hung up");
			break;
		}
		if (poll_fd.revents & POLLNVAL) {
			Log(ERROR_LOG,LOG_CRIT, tpparam.name,
			    "Poll Invalid request: fd not open");
			break;
		}

		peer_address_len = sizeof(peer_address);
		conn_socket = accept(accept_socket,
		    reinterpret_cast<struct sockaddr *>(&peer_address),
		    &peer_address_len);
		if (conn_socket == -1) {
			if (errno != EWOULDBLOCK && errno != EAGAIN) {
				Log(ERROR_LOG, LOG_EMERG, tpparam.name, "Accept at socket " <<
				    accept_socket << " failed, error: " << strerror(errno));
				break;
			} else 
				continue;
		} else {
		  // we could use SCTP_STATUS to find out the primary peer address
		  // struct sctp_paddrinfo sstat_primary
		  sctp_status assoc_status;
		  socklen_t optlen= sizeof(assoc_status);

		  if (getsockopt(conn_socket, SOL_SCTP, SCTP_STATUS, &assoc_status, &optlen))
		  {
		    ERRLog(tpparam.name,"getsocktopt SCTP_STATUS failure:" << strerror(errno));
		  }
		  const sockaddr_in6* sctp_primary_peer_addr=reinterpret_cast<const sockaddr_in6*>(&assoc_status.sstat_primary.spinfo_address);
		  peer_address= *sctp_primary_peer_addr;
	          
			// Create appladdress objects from sockaddr
			appladdress peer_addr(peer_address, IPPROTO_SCTP);
			Log(DEBUG_LOG, LOG_NORMAL, tpparam.name,
			    "<<--Received connect--<< request from " <<
			    peer_addr.get_ip_str() << " port #" << peer_addr.get_port());

			struct sockaddr_in6 local_address;
			socklen_t local_address_len = sizeof(local_address);
			getsockname(conn_socket,
			    reinterpret_cast<struct sockaddr*>(&local_address),
			    &local_address_len);
			appladdress local_addr(local_address, IPPROTO_SCTP);
			Log(DEBUG_LOG, LOG_NORMAL, tpparam.name,
			    "<<--Received connect--<< request at " <<
			    local_addr.get_ip_str() << " port #" << local_addr.get_port());

			AssocData *peer_assoc = new(nothrow) AssocData(conn_socket,
			    peer_addr, local_addr);

			if (peer_assoc == NULL) {
				Log(ERROR_LOG, LOG_CRIT, tpparam.name, "Out of memory!");
				close(conn_socket);
				break;  // Graceful?
			}

			lock();
			bool insert_success = connmap.insert(peer_assoc);
			unlock();

			if (insert_success == false) {
				Log(ERROR_LOG, LOG_CRIT, tpparam.name,
				    "Cannot insert AssocData for socket " << conn_socket  <<
				    ", " << peer_addr.get_ip_str() << ", port #" <<
				    peer_addr.get_port() <<
				    " into connection map, aborting connection...");

				// abort connection, delete its AssocData
				close (conn_socket);
				delete peer_assoc;

				break;  // Graceful?
			}

			// create a new thread for each new connection
			create_new_receiver_thread(peer_assoc);
		}
	}

	// Try to clean up as much as possible
	close(accept_socket);
	accept_thread_running = false;
}

void *
TPoverSCTP::accept_thread_starter(void *argp)
{
	assert(argp != NULL);

	(static_cast<TPoverSCTP*>(argp))->accept_thread();

	return NULL;
}

void
TPoverSCTP::receiver_thread(void *argp)
{
	static const char *const methodname="receiver - ";
	receiver_thread_arg_t *receiver_thread_argp =
	    static_cast<receiver_thread_arg_t *>(argp);

	int conn_socket = 0;
	const appladdress *peer_addr = NULL;
	const appladdress *own_addr = NULL;

	// argument parsing - start
	if (receiver_thread_argp == NULL) {
		ERRCLog(tpparam.name, methodname <<
		    "No arguments given at start of receiver thread <" <<
		    pthread_self() << ">, exiting.");
		return;
	} else {
		// change status to running, i.e., not terminated
		receiver_thread_argp->terminated = false;
		DLog(tpparam.name, methodname << "New receiver thread <" <<
		    pthread_self() << "> started. ");
	}

	if (receiver_thread_argp->peer_assoc) {
		// get socket descriptor from arg
		conn_socket = receiver_thread_argp->peer_assoc->socketfd;
		// get pointer to peer address of socket from arg
		peer_addr = &receiver_thread_argp->peer_assoc->peer;
		own_addr = &receiver_thread_argp->peer_assoc->ownaddr;
	} else {
		ERRCLog(tpparam.name, methodname <<
		    "No peer assoc available - pointer is NULL");
		// XXX: terminated?
		return;
	}

	if (peer_addr == NULL || own_addr == NULL || conn_socket <= 0) {
		ERRCLog(tpparam.name, methodname << "Parameter problem for socket " <<
		    conn_socket << ", exiting.");
		// XXX: terminated?
		return;
	}
	// argument parsing - end
	Log(DEBUG_LOG, LOG_UNIMP, tpparam.name, methodname <<
	    "Preparing to wait for data at socket " << conn_socket <<
	    " from " << peer_addr << " at " << own_addr);

	// set options for polling
	const unsigned int number_poll_sockets = 1; 
	struct pollfd poll_fd = { 0 };
	poll_fd.fd = conn_socket;
	poll_fd.events = POLLIN | POLLPRI; 
	poll_fd.revents = 0;
	int poll_status, poll_errno, msg_flags, recv_bytes;

	NetMsg *msg = new NetMsg(NetMsg::max_size);
	if (msg == NULL) {
		// XXX: terminate?
		return;
	}
	TPMsg *tpmsg = NULL;
	uint32 msg_clen;

	while (receiver_thread_argp->sig_terminate == false) {
		poll_status = poll(&poll_fd, number_poll_sockets,
		    ThreadParam::default_sleep_time);
		poll_errno = errno;		// save errno

		if (poll_status == 0)	// poll timed out
			continue;

		if (poll_status == -1) {
			if (poll_errno == 0 || poll_errno == EINTR) {
				// Handle gracefully
				continue;
			} else {
				// Die
				break;
			}
		}

		if (poll_fd.revents & POLLERR) {
			if (errno == 0 || errno == EINTR) {
				EVLog(tpparam.name, methodname << "poll(): " << strerror(errno));
				// continue;
			} else {
				ERRCLog(tpparam.name, methodname << "Poll indicates error: " <<
				    strerror(errno));
				break;
			}
		}

		if (poll_fd.revents & POLLHUP) { 
			Log(EVENT_LOG, LOG_CRIT, tpparam.name, methodname << "Poll hung up");
			break;
		}
		if (poll_fd.revents & POLLNVAL) {
			EVLog(tpparam.name, methodname <<
			    "Poll Invalid request: fd not open");
			break;
		}

		msg_flags = MSG_DONTWAIT;
		recv_bytes = sctp_recvmsg(conn_socket, msg->get_buffer(),
		    msg->get_size(), NULL, NULL, NULL, &msg_flags);
		if (recv_bytes == -1) {
			if (errno == EAGAIN || errno == EWOULDBLOCK) {
				continue;
			}
			break;
		}
		if (recv_bytes == 0) {
			// Connection reset by peer
			break;
		} 

		if (recv_bytes < common_header_length) {
			// Incomplete message from recvmsg
			continue;
		}

		if (!getmsglength(*msg, msg_clen)) {
			ERRCLog(tpparam.name, methodname <<
			    "Not a valid protocol header - discarding received packet. received size " <<
			    msg_clen);

			ostringstream hexdumpstr;
			msg->hexdump(hexdumpstr, msg->get_buffer(), recv_bytes);
			DLog(tpparam.name, methodname << "dumping received bytes:" <<
			    hexdumpstr.str());

			continue;
		}

		// recv_bytes is >0 at this point, cast to uint to avoid sign/usign
		// mismatch
		if ((uint)recv_bytes - common_header_length < msg_clen) {
			// Incomplete message from recvmsg
			continue;
		}
		if ((uint)recv_bytes - common_header_length >= msg_clen) {
			// Trailing ?garbage? from recvmsg
			msg->truncate(common_header_length + msg_clen);
			// proceed with truncated message, bin the rest
		}

		// Ready to deliver message
		// Create TPMsg from NetMsg
		tpmsg = new(nothrow) TPMsg(msg, peer_addr->copy(), own_addr->copy());

		// TODO: Debug log
		EVLog(tpparam.name, methodname << "<<--Received--<< packet (" << recv_bytes << " bytes) at socket " << conn_socket << " from " << *peer_addr);


		if (tpmsg == NULL || tpmsg->get_peeraddress() == NULL ||
		    !tpmsg->send(message::qaddr_tp_over_sctp,tpparam.dest)) {
			// Couldn't create/send TPMsg
			if (tpmsg != NULL)
				delete tpmsg;
		} else {
			// We're done and need a new buffer for next recv
			// TPMsg consumes the input buffer
			msg = new(nothrow) NetMsg(NetMsg::max_size);
			if (msg == NULL) {
				// No memory
				break;
			}
		}
	}

	Log(DEBUG_LOG, LOG_NORMAL, tpparam.name, methodname << "Thread <" <<
	    pthread_self() << "> shutting down and closing socket " <<
	    receiver_thread_argp->peer_assoc->peer);

	// shutdown socket
	if (shutdown(conn_socket, SHUT_RD)) {
		if (errno != ENOTCONN)
			Log(ERROR_LOG, LOG_NORMAL, tpparam.name, methodname << "Thread <" <<
			    pthread_self() << "> shutdown (read) on socket failed, reason: "
			    << strerror(errno));
	}

	// close socket
	close(conn_socket);

	receiver_thread_argp->terminated = true;

	Log(DEBUG_LOG, LOG_NORMAL, tpparam.name, methodname << "Thread <" <<
	    pthread_self() << "> terminated");
	Log(DEBUG_LOG, LOG_NORMAL, tpparam.name, methodname <<
	    "Signaling main loop for cleanup");

	// notify master thread for invocation of cleanup procedure
	TPoverSCTPMsg *newmsg = new(nothrow)
	    TPoverSCTPMsg(receiver_thread_argp->peer_assoc);

	// send message to main loop thread
	if (newmsg != NULL)
		newmsg->send_to(tpparam.source);	
}

void *
TPoverSCTP::receiver_thread_starter(void *argp)
{
	receiver_thread_start_arg_t *rargp =
	    static_cast<receiver_thread_start_arg_t *>(argp);

	if (rargp == NULL || rargp->instance == NULL) {
		Log(ERROR_LOG, LOG_CRIT, "receiver_thread_starter",
		    "while starting receiver_thread: NULL pointer argument");
		return NULL;
	}

	rargp->instance->receiver_thread(rargp->rtargp);
	delete rargp;
	
	return NULL;
}

void
TPoverSCTP::create_new_receiver_thread(AssocData *peer_assoc)
{
	receiver_thread_arg_t *argp = new(nothrow) receiver_thread_arg(peer_assoc);
	Log(EVENT_LOG,LOG_NORMAL, tpparam.name, "Starting new receiver thread...");

	// create new thread; (arg == 0) is handled by thread, too
	int pthread_status= pthread_create(&peer_assoc->thread_ID, 
	    NULL,	// NULL: default attributes: thread is joinable and has a 
	    		//       default, non-realtime scheduling policy
	    receiver_thread_starter,
	    new(nothrow) receiver_thread_start_arg_t(this,argp));

	if (pthread_status) {
		Log(ERROR_LOG,LOG_CRIT, tpparam.name,
		    "A new thread could not be created: " <<  strerror(pthread_status));
		delete argp;
	} else {
		lock();
		// remember pointer to thread arg structure
		// thread arg structure should be destroyed after thread
		// termination only
		pair<recv_thread_argmap_t::iterator, bool> tmpinsiterator =
		    recv_thread_argmap.insert(
		    pair<pthread_t,receiver_thread_arg_t*> (peer_assoc->thread_ID,argp));
		if (tmpinsiterator.second == false) {
			Log(ERROR_LOG, LOG_CRIT, tpparam.name,
			    "Thread argument could not be inserted into hashmap");
			// XXX: Do something about it?
		}
		unlock();
	}
}

void
TPoverSCTP::stop_receiver_thread(AssocData *peer_assoc)
{
	if (peer_assoc == NULL)
		return;

	pthread_t thread_id =  peer_assoc->thread_ID;

	// try to clean up receiver_thread_arg
	receiver_thread_arg_t *recv_thread_argp = NULL;
	recv_thread_argmap_t::iterator recv_thread_arg_iter = recv_thread_argmap.find(thread_id);

	if (recv_thread_arg_iter != recv_thread_argmap.end())
	    recv_thread_argp = recv_thread_arg_iter->second;

	if (recv_thread_argp) {
		if (!recv_thread_argp->terminated) {
			// thread signaled termination, but is not?
			Log(EVENT_LOG, LOG_NORMAL, tpparam.name,
			    "stop_receiver_thread() - Receiver thread <" << thread_id <<
			    "> signaled for termination");
			// signal thread for its termination
			recv_thread_argp->sig_terminate = true;
			// wait for thread to join after termination
			pthread_join(thread_id, 0);
			// the dying thread will signal main loop to call this method, but this time we should enter the else branch
			return;
		}
	} else
		Log(ERROR_LOG,LOG_NORMAL, tpparam.name,"stop_receiver_thread() - Receiver thread <" << thread_id << "> not found");
}

void
TPoverSCTP::cleanup_receiver_thread(AssocData *peer_assoc)
{
	// All operations on  recv_thread_argmap and connmap require an already acquired lock
	// after this procedure peer_assoc may be invalid because it was erased

	if (peer_assoc == NULL)
		return;

	// start critical section
	pthread_t thread_id =  peer_assoc->thread_ID;

	// try to clean up receiver_thread_arg
	receiver_thread_arg_t *recv_thread_argp = NULL;  
	recv_thread_argmap_t::iterator recv_thread_arg_iter = recv_thread_argmap.find(thread_id);
	if (recv_thread_arg_iter != recv_thread_argmap.end())
		recv_thread_argp = recv_thread_arg_iter->second;

	if (recv_thread_argp) {
		if (!recv_thread_argp->terminated) {
			// thread signaled termination, but is not?
			Log(ERROR_LOG, LOG_NORMAL, tpparam.name,
			    "cleanup_receiver_thread() - Receiver thread <" <<
			    thread_id << "> not terminated yet?!");
			return;
		} else { // thread is already terminated
			Log(EVENT_LOG, LOG_NORMAL, tpparam.name,
			    "cleanup_receiver_thread() - Receiver thread <" << thread_id <<
			    "> is terminated");
			// delete it from receiver map
			recv_thread_argmap.erase(recv_thread_arg_iter);
			// then delete receiver arg structure
			delete recv_thread_argp;
		}
	}

	// delete entry from connection map

	// cleanup sender thread
	// no need to lock explicitly, because caller of cleanup_receiver_thread() must already locked
	terminate_sender_thread(peer_assoc);

	// delete the AssocData structure from the connection map
	// also frees allocated AssocData structure
	connmap.erase(peer_assoc);

	// end critical section
	Log(DEBUG_LOG, LOG_NORMAL, tpparam.name,
	    "cleanup_receiver_thread() - Cleanup receiver thread <" << thread_id <<
	    ">. Done.");
}

void
TPoverSCTP::sender_thread(void *argp)
{
#ifndef _NO_LOGGING
	const char *const methodname="senderthread - ";
#endif

	FastQueue *fq =  reinterpret_cast<FastQueue*>(argp);

	if (fq == NULL) {
		ERRLog(tpparam.name, methodname << "thread <" << pthread_self() << "> no valid pointer to msg queue. Stop.");
		return;
	}

	message *msg;
	TPoverSCTPMsg *local_msg;
	uint32_t sctp_context = 0;
	int ret, retry_count = 3;
	appladdress *addr;
	NetMsg *net_msg;
	AssocData *assoc;


	while ((msg = fq->dequeue()) != NULL) {
		local_msg = dynamic_cast<TPoverSCTPMsg *>(msg);
		if (local_msg == NULL) {
			ERRLog(tpparam.name, methodname <<
			    "received not a TPoverSCTPMsg but a" << msg->get_type_name());
			continue;      
		}

		if (local_msg->get_msgtype() == TPoverSCTPMsg::send_data) {
			if ((net_msg = local_msg->get_netmsg()) == NULL ||
			    (addr = local_msg->get_appladdr()) == NULL) {
				ERRLog(tpparam.name, methodname << "problem with passed arguments references, they point to 0");
				continue;
			}

			addr->convert_to_ipv6();
			try {
				check_send_args(*net_msg, *addr);
			} catch (...) {
				ERRLog(tpparam.name, methodname << "problem with passed arguments");
				delete net_msg;
				delete addr;
				continue;
			}
			if ((assoc = get_connection_to(*addr)) == NULL) {
				ERRLog(tpparam.name, methodname << "Can't get connection to " <<
				    *addr);
				delete net_msg;
				delete addr;
				continue;
			}
			if (assoc->shutdown || assoc->socketfd <= 0) {
				ERRLog(tpparam.name, methodname << "Association found, but unusable for sending");
				delete net_msg;
				delete addr;
				continue;				
			}

			uint32 msgsize= net_msg->get_size();

			// RETRY LOOP - BEGIN
			retry_count = 3;
			do {	// try sending up to retry_count times for retryable errors

			  ret = sctp_sendmsg(assoc->socketfd,
					     net_msg->get_buffer(), msgsize,		// message
					     NULL, 0,			// use primary address
					     sctp_context,		// supply dummy context
					     0,					// no flags
					     0,					// stream no. might belong in parameters?
					     0,					// no partitial reliability tmo --"--
					     sctp_context);
			  sctp_context++;

			  if (ret < 0) {
			    // error occured retry after 1 sec
			    if (errno == EAGAIN ||
				errno == EWOULDBLOCK ||
				errno == EINTR ||
				errno == ENOBUFS ||
				errno == ENOMEM) {
			      sleep(1);
			      continue;
			    }

			    // Non-retryable error - what now?
			    ERRLog(tpparam.name, methodname << "couldn't send message" <<
				   strerror(errno));
			    break;
			  }

			  if ((uint)ret < msgsize) {
			    // Something went wrong, not really defined behavior
			    ERRLog(tpparam.name, methodname << "could only send " <<
				   ret << " out of " << msgsize << " bytes message");
			    break;	// retry instead?
			  }
			  else
			  { // everything is fine now, don't retry sending
			    break;
			  } 
			} 
			while (--retry_count > 0);
			// RETRY LOOP - END
			if (ret>0) 
			  EVLog(tpparam.name, methodname << ">>----Sent---->> message (" << msgsize << " bytes) using socket " << assoc->socketfd  << " to " << *addr);

			delete net_msg;
			delete addr;
		} 
		else 
		  if (local_msg->get_msgtype() == TPoverSCTPMsg::stop) {
		    break;
		  }
	} // while waiting for internal messages
}

void *
TPoverSCTP::sender_thread_starter(void *argp)
{
	sender_thread_start_arg_t *sargp = static_cast<sender_thread_start_arg_t *>(argp);

	// invoke sender thread method
	if (sargp != NULL && sargp->instance != NULL) {
		// call receiver_thread member function on object instance
		sargp->instance->sender_thread(sargp->sender_thread_queue);
		// no longer needed
		delete sargp;
	} else {
		Log(ERROR_LOG, LOG_CRIT, "sender_thread_starter","while starting sender_thread: 0 pointer to arg or object");
	}

	return NULL;
}

void
TPoverSCTP::create_new_sender_thread(FastQueue *senderqueue)
{
	if (senderqueue == NULL)
		return;

	pthread_t senderthreadid;
	int pthread_status = pthread_create(&senderthreadid, 
	    NULL,	// NULL: default attributes: thread is joinable and has a 
	    		//       default, non-realtime scheduling policy
	    TPoverSCTP::sender_thread_starter,
		new sender_thread_start_arg_t(this, senderqueue));
	if (pthread_status) {
		Log(ERROR_LOG,LOG_CRIT, tpparam.name,
		    "A new thread could not be created: " <<  strerror(pthread_status));
		delete senderqueue;
	}
}

void
TPoverSCTP::terminate_sender_thread(const AssocData *assoc)
{
	if (assoc == NULL) {
		Log(ERROR_LOG, LOG_NORMAL, tpparam.name,
		    "terminate_sender_thread() - assoc data == NULL");
		return;
	}

	sender_thread_queuemap_t::iterator it = senderthread_queuemap.find(assoc->peer);

	if (it != senderthread_queuemap.end()) {
		 // we have a sender thread: send a stop message to it
		FastQueue *destqueue = it->second; 
		if (destqueue) {
			TPoverSCTPMsg *internalmsg= new
			    TPoverSCTPMsg(assoc, tpparam.source, TPoverSCTPMsg::stop);
			if (internalmsg) {
				// send the internal message to the sender thread queue
				internalmsg->send(tpparam.source, destqueue);
			}
		} else {
			Log(WARNING_LOG, LOG_NORMAL, tpparam.name, "terminate_sender_thread() - found entry for address, but no sender thread. addr:" << assoc->peer);
		}
		// erase entry from map
		senderthread_queuemap.erase(it);
	}
}

void
TPoverSCTP::terminate_all_threads()
{
	// Frist, wait for the accept_thread to stop so we don't race with it
	if (accept_thread_running) {
		Log(DEBUG_LOG, LOG_NORMAL, tpparam.name,
		    "Waiting for accept thread to terminate");
		pthread_join(accept_thread_ID, 0);
		Log(DEBUG_LOG, LOG_NORMAL, tpparam.name,
		    "accept thread terminated");
	}

	AssocData *assoc = NULL;
	receiver_thread_arg_t *terminate_argp;
	recv_thread_argmap_t::iterator terminate_iterator;

	for (terminate_iterator = recv_thread_argmap.begin();
	    terminate_iterator !=  recv_thread_argmap.end(); terminate_iterator++) {
		if ((terminate_argp = terminate_iterator->second) != 0) {
			// we need a non const pointer to erase it later on
			assoc = const_cast<AssocData *>(terminate_argp->peer_assoc);

			// check whether thread is still alive
			if (terminate_argp->terminated == false) {
				terminate_argp->sig_terminate = true;
				// then wait for its termination
				Log(DEBUG_LOG, LOG_NORMAL, tpparam.name,
				    "Signaled receiver thread <" << terminate_iterator->first <<
				    "> for termination");
				pthread_join(terminate_iterator->first, 0);
				Log(DEBUG_LOG, LOG_NORMAL, tpparam.name, "Thread <" <<
				    terminate_iterator->first  << "> is terminated");
			} else
				Log(DEBUG_LOG, LOG_NORMAL, tpparam.name,  "Receiver thread <" <<
				    terminate_iterator->first << "> already terminated");

			// cleanup all remaining argument structures of terminated threads
			delete terminate_argp;

			// terminate any related sender thread that is still running
			terminate_sender_thread(assoc);

			// delete assoc is not necessary, because connmap.erase() will do the job
			connmap.erase(assoc);
		}
	}
}

AssocData *
TPoverSCTP::get_connection_to(const appladdress& addr)
{
	AssocData *assoc = NULL;
	int status;

	lock();
	assoc = connmap.lookup(addr);
	// XXX: We race here, might be better to return locked?
	unlock();

	if (assoc != NULL) {
		// We already have an association with addr
		if (assoc->shutdown) {
			// ... but it's not in a useable state
			return NULL;
		} else {
			return assoc;
		}
	}

	int new_socket = socket(AF_INET6, SOCK_STREAM, IPPROTO_SCTP);
	if (new_socket == -1) {
		Log(ERROR_LOG, LOG_CRIT, tpparam.name, "Could not create a new socket, error: " << strerror(errno));
		return NULL;
	}

	// Disable Nagle Algorithm, set (SCTP_NODELAY)
	int nodelayflag = 1;
	status = setsockopt(new_socket, SOL_SCTP, SCTP_NODELAY, &nodelayflag,
	    sizeof(nodelayflag));
	if (status) {
		Log(ERROR_LOG, LOG_NORMAL, tpparam.name, "Could not set socket option SCTP_NODELAY:" << strerror(errno));
	}

	// Reuse ports, even if they are busy
	// XXX: Good idea?
	int socketreuseflag = 1;
	status = setsockopt(new_socket, SOL_SOCKET, SO_REUSEADDR,
	    (const char *)&socketreuseflag, sizeof(socketreuseflag));
	if (status) {
		Log(ERROR_LOG, LOG_NORMAL, tpparam.name, "Could not set socket option SO_REUSEADDR:" << strerror(errno));
	}

	// TODO: Do we need more socket options?  RTO, v4-mapped, ...

    struct sockaddr_in6 dest_address = { 0 };
    addr.get_sockaddr(dest_address);
      
    // connect the socket to the destination address
    status = connect(new_socket,
	    reinterpret_cast<const struct sockaddr*>(&dest_address),
	    sizeof(dest_address));
	if (status == -1) {
		Log(ERROR_LOG, LOG_NORMAL, tpparam.name, "Could not connect socket to " <<
		    addr << " :" << strerror(errno));
		return NULL;
	}

    struct sockaddr_in6 own_address;
    socklen_t own_address_len = sizeof(own_address);
    getsockname(new_socket, reinterpret_cast<struct sockaddr*>(&own_address),
        &own_address_len);

	Log(DEBUG_LOG, LOG_UNIMP, tpparam.name, ">>--Connect-->> to " <<
	    addr.get_ip_str() << " port #" << addr.get_port() << " from " <<
	    inet_ntop(AF_INET6, &own_address.sin6_addr, in6_strbuf,
	    INET6_ADDRSTRLEN) << " port #" << ntohs(own_address.sin6_port));

	struct timespec ts;
	get_time_of_day(ts);
	ts.tv_nsec += tpparam.sleep_time * 1000000L;
	if (ts.tv_nsec >= 1000000000L) {
		ts.tv_sec += ts.tv_nsec / 1000000000L;
		ts.tv_nsec= ts.tv_nsec % 1000000000L;
	}

	bool insert_success = false;
	do {
		if (assoc == NULL) {
			// create new AssocData record (will copy addr)
			assoc = new(nothrow) AssocData(new_socket, addr,
			    appladdress(own_address, IPPROTO_SCTP));

			if (assoc == NULL)
				continue;

			lock();
			insert_success = connmap.insert(assoc);
			unlock();
		}

		assert(assoc != NULL);

		if (insert_success) {
			// notify master thread to start a receiver thread
			TPoverSCTPMsg *newmsg = new(nothrow)TPoverSCTPMsg(assoc,
			    tpparam.source, TPoverSCTPMsg::start);
			if (newmsg) {
				newmsg->send_to(tpparam.source);
				return assoc;
			} else
				continue;
		} else {
			delete assoc;
			return NULL;
		}
	} while (wait_cond(ts) != ETIMEDOUT);

	return NULL;
}

}
// @}
