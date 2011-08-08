#include "routing_util.h"

#include <net/if.h>
#include <linux/types.h> // in case that this one is not included in netlink.h
#include <linux/netlink.h>
#include <linux/rtnetlink.h>

#include "readnl.h"

#include <cerrno>
#include <err.h>

namespace protlib {

	namespace util {
/**
 * Returns the interface number the kernel would use for this address
 *
 * This depends on the Direction flag (upstream/downstream).
 *
 * TODO: Review and cleanup necessary.
 */
uint16 
get_local_if(const hostaddress& sourceaddress, const hostaddress& destaddress)
{
    //#######################################################################
    //
    // Query the kernel via RTNetlink socket with RTM_GET_ROUTE
    //
    // currently, this fails for IPv4, no idea as to why
    //
    //#######################################################################

    // Buffer for error messages, used by the strerror_r() calls below.
    char msg_buf[1024];

    // We must decide whether IPv4 or IPv6 is used
    bool ipv6 = destaddress.is_ipv6();


    // 2 sockaddr_in6?
    struct in6_addr dstaddr, srcaddr;
    struct in_addr dstaddr4, srcaddr4;
    
    // Netlink Message header plus RTNetlink Message Header plus 1024byte data
    struct {
	struct nlmsghdr n;
	struct rtmsg r;
	char data[1024];
    } req;

    // Struct rtattr
    struct rtattr *attr;
    struct rtattr *attr2;

    
    // int outgoinginterface
    uint16 outgoinginterface = 0;

    
    // enable this routine here
    if (true) {

	// OPEN a RTNETLINK SOCKET
	
	int sock = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
	if (sock < 0) {
	    strerror_r(errno, msg_buf, sizeof msg_buf);
	    ERRCLog("get_local_if()",
			"error opening Netlink socket: " << msg_buf);
	}	

	// if IPv6

	if (ipv6) {

	    destaddress.get_ip(dstaddr);
	    sourceaddress.get_ip(srcaddr);

	    memset(&req, 0, sizeof(req));
	    req.n.nlmsg_len =
		NLMSG_ALIGN(NLMSG_LENGTH(sizeof(req.r))) +
		RTA_LENGTH(sizeof(dstaddr)) + RTA_LENGTH(sizeof(srcaddr));
	    req.n.nlmsg_flags = NLM_F_REQUEST;
	    req.n.nlmsg_type = RTM_GETROUTE;
	    req.r.rtm_family = AF_INET6;
	    req.r.rtm_dst_len = 128;
	    req.r.rtm_src_len = 128;

	    attr = (rtattr*) (((char *) &req) + NLMSG_ALIGN(NLMSG_LENGTH(sizeof(req.r))));
	    attr->rta_type = RTA_DST;
	    attr->rta_len = RTA_LENGTH(sizeof(dstaddr));

	    memcpy(RTA_DATA(attr), &dstaddr, sizeof(dstaddr));

	    attr2 = (rtattr*) (((char*) &req) + NLMSG_ALIGN(NLMSG_LENGTH(sizeof(req.r))) + attr->rta_len);
	    attr2->rta_type = RTA_SRC;
	    attr2->rta_len = RTA_LENGTH(sizeof(srcaddr));

	    memcpy(RTA_DATA(attr2), &srcaddr, sizeof(srcaddr));

	    // Send to Kernel

	     /*Send the message*/

	    if (send(sock, &req, req.n.nlmsg_len, 0) < 0) {
		strerror_r(errno, msg_buf, sizeof msg_buf);
		ERRCLog("get_local_if()",
			"error sending Netlink message: " << msg_buf);
		close(sock);
	    }

	    char* returnbuf = new char[NL_BUFSIZE];

	    int read = util::readnl(sock, returnbuf);

	    if (read < 0) return 0;

	    nlmsghdr* nlhdrp = (struct nlmsghdr *)returnbuf;

	    rtmsg* rtmsg = (struct rtmsg*)NLMSG_DATA(nlhdrp);
	    

	    // Iterate through data area serachin

	    rtattr* attrp = RTM_RTA(rtmsg);
	    uint32 attrlen = RTM_PAYLOAD(nlhdrp);

	    for( ; RTA_OK(attrp, attrlen); attrp = RTA_NEXT(attrp, attrlen) ){

		
		if ((attrp->rta_type) == RTA_OIF) {
		    
		    char* ifname= new char[IF_NAMESIZE];
		    if_indextoname(*(int *)RTA_DATA(attrp), ifname);

		    Log(EVENT_LOG, LOG_NORMAL, "mri_pathcoupled",  "Kernel decided the outgoing interface index should be " << *(int *)RTA_DATA(attrp) << ", interface Name " << ifname);
		
		    outgoinginterface=*(int *)RTA_DATA(attrp);
		    delete ifname;
		}


	    }

	} //end if IPv6
	else
	{
	    //cout << "IPv4 used, query for IPv4" << endl;

	    destaddress.get_ip(dstaddr4);
	    sourceaddress.get_ip(srcaddr4);

	    //cout << "Got socket addresses from data structures" << endl;
	    
	    memset(&req, 0, sizeof(req));
	    req.n.nlmsg_len =
		NLMSG_ALIGN(NLMSG_LENGTH(sizeof(req.r))) +
		RTA_LENGTH(sizeof(dstaddr4)) + RTA_LENGTH(sizeof(srcaddr4));
	    req.n.nlmsg_flags = NLM_F_REQUEST;
	    req.n.nlmsg_type = RTM_GETROUTE;
	    req.r.rtm_family = AF_INET;
	    req.r.rtm_dst_len = 12;
	    req.r.rtm_src_len = 12;

	    attr = (rtattr*) (((char *) &req) + NLMSG_ALIGN(NLMSG_LENGTH(sizeof(req.r))));
	    attr->rta_type = RTA_DST;
	    attr->rta_len = RTA_LENGTH(sizeof(dstaddr4));

	    memcpy(RTA_DATA(attr), &dstaddr4, sizeof(dstaddr4));

	    attr2 = (rtattr*) (((char*) &req) + NLMSG_ALIGN(NLMSG_LENGTH(sizeof(req.r))) + RTA_LENGTH(sizeof(dstaddr4)));
	    attr2->rta_type = RTA_SRC;
	    attr2->rta_len = RTA_LENGTH(sizeof(srcaddr4));


	    memcpy(RTA_DATA(attr2), &srcaddr4, sizeof(srcaddr4));


	    // Send to Kernel

	    if (send(sock, &req, req.n.nlmsg_len, 0) < 0) {
		strerror_r(errno, msg_buf, sizeof msg_buf);
		ERRCLog("get_local_if()",
			"error sending Netlink message: " << msg_buf);
		close(sock);
	    }

	    char* returnbuf = new char[NL_BUFSIZE];

	    int read = util::readnl(sock, returnbuf);

	    if (read <0 ) return 0;

	    nlmsghdr* nlhdrp = (struct nlmsghdr *)returnbuf;

	    rtmsg* rtmsg = (struct rtmsg*)NLMSG_DATA(nlhdrp);
	    
	    // Iterate through data area serachin
	    rtattr* attrp = RTM_RTA(rtmsg);
	    uint32 attrlen = RTM_PAYLOAD(nlhdrp);

	    for( ; RTA_OK(attrp, attrlen); attrp = RTA_NEXT(attrp, attrlen) ){

		if ((attrp->rta_type) == RTA_OIF) {
		    
		    char* ifname= new char[IF_NAMESIZE];
		    if_indextoname(*(int *)RTA_DATA(attrp), ifname);

		    Log(EVENT_LOG, LOG_NORMAL, "mri_pathcoupled",  "Linux Kernel decided the outgoing interface index should be " << *(int *)RTA_DATA(attrp) << ", interface Name " << ifname);
		
		    outgoinginterface=*(int *)RTA_DATA(attrp);
		    delete ifname;
		}


	    }

	    if (returnbuf) delete returnbuf;
	    close(sock);

	    if (!outgoinginterface) {
		
		EVLog("get_local_if()", "Linux Kernel was unable to provide a outgoing interface for this flow, taking default route.");

	    }
	}
    }
    
    return outgoinginterface;
}


/**
 * returns a source address used for forwarding an outgoing 
 * packet with a given destination address
 **/
netaddress *
get_src_addr(const netaddress &dest)
{
	netaddress *res;
	int sfd;

	netaddress canonical_dest(dest);
	
	// convert to canonical representation (do not use IPv4 mapped addresses!)
	// either plain IPv4 or plain IPv6
	canonical_dest.convert_to_ipv4();

	sfd = socket(canonical_dest.is_ipv4() ? AF_INET : AF_INET6, SOCK_DGRAM, 0);
	if (sfd == -1) {
		err(1, "socket");
		return NULL;
	}

	if (canonical_dest.is_ipv4()) {
		struct sockaddr_in sin = {0};
		socklen_t slen = sizeof(sin);
		sin.sin_family = AF_INET;
		sin.sin_port = htons(4);
		canonical_dest.get_ip(sin.sin_addr);
		if (connect(sfd, (struct sockaddr *)&sin, sizeof(sin)) == -1) {
			err(1, "connect");
			close(sfd);
			return NULL;
		}
		if (getsockname(sfd, (struct sockaddr *)&sin, &slen) == -1) {
			err(1, "getsockname");
			close(sfd);
			return NULL;
		}
		close(sfd);
		res = new netaddress();
		res->set_ip(sin.sin_addr);
		res->set_pref_len(32);
		return (res);
	} else {
		struct sockaddr_in6 sin6 = {0};
		socklen_t slen = sizeof(sin6);
		sin6.sin6_family = AF_INET6;
		sin6.sin6_port = htons(4);
		canonical_dest.get_ip(sin6.sin6_addr);
		if (connect(sfd, (struct sockaddr *)&sin6,
		    sizeof(sin6)) == -1) {
			err(1, "connect");
			close(sfd);
			return NULL;
		}
		if (getsockname(sfd, (struct sockaddr *)&sin6, &slen) == -1) {
			err(1, "getsockname");
			close(sfd);
			return NULL;
		}
		close(sfd);
		res = new netaddress();
		res->set_ip(sin6.sin6_addr);
		res->set_pref_len(128);
		return (res);
	}
}

	} // end namespace

} // end namespace
