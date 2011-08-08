/***********************************************************************
 * copyright (c) 2005 Christian Dickmann, Andreas Westermaier          *
 *                                                                     *
 * This library is free software; you can redistribute it and/or       *
 * modify it under the terms of the GNU Lesser General Public          *
 * License as published by the Free Software Foundation; either        *
 * version 2.1 of the License, or (at your option) any later version.  *
 *                                                                     *
 * This library is distributed in the hope that it will be useful,     *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of      *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU   *
 * Lesser General Public License for more details.                     *
 *                                                                     *
 * You should have received a copy of the GNU Lesser General Public    *
 * License along with this library; if not, write to the Free Software *
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,          *
 * MA  02110-1301  USA                                                 *
 *                                                                     *
 * ------------------------------------------------------------------- *
 * project:  NSIS-Project                                              *
 * author :  Christian Dickmann  <nsis@christian-dickmann.de>          * 
 * author :  Andreas Westermaier <awester@fh-landshut.de>              *
 ***********************************************************************/


#ifndef GIST_MSG_CONSTANTS
#define GIST_MSG_CONSTANTS

#define GIST_MRI_TLV        0
#define GIST_SESSION_TLV    1
#define GIST_NETLAYER_TLV   2
#define GIST_STACK_TLV      3
#define GIST_STACKCONF_TLV  4
#define GIST_QCOOKIE_TLV    5
#define GIST_RCOOKIE_TLV    6
#define GIST_NAT_TLV        7
#define GIST_NSLP_DATA_TLV  8
#define GIST_ERROR_TLV      9

#define GIST_QUERY         0
#define GIST_RESPONSE      1
#define GIST_CONFIRM       2
#define GIST_DATA          3
#define GIST_ERROR         4
#define GIST_MAHELLO       5

#define GIST_DEFAULT_MRM    0
#define GIST_LOOSEEND_MRM   1

#define GIST_VERSION		1

#define GIST_STACK_MAXPROFILES 20

#define GIST_STACK_PROTO_TCP  1
#define GIST_STACK_PROTO_TLS  2
#define GIST_STACK_PROTO_SCTP 3

#define GIST_ERROR_INFORMATIONAL      0
#define GIST_ERROR_SUCCESS            1
#define GIST_ERROR_PROTOCOL           2
#define GIST_ERROR_TRANSIENT          3
#define GIST_ERROR_PERMANENT          4

#define GIST_ERRORCODE_COMMONHEADERPARSEERROR        1
#define GIST_ERRORCODE_HOPLIMITEXCEEDED              2
#define GIST_ERRORCODE_INCORRECTENCAPSULATION        3
#define GIST_ERRORCODE_INCORRECTLYDELIVEREDMSG       4
#define GIST_ERRORCODE_NOROUTINGSTATE                5
#define GIST_ERRORCODE_UNKNOWNNSLPID                 6
#define GIST_ERRORCODE_ENDPOINTFOUND                 7
#define GIST_ERRORCODE_MESSAGETOOLARGE               8
#define GIST_ERRORCODE_OBJECTTYPEERROR               9
#define GIST_ERRORCODE_OBJECTVALUEERROR             10
#define GIST_ERRORCODE_INVALIDIPTTL                 11
#define GIST_ERRORCODE_MRIVALIDATIONFAILED          12

#define GIST_ERROR_OBJTYPE_DUPLICATE         0
#define GIST_ERROR_OBJTYPE_UNRECOGNISED      1
#define GIST_ERROR_OBJTYPE_MISSING           2
#define GIST_ERROR_OBJTYPE_INVALID           3
#define GIST_ERROR_OBJTYPE_UNTRANSLATED      4
#define GIST_ERROR_OBJTYPE_INVALIDEXTFLAGS   5 

#define GIST_ERROR_OBJVALUE_INCORRECTLENGTH     0
#define GIST_ERROR_OBJVALUE_VALUENOTSUPPORTED   1
#define GIST_ERROR_OBJVALUE_INVALIDFLAG         2
#define GIST_ERROR_OBJVALUE_EMPTYLIST           3
#define GIST_ERROR_OBJVALUE_INVALIDCOOKIE       4
#define GIST_ERROR_OBJVALUE_SPSCPMISMATCH       5

#define GIST_MAGIC_NUMBER		(htonl(0x4e04bda5))   

#endif


#ifndef HEADERS_H
#define HEADERS_H

#ifndef WIN32
#include <sys/param.h>
#endif

/*
 * The GIST Common Header (A.1)
 * 
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |    Version    |  GIST hops    |        Message length         |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |   Signaling Application ID    |   Type        |S|R|E| Reserved|
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 */
   
struct common_header_t {
	unsigned char	version	:	8;
	unsigned char	hops	:	8;
	unsigned short	length	:	16;
	unsigned short	sappid	:	16;
	unsigned char	type	:	8;
#if BYTE_ORDER == LITTLE_ENDIAN	
	unsigned char	resrvd	:	5;
	unsigned char   e       :   1;
	unsigned char   r       :   1;
	unsigned char	s		:	1;
#else
#if BYTE_ORDER == BIG_ENDIAN
	unsigned char	s		:	1;
	unsigned char   r       :   1;
	unsigned char   e       :   1;
	unsigned char	resrvd	:	5;
#endif
#endif	
};


struct common_header_withMagicNumber_t {
	unsigned int magicNumber;
	struct common_header_t header;
};

/* Standard Object Header (A.2)
 * 
 *     0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 *    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *    |A|B|r|r|         Type          |r|r|r|r|        Length         |
 *    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 */


struct object_header_t{
	unsigned short left;
	unsigned short right;
};
 

#define GIST_OBJECT_HEADER_A(x)  ((ntohs(((object_header_t*)(x))->left) >> 15) & 1)
#define GIST_OBJECT_HEADER_B(x)  ((ntohs(((object_header_t*)(x))->left) >> 14) & 1)

#define SET_GIST_OBJECT_HEADER_A(x) (((object_header_t*)(x))->left) = htons(ntohs(((object_header_t*)(x))->left) | (1  << 15))
#define UNSET_GIST_OBJECT_HEADER_A(x) (((object_header_t*)(x))->left) = htons(ntohs(((object_header_t*)(x))->left) & (65535 ^ (1 << 15)));

#define SET_GIST_OBJECT_HEADER_B(x) (((object_header_t*)(x))->left) = htons(ntohs(((object_header_t*)(x))->left) | (1  << 14))
#define UNSET_GIST_OBJECT_HEADER_B(x) (((object_header_t*)(x))->left) = htons(ntohs(((object_header_t*)(x))->left) & (65535 ^ (1 << 14)));

#define GIST_OBJECT_HEADER_TYPE(x)  (ntohs(((object_header_t*)(x))->left) & 4095)
#define GIST_OBJECT_HEADER_LENGTH(x)  ((ntohs(((object_header_t*)(x))->right) & 4095) * 4 + sizeof(object_header_t))

#define SET_GIST_OBJECT_HEADER_LENGTH(x, y)  (((object_header_t*)(x))->right) = htons(((y - sizeof(object_header_t)) / 4) & 4095);
#define SET_GIST_OBJECT_HEADER_TYPE(x, y)  (((object_header_t*)(x))->left) = \
	htons((ntohs(((object_header_t*)(x))->left) & (3 << 14)) | (y & 4095));



/* Message-Routing-Information (A.3.1.1) - Path Coupled
 * 
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |     MRM-ID    |N|  Reserved   |IP-Ver |P|T|F|S|A|B|D|Reserved |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * //                       Source Address                        //
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * //                      Destination Address                    //
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * | Source Prefix |  Dest Prefix  |   Protocol    | DS-Field  |Rsv|
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * :       Reserved        |              Flow Label               :
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * :                              SPI                              :
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * :          Source Port          :       Destination Port        :
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 */
struct msg_routing_info_path_coupled_t {
	unsigned char msgRoutingMethod;
#if BYTE_ORDER == LITTLE_ENDIAN	
	unsigned char rsvd : 7;
	unsigned char n : 1;
	unsigned char s : 1;
	unsigned char f : 1;
	unsigned char t : 1;
	unsigned char p : 1;
	unsigned char ipVersion 		  : 4;
	unsigned char reserved : 5;
	unsigned char d : 1;
	unsigned char b : 1;
	unsigned char a : 1;
#else
#if BYTE_ORDER == BIG_ENDIAN	
	unsigned char n : 1;
	unsigned char rsvd : 7;
	unsigned char ipVersion 		  : 4;
	unsigned char p : 1;
	unsigned char t : 1;
	unsigned char f : 1;
	unsigned char s : 1;
	unsigned char a : 1;
	unsigned char b : 1;
	unsigned char d : 1;
	unsigned char reserved : 5;
#endif
#endif	

	union {
		struct {
			unsigned char addr[16];
		} v6;
		struct  {
			unsigned int addr;
			unsigned int rsrv[3];
		} v4;
	} srcAddr;

	union {
		struct {
			unsigned char addr[16];
		} v6;
		struct  {
			unsigned int addr;
			unsigned int rsrv[3];
		} v4;
	} destAddr;

	unsigned char srcPrefix : 8;
	unsigned char destPrefix : 8;
	unsigned char protocol : 8;

#if BYTE_ORDER == LITTLE_ENDIAN	
	unsigned char rsvd2 : 2;
	unsigned char diffserv_field : 6;
#else
#if BYTE_ORDER == BIG_ENDIAN	
	unsigned char diffserv_field : 6;
	unsigned char rsvd2 : 2;
#endif
#endif	

	unsigned int flowLabel;
	unsigned int spi;
	unsigned short srcPort;
	unsigned short destPort;
} ;


/* Message-Routing-Information (A.3.1.2) - LooseEnd
 * 
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |     MRM-ID    |N|  Reserved   |IP-Ver |D|      Reserved       |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * //                       Source Address                        //
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * //                      Destination Address                    //
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 */
struct msg_routing_info_loose_end_t {
	unsigned char msgRoutingMethod;
#if BYTE_ORDER == LITTLE_ENDIAN	
	unsigned char rsvd : 7;
	unsigned char n : 1;
	unsigned char reserved  : 3;
	unsigned char d         : 1;
	unsigned char ipVersion : 4;
#else
#if BYTE_ORDER == BIG_ENDIAN	
	unsigned char n : 1;
	unsigned char rsvd : 7;
	unsigned char ipVersion : 4;
	unsigned char d         : 1;
	unsigned char reserved  : 3;
#endif
#endif	
	unsigned char rsrv2     : 8;

	union {
		struct {
			unsigned char addr[16];
		} v6;
		struct  {
			unsigned int addr;
			unsigned int rsrv[3];
		} v4;
	} srcAddr;

	union {
		struct {
			unsigned char addr[16];
		} v6;
		struct  {
			unsigned int addr;
			unsigned int rsrv[3];
		} v4;
	} destAddr;
};

struct msg_routing_info_base_t {
	unsigned char msgRoutingMethod;
#if BYTE_ORDER == LITTLE_ENDIAN	
	unsigned char rsvd : 7;
	unsigned char n : 1;
	unsigned char reserved  : 4;
	unsigned char ipVersion : 4;
#else
#if BYTE_ORDER == BIG_ENDIAN	
	unsigned char n : 1;
	unsigned char rsvd : 7;
	unsigned char ipVersion : 4;
	unsigned char reserved  : 4;
#endif
#endif	
	unsigned char rsrv2     : 8;

	union {
		struct {
			unsigned char addr[16];
		} v6;
		struct  {
			unsigned int addr;
			unsigned int rsrv[3];
		} v4;
	} srcAddr;

	union {
		struct {
			unsigned char addr[16];
		} v6;
		struct  {
			unsigned int addr;
			unsigned int rsrv[3];
		} v4;
	} destAddr;
};

struct msg_routing_info_port_word_t
{
	unsigned short srcPort;
	unsigned short destPort;
};


/* Session Identification (A.3.2) */
typedef struct {
	unsigned char val[16];
} session_id_t;


/* Network-Layer-Information (A.3.3)
 * 
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * | PI-Length     |    IP-TTL     |IP-Ver |     Reserved          |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |                 Routing State Validity Time                   |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * //                       Peer Identity                         //
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * //                     Interface Address                       //
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 */
struct network_layer_info_t {
	unsigned char pi_len;
	unsigned char ip_ttl;
#if BYTE_ORDER == LITTLE_ENDIAN	
	unsigned char rsvd1   : 4;
	unsigned char ip_ver  : 4;
#else
#if BYTE_ORDER == BIG_ENDIAN	
	unsigned char ip_ver  : 4;
	unsigned char rsvd1   : 4;
#endif
#endif	
	unsigned char rsvd2;
	unsigned int  route_state_valid_time;
};


/* Stack Proposal (A.3.4)
 * 
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |  Prof-Count   |     Reserved                                  |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * //                    Profile 1                                //
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * :                                                               :
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * //                    Profile 2                                //
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 */
struct stack_proposal_t {
  unsigned char  profCnt  :  8;
  unsigned char  rsvd1;
  unsigned char  rsvd2;
  unsigned char  rsvd3;
};


/* Stack-Configuration-Data (A.3.5)
 * 
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |   MPO-Count   |       Reserved                                |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |                        MA-Hold-Time                           |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * //                   MA-protocol-options 1                     //
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * :                                                               :
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * //                   MA-protocol-options N                     //
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 */
struct stack_config_data_t {
	unsigned char hl_cnt;
	unsigned char rsvd1;
	unsigned char rsvd2;
	unsigned char rsvd3;
	unsigned int maholdtime;
};

/* NAT Traversa object (A.3.8)
 *       
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * | MRI-Length    | Type-Count    | NAT-Count     | Reserved      |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * //        Original Message-Routing-Information                 //
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 */
struct nat_traversal_t {
	unsigned char mri_length;
	unsigned char type_count;
	unsigned char nat_count;
	unsigned char rsvd;
};


/* Query Cookie (A.3.6) */
struct query_cookie_t {
  char *value;
};

/* Responder Cookie (A.3.7) */
struct responder_cookie_t {
  char *value;
};

/*
 * C.5.1	Error Object
 * 
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |   Error Class |                   Error Code                  |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * //             Optional error-specific information             //
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * 
 * OUTDATED figure! (see A.4.1)
 */

#define GIST_ERROR_GETCODE(x) ntohs(*((unsigned short*)((char*)x + 1)))
 
struct error_object_t {
	unsigned char errClass;
	unsigned char errCode1;
	unsigned char errCode2;
	unsigned char errSubCode;

#if BYTE_ORDER == LITTLE_ENDIAN	
	unsigned char rsrv1 : 3;
	unsigned char q : 1;
	unsigned char d : 1;
	unsigned char c : 1;
	unsigned char m : 1;
	unsigned char s : 1;
#else
#if BYTE_ORDER == BIG_ENDIAN	
	unsigned char s : 1;
	unsigned char m : 1;
	unsigned char c : 1;
	unsigned char d : 1;
	unsigned char q : 1;
	unsigned char rsrv1 : 3;
#endif
#endif	
	unsigned char rsrv2;
	unsigned char mriLength;
	unsigned char infoCount;
	unsigned int commonHeader[2];
};

#endif // HEADERS_H
