/// ----------------------------------------*- mode: C++; -*--
/// @file tp_queryencap.cpp
/// Transport module for GIST query encapsulation (UDP+RAO)
/// ----------------------------------------------------------
/// $Id: tp_queryencap.cpp 6148 2011-05-18 09:36:25Z bless $
/// $HeadURL: https://svn.tm.kit.edu/nsis/protlib/trunk/src/tp_queryencap.cpp $
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

  //#define _SOCKADDR_LEN    /* use BSD 4.4 sockets */
#include <unistd.h>		/* gethostname */
#include <sys/types.h>		/* network socket interface */
#include <netinet/ip.h>		/* iphdr */
#include <netinet/ip6.h>	/* ip6_hdr */
#include <netinet/in.h>		/* network socket interface */
#include <netinet/tcp.h>	/* for TCP Socket Option */
#include <netinet/udp.h>	/* for UDP header */
#include <sys/socket.h>
#include <arpa/inet.h>		/* inet_addr */

#include <fcntl.h>
#include <sys/poll.h>
#include <libipq.h>
}

#include <iostream>
#include <errno.h>
#include <string>
#include <sstream>

#include "protlibconf.h"
#include "tp_queryencap.h"
#include "threadsafe_db.h"
#include "cleanuphandler.h"
#include "setuid.h"
#include "queuemanager.h"
#include "logfile.h"
#include "cmsghdr_util.h"
#include "linux/netfilter.h"

#define BUFSIZE 2048000



namespace protlib {
  using namespace log;

/**
 * @defgroup tpqueryencap TP over Query Encapsulation
 * @ingroup network
 * @{
 */


/******* class TPqueryEncap *******/

TPqueryEncap::TPqueryEncap(const TPqueryEncapParam& p) :
    TP(prot_query_encap,"query_encap",p.name,p.common_header_length,p.getmsglength),
    Thread(p), tpparam(p), already_aborted(false), msgqueue(NULL), debug_pdu(p.debug_pdu)
{
  // perform some initializing actions
  // currently not required (SCTP had to init its library)
  init= true; ///< init done;
}


void
TPqueryEncap::send (NetMsg * netmsg, const address & addr_in, bool use_existing_connection, const address *local_addr)
{
  appladdress* addr = NULL;
  addr = dynamic_cast<appladdress*>(addr_in.copy());
  const hostaddress *own_addr = NULL;
  if (local_addr)
    own_addr = dynamic_cast<const hostaddress *>(local_addr);
  // we can safely ignore the use_existing_connection attribute since q-mode is connectionless
  udpsend(netmsg, addr, own_addr);
}


/**
 * Set socket options for IPv4.
 *
 * @param sockfd a socket descriptor, returned by a successful call to socket()
 * @param hop_limit a value between 0 and 255, or -1 to use the route's default
 * @param rao the RAO in host byte order (a 16 bit value, or -1)
 */
void TPqueryEncap::setup_socket_ipv4(int sockfd, int hop_limit, int rao) {

	/*
	 * Enable MTU path discovery and also set the Don't Fragment flag.
	 *
	 * NOTE: This has the side effect of setting the TTL to 1, so we only
	 *       allow this if the TTL (hop_limit) parameter is set.
	 *
	 */
	if ( hop_limit != -1 ) {
		int dont_fragment = 1;
		int ret = setsockopt(sockfd, SOL_IP, IP_PMTUDISC_DO,
					&dont_fragment, sizeof dont_fragment);
		if ( ret != 0 )
			ERRLog(tpparam.name,
				"enabling IPv4 MTU path discovery failed");
	}


	/*
	 * Set the Time To Live field.
	 *
	 * NOTE: This has to be done *AFTER* setting IP_PMTUDISC_DO, because
	 *       it has the side effect of setting the TTL to 1!
	 */
	if ( hop_limit != -1 ) {
		int ret = setsockopt(sockfd, SOL_IP, IP_TTL,
					&hop_limit, sizeof hop_limit);
		if ( ret != 0 )
			ERRLog(tpparam.name, "setting IPv4 TTL failed");
	}


	/*
	 * Set the Router Alert Option (RFC-2113). This adds an IPv4 header
	 * option to outgoing IP datagrams.
	 */
	if ( rao != -1 ) {
		unsigned char rao_opt[4] = { 148, 4, 0, 0 };
		rao_opt[2] = rao >> 8;
		rao_opt[3] = rao & 0xFF;

		int ret = setsockopt(sockfd, SOL_IP, IP_OPTIONS,
					rao_opt, sizeof rao_opt);
		if ( ret != 0 )
			ERRLog(tpparam.name, "setting RAO for IPv4 failed");
	}
	else
	{
		int ret = setsockopt(sockfd, SOL_IP, IP_OPTIONS, 0, 0);
		if ( ret != 0 )
			ERRLog(tpparam.name, "unsetting IP options for IPv4 failed");
	}
}


/**
 * Set socket options for IPv6.
 *
 * NOTE: Currently (kernel 2.6.15), it is only possible to set the router alert
 *       option using setsockopt() on a RAW socket. This is weird, because
 *       anybody may set IP options in IPv4, which also allows us to set the
 *       router alert option. Anyway, we have to set ancillary data per
 *       outgoing message. Weird enough, but this works.
 *
 * @param sockfd a socket descriptor, returned by a successful call to socket()
 * @param hop_limit a value between 0 and 255, or -1 to use the route's default
 * @param rao the RAO in host byte order (a 16 bit value, or -1)
 */
void TPqueryEncap::setup_socket_ipv6(int sockfd, int hop_limit, int rao) {

	/*
	 * Set the IPv6 hop limit for outgoing datagrams.
	 * Don't confuse this with IPV6_HOP_LIMIT, which only tells the kernel
	 * to hand the hop limit field of *incoming* datagrams to the user!
	 *
	 * This setting works with kernel 2.6.15.
	 */
	if ( hop_limit != -1 ) {
		int ret = setsockopt(sockfd, SOL_IPV6, IPV6_UNICAST_HOPS,
					&hop_limit, sizeof hop_limit);
		if ( ret != 0 )
			ERRLog(tpparam.name, "setting IPv6 hop limit failed");
	}
}



/**
 * Send the given NetMsg via UDP to the given address.
 *
 * NOTE: This method deletes both the netmsg and the addr parameter!
 *
 * @param netmsg a non-NULL, non-empty NetMsg
 * @param addr the address (and port) of the peer
 */
void
TPqueryEncap::udpsend(NetMsg *netmsg, appladdress *addr, const hostaddress *own_addr)
{
  assert( netmsg != NULL );
  assert( addr != NULL );

  /*
   * If addr is an IPv4 address, we create a v4-mapped IPv6 address.
   * All messages (even IPv4 ones) will be sent via an IPv6 socket
   * except for an active ipv4-only option
   */
  in_addr ipaddr;
  in6_addr ip6addr;
  if ( plibconf.getpar<bool>(protlibconf_ipv4_only) ) 
  {
    if ( addr->is_mapped_ip() )
    {
      addr->convert_to_ipv4();
    }
    if ( addr->is_ipv4() )
      addr->get_ip(ipaddr);
    else
    {
      ERRLog(tpparam.name, "Trying to send to non IPv4 destination while in IPv4 only mode. Discarded packet.");
      return;
    }
  }
  else
  {
	  addr->convert_to_ipv6();
	  addr->get_ip(ip6addr);
  }

  int hop_limit = ( addr->get_ip_ttl() ? addr->get_ip_ttl() : -1 );
  if ( hop_limit != -1 )
    DLog(tpparam.name, "Setting IP hop limit to " << hop_limit);

  int rao = ( addr->rao_present() ? addr->get_rao() : -1 );
  if ( rao != -1 )
    DLog(tpparam.name, "Setting IP RAO to " << rao);

  uint16_t oif = addr->get_if_index();
  if ( oif != 0 )
    DLog(tpparam.name, "Setting outgoing interface index to " << oif);

  try {
    check_send_args(*netmsg, *addr);
  }
  catch ( ... ) {
    delete netmsg;
    delete addr;
    throw;
  }


  static int sock = -1;

  // create a socket if it doesn't exist yet
  if ( sock == -1 ) {
    assert( tpparam.tpoverudp != NULL );

    while ( sock == -1 ) {
      sock = tpparam.tpoverudp->get_listener_socket();

      if (sock == -1) {
	ERRLog(tpparam.name, "Could not get the socket "
	       "number for sending socket, "
	       "waiting 1s then retry...");
	sleep(1);
      }
    }

    DLog(tpparam.name,
	 "Using UDP socket " << sock << " for sending");
  }

  setup_socket_ipv4(sock, hop_limit, rao);
  if ( ! plibconf.getpar<bool>(protlibconf_ipv4_only) ) 
    setup_socket_ipv6(sock, hop_limit, rao);


  /*
   * Build the argument for sendmsg(2).
   */
  struct sockaddr_in6 dest_sa;
  memset(&dest_sa, 0, sizeof dest_sa);
  dest_sa.sin6_family	= AF_INET6;
  dest_sa.sin6_port	= htons(addr->get_port());
  dest_sa.sin6_addr	= ip6addr;

  struct sockaddr_in dest_sa_v4;
  if( plibconf.getpar<bool>(protlibconf_ipv4_only) )
  {
        char dest_addr_str[INET6_ADDRSTRLEN];

          // only required if ipv4 only option active
        memset(&dest_sa_v4, 0, sizeof(dest_sa_v4));
        dest_sa_v4.sin_family   = AF_INET;
        dest_sa_v4.sin_port     = htons(addr->get_port());
        dest_sa_v4.sin_addr.s_addr = ipaddr.s_addr;
        DLog(tpparam.name,"Sending to destination: " << inet_ntop(AF_INET, &dest_sa_v4.sin_addr, dest_addr_str, INET6_ADDRSTRLEN));

  }

  struct iovec iov;
  memset(&iov, 0, sizeof iov);
  iov.iov_base		= netmsg->get_buffer();		// the payload
  iov.iov_len		= netmsg->get_size();

  struct msghdr msg;
  memset(&msg, 0, sizeof msg);
  msg.msg_name		= &dest_sa;		// socket address v6/v4

  if (plibconf.getpar<bool>(protlibconf_ipv4_only))
	  msg.msg_name		= &dest_sa_v4;		// socket address v4

  msg.msg_namelen	= plibconf.getpar<bool>(protlibconf_ipv4_only) ? sizeof(dest_sa_v4) : sizeof(dest_sa);
  msg.msg_iov		= &iov;			// payload
  msg.msg_iovlen	= 1;

  // this sets msg.msg_control and msg.msg_controllen
  util::set_ancillary_data(&msg, rao, own_addr, oif, hop_limit);

  /*
   * We have a socket, the 'msg' argument for sendmsg(2) has been built,
   * so we can finally send the message.
   */

  // return code will be checked below
  int ret = sendmsg(sock, &msg, MSG_DONTWAIT);

  if ( debug_pdu ) {
    ostringstream out;
    netmsg->hexdump(out);
    DLog(tpparam.name, "PDU debugging enabled, sent:" << out.str());
  }

  // free ancillary data
  free(msg.msg_control);

  // we don't need this anymore
  delete netmsg;

  // Sending failed if sendmsg(2) returned something < 0.
  if ( ret < 0 ) {
    ERRLog (tpparam.name, "sender: UDP error: " << strerror(errno));

    delete addr;

    throw TPErrorSendFailed();
  }

  EVLog(tpparam.name,
	" sender: >>----Sent---->> message (" << netmsg->get_size() <<
	" bytes) using socket " << sock << " to " << *addr);

  delete addr;
}


/* terminate all active threads
 * note: locking should not be necessary here because this message is called as last method from
 * main_loop()
 */
void
TPqueryEncap::terminate_all_threads ()
{

  // empty, as no association is installed, there is none to be cleaned

}



/**
 * IPv6 catcher thread starter:
 * just a static starter method to allow starting the
 * actual master_listener_thread() method.
 *
 * @param argp - pointer to the current TPqueryEncap object instance
 */
void *
TPqueryEncap::ipv6_catcher_thread_starter (void *argp)
{
  // invoke listener thread method
  if (argp != 0)
    {
      (static_cast < TPqueryEncap * >(argp))->ipv6_catcher_thread ();
    }
  return 0;
}

/**
 * IPv4 catcher thread starter:
 * just a static starter method to allow starting the
 * actual master_listener_thread() method.
 *
 * @param argp - pointer to the current TPqueryEncap object instance
 */
void *
TPqueryEncap::ipv4_catcher_thread_starter (void *argp)
{
  // invoke listener thread method
  if (argp != 0)
    {
      (static_cast < TPqueryEncap * >(argp))->ipv4_catcher_thread ();
    }
  return 0;
}


// Yeaha, if we get no handle, we die and destroy it.
static void die (struct ipq_handle *h)
{
  ipq_perror ("LibIPQ packet interceptor");
  ipq_destroy_handle (h);
}


// This struct is used to read IPv4 IP-Options of length 4byte

struct ip_opt
{
  unsigned char opt1, opt2;
  unsigned short int opt3;
};


const uint16 udp_header_size= 8; // UDP header is 8 bytes

/**
 * IPv6 catcher thread: waits for incoming connections at the well-known udp port
 * when a connection request is received this thread spawns a receiver_thread for
 * receiving packets from the peer at the new socket.
 */
void
TPqueryEncap::ipv6_catcher_thread ()
{

  // create a new address-structure for the listening masterthread
  struct sockaddr_in6 own_address;
  own_address.sin6_family = AF_INET6;
  own_address.sin6_flowinfo = 0;
  own_address.sin6_port = htons (tpparam.port);	// use port number in param structure

  // accept incoming connections on all interfaces
  own_address.sin6_addr = in6addr_any;
  own_address.sin6_scope_id = 0;


  /*********************************************************************************************************************

     We grab UDP packets from libipq which are fed there via iptables!

  **********************************************************************************************************************/


  // initialize receiving infrastructure
  int status;
  unsigned char buf[BUFSIZE];
  struct ipq_handle *h;

  // create a handle on the ip-Queue
  h = ipq_create_handle (0, PF_INET6);
  if (!h)
    die (h);

  // we want to get copies of the packets
  status = ipq_set_mode (h, IPQ_COPY_PACKET, BUFSIZE);
  if (status < 0)
    die (h);

  bool terminate = false;
  // check for thread terminate condition using get_state()
  state_t currstate = get_state ();
  struct ip_opt *option = 0;

  // check whether this thread is signaled for termination

  // IMPORTANT: Send some freaky UDP packet to localhost to unblock after having scheduled this thread for destruction

  // if we set this var to "1" the packet will be taken, if we set it to "0", we don't take it
  bool intercept= false;

  TPMsg *tpmsg = 0;
  bool msgcontentlength_known = false;
  uint32 msgcontentlength = 0;

  appladdress* peer_addr = NULL;
  appladdress* own_addr = NULL;

  while (!
	 (terminate =
	  (currstate == STATE_ABORT || currstate == STATE_STOP)))
  {
    /***********************************************************

        Thread looping and waiting on libipq.

    ************************************************************/

    // read Packets, main loop
    status = ipq_read (h, buf, BUFSIZE, 0);
    if (status < 0)
      die (h);

    // if we dont't get packets but error messages, we complain
    switch (ipq_message_type (buf))
    {
      case NLMSG_ERROR:
	ERRCLog("TPqueryEncap", "Failed to plug IPv6 Packet interceptor via libipq to netfilter, error code:" << ipq_get_msgerr (buf));
	break;
	
      case IPQM_PACKET:
	{
	  //DLog(tpparam.name, "Intercepted IPv6 packet");
	  // we got a packet, now we read it into our buffer
	  ipq_packet_msg_t *m = ipq_get_packet (buf);
	  struct ip6_hdr *ip = (struct ip6_hdr *) m->payload;
	  uint8 next_header= ip->ip6_nxt;
	  uint8 ip_ttl = ip->ip6_hlim;
	  const uint8 ip6_headerlen= 40;

	  // this is the IPv6 capturer, we have got a handle to the IPv6 Queue, so we can be sure to receive only IPv6 packets

	  // We have to check, if Hop-By-Hop Option Header is present. If it is, it is the first extension header
	  // with code "0" in ip6_next of base header
	  if (next_header == 0) intercept= true;
	
	  // We have to look for RAO option in Hop-by-Hop extension header, if it is NOT set to a value we look for, don't intercept
	  if (intercept)
	  {
	    struct ip6_hbh *optheader = (struct ip6_hbh *) (m->payload + ip6_headerlen);
	    DLog(tpparam.name, "[IPv6catcher] - IPv6 Packet with HbH-option header received, inspecting it");
	    intercept= false;
	
	    int i = 0;
	    option = (ip_opt *) (m->payload + ip6_headerlen + 2);
	
	    if (option->opt1 == IP6_RAO_OPT_TYPE) {
	      intercept = true;
	    }
	    else
	    {
	      do
	      {
		// PADDING! We must move one right!
		if (option->opt1 == 0)
		{
		  i++;
		  option = (ip_opt *) (m->payload + ip6_headerlen + 2 +i);
		}
		// No PADDING! We have hit an option and its not Router alert! We must move "LENGTH+2" right!
		if ((option->opt1 != 0)&(option->opt1 != IP6_RAO_OPT_TYPE)) i = option->opt2+2;
		option = (ip_opt *) (m->payload + ip6_headerlen + 2 + i);
		// We have hit Router Alert! Break loop, leave it alone!
		if (option->opt1 == IP6_RAO_OPT_TYPE) { intercept= true; break; }
	      }
	      while (i <= optheader->ip6h_len); // don't overrun end of ip hop-by-hop options header!
	    } // end else
	  } // endif intercept
	
	  // 1st check: now check for matching RAO values
	  if (intercept)
	  {
	    ostringstream os;
		
	    for (unsigned int i = 0; i < tpparam.raovec.size(); i++) {
	      os << "|" << (int) tpparam.raovec[i];
	    }
		
	    DLog(tpparam.name, "[IPv6catcher] - Listening for RAOs: " << os.str().c_str());
	    DLog(tpparam.name, "[IPv6catcher] - Inspecting RAO value of: " << ntohs(option->opt3));

	    intercept = false;
	    // inspect RAO vector
	    for (unsigned int i = 0; i < tpparam.raovec.size(); i++) {
	      if (tpparam.raovec[i] == ntohs(option->opt3)) intercept= true;
	    } // end for
	  } // end if intercept

	  // if intercept is false, RAO value did not match, stop here if strict RAO matching is required
	  if (intercept == false && tpparam.strict_rao)
	  {
	    // we don't care about this packet, let it pass the firewall
	    status = ipq_set_verdict (h, m->packet_id, NF_ACCEPT, 0, NULL);
	    if (status < 0)
	      die (h);
	    //Log (DEBUG_LOG, LOG_NORMAL, tpparam.name, "[IPv6catcher] - I let a packet without RAO set pass");
	  }
	  else
	  {
	    if (intercept)
	    {
	      // so far the RAO instructs us to intercept this packet, but we still have to check for the magic number
	      DLog(tpparam.name, "[IPv6catcher] - I am instructed to intercept this RAO");
	    }
	    else
	    {
	      DLog(tpparam.name, "[IPv6catcher] - Checking for interception even if no RAO used");
	      intercept= true;
	    }
	  }

	  int offset = ip6_headerlen; // will point to IP payload, initially size of IPv6 header

	  // one RAO value matched (or we do not require strict RAO checking), now check for magic number
	  if (intercept)
	  {	
	    peer_addr = new appladdress();
	    own_addr = new appladdress();
	     		
	    // Now to do: iterate over extension headers to find the start of the UDP header
	    // the first extension header is the hop-by-hop options header and it is present
	    // if RAO was used
	    struct ip6_ext *extheader = (struct ip6_ext *) (m->payload + offset);
		
	    // if the 2nd extension header is upper layer header, we found the UDP offset
	    if (next_header == IPPROTO_UDP)
	    {
	      DLog(tpparam.name, "[IPv6catcher] - Instantly found UDP header, do not loop over headers");
	    }
	    else
	    {
	      // assume that some extension header is present
	      next_header= extheader->ip6e_nxt;
	      // iterate if next header is not UDP
	      while (next_header != IPPROTO_UDP)
	      {
		// advance to next extension header
		offset = offset + 8 + (extheader->ip6e_len * 8);
		if (offset > 20000) // sanity check
		{
		  ERRCLog(tpparam.name,  "[IPv6catcher] - IP Extension header parsing not successful, I missed UDP header");
		  // rollback
		  offset = offset - 8 - (extheader->ip6e_len * 8);
		  break;
		}
		extheader = (struct ip6_ext *) (m->payload + offset);
		next_header= extheader->ip6e_nxt;
	      } // end while
	    }
	    // set offset to next header following the last extension header that we saw
	    offset = offset + 8 + (extheader->ip6e_len * 8);

	    // magic number is only checked for if != 0
	    if (tpparam.magic_number != 0)
	    {
	      uint32 *magic_number_field= reinterpret_cast<uint32 *>(m->payload + offset + udp_header_size);
	      // now check for magic number in UDP payload
	      if (tpparam.magic_number != ntohl(*magic_number_field))
	      { // magic_number does not fit -> do not intercept the packet
		// we don't care about this packet, let it pass the firewall
		WLog(tpparam.name, "[IPv6catcher] - magic number mismatch, read: 0x" << hex << ntohl(*magic_number_field) << dec << " @ byte pos:" << offset + udp_header_size);
		
		status = ipq_set_verdict (h, m->packet_id, NF_ACCEPT, 0, NULL);
		if (status < 0)
		  die (h);
		// stop interception
		intercept= false;
	      }
	      else
	      {
		DLog(tpparam.name, "[IPv6catcher] - " << color[green] << "magic number matched" << color[off]);
		uint32 *common_header_word2= reinterpret_cast<uint32 *>(m->payload + offset + udp_header_size + 8);
		if ( !tpparam.strict_rao && (ntohl(*common_header_word2) & 0x8000)==0 )
		{
		  DLog(tpparam.name, "[IPv6catcher] - C-Flag not set - will not intercept");
		  intercept= false;
		}
	      }

	    } // endif magic number given
	  } // endif at least one RAO matched

	  // packet passed all checks, so this packet is to be processed by this node
	  if (intercept)
	  {
	    // we let the firewall stop it and process it further in userspace

	    status = ipq_set_verdict (h, m->packet_id, NF_DROP, 0, NULL);
	    if (status < 0)
	      die (h);

	    // build a new NetMsg
	    // we need a NetMsg to memcpy our received buffer into and then send to signaling thread
	    // we delete it on failure of sending TPMsg.
	    NetMsg *netmsg = new NetMsg (NetMsg::max_size);

	    in6_addr saddr;
	    in6_addr daddr;
	    saddr = (ip->ip6_src);
	    daddr = (ip->ip6_dst);
	
	    struct udphdr *udp = (struct udphdr *) (m->payload + offset);
	    // Fill peer_addr
	    peer_addr->set_ip(ip->ip6_src);
	    peer_addr->set_protocol(prot_query_encap);
	    peer_addr->set_port(htons(udp->source));
	    peer_addr->set_ip_ttl(ip_ttl);
	
	    // register incoming interface of query, this may be carried in the responder cookie later
	    // please note that interface index is still too coarse in case of mobility if you have
            // different and changing addresses (Care-of addresses) at the same interface
	    peer_addr->set_if_index(if_nametoindex (m->indev_name));

	    DLog(tpparam.name, "[IPv6catcher] - Incoming interface: " << m->indev_name << ", if_index:" << peer_addr->get_if_index());	
		
	    // copy payload to netmsg buffer (including magic number)
	    DLog (tpparam.name, "[IPv6catcher] - UDP packet received, memcpy into netMsg, UDP length: " << ntohs(udp->len));
	
	    memcpy (netmsg->get_buffer(), m->payload + offset + udp_header_size,  ntohs(udp->len) - udp_header_size);
	    // truncate buffer to total length - UDP header
	    netmsg->truncate(ntohs(udp->len) - udp_header_size);

	    // if magic number present, skip it for length check
	    if (tpparam.magic_number)
	      netmsg->decode32();
		
	    // get message content length in number of 32-bit words
	    if (tpparam.getmsglength (*netmsg, msgcontentlength))
	      msgcontentlength_known = true;
	    else
	    {
	      ERRCLog (tpparam.name,
		   "[IPv6catcher] - Not a valid Protocol header - discarding received packet.");
	
	      ostringstream hexdumpstr;
	      netmsg->hexdump (hexdumpstr, netmsg->get_buffer (), 40);
	      DLog (tpparam.name, "[IPv6catcher] - dumping received bytes:" << hexdumpstr.str ());
	    }
		
	    // go back to beginning of netmsg
	    netmsg->to_start();

	    // create TPMsg and send it to the signaling thread
	    tpmsg = new (nothrow) TPMsg (netmsg, peer_addr->copy(), own_addr->copy());

	    Log (DEBUG_LOG, LOG_NORMAL, tpparam.name,
		 "[IPv6catcher] - receipt of PDU now complete, sending msg#" << tpmsg->get_id() << " to signaling module");
		
	    if (!tpmsg->send(message::qaddr_tp_queryencap, message::qaddr_signaling))
	    {
	      Log (ERROR_LOG, LOG_NORMAL, tpparam.name,
		   "[IPv6catcher] - " << "Cannot allocate/send TPMsg");
	      if (tpmsg)
		delete tpmsg;
	      if (netmsg)
		delete netmsg;
	    } // endif
	  } // endif intercept
	} // end case IQM_PACKET
	break;
	
      default:
	ERRCLog(tpparam.name, "[IPv6catcher] - IP6Queue Kernel Module not loaded/accessible, glibc kills me, am I run as root?");
	break;
    } // end switch
	
    // get new thread state
    currstate = get_state ();
    intercept= false;
  } // end while(!terminate)

  ipq_destroy_handle (h);

  return;
} // end listen_for_connections()





/**
 * IPv4 catcher thread: waits for incoming connections at the well-known udp port
 * when a connection request is received this thread spawns a receiver_thread for
 * receiving packets from the peer at the new socket.
 */
void
TPqueryEncap::ipv4_catcher_thread ()
{
    // create a new address-structure for the listening masterthread
    struct sockaddr_in6 own_address;
    own_address.sin6_family = AF_INET6;
    own_address.sin6_flowinfo = 0;
    own_address.sin6_port = htons (tpparam.port);	// use port number in param structure
    // accept incoming connections on all interfaces
    own_address.sin6_addr = in6addr_any;
    own_address.sin6_scope_id = 0;


    /*********************************************************************************************************************
     grab UDP packets from libipq which are fed there via iptables!
    **********************************************************************************************************************/


    //now we leave it alone and initialize receiving infrastructure


    int status;
    unsigned char buf[BUFSIZE];
    struct ipq_handle *h;

    //create a handle on the ip-Queue

    h = ipq_create_handle (0, PF_INET);
    if (!h)
      die (h);


    //we want to get copies of the packets

    status = ipq_set_mode (h, IPQ_COPY_PACKET, BUFSIZE);
    if (status < 0)
      die (h);

    bool terminate = false;
    // check for thread terminate condition using get_state()
    state_t currstate = get_state ();
    // int poll_status = 0;
    // const unsigned int number_poll_sockets = 1;
    // struct sockaddr_in6 peer_address;
    // socklen_t peer_address_len;
    // int conn_socket;

    // check whether this thread is signaled for termination

    // IMPORTANT: Send some freaky UDP packet to localhost to unblock after having scheduled this thread for destruction


    // if we set this var to "1" the packet will be punted, if we set it to "0", we don't punt it
    bool intercept= false;

    TPMsg *tpmsg = 0;
    bool msgcontentlength_known = false;
    uint32 msgcontentlength = 0;

    appladdress* peer_addr = NULL;
    appladdress* own_addr = NULL;

    while (!
	   (terminate =
	    (currstate == STATE_ABORT || currstate == STATE_STOP)))
      {
	

	/***********************************************************

        Thread looping and waiting on libipq.

	************************************************************/

	NetMsg *netmsg = NULL;

	// read Packets, main loop

	status = ipq_read (h, buf, BUFSIZE, 0);
	if (status < 0)
	  die (h);

	// if we dont't get packets but error messages, we complain

	switch (ipq_message_type (buf))
	{
	  case NLMSG_ERROR:
	    ERRCLog("TPqueryEncap", "Failed to plug IPv4 Packet interceptor via libipq to netfilter, error code:" << ipq_get_msgerr (buf));

	    break;

	  case IPQM_PACKET:
	  {

	    // we got a packet, now we read it into our buffer
	
	    //ERRCLog("TPqueryEncap", "[IPv4catcher] Intercepted IPv4 packet");	
		
	    ipq_packet_msg_t *m = ipq_get_packet (buf);
	    struct iphdr *ip = (struct iphdr *) m->payload;

	    // this is the IPv4 capturer, we have got a handle to the IPv4 Queue, so we can be sure to receive IPv4 packets
	
	    // We have to check if Option field is present, packets without options will be allowed to pass netfilter
	    // Packets with Options set will be stopped at firewall and the copies processed
	    if ((ip->ihl) > 5)
	    {

	      // IPv4: Header length > 5 32bit words -> options included!

	      // 1st: Try it and get the first 4 bytes:

	      struct ip_opt *option =
		(ip_opt *) (m->payload + sizeof (iphdr));
	
	      // 2nd: If the option type read is NOT "148", then we got the wrong option field, iterate through buffer!!
	
	      if (option->opt1 != 148)
	      {
		int i = 0;
		do
		{
		  struct ip_opt *option =
		    (ip_opt *) (m->payload + sizeof (iphdr) + 4 * i);
		
		  // we must shift by defined option length!
		  i += (option->opt2);
		
		  // we MUST break, if we reach (m->payload + 4* (ip->ihl)!!!
		
		  if (sizeof (iphdr) + 4 * i >= (size_t)4 * (ip->ihl))
		    break;
		
		}
		while (option->opt1 != 148);
	      }

	      if (option->opt1 == 148)
	      {
		// we got a packet with RAO set
		ostringstream os;
		
		for (unsigned int i = 0; i < tpparam.raovec.size(); i++)
		{
		  os << "|" << (int) tpparam.raovec[i];
		}
		
		uint16 rao = htons (option->opt3);

		DLog(tpparam.name, "[IPv4catcher] - Listening for RAOs: " << os.str().c_str());
		
		DLog(tpparam.name, "[IPv4catcher] - Inspecting RAO value of: " << rao);

		intercept= false;
		for (unsigned int i = 0; i < tpparam.raovec.size(); i++)
		{
		  if (tpparam.raovec[i] == rao){
		    intercept = true;
		  }
		} // end for

		if (intercept)
		  DLog(tpparam.name, "[IPv4catcher] - I am instructed to intercept packages with this RAO (" << (int) rao << ")");
	      } // opt 148
	      else
	      {
		// we got a packet with RAO not set, but other options set
		// we let it pass the firewall		
		intercept= false;
	      }
	    } // end if options present
	    else
	    {
	      // we got a packet which has no options in the header
	      	
	      //DLog("[IPv4catcher]" , "Let a Packet pass");
	      if (tpparam.strict_rao)
		intercept= false;
	      else // let the magic number decide
	      {
		DLog(tpparam.name, "[IPv4catcher] - Checking for interception even if no RAO used");
		intercept= true;
	      }
	    }

	    // magic number is only checked for if not zero
	    if (intercept && tpparam.magic_number != 0)
	    {
	      uint32 *magic_number_field= reinterpret_cast<uint32 *>(m->payload + 4 * (ip->ihl) + udp_header_size);
	      // now check for magic number in UDP payload
	      if (tpparam.magic_number != ntohl(*magic_number_field))
		{ // magic_number does not fit -> do not intercept the packet
		  // we don't care about this packet, let it pass the firewall
		  WLog(tpparam.name, "[IPv4catcher] - magic number mismatch, read: 0x" << hex << ntohl(*magic_number_field) << dec);
		  // do not intercept
		  intercept= false;
		}
	      else
	      {
		DLog(tpparam.name, "[IPv4catcher] - " << color[green] << "magic number matched" << color[off]);
		uint32 *common_header_word2= reinterpret_cast<uint32 *>(m->payload + 4 * (ip->ihl) + udp_header_size + 8);
		if ( !tpparam.strict_rao && (ntohl(*common_header_word2) & 0x8000)==0 )
		{
		  DLog(tpparam.name, "[IPv4catcher] - C-Flag not set - will not intercept");
		  intercept= false;
		}
	      }
	
	    } // endif magic number given
	
	    // if intercept is still true, we really will intercept it now
	    if (intercept == true)
	    {

	      status = ipq_set_verdict (h, m->packet_id,
					NF_DROP, 0, NULL);
	      if (status < 0)
		die (h);

	      // build a new NetMsg
	      // we need a NetMsg to memcpy our received buffer into and then send to signaling thread
	      netmsg = new NetMsg (NetMsg::max_size);
	
	      peer_addr = new appladdress();
	      own_addr = new appladdress();
	
		
	      struct udphdr *udp =
		(struct udphdr *) (m->payload + 4 * (ip->ihl));
	      in_addr saddr;
	      in_addr daddr;
	      saddr.s_addr = (ip->saddr);
	      daddr.s_addr = (ip->daddr);
	
	      // fill peer_addr
	      peer_addr->set_protocol(prot_query_encap);
	      peer_addr->set_port(htons(udp->source));
	      peer_addr->set_ip(saddr);
	      peer_addr->set_ip_ttl(ip->ttl);

	      // register incoming interface of query, this may be carried in the responder cookie later
	      // please note that interface index is still too coarse in case of mobility if you have
	      // different and changing addresses (Care-of addresses) at the same interface
	      peer_addr->set_if_index(if_nametoindex (m->indev_name));
	
	      // fill own addr
	      DLog(tpparam.name, "[IPv4catcher] - Incoming interface: " << m->indev_name << ", if_index:" << peer_addr->get_if_index());
	
	      // fill own addr
	      DLog(tpparam.name, "[IPv4catcher] - Received packet from: " << *peer_addr);
	
	
	      // copy payload to netmsg buffer
	      DLog(tpparam.name, "[IPv4catcher] - Sending received UDP packet to signaling module, UDP data length:" << ntohs(udp->len));
	
	      memcpy (netmsg->get_buffer(), (m->payload + 4 *(ip->ihl) + udp_header_size), ntohs(udp->len) - udp_header_size);
	
	      // skip magic number if required
	      if (tpparam.magic_number)
		netmsg->decode32();

	      // get message content length in number of 32-bit words
	      if (getmsglength (*netmsg, msgcontentlength))
		msgcontentlength_known = true;
	      else
	      {
		Log (ERROR_LOG, LOG_CRIT, tpparam.name,
		     "[IPv4catcher] - " <<
		     "Not a valid protocol header - discarding received packet.");
	      }
	      // reset to beginning (including magic number)
	      netmsg->to_start();
	      // truncate buffer to total length - UDP header
	      netmsg->truncate(ntohs(udp->len) - udp_header_size);

	      // create TPMsg and send it to the signaling thread
	
	      tpmsg = new (nothrow) TPMsg (netmsg, peer_addr->copy(), own_addr->copy());

	      Log (DEBUG_LOG, LOG_NORMAL, tpparam.name,
		   "[IPv4catcher] - " <<
		   "receipt of PDU now complete, sending msg#" << tpmsg->get_id() << " to signaling module");
	
	    /*
	    if (debug_pdu)
	      {
		ostringstream hexdump;
		netmsg->hexdump (hexdump, netmsg->get_buffer (),
				 bytes_received);
		Log (DEBUG_LOG, LOG_NORMAL, tpparam.name,
		     "PDU debugging enabled - Received:" << hexdump.str ());
	      }
	    */

	    // send the message if it was successfully created
	      if (!tpmsg->send(message::qaddr_tp_queryencap, message::qaddr_signaling))
	      {
		Log (ERROR_LOG, LOG_NORMAL, tpparam.name,
		     "[IPv4catcher] - " << "Cannot allocate/send TPMsg");
		if (tpmsg)
		  delete tpmsg;
		if (netmsg)
		    delete netmsg;
	
	      }
	
	    }
	    else
	    { // intercept == false
	      // either no IP options, different IP options,
              // non matching RAO value or magic number mismatch

	      // we let it pass the firewall
	      status = ipq_set_verdict (h, m->packet_id,
					NF_ACCEPT, 0, NULL);
	      if (status < 0)
		die (h);
	    }
	    break;
	  }
	
	default:
	  ERRCLog(tpparam.name, "[IPv4catcher] - ip_queue kernel module not accessible, glibc kills me, run me as root!");
	  break;
	} // end switch
	
	// get new thread state
	currstate = get_state ();
	intercept = 0;
      }				// end while(!terminate)

    ipq_destroy_handle (h);

    return;
}				// end listen_for_connections()


TPqueryEncap::~TPqueryEncap ()
{
  init = false;

  Log (DEBUG_LOG, LOG_NORMAL, tpparam.name, "Destructor called");

  QueueManager::instance ()->unregister_queue (message::qaddr_tp_queryencap);
}

/** TPqueryEncap Thread main loop.
 * This loop checks for internal messages of either
 * a send() call to start a new receiver thread, or,
 * of a receiver_thread() that signals its own termination
 * for proper cleanup of control structures.
 *
 * @param nr number of current thread instance
 */
void
TPqueryEncap::main_loop (uint32 nr)
{

  ostringstream os;

  for (unsigned int i = 0; i < tpparam.raovec.size(); i++) {

    os << "|" << (int) tpparam.raovec[i];

  }

  DLog(tpparam.name, "Listening for RAOs: " << os.str().c_str());

  // get internal queue for messages from receiver_thread
  FastQueue *fq = get_fqueue ();
  if (!fq)
  {
    Log (ERROR_LOG, LOG_CRIT, tpparam.name, "Cannot find message queue");
    return;
  }				// end if not fq
  // register queue for receiving internal messages from other modules
  QueueManager::instance ()->register_queue (fq,
					       message::qaddr_tp_queryencap);




    // start IPv6 catcher thread
    pthread_t master_listener_thread_ID;
    int pthread_status = pthread_create (&master_listener_thread_ID,
					 NULL,	// NULL: default attributes: thread is joinable and has a
					 //       default, non-realtime scheduling policy
					 ipv6_catcher_thread_starter,
					 this);
    if (pthread_status)
      {
	Log (ERROR_LOG, LOG_CRIT, tpparam.name,
	     "New master listener thread could not be created: " <<
	     strerror (pthread_status));
      }
    else
      Log (EVENT_LOG, LOG_NORMAL, tpparam.name, color[green] << "IPv6 packet interceptor thread started" << color[off]);

    // start IPv4 catcher thread
    pthread_t ipv4_catcher_thread_ID;
    pthread_status = pthread_create (&ipv4_catcher_thread_ID, NULL,	//NULL: default attributes
				     ipv4_catcher_thread_starter, this);
    if (pthread_status)
      {
	Log (ERROR_LOG, LOG_CRIT, tpparam.name,
	     "IPv4 catcher thread could not be created: " <<
	     strerror (pthread_status));
      }
    else
      Log (EVENT_LOG, LOG_NORMAL, tpparam.name, color[green] << "IPv4 packet interceptor thread started" << color[off]);


    // define max latency for thread reaction on termination/stop signal
    timespec wait_interval = { 0, 250000000L };	// 250ms
    message *internal_thread_msg = NULL;
    state_t currstate = get_state ();

    // check whether this thread is signaled for termination
    while (currstate != STATE_ABORT && currstate != STATE_STOP)
      {
	// poll internal message queue (blocking)
	if ((internal_thread_msg =
	     fq->dequeue_timedwait (wait_interval)) != 0)
	  {
	    TPqueryEncapMsg *internalmsg =
	      dynamic_cast < TPqueryEncapMsg * >(internal_thread_msg);
	    if (internalmsg)
	      {
		if (internalmsg->get_msgtype () == TPqueryEncapMsg::stop)
		  {
		    // a receiver thread terminated and signaled for cleanup
		    // by master thread
		    DLog (tpparam.name, "Got cleanup request for thread <"
			 << internalmsg->get_peer_assoc()->thread_ID << '>');
		  }
		else if (internalmsg->get_msgtype () == TPqueryEncapMsg::start)
		  {
		    // start a new receiver thread
		    /*create_new_receiver_thread (const_cast <
						AssocData *
						>(internalmsg->
						  get_peer_assoc ()));
		    */
		  }
		else
		  Log (ERROR_LOG, LOG_CRIT, tpparam.name,
		       "unexpected internal message:" << internalmsg->
		       get_msgtype ());

		delete internalmsg;
	      }
	    else
	      {
		Log (ERROR_LOG, LOG_CRIT, tpparam.name,
		     "Dynamic_cast failed - received unexpected and unknown internal message source "
		     << internal_thread_msg->get_source ());
	      }
	  }			// endif

	// get thread state
	currstate = get_state ();
      }				// end while

    if (currstate == STATE_STOP)
      {
	// start abort actions
	Log (INFO_LOG, LOG_NORMAL, tpparam.name,
	     "Asked to abort, stopping all receiver threads");
      }				// end if stopped

    // do not accept any more messages
    fq->shutdown ();
    // terminate all receiver and sender threads that are still active
    terminate_all_threads ();
  }

}				// end namespace protlib

///@}
