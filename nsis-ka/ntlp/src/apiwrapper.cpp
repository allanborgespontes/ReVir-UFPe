/// ----------------------------------------*- mode: C++; -*--
/// @file apiwrapper.cpp
/// GIST API wrapper - maps internal API to Unix Domain Socket API
/// ----------------------------------------------------------
/// $Id: apiwrapper.cpp 6282 2011-06-17 13:31:57Z bless $
/// $HeadURL: https://svn.tm.kit.edu/nsis/ntlp/trunk/src/apiwrapper.cpp $
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
#include "apiwrapper.h"
#include "queuemanager.h"
#include "logfile.h"
#include "ntlp_proto.h"
#include <net/if.h>
#include <cerrno>
#include <iomanip>
#include <sstream>

#include "mri_le.h"

#include "mri_le.h"

namespace ntlp {
    using namespace protlib;




/***** struct SignalingParam *****/

    APIWrapperParam::APIWrapperParam(TP* proto,
				     const message::qaddr_t clientqueue,
				     AddressList &addresses,
				     bool instantestablish,
				     uint32 sleep_time) 
	: ThreadParam(sleep_time,"GIST Sock API"),
	  proto(proto),
	  clientqueue(clientqueue),
	  instantestablish(instantestablish),
	  addresses(addresses)
	
    {
    } // end constructor


    
//@}


const char* const APIWrapper::errstr[] = {
	"success"	
	
}; // end errstr

const char* APIWrapper::get_error_string(error_t e) { return errstr[e]; }

/** Initialize Thread base class and GIST protocol. */
APIWrapper::APIWrapper(const APIWrapperParam& p)
  : Thread(p),
    param(p)
{
  // register queue
  
  QueueManager::instance()->register_queue(get_fqueue(), message::qaddr_api_0);
  Log(DEBUG_LOG,LOG_NORMAL, p.name, "Created APIWrapper");
	
} // end constructor

APIWrapper::~APIWrapper() {
  Log(DEBUG_LOG,LOG_NORMAL, param.name, "Destroying APIWrapper");
  QueueManager::instance()->unregister_queue(message::qaddr_api_0);

} // end destructor

void APIWrapper::main_loop(uint32 nr) {
  Log(DEBUG_LOG,LOG_NORMAL, param.name, "Starting thread #" << nr);
	
  if (param.instantestablish) DLog(param.name, color[red] << "Attention: Running in backwards-compatible 'InstantEstablish mode' - see manual");


  // process queue of internal messages
  process_queue();

  Log(DEBUG_LOG,LOG_NORMAL, param.name, "Thread #" << nr << " stopped");
} // end main_loop



/**
 * process queue of internal messages for the signaling module
 * (usually message::qaddr_signaling) 
 */
void 
APIWrapper::process_queue() 
{
  uint32 wait = param.sleep_time*1000; // max. waiting time at internal msg queue
  message* msg = NULL;
  APIMsg* apimsg = NULL;
  TPMsg* tpmsg = NULL;

  // get queue (param.source is usually message::qaddr_signaling) 
  FastQueue* fq = QueueManager::instance()->get_queue(message::qaddr_api_0);
  if (!fq)
  {
    Log(ERROR_LOG,LOG_ALERT, param.name, "Cannot find input msg queue");
    
    return;
  } // end if not fq

  
  /// wait for messages if in state running
  while (get_state()==STATE_RUN) 
  {
    // dequeue message from internal message queue
    msg= fq->dequeue_timedwait(wait);
    if (msg) 
    {
	Log(DEBUG_LOG,LOG_NORMAL, param.name, "Received incoming message on Queue");
	switch( msg->get_type() )
      {
	// message from transport
	case message::type_transport: 
	  {
	      tpmsg = dynamic_cast<TPMsg*>(msg);
	      if (tpmsg)
	      {
		  // process incoming message from TP module (network)
		  process_tp_msg(tpmsg);
		  Log(DEBUG_LOG,LOG_NORMAL, param.name, color[red] << "Received incoming message from TP Module" << color[off]);
	      }
	      else 
	      {
		  Log(ERROR_LOG,LOG_ALERT, param.name, "Cannot cast message from source " << msg->get_qaddr_name() << " of type " << msg->get_type_name() << " to TPMsg");
		  
		  delete msg;
	      } // end if tpmsg
	  } break;

	// message from coordinator
	case message::type_API:
	  {
	      apimsg = dynamic_cast<APIMsg*>(msg);
	    if (apimsg) 
	    {
	      // process message from coordinator to TP
	      // (outgoing message to network)
	      process_api_msg(apimsg);
	    }
	    else 
	    {
	      Log(ERROR_LOG,LOG_ALERT, param.name, "Cannot cast message from source " << msg->get_qaddr_name() << " of type " << msg->get_type_name() << " to APIWrapperMsg");
	      delete msg;
	    } // end if sigmsg
	  } break;
	
	default:
	  {
	    Log(ERROR_LOG,LOG_NORMAL, param.name, "Received a message from " << msg->get_qaddr_name() << " of type " << msg->get_type_name() << " that cannot be processed here");
	    
	    delete msg;
	  }

	  //clean up, messages directed to US are disposable
	  if (msg) delete msg;
      } // end switch
    } // end if msg
  } // end while running
} // end process_queue





/** process incoming message from TP module
  * (incoming signaling message from network)
  * @param tpmsg Internal TPMsg from TP module
  */
void 
APIWrapper::process_tp_msg(TPMsg* tpmsg) 
{

    //if the API Call could be decoded, this is the message sent to coordinator.
    APIMsg* apimsg = NULL;

    assert(tpmsg!=NULL);
    
    Log(DEBUG_LOG,LOG_NORMAL,param.name,"process_tp_msg() - received message #"<< tpmsg->get_id() <<" from TP");
    
    // remember netmsg and peer from TPmsg
    NetMsg* netmsg= tpmsg->get_message();
    // peer may be updated by process_response()
    udsaddress* peer= 0;
    if (tpmsg->get_peeraddress())
    { // this will copy the peer address
	peer= dynamic_cast<udsaddress*>(tpmsg->get_peeraddress()->copy());
    }
    else
    {
	// peer address not set, create empty address
	peer= new udsaddress();
    }

    // Process here the NetMsg contents. Afterwards, create a APIMsg and send to clientqueue

    uint8 type = netmsg->decode8();

    Log(EVENT_LOG, LOG_NORMAL, param.name, "Received an incoming API call from peer: " << *peer << " - translating");
    
    switch (type) {

	case SEND_MESSAGE: 
	    DLog(param.name, "Received: " << color[red] << "SEND_MESSAGE" << color[off]);
	    apimsg=decode_netmsg(netmsg);
	    break;
	case RECV_MESSAGE:
	    DLog(param.name, "Unimplemented: " << color[red] << "RECV_MESSAGE" << color[off]);
	    break;
	case MESSAGE_STATUS:
	    DLog(param.name, "Unimplemented: " << color[red] << "MESSAGE_STATUS" << color[off]);
	    break;
	case NW_NOTIFICATION:
	    DLog(param.name, "Unimplemented: " << color[red] << "NW_NOTIFICATION" << color[off]);
	    break;
	case SET_STATE_LIFETIME:
	    DLog(param.name, "Unimplemented: " << color[red] << "SET_STATE_LIFETIME" << color[off]);
	    break;
	case INV_ROUTING_STATE:
	    DLog(param.name, "Unimplemented: " << color[red] << "INV_ROUTINGSTATE" << color[off]);
	    break;
	case RECEIVE_LOCAL:
	    DLog(param.name, "Unimplemented: " << color[red] << "RECEIVE_LOCAL" << color[off]);
	    break;
	case TIMER_TIMEOUT:
	    DLog(param.name, "Unimplemented: " << color[red] << "TIMER_TIMEOUT" << color[off]);
	    break;
	case REQUEST_LOCAL_IP:
	    DLog(param.name, "Received: " << color[red] << "REQUEST_LOCAL_IP" << color[off]);
	    send_local_ip(netmsg, *peer);
	    break;
	case SEND_LOCAL_IP:
	    DLog(param.name, "Unimplemented: " << color[red] << "SEND_LOCAL_IP" << color[off]);
	    break;
	case TEST_RECV_MSG:
	    DLog(param.name, "Unimplemented: " << color[red] << "TEST_RECV_MSG" << color[off]);
	    break;
	case TEST_MSG_STATUS:
	    DLog(param.name, "Unimplemented: " << color[red] << "TEST_MSG_STATUS" << color[off]);
	    break;
	case TEST_NWN:
	    DLog(param.name, "Unimplemented: " << color[red] << "TEST_NWN" << color[off]);
	    break;
	case REGISTER_NSLPID:
	    DLog(param.name, "Received: " << color[red] << "REGISTER_NSLPID" << color[off]);
	    apimsg = decode_netmsg(netmsg);
	    break;
	case UNREGISTER_NSLPID:
	    DLog(param.name, "Unimplemented: " << color[red] << "UNREGISTER_NSLPID" << color[off]);
	    break;
	default: DLog(param.name, "Something went wrong. Maybe type not correctly decoded?");
    }



    // because pointers are cleared, objects peer and netmsg 
    // are not deleted by calling tpmsg destructor now
    // delete netmsg;
    // tpmsg->clear_pointers();
    delete tpmsg;

    // decoding went fine, if apimsg is filled
    if (apimsg) {
	DLog(param.name, "Saving socket address" << *peer << " for NSLPID " << apimsg->get_nslpid());
	//enter peer and NSLPID to registry for later answers
	registry[apimsg->get_nslpid()] = *peer;
	//peer is copied into registry, delete the original one
	delete peer;
	apimsg->set_source(message::qaddr_api_0);
	apimsg->send_to(param.clientqueue);
 
   
    }
        
    Log(DEBUG_LOG,LOG_NORMAL,param.name,"process_tp_msg() - done.");


} // end process_tp_msg


/**
 * process message from coordinator
 * (outgoing signaling message, sent subsequently to TP module)
 */
void 
APIWrapper::process_api_msg(APIMsg* apimsg) 
{
    udsaddress* peer;

    Log(EVENT_LOG, LOG_NORMAL, param.name, "Received an APIMsg to translate and deliver to connected clients");

    hashmap_t<uint32, udsaddress>::iterator hm_iter;

    hm_iter=registry.find(apimsg->get_nslpid());

    if (hm_iter == registry.end()) {
	Log(ERROR_LOG, LOG_NORMAL, param.name, "This NSLPID ("<< apimsg->get_nslpid()  << ") is unknown, I have no client who is using it. Dropping.");	
	delete apimsg;
	return;
    } else {
	DLog(param.name, "Address looked up: " << (*hm_iter).second);
	peer = new udsaddress((*hm_iter).second);
    }

    assert(peer != NULL);

    if (param.instantestablish) {
	if (apimsg->get_subtype() == APIMsg::RecvMessage) {
	    if (apimsg->get_adjacency_check()) {
		Log(EVENT_LOG, LOG_NORMAL, param.name, "Running in compatibility mode. Requesting backwards path setup for client");
		APIMsg* answer = new APIMsg();
		answer->set_source(message::qaddr_api_0);
		answer->set_recvmessageanswer(apimsg->get_nslpid(), apimsg->get_sessionid()->copy(), apimsg->get_mri()->copy(), NULL, APIMsg::directive_establish);
		
		answer->send_to(param.clientqueue);

	    }
	}
    }

    NetMsg* netmsg = generate_netmsg(apimsg);

    if (peer) {

	param.proto->send(netmsg, *peer); 
	Log(EVENT_LOG, LOG_NORMAL, param.name, "Delivering translated message to client at address " << *peer);
    } else {
	Log(EVENT_LOG, LOG_NORMAL, param.name, "Something went wrong, the message could not be translated.");
    }

} // end process_sig_msg



APIWrapper::error_t APIWrapper::send_tp(NetMsg* msg, const udsaddress& addr, uint16 oif) {
	error_t res = error_ok; 
	if (param.proto) {
		// send message
		try {
		  Log(EVENT_LOG,LOG_NORMAL,param.name,"Sending msg to Protocol #" << addr);

		  if (oif) DLog(param.name, "send_tp: Request to use specific interface received, index: " << oif);

		  // take a copy of the address for putting into TPMsg
		  udsaddress* tmp = addr.copy();
		  

		  // the following call should only return after the message has been sent (synchronous call)
		  // or the TP must ensure that it works on copies of both parameters since they might get freed
		  // soon after this method returns...
		  
		  param.proto->send(msg,*(tmp));

		} catch(TPError& e) {
			switch (e.errtype) {
				case TPError::TPERR_BAD_ADDRESS: res = error_bad_address; break;
				case TPError::TPERR_BAD_NETMSG: res = error_internal; break;
				case TPError::TPERR_UNREACHABLE: res = error_unreachable; break;
				case TPError::TPERR_PAYLOAD: res = error_max_payload; break;
				default: res = error_tp_internal; break;
			}
		} catch(...) {
			res = error_tp_internal;
		} // end try-catch
	} 
	else
	{
	  Log(ERROR_LOG,LOG_NORMAL,param.name,"Protocol #" << addr << ") not found");
	  res = error_no_tp;
	}
	return res;
} // end send_tp


void convertKaMriToGoe(unsigned char mrm, mri * kamri, NetMsg* result) {
	if (mrm == GIST_DEFAULT_MRM) {
	    msg_routing_info_path_coupled_t mr_pc;
	    mri_pathcoupled* mymri = dynamic_cast<mri_pathcoupled*>(kamri);
	    assert (mymri != NULL);
	
	    //mymri->print(cout, 0, 0,"MRI to translate:");
	
	    //Now FILL it
	
	    mr_pc.msgRoutingMethod = GIST_DEFAULT_MRM;
	    mr_pc.ipVersion = mymri->get_ip_version();
		    
	    uint8 flags = mymri ->get_flags();
	    
	    if (flags & mri_pathcoupled::IP_protocol) mr_pc.p = 1; else mr_pc.p = 0; 
	    if (flags & mri_pathcoupled::DS_Field) mr_pc.t = 1; else mr_pc.t = 0; 
	    if (flags & mri_pathcoupled::Flow_Label) mr_pc.f = 1; else mr_pc.f = 0; 
	    if (flags & mri_pathcoupled::SPI) mr_pc.s = 1; else mr_pc.s = 0; 
	    if (flags & mri_pathcoupled::Source_Port) mr_pc.a = 1; else mr_pc.a = 0; 
	    if (flags & mri_pathcoupled::Destination_Port) mr_pc.b = 1; else mr_pc.b = 0; 
	    if (flags & mri_pathcoupled::Direction) mr_pc.d = 1; else mr_pc.d = 0; 
		    
	    mr_pc.reserved = 0;
	 
	    if (mr_pc.ipVersion == 4) {
			in_addr tmp;
			mymri->get_sourceaddress().get_ip(tmp);
			memcpy(&mr_pc.srcAddr.v4.addr, &tmp, 4);
			mymri->get_destaddress().get_ip(tmp);
			memcpy(&mr_pc.destAddr.v4.addr, &tmp, 4);
			memset(&mr_pc.srcAddr.v4.rsrv, 0, 12);
			memset(&mr_pc.destAddr.v4.rsrv, 0, 12);
	    } else {
			in6_addr tmp;
			mymri->get_sourceaddress().get_ip(tmp);
			memcpy(&mr_pc.srcAddr.v6.addr, &tmp, 16);
			mymri->get_destaddress().get_ip(tmp);
			memcpy(&mr_pc.destAddr.v6.addr, &tmp, 16);
	    }

	    mr_pc.srcPrefix = mymri -> get_sourceprefix();
	    mr_pc.destPrefix = mymri -> get_destprefix();
	    mr_pc.protocol = mymri ->get_protocol();
	    // TODO: traffic_class is outdated! We already use the new DS-field (christian.dickmann)
	    mr_pc.diffserv_field = mymri->get_ds_field();
	    mr_pc.flowLabel = mymri->get_flowlabel();
	    mr_pc.spi = mymri->get_spi();
	    mr_pc.srcPort = mymri->get_sourceport();
	    mr_pc.destPort = mymri->get_destport();

		    
	    //Now ENCODE it
	    result->encode((uchar*) &mr_pc, sizeof(msg_routing_info_path_coupled_t));
	}
	
	if (mrm == GIST_LOOSEEND_MRM) {
	    msg_routing_info_loose_end_t mr_le;
	    mri_looseend* mymri = dynamic_cast<mri_looseend*>(kamri);
	    assert (mymri != NULL);
	    //Now FILL it
		    
	    mr_le.msgRoutingMethod = GIST_LOOSEEND_MRM;
		    
	    if (mymri->get_flags() >> 15) mr_le.d = 1; else mr_le.d =0;
	    mr_le.ipVersion = mymri->get_ip_version();
	    
	    if (mr_le.ipVersion == 4) {
			in_addr tmp;
			mymri->get_sourceaddress().get_ip(tmp);
			memcpy(&mr_le.srcAddr.v4.addr, &tmp, 4);
			mymri->get_destaddress().get_ip(tmp);
			memcpy(&mr_le.destAddr.v4.addr, &tmp, 4);
			memset(&mr_le.srcAddr.v4.rsrv, 0, 12);
			memset(&mr_le.destAddr.v4.rsrv, 0, 12);
	    } else {
			in6_addr tmp;
			mymri->get_sourceaddress().get_ip(tmp);
			memcpy(&mr_le.srcAddr.v6.addr, &tmp, 16);
			mymri->get_destaddress().get_ip(tmp);
			memcpy(&mr_le.destAddr.v6.addr, &tmp, 16);
	    }

	
	    //Now ENCODE it
	    result->encode((uchar*) &mr_le, sizeof(msg_routing_info_loose_end_t));
	}
	
}


/**
 * Static Member function providing translation of GISTka internal APIMsg to
 * a NetMsg containing the correct bit format defined by the implementation
 * of University of Goettingen. Headers of goettingen used according to GPL.
 *
 * Note that in case of unsupported properties, this method generates feasible defaults
 */
/// encode a APIMsg using Goettingen's API definitions into a NetMsg
NetMsg* APIWrapper::generate_netmsg(APIMsg* apimsg) {


    //******************************** SendMessage ********************************


    if (apimsg->get_subtype() ==APIMsg::SendMessage) {
	    
		// generate a SendMessage according to the API.
	
		if (!apimsg->get_data()) {
		    DLog("API encoding", "No NSLPDATA for SendMessage. Weird. Bailing out.");
		    return NULL;
		}
	
		if (!apimsg->get_mri()) {
		    DLog("API encoding", "No MRI for SendMessage. Weird. Bailing out.");
		    return NULL;
		}
		
		api_send_msg s;
		s.nd_size = apimsg->get_data()->get_size();
		s.nslp_id = apimsg->get_nslpid();
		
		// fill SessionID
		uint32 a,b,c,d;
		sessionid* mysid = apimsg->get_sessionid();
		mysid->get_sessionid(a,b,c,d);
	
		uchar* buf = new uchar[16];
		memset(buf, 0, 16);
		
                uint32 *bufint= reinterpret_cast<uint32 *>(buf);
		*bufint++= ntohl(a);
		*bufint++= ntohl(b);
		*bufint++= ntohl(c);
		*bufint++= ntohl(d);
		
		memcpy(&(s.sid), buf, 16);
	
		//for (int i=0; i<16; i++) {
		//    cout << hex << (int) s.sid[i] << " ";
		//}
		
		s.reliability=apimsg->get_tx_attr().reliable;
		s.security=apimsg->get_tx_attr().secure;
		s.local_processing=0;
		s.timeout=apimsg->get_timeout();
		s.ip_ttl=apimsg->get_ip_ttl();
		s.nslp_data = NULL;
		s.mri= NULL;
		
		// conversion is not obvious
#ifdef MSG_HANDLE_GOETTINGEN
		for (int i = 0; i < 16; i++) {
			msghandle_t handle = apimsg->get_nslpmsghandle();
			s.nslp_message_handle[i] = handle.handle[i];
		} 
#else 
		*((unsigned int*)s.nslp_message_handle) = apimsg->get_nslpmsghandle(); 
		*((unsigned int*)s.nslp_message_handle + 1) = apimsg->get_nslpmsghandle(); 
		*((unsigned int*)s.nslp_message_handle + 2) = apimsg->get_nslpmsghandle(); 
		*((unsigned int*)s.nslp_message_handle + 3) = apimsg->get_nslpmsghandle(); 
#endif
	
		// get MRI sizes
		
		int mrm = apimsg->get_mri()->get_mrm();
		int mrilen = 0;
	
	
		if (mrm == GIST_DEFAULT_MRM) {
		    mrilen = sizeof(msg_routing_info_path_coupled_t);
		} else {
		    mrilen = sizeof(msg_routing_info_loose_end_t);
		}
			
		// encode header, length and api_send_msg struct
	
		//uint32 
	
		NetMsg* result = new NetMsg(api_header_length + SND_MSG_FSIZE + mrilen + s.nd_size);
		result->encode8(SEND_MESSAGE);
		uint32 tmp = SND_MSG_FSIZE + mrilen + s.nd_size;
		result->encode((uchar*) &tmp, 4);
		result->encode((uchar*) &s, SND_MSG_FSIZE);
	
		// encode NSLPDATA
	
		result->encode(apimsg->get_data()->get_buffer(), s.nd_size);
	
		// encode the appropriate MRI
		convertKaMriToGoe(mrm, apimsg->get_mri(), result);
	
		// This should be it. More objects of SendMessage (MessageHandle, SII-Handle) are currently NOT SUPPORTED by Goettingen's API.
	
		//ostringstream os;
		//result->hexdump(os);
		//DLog("API encoding", "Output: " << os.str());
		return result;

    } //end if SendMessage



    //*************************************** RecvMessage *************************************************+

    // mydata, s.nslp_id, sid, mymri, false, 0, tx, s.ip_ttl, s.ip_distance, 20

    if (apimsg->get_subtype() ==APIMsg::RecvMessage) {
    
		//generate a SendMessage according to the API.
	
		if (!apimsg->get_data()) {
		    DLog("API encoding", "No NSLPDATA for SendMessage. Weird. Bailing out.");
		    return NULL;
		}
	
		if (!apimsg->get_mri()) {
		    DLog("API encoding", "No MRI for SendMessage. Weird. Bailing out.");
		    return NULL;
		}
		
		api_recv_msg r;
		r.nd_size = apimsg->get_data()->get_size();
		r.nslp_id = apimsg->get_nslpid();
		
		//fill SessionID
		uint32 a,b,c,d;
		sessionid* mysid = apimsg->get_sessionid();
		mysid->get_sessionid(a,b,c,d);
	
		uchar* buf = new uchar[16];
		memset(buf, 0, 16);
                uint32 *bufint= reinterpret_cast<uint32 *>(buf);
		*bufint++= htonl(a);
		*bufint++= htonl(b);
		*bufint++= htonl(c);
		*bufint++= htonl(d);
		
		memcpy(&(r.sid), buf, 16);
	
		//for (int i=0; i<16; i++) {
		//    cout << hex << (int) r.sid[i] << " ";
		//}
		
		r.reliability=apimsg->get_tx_attr().reliable;
		r.security=apimsg->get_tx_attr().secure;
		r.ip_ttl=apimsg->get_ip_ttl();
		r.ip_distance=apimsg->get_ip_distance();
		r.gistHopCount = apimsg->get_ghc();
		r.nslp_data = NULL;
		r.mri= NULL;
	
		// get MRI sizes
		
		int mrm = apimsg->get_mri()->get_mrm();
		int mrilen = 0;
	
	
		if (mrm == GIST_DEFAULT_MRM) {
		    mrilen = sizeof(msg_routing_info_path_coupled_t);
		} else {
		    mrilen = sizeof(msg_routing_info_loose_end_t);
		}
			
		// encode header, length and api_send_msg struct
	
		//uint32 
	
		NetMsg* result = new NetMsg(api_header_length + RCV_MSG_FSIZE + mrilen + r.nd_size);
		result->encode8(RECV_MESSAGE);
		uint32 tmp = RCV_MSG_FSIZE + mrilen + r.nd_size;
		result->encode((uchar*) &tmp, 4);
		result->encode((uchar*) &r, RCV_MSG_FSIZE);
	
		// encode NSLPDATA
	
		if (r.nd_size) result->encode(apimsg->get_data()->get_buffer(), r.nd_size);
	
		// encode the appropriate MRI
		convertKaMriToGoe(mrm, apimsg->get_mri(), result);
			
		//This should be it. More objects of SendMessage (MessageHandle, SII-Handle) are currently NOT SUPPORTED by Goettingen's API.
	
		//ostringstream os;
		//result->hexdump(os);
		//DLog("API encoding", "Output: " << os.str());
		return result;

    } //end if RecvMessage



    //******************************** NetworkNotification ************************

    if (apimsg->get_subtype() == APIMsg::NetworkNotification) {

		// get MRI sizes
		
		int mrm = apimsg->get_mri()->get_mrm();
		int mrilen = 0;
	
	
		if (mrm == GIST_DEFAULT_MRM) {
		    mrilen = sizeof(msg_routing_info_path_coupled_t);
		} else {
		    mrilen = sizeof(msg_routing_info_loose_end_t);
		}
	
	
		NetMsg* result = new NetMsg(api_header_length + NWN_FSIZE);
		result->encode8(NW_NOTIFICATION);
		uint32 tmp = NWN_FSIZE;
		result->encode((uchar*) &tmp, 4);
	
		// encode NWN members: Only a CHAR!
		
		api_nw_notification n;
		n.type = (NetworkNotificationType)0;
		if (apimsg->get_error() == APIMsg::last_node) {
			n.type = LastNode;
		}

		uint32 a,b,c,d;
		sessionid* mysid = apimsg->get_sessionid();
		mysid->get_sessionid(a,b,c,d);
	
		uchar* buf = new uchar[16];
		memset(buf, 0, 16);

                uint32 *bufint= reinterpret_cast<uint32 *>(buf);
		*bufint++= htonl(a);
		*bufint++= htonl(b);
		*bufint++= htonl(c);
		*bufint++= htonl(d);
		
		memcpy(&(n.sid), buf, 16);
		
		
		if (n.type == (NetworkNotificationType)0) {
			result->encode((uchar*) &n, NWN_FSIZE);
	
			return result;
		}
	
		// encode MRI
		
		// encode the appropriate MRI
		//convertKaMriToGoe(mrm, apimsg->get_mri(), result);
		
		return NULL;
		//ostringstream os;
		//result->hexdump(os);
		//DLog("API encoding", "Output: " << os.str());
    }

    //******************************** MessageStatus ************************

    if (apimsg->get_subtype() == APIMsg::MessageStatus) {

		// get MRI sizes
		
		int mrm = apimsg->get_mri()->get_mrm();
		int mrilen = 0;
	
	
		if (mrm == GIST_DEFAULT_MRM) {
		    mrilen = sizeof(msg_routing_info_path_coupled_t);
		} else {
		    mrilen = sizeof(msg_routing_info_loose_end_t);
		}
	
	
		NetMsg* result = new NetMsg(api_header_length + MSG_STATUS_FSIZE);
		result->encode8(NW_NOTIFICATION);
		uint32 tmp = MSG_STATUS_FSIZE;
		result->encode((uchar*) &tmp, 4);
	
		bool convertable = false;
	
		api_msg_status s;
		s.reliability = apimsg->get_tx_attr().reliable;
		s.security = apimsg->get_tx_attr().secure;
		
		// TODO: Conversion of message handle is missing
		
		s.error_type = 0;
		if (apimsg->get_error() == APIMsg::error_msg_node_not_found) {
			s.error_type = messagestatus_unabletoestablishroutingstate;
			convertable = true;
		}
		
		if (convertable) {
			result->encode((uchar*) &s, MSG_STATUS_FSIZE);
	
			return result;
		}
	
		// encode MRI
		
		// encode the appropriate MRI
		//convertKaMriToGoe(mrm, apimsg->get_mri(), result);
		
		return NULL;
		//ostringstream os;
		//result->hexdump(os);
		//DLog("API encoding", "Output: " << os.str());
    }

    //******************************** Register ***********************************

    if (apimsg->get_subtype() ==APIMsg::Register) {
    
		// generate a RegisterMessage according to the API.
		
		// does NOT provide RAO value, the "GetLooseEnd" is unsupported by GISTka, it will always deliver LooseEnd MRI messages
	
		NetMsg* result = new NetMsg(api_header_length);
		result->encode8(REGISTER_NSLPID);
		uint32 tmp = apimsg->get_nslpid() | (1 << 16);
		result->encode((uchar*) &tmp, 4);
		
		return result;

    } //end if Register




    return NULL;

}


mri * convertGoeMriToKa(unsigned char mrm, const msg_routing_info_base_t* goemri) 
{
    mri* mymri = NULL;
    hostaddress* sourceaddr = NULL; 
    hostaddress* destaddr = NULL;

    sourceaddr = new hostaddress();
    destaddr = new hostaddress();
    
    if (mrm == GIST_DEFAULT_MRM) {

		const msg_routing_info_path_coupled_t* mr = reinterpret_cast<const msg_routing_info_path_coupled_t*>(goemri);
		
		//DLog("API Decode","convertGoeMriToKa -" 
		//<< "Decoded values:"
		//     << " IP Version: " << (int) mr->ipVersion 
		//     << " srcPrefix: " << (int) mr->srcPrefix 
		//     << " destPrefix: " << (int) mr->destPrefix 
		//     << " srcPort: " << (int) mr->srcPort 
		//     << " destPort: " << (int) mr->destPort);
	
		if (mr->ipVersion == 4) {
		    in_addr tmp;
		    memcpy(&tmp, &mr->srcAddr.v4.addr, 4);
		    sourceaddr->set_ip(tmp);
		    memcpy(&tmp, &mr->destAddr.v4.addr, 4);
		    destaddr->set_ip(tmp);   
		} else {
		    in6_addr tmp;
		    memcpy(&tmp, &mr->srcAddr.v6.addr, 16);
		    sourceaddr->set_ip(tmp);
		    memcpy(&tmp, &mr->destAddr.v6.addr, 16);
		    destaddr->set_ip(tmp);
		}
	
	
		// filling our MRI
	    // TODO: traffic_class is outdated! We already use the new DS-field (christian.dickmann)
		mymri = new mri_pathcoupled(*sourceaddr, mr->srcPrefix, mr->srcPort, *destaddr, mr->destPrefix, mr->destPort, 
			mr->protocol, mr->flowLabel, 0 /* mr->traffic_class */, mr->spi, !(mr->d));
		
    } else {

		const msg_routing_info_loose_end_t* mr = reinterpret_cast<const msg_routing_info_loose_end_t*>(goemri);
			
	       
		if (mr->ipVersion == 4) {
		    in_addr tmp;
		    memcpy(&tmp, &mr->srcAddr.v4.addr, 4);
		    sourceaddr->set_ip(tmp);
		    memcpy(&tmp, &mr->destAddr.v4.addr, 4);
		    destaddr->set_ip(tmp);   
		} else {
		    in6_addr tmp;
		    memcpy(&tmp, &mr->srcAddr.v6.addr, 16);
		    sourceaddr->set_ip(tmp);
		    memcpy(&tmp, &mr->destAddr.v6.addr, 16);
		    destaddr->set_ip(tmp);
		}
	
	
	
		// Construct the MRI
		mymri = new mri_looseend(*sourceaddr, *destaddr, !(mr->d));

    }

    delete sourceaddr;
    delete destaddr;

	return mymri;
}

/**
 * Static Member function providing translation of a NetMsg containing API messages
 * defined by the implementation of University of Goettingen into GISTka internal
 * APIMsg.
 *
 * Note, that in case of unsupported properties, this method generates a feasible default value
 */    
APIMsg* APIWrapper::decode_netmsg(NetMsg* m) {

    //cout << "Decoding following buffer:" << endl;
    //m->hexdump(cout, 0, 256);

    APIMsg* result = new APIMsg;
    // reset to beginning (just in case...)
    m->to_start();
    uint8 type = m->decode8();
    uint32 totallen = *((const uint32*) (m->get_buffer() + 1));
    //m->set_pos_r(5);
    DLog("API decoding", "Infofield: (length OR NSLPID) " << totallen);

    // do the decoding here


    mri* mymri = NULL;
    nslpdata* mydata = NULL;
    sessionid* sid = NULL;

   
    hostaddress* sourceaddr = NULL; 
    hostaddress* destaddr = NULL;


    
    if (type == SEND_MESSAGE) {

    //******************************** SendMessage ********************************
	    m->decode((uchar*) &totallen, 4);
	    api_send_msg s;
	    m->decode((uchar*) &s, SND_MSG_FSIZE);
	    
	    //DLog("API decoding", "after reading api_send_msg @" << m->get_pos() <<  " total len= " << totallen << " NSLP data len: " << s.nd_size << " NSLP ID: " << s.nslp_id << " SND_MSG_FSIZE=" << SND_MSG_FSIZE);
	    //cout << "SessionID:        ";

	    uint32 a;
	    uint32 b;
	    uint32 c;
	    uint32 d;
	    uint16 mrm;
	    
	    a = ntohl( *((const uint32*) &(s.sid[0])) );
	    b = ntohl( *((const uint32*) &(s.sid[4])) );
	    c = ntohl( *((const uint32*) &(s.sid[8])) );
	    d = ntohl( *((const uint32*) &(s.sid[12])) );

	    //cout << a << " " << b << " " << c << " " << d <<endl;

	    //cout << "Reliable:         " << s.reliability      << endl;
            //cout << "Secure:           " << s.security         << endl;
	    //cout << "Local Processing: " << s.local_processing << endl;
	    //cout << "Timeout:          " << s.timeout          << endl;
	    //cout << "IP TTL:           " << (int) s.ip_ttl     << endl;

	    //DLog("API decoding", "before reading NSLP data get_pos()= " << m->get_pos());
	    // read NSLP data
	    mydata = new nslpdata(m->get_buffer()+m->get_pos(), s.nd_size);
	    // advance buffer pointer
	    m->set_pos_r(s.nd_size);

	    // Fill Goettingen's MRI structure
	    //DLog("API decoding", "get_pos()= " << m->get_pos());

	    mrm = (reinterpret_cast<msg_routing_info_base_t*>(m->get_buffer()+m->get_pos()))->msgRoutingMethod;
	    //DLog("API decoding", "mrm= " << mrm << " new mrm @" << (void *) (reinterpret_cast<msg_routing_info_base_t*>(m->get_buffer()+m->get_pos())));
	    int mriLength= 0;
	    if (mrm == GIST_DEFAULT_MRM) {
			mriLength = sizeof(msg_routing_info_path_coupled_t);
	    } else if (mrm == GIST_LOOSEEND_MRM) {
			mriLength = sizeof(msg_routing_info_loose_end_t);
	    } else {
			ERRLog("API decoding","Unknown MRM - this will not be processed");
			delete mydata;
			return NULL;
	    };
	    
	    s.mri = reinterpret_cast<msg_routing_info_base_t*>(new uchar[mriLength]);
	    m->decode(reinterpret_cast<uchar *>(s.mri), mriLength);
	    //DLog("API decoding", "mriLength= " << mriLength << " getpos()=" << m->get_pos());

	    // Put contents in OUR MRI.
	    mymri = convertGoeMriToKa(mrm, s.mri);

	    msghandle_t handle;
#ifdef MSG_HANDLE_GOETTINGEN
		for (int i = 0; i < 16; i++) {
			handle.handle[i] = s.nslp_message_handle[i];
		}
#else
		handle= 0;
#endif
	    
	    tx_attr_t tx;
	    tx.reliable=s.reliability;
	    tx.secure=s.security;
	    // is that even relevant for SendMessage
	    tx.final_hop=false; // Goettingen API doesn't support it

	    sid = new sessionid(a, b, c, d);

	    // currently unsupported: MsgHandle, SII-Handle, GHC (it is NOT PART of Goettingen's API)
	    result->set_sendmessage(mydata, handle /* Msg handle */, 
	    	s.nslp_id, 
	    	sid, mymri, 0 /* SII-Handle */, 
	    	tx, s.timeout, s.ip_ttl, 
	    	s.gist_hop_count /* GIST HopCount */);

	    delete s.mri;
    
    } // end SendMessage




    if (type == NW_NOTIFICATION) {

    //******************************** NetworkNotification ************************


//		sourceaddr = new hostaddress();
//		destaddr = new hostaddress();
//	
//	
//		api_nw_notification s;
//		
//		m->decode8(s.nwnt);
//		    
//		//cout << "NetworkNotification Value:    " << s.nwnt << endl;
//		
//		uint16 mrm;
//		    
//		//Fill Goettingen's MRI structure
//		
//		mrm = ((msg_routing_info_base_t*)m->get_buffer()+m->get_pos())->msgRoutingMethod;
//		int mriLength;
//		if (mrm == GIST_DEFAULT_MRM) {
//		    mriLength = sizeof(msg_routing_info_path_coupled_t);
//		} else if (mrm == GIST_LOOSEEND_MRM) {
//		    mriLength = sizeof(msg_routing_info_loose_end_t);
//		} else {
//		    DLog("API decoding", "Unknown MRM, this will not be processed");
//		    return NULL;
//		};
//		
//		s.mri = (msg_routing_info_base_t *)calloc(1, mriLength);
//		m->decode((uchar*) s.mri, mriLength);
//		
//		//Put contents in OUR MRI.
//		
//		if (mrm == GIST_DEFAULT_MRM) {
//		    
//		    msg_routing_info_path_coupled_t* mr = reinterpret_cast<msg_routing_info_path_coupled_t*>(s.mri);
//		    
//		    
//		    
//		    if (mr->ipVersion == 4) {
//			in_addr tmp;
//			memcpy(&tmp, &mr->srcAddr.v4.addr, 4);
//			sourceaddr->set_ip(tmp);
//			memcpy(&tmp, &mr->destAddr.v4.addr, 4);
//			destaddr->set_ip(tmp);   
//		    } else {
//			in6_addr tmp;
//			memcpy(&tmp, &mr->srcAddr.v6.addr, 16);
//			sourceaddr->set_ip(tmp);
//			memcpy(&tmp, &mr->destAddr.v6.addr, 16);
//			destaddr->set_ip(tmp);
//		    }
//		    
//		    
//		    //filling our MRI
//			    // TODO: traffic_class is outdated! We already use the new DS-field (christian.dickmann)
//		    mymri = new mri_pathcoupled(*sourceaddr, mr->srcPrefix, mr->srcPort, *destaddr, mr->destPrefix, 
//		    	mr->destPort, mr->protocol, mr->flowLabel, 0 /* mr->traffic_class */, mr->spi, !(mr->d));
//		    
//		} else {
//		    
//		    msg_routing_info_loose_end_t* mr = reinterpret_cast<msg_routing_info_loose_end_t*>(s.mri);
//		    
//		    
//		    if (mr->ipVersion == 4) {
//			in_addr tmp;
//			memcpy(&tmp, &mr->srcAddr.v4.addr, 4);
//			sourceaddr->set_ip(tmp);
//			memcpy(&tmp, &mr->destAddr.v4.addr, 4);
//			destaddr->set_ip(tmp);   
//		    } else {
//			in6_addr tmp;
//			memcpy(&tmp, &mr->srcAddr.v6.addr, 16);
//			sourceaddr->set_ip(tmp);
//			memcpy(&tmp, &mr->destAddr.v6.addr, 16);
//			destaddr->set_ip(tmp);
//		    }
//		    
//		    
//	
//		    // construct the MRI
//		    mymri = new mri_looseend(*sourceaddr, *destaddr, !(mr->d));
//		    
//		}
//		
//		// NSLP-ID is not available here! Also SII-Handle is not available!!!
//		// GISTka will NOT process this, but maybe it is an issue for Karlsruhe's QoS-NSLP!
//		
//		sessionid* mysid = new sessionid(1,2,3,4);
//	
//		result->set_networknotification(0, mysid, mymri, (APIMsg::error_t) s.nwnt, 0);
//	
//		
//		delete s.mri;
//		delete sourceaddr;
//		delete destaddr;
//	
//	
//	
    } //end if RecvMessage
    
    

    if (type == RECV_MESSAGE) {

    //******************************** RecvMessage ********************************

	    api_recv_msg s;
	    m->decode((uchar*) &s, RCV_MSG_FSIZE);
	    
	    //cout << "NSLP data len:    " << s.nd_size          << endl;
	    //cout << "NSLP ID:          " << s.nslp_id          << endl;
	    //cout << "SessionID:        ";

	    sourceaddr = new hostaddress();
	    destaddr = new hostaddress();


	    uint32 a;
	    uint32 b;
	    uint32 c;
	    uint32 d;
	    uint16 mrm;
	    
	    a = ntohl( *((uint32*) &(s.sid[0])) );
	    b = ntohl( *((uint32*) &(s.sid[4])) );
	    c = ntohl( *((uint32*) &(s.sid[8])) );
	    d = ntohl( *((uint32*) &(s.sid[12])) );

	    //cout << a << " " << b << " " << c << " " << d <<endl;

	    //cout << "Reliable:         " << s.reliability      << endl;
            //cout << "Secure:           " << s.security         << endl;
	    //cout << "IP TTL:           " << (int) s.ip_ttl     << endl;
	    //cout << "IP Distance:      " << (int) s.ip_distance<< endl;
	    
	    uchar* buf = new uchar[s.nd_size];
	    m->decode(buf, s.nd_size);
	    mydata = new nslpdata(buf, s.nd_size);

	    
	    //Fill Goettingen's MRI structure

	    mrm = ((msg_routing_info_base_t*)m->get_buffer()+m->get_pos())->msgRoutingMethod;
	    int mriLength;
	    if (mrm == GIST_DEFAULT_MRM) {
			mriLength = sizeof(msg_routing_info_path_coupled_t);
	    } else if (mrm == GIST_LOOSEEND_MRM) {
			mriLength = sizeof(msg_routing_info_loose_end_t);
	    } else {
			ERRLog("API decoding", "Unknown MRM - this will not be processed");
			delete mydata;
			return NULL;
	    };
	    
	    s.mri = (msg_routing_info_base_t *)calloc(1, mriLength);
	    m->decode((uchar*) s.mri, mriLength);

	    //Put contents in OUR MRI.
	    
	    if (mrm == GIST_DEFAULT_MRM) {

		msg_routing_info_path_coupled_t* mr = reinterpret_cast<msg_routing_info_path_coupled_t*>(s.mri);
		

		
		if (mr->ipVersion == 4) {
		    in_addr tmp;
		    memcpy(&tmp, &mr->srcAddr.v4.addr, 4);
		    sourceaddr->set_ip(tmp);
		    memcpy(&tmp, &mr->destAddr.v4.addr, 4);
		    destaddr->set_ip(tmp);   
		} else {
		    in6_addr tmp;
		    memcpy(&tmp, &mr->srcAddr.v6.addr, 16);
		    sourceaddr->set_ip(tmp);
		    memcpy(&tmp, &mr->destAddr.v6.addr, 16);
		    destaddr->set_ip(tmp);
		}


		//filling our MRI
		// TODO: traffic_class is outdated! We already use the new DS-field (christian.dickmann)
		mymri = new mri_pathcoupled(*sourceaddr, mr->srcPrefix, mr->srcPort, *destaddr, mr->destPrefix, 
			mr->destPort, mr->protocol, mr->flowLabel, 0 /* mr->traffic_class */, mr->spi, !(mr->d));
		
	    } else {

		msg_routing_info_loose_end_t* mr = reinterpret_cast<msg_routing_info_loose_end_t*>(s.mri);
		
	       
		if (mr->ipVersion == 4) {
		    in_addr tmp;
		    memcpy(&tmp, &mr->srcAddr.v4.addr, 4);
		    sourceaddr->set_ip(tmp);
		    memcpy(&tmp, &mr->destAddr.v4.addr, 4);
		    destaddr->set_ip(tmp);   
		} else {
		    in6_addr tmp;
		    memcpy(&tmp, &mr->srcAddr.v6.addr, 16);
		    sourceaddr->set_ip(tmp);
		    memcpy(&tmp, &mr->destAddr.v6.addr, 16);
		    destaddr->set_ip(tmp);
		}



		// construct the mri
		mymri = new mri_looseend(*sourceaddr, *destaddr, !(mr->d));

	    }

	    
	    tx_attr_t tx;
	    tx.reliable=s.reliability;
	    tx.secure=s.security;
	    tx.final_hop=false; // Not supported by the Goettingen API!

	    sid = new sessionid(a, b, c, d);

	    //currently unsupported: SII-Handle [defaulting 0], GHC [defaulting 20] (it is NOT PART of Goettingen's API)
	    result->set_recvmessage(mydata, s.nslp_id, sid, mymri, false, 0, tx, s.ip_ttl, s.ip_distance, 20);
    
	    delete s.mri;
	    delete sourceaddr;
	    delete destaddr;



    } //end if NetworkNotification





   //******************************** Register ***********************************

    if (type == REGISTER_NSLPID) {

      
	uint16 nslpid = (unsigned short) (totallen & 0xffff);
	    
	    // registering to RAO value '0' by default (shortcoming of API)
	    result->set_register(nslpid, 0);
	    
	    DLog("API decoding", "Created Register APIMsg for NSLPID " << nslpid);
    }

    return result;
}



///helper function providing a local IP address to the querying NSLP
void APIWrapper::send_local_ip(NetMsg* netmsg, const udsaddress& peer) {

    netmsg->to_start();
    
    netmsg->decode8();
    netmsg->decode32();

    NetMsg* answer = NULL;

    uint8 ip_version = netmsg->decode8();

    if (ip_version == 4) {
	DLog(param.name, "Sending back local IPv4 address");
	
	
	answer = new NetMsg(api_header_length + 4);
	
	answer->encode8(SEND_LOCAL_IP);
	uint32 tmp = 4;
	answer->encode((uchar*) &tmp, 4);

	netaddress *na = param.addresses.get_first(AddressList::ConfiguredAddr_P, true);
	struct in_addr ia;
	na->get_ip(ia);

	answer->encode((uchar*) &(ia), 4);
	delete na;

    } else {
	DLog(param.name, "Sending back local IPv6 address");
	
	
	answer = new NetMsg(api_header_length + 16);
	
	answer->encode8(SEND_LOCAL_IP);
	uint32 tmp = 4;
	answer->encode((uchar*) &tmp, 4);
	netaddress *na = param.addresses.get_first(AddressList::ConfiguredAddr_P, false);
	struct in6_addr ia;
	na->get_ip(ia);

	answer->encode((uchar*) &(ia), 4);
	delete na;

    }

    param.proto->send(answer, *(peer.copy()));
}



}//end namespace protlib
