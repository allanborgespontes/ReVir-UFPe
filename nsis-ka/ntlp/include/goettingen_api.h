/***********************************************************************
 * api_def.h --                                                        *
 *                                                                     *
 * copyright (c) 2005 Bernd Schloer                                    *
 *                                                                     *
 * This program is free software; you can redistribute it and/or       *
 * modify it under the terms of the GNU General Public License         *
 * as published by the Free Software Foundation; either version 2      *
 * of the License, or (at your option) any later version.              *
 *                                                                     *
 * This program is distributed in the hope that it will be useful,     *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of      *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the       *
 * GNU General Public License for more details.                        *
 *                                                                     *
 * You should have received a copy of the GNU General Public License   *
 * along with this program; if not, write to the Free Software         *
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston,              *
 * MA  02111-1307, USA.                                                *
 *                                                                     *
 * ------------------------------------------------------------------- *
 * project:  NSIS-Project                                              *
 * author :  Bernd Schloer <bschloer@cs.uni-goettingen.de>             *
 * created:  Mon Mar 14, 2005                                          *
 ***********************************************************************/


#include <sys/types.h>
#include <sys/socket.h>
#include <sys/poll.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include "goettingen_headers.h"
//#include "constants.h"

#ifndef API_DEF_H
#define API_DEF_H


#define PATH_TO_UNIX_SOCKET "/tmp/gist"

#ifndef __cplusplus
typedef char bool;
#endif
#ifndef FALSE
#define FALSE 0
#endif
#ifndef TRUE
#define TRUE  1
#endif

#define BUFFER_SIZE     4096
#define DOWNSTREAM         0
#define UPSTREAM           1

#define SID_SIZE          16
#define MSGHANDLE_SIZE    16

struct IPaddr {
	unsigned int content[1 + 1 + 4];
};

enum NetworkNotificationType {

	LastNode,
	RoutingStateChange

};

enum {
	messagestatus_unknownerror,
	messagestatus_errorwhilesending,
	
	/* GIST was unable to establish routing state with the peer.
	 * This is caused, when GIST receives no Response to an initial
	 * Query (after a configured number of retransmissions). 
	 * All pending message have been dropped and could not be transmitted
	 */
	messagestatus_unabletoestablishroutingstate
};


//#define SND_MSG_FSIZE     (2 * sizeof(int) + 1 * sizeof(short) + SID_SIZE + 3 * sizeof(bool) + 4 * sizeof(char))
#define SND_MSG_FSIZE     (2 * sizeof(int) + 1 * sizeof(short) + SID_SIZE + 4 * sizeof(bool) +\
		5 * sizeof(char) + MSGHANDLE_SIZE) + sizeof(IPaddr)

//#define RCV_MSG_FSIZE     (2 * sizeof(int) + 1 * sizeof(short) + SID_SIZE + 2 * sizeof(bool) + 4 * sizeof(char))
#define RCV_MSG_FSIZE     (2 * sizeof(int) + 1 * sizeof(short) + SID_SIZE + 3 * sizeof(bool) +\
		4 * sizeof(char)) + sizeof(IPaddr)

#define MSG_STATUS_FSIZE  (sizeof(api_msg_status))
#define NWN_FSIZE         (sizeof(api_nw_notification))
#define STATE_LT_FSIZE    (1 * sizeof(bool) + 1 * sizeof(char))
#define ROUTING_ST_FSIZE  (1 * sizeof(short) + 2 * sizeof(bool))


//typedef struct {
//
//   unsigned int       nd_size;
//   unsigned short     nslp_id;
//   unsigned char      sid[SID_SIZE];
//   bool               reliability;
//   bool               security;
//   bool               local_processing;
//   unsigned int       timeout;
//   unsigned char      ip_ttl;
//	char               *nslp_data;
//	msg_routing_info_base_t *mri;
//	char               *nslp_message_handle;
//	char               *source_s2_handle;
//	char               *peer_s2_handle;
//
//} api_send_msg;

typedef struct {
	unsigned int       nd_size;
	unsigned short     nslp_id;
	unsigned char      sid[SID_SIZE];
	bool               reliability;
	bool               security;
	bool               local_processing;
	bool               installRoutingState;
	unsigned int       timeout;
	unsigned char      ip_ttl;
	unsigned char 	   gist_hop_count;
	unsigned char      nslp_message_handle[MSGHANDLE_SIZE];
	IPaddr             sii_handle;
	char               *nslp_data;
	msg_routing_info_base_t *mri;

} api_send_msg;

//typedef struct {
//
//   unsigned int       nd_size;
//   unsigned short     nslp_id;
//   unsigned char      sid[SID_SIZE];
//   bool               reliability;
//   bool               security;
//   unsigned char      ip_ttl;
//   int                ip_distance;
//	char               *nslp_data;
//	msg_routing_info_base_t *mri;
//	char               *s2_handle;
//
//} api_recv_msg;



typedef struct {

	unsigned int            nd_size;
	unsigned short          nslp_id;
	unsigned char           sid[SID_SIZE];
	bool                    reliability;
	bool                    security;
	bool                    explicitlyRouted;
	bool                    noGistState;
	unsigned char           ip_ttl;
	unsigned int            ip_distance;
	unsigned int			gistHopCount;
	IPaddr                  sii_handle;
	char                    *nslp_data;
	msg_routing_info_base_t *mri;

} api_recv_msg;

typedef struct {
	bool          reliability;
	bool          security;
	unsigned char error_type;
	unsigned char nslp_message_handle[MSGHANDLE_SIZE];
} api_msg_status;

//typedef struct {
//
//	unsigned char      nwnt; //network_notification type
//	msg_routing_info_base_t *mri;
//
//} api_nw_notification;

typedef struct {

	unsigned char sid[16];
	NetworkNotificationType type;

} api_nw_notification;

typedef struct {

   bool               direction;
   unsigned char      state_lifetime;
	msg_routing_info_base_t *mri;

} api_state_lifetime;

typedef struct {

   unsigned short     nslp_id;
   bool               direction;
   bool               urgency;
	msg_routing_info_base_t *mri;

} api_routing_state;


#endif //API_DEF_H
