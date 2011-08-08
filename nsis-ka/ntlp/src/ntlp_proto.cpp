/// ----------------------------------------*- mode: C++; -*--
/// @file ntlp_proto.cpp
/// GIST protocol state machine
/// ----------------------------------------------------------
/// $Id: ntlp_proto.cpp 6199 2011-05-25 15:25:57Z bless $
/// $HeadURL: https://svn.tm.kit.edu/nsis/ntlp/trunk/src/ntlp_proto.cpp $
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
/** @ingroup ntlpproto
 * NTLP protocol serialization manager
 */

#include <iomanip>
#include <sstream>

#include "query.h"
#include "ntlp_error.h"

#include "logfile.h"
#include "ntlp_proto.h"
#include "ntlp_global_constants.h" // required for GIST_magic_number
#include "routing_util.h"
#include "gist_conf.h" // required for peerid

namespace ntlp {
    using namespace protlib;

/** @addtogroup ntlpproto NTLP protocol serialization manager
 * @{
 */

  using namespace protlib::log;

  const char *const NTLP::default_host_signaling_transport_protocol="tcp";
  const char *const NTLP::modname="NTLPprotocol";

  const char *const NTLP::errstr[] = {
    		"ok",
		"ok_in_auth",
		"ok_out_auth",
		"ok_but_ignore",
		"internal",
		"wrong_pdu_type",
		"no_request_found",
		"no_mem",
		"serialize",
		"pdu_too_big",
		"reference_in_use",
		"send_error_pdu",
		"log_incpduerror_only",
		"ref_not_found",
		//GIST error codes/classes
		
		"error_gist_unknown_version",
		"error_gist_unknown_type",
		"error_gist_inconsistent_r_flag",
		"error_gist_incorrect_msg_length",
		"error_gist_hoplimit_exceeded",
		"error_gist_incorrect_encapsulation",
		"error_gist_incorrectly_delivered",
		"error_gist_no_routing_state",
		"error_gist_unknown_nslpid",
		"error_gist_endpoint_found",
		"error_gist_message_too_large",
		"error_gist_duplicate_object",
		"error_gist_unrecognised_object",
		"error_gist_missing_object",
		"error_gist_invalid_object",
		"error_gist_untranslated_object",
		"error_gist_incorrect_object_length",
		"error_gist_value_not_supported",
		"error_gist_invalid_flag_field_combination",
		"error_gist_empty_list",
		"error_gist_invalid_cookie",
		"error_gist_sp_scd_mismatch",
		"error_gist_MRI_ip_version_mismatch",

		"END OF GISTERROR STRINGS"
  };
/***** class GIST *****/

NTLP::NTLP(AddressList &addresses, uint16 wkp, IE::coding_t c, bool acct_unauth) 
	: addresses(addresses), coding(c), accept_unauthenticated(acct_unauth), wkp(wkp) {
} // end constructor

NTLP::~NTLP() {
	// sdtable is cleared in its own destructor
} // end destructor



/// checks whether address belongs to pool of own addresses
bool 
NTLP::isown_address(const hostaddress* ha) const
{
  if (ha)
  {
    netaddress na(*ha);
    return addresses.addr_is(na, AddressList::ConfiguredAddr_P);
  }
  else
    return false;
}

/** process/parse received incoming message from network
 * this method parses the netmsg and returns the parsed PDU
 * in respdu. It also adds an entry in the signaling table
 * for a request and deletes an existing sdtable entry if the 
 * corresponding response arrived.
 *
 * @param netmsg    Incoming Network message (byte buffer)
 * @param peeraddr  Peer address (msg sender), NOT const in order to let generate_errorpdu modify it!
 * @retval resultid    Resultid (Internal Id of signaling table entry)
 * @retval resultpdu   Result PDU (or errormsg case of error)
 * @retval refid    Reference to Id (for contained encapsulated)
 * @retval encappdu Encapsulated PDU (if contained in response)
 * @return result   error value GIST::error_t 
 * @todo Pass message to authentication module if there is one. 
 */
NTLP::error_t 
NTLP::process_tp_recv_msg(NetMsg& netmsg, 
			  appladdress& peeraddr,
			  gp_id_t& resultid, known_ntlp_pdu*& resultpdu,
			  gp_id_t& refid, known_ntlp_pdu*& encappdu) 
{
  // initialize result variables
  resultid= 0;
  refid= 0;
  resultpdu= NULL;
  encappdu= NULL;
  error_t gistresult= error_ok;

  // parse NetMsg
  IEErrorList errorlist;
  uint32 nbytesread= 0;
  IE* tmpie= NTLP_IEManager::instance()->deserialize(netmsg,NTLP_IE::cat_known_pdu,coding,errorlist,nbytesread,true);
  Log(DEBUG_LOG,LOG_UNIMP, NTLP::modname, "process_tp_recv_msg(): deserialization completed (read " << nbytesread << " bytes).");

  if (tmpie==NULL) {
      ERRCLog(NTLP::modname, "deserialization returned NO OBJECT");
   }

  ntlp_pdu* pdu= (tmpie!=NULL) ? dynamic_cast<ntlp_pdu*>(tmpie) : NULL;

  if (!pdu) 
  {
      ERRCLog(NTLP::modname, "cannot cast result to ntlp_pdu!!!");
  }

  // known_pdu contains parsed objects
  known_ntlp_pdu* known_pdu= pdu ? dynamic_cast<known_ntlp_pdu*>(pdu) : NULL;

  if (!known_pdu) 
  {
    ERRCLog(NTLP::modname, "cannot cast result to known_ntlp_pdu!!!");
  }

  // check for errors from deserialization and generate response pdu
  if (!errorlist.is_empty())
  {
    // TODO: handle HMAC integrity check failure here...

    ERRLog(NTLP::modname, "Errors occured during parsing, generating error pdu");
    return generate_errorpdu(netmsg,errorlist,known_pdu,resultpdu, peeraddr);
  }
  assert(pdu!=NULL);
  assert(known_pdu!=NULL);

  //Log(DEBUG_LOG,LOG_UNIMP, NTLP::modname, "assertion fulfilled, we HAVE a resultpdu!!");


  // check for Incoming Upstream Query with IP TTL < 254 (TPqueryEncap MUST provide IP-TTL)
  if (peeraddr.get_ip_ttl()) {
      // Transport Module provided us the IP TTL value
      if (pdu->get_type()==known_ntlp_pdu::Query) {
	  // it is a QUERY
	  if (pdu->get_mri()) {
	      // There is a MRI
	      if (!(pdu->get_mri()->get_downstream())) {
		  //jeeez, an UPSTREAM QUERY! We must look into it!!
		  if (peeraddr.get_ip_ttl() < 254) {
		      //BINGO! This is an upstream query we must DISCARD!! 
		      DLog("NTLP proto", "IP TTL of: " << peeraddr.get_ip_ttl());
		      errorlist.put(new GIST_InvalidIPTTL(IE::protocol_v1));
		      return generate_errorpdu(netmsg, errorlist, known_pdu, resultpdu, peeraddr);
		      
		  }
	      }
	  }
      }
  }

  // check for incorrect Encapsulation: Query arriving via Non-QE, Non-Query arriving via QE
  // check also for IPv4 query containing IPv6 NLI and vice versa
  if (pdu->get_type()==known_ntlp_pdu::Query) 
  {
    // Query must be sent via Query Encapsulation (check for C-flag set)
    if ( pdu->get_C() == false ) 
    {
      //BINGO! This Query was coming to us accidentally. Tell the sender, he is doing somethin WEIRD
      // note: It could als be our interceptor's fault, if we are the end node of the flow and we do not
      // intercept correctly, the Query would arrive at TPoverUDP like a normal package.

      // note that other MRMs may require a different encapsulation, thus we must check
      // the message routing method type, if no MRI exist we would expect path_coupled signaling
      uint8 mrm_type= pdu->get_mri() ? pdu->get_mri()->get_mrm() : 0;
      switch (mrm_type)
      {
	case mri::mri_t_pathcoupled:
	case mri::mri_t_looseend:
	  errorlist.put(new GIST_IncorrectEncapsulation(IE::protocol_v1));
	  return generate_errorpdu(netmsg, errorlist, known_pdu, resultpdu, peeraddr);
	  break;

	case mri::mri_t_explicitsigtarget:
	  // usually directly targeted
	  break;

        default:
	  break;
      } // end switch
    } // endif not query encapsulated

    if (pdu->get_mri()) // MRI present
    {
      // check for NLI transport match
      if ( 
	   (peeraddr.is_ipv4() && pdu->get_mri()->get_ip_version()!=4)
           ||
	   (peeraddr.is_ipv6() && !peeraddr.is_mapped_ip() && pdu->get_mri()->get_ip_version()!=6) 
	 )
      { 
	ERRLog(NTLP::modname, "MRI IP version ("<< (int) pdu->get_mri()->get_ip_version()  
	       << ") mismatch with received message (" << (peeraddr.is_ipv4() ? '4' : '6') << ")");
	errorlist.put(new GIST_MRI_IPVerMismatch(IE::protocol_v1));
	return generate_errorpdu(netmsg, errorlist, known_pdu, resultpdu, peeraddr);
      }
    }
    else
      ERRLog(NTLP::modname, "MRI is missing, though mandatory for Query"); 

    
    // check if Query has no R flag set which is also an error
    if (pdu->get_R() == false)
    { // an R flag must be always set in a Query message
      errorlist.put(new GIST_InvalidRFlag(IE::protocol_v1, pdu->get_version(), pdu->get_hops(), pdu->get_length(), pdu->get_nslpid(), pdu->get_type(), pdu->get_flags()));
	  return generate_errorpdu(netmsg, errorlist, known_pdu, resultpdu, peeraddr);
    }
  } 
  else 
  { // not a QUERY message
    if ( pdu->get_C() && (pdu->get_type()!=known_ntlp_pdu::Data)) 
    {
      //This is always NOT OUR FAULT, no one should send something other than a QUERY in QE mode
      //
      // note: QE can be used for GIST Data messages optionally. We will provide support for these, of course. 
      
      errorlist.put(new GIST_IncorrectEncapsulation(IE::protocol_v1));
      return generate_errorpdu(netmsg, errorlist, known_pdu, resultpdu, peeraddr);
    }

    /** A.1.:
     * The E flag MUST NOT be set unless the message type
     * is a Data message.
     *
     *   Parsing failures may be caused by unknown Version or Type values,
     *   inconsistent R or E flag setting, or a Message Length inconsistent
     *   with the set of objects carried.  In all cases the receiver MUST if
     *   possible return a "Common Header Parse Error" message
     **/
    // if E flag set and not a Data PDU we must return an Error PDU Common Header Parse Error/Invalid E flag
    if ( pdu->get_E() && (pdu->get_type() != known_ntlp_pdu::Data) ) 
    {
      errorlist.put(new GIST_InvalidEFlag(IE::protocol_v1, pdu->get_version(), pdu->get_hops(), pdu->get_length(), pdu->get_nslpid(), pdu->get_type(), pdu->get_flags()));
      return generate_errorpdu(netmsg, errorlist, known_pdu, resultpdu, peeraddr);
    }

    /**
     * A.1.
     *   The rules governing the use of the R-flag depend on the GIST message
     * type.  It MUST always be set (R=1) in Query messages, since these
     * always elicit a Response, and never in Confirm, Data or Error
     * messages.  It MAY be set in an MA-Hello; if set, another MA-Hello
     * MUST be sent in reply.  It MAY be set in a Response, but MUST be set
     * if the Response contains a Responder cookie; if set, a Confirm MUST
     * be sent in reply.  
     **/

    // if R flag set in Confirm, Data or Error we send an error back
    if ( pdu->get_R() )
    { // R flag is set

      switch (pdu->get_type())
      {
	case known_ntlp_pdu::Confirm:
	case known_ntlp_pdu::Data:
	case known_ntlp_pdu::Error:
	
	  errorlist.put(new GIST_InvalidRFlag(IE::protocol_v1, pdu->get_version(), pdu->get_hops(), pdu->get_length(), pdu->get_nslpid(), pdu->get_type(), pdu->get_flags()));
	  return generate_errorpdu(netmsg, errorlist, known_pdu, resultpdu, peeraddr);
	  break;
      } // end switch

    }
    else
    { // R flag is not set

      // R flag MUST be set if the Response contains a Responder cookie
      if ( pdu->get_type() == known_ntlp_pdu::Response && pdu->get_respcookie() != NULL )
      {
	errorlist.put(new GIST_InvalidRFlag(IE::protocol_v1, pdu->get_version(), pdu->get_hops(), pdu->get_length(), pdu->get_nslpid(), pdu->get_type(), pdu->get_flags()));
	return generate_errorpdu(netmsg, errorlist, known_pdu, resultpdu, peeraddr);
      }

    }

  }
// delete tmpie;
  
  if (!known_pdu->has_response())
  {
    // neither request that requires a response nor a plain response.
    // This is an error, info, refreq or heartbeat PDU.
    // Just pass it on without making an entry to the table.
    resultid= 0;
    resultpdu= known_pdu;
    gistresult= error_ok;
  } // end elseif (not a request or response)

  resultid=0;
  resultpdu = known_pdu;
  gistresult=error_ok;
  return gistresult;
} // end process_tp_recv_msg



/** process_response() sends a response PDU to a peer
 *
 * @param reqid   id of the corresponding request to the response
 * @param rsppdu  response pdu
 * @param encapid id of a pdu that should be encapsulated in the response
 * @param msg     buffer for the actual network message of the response
 * @param peeraddr address of signaling peer (is set to sender/forwarder of the request)
 */

NTLP::error_t
NTLP::process_response(gp_id_t reqid, known_ntlp_pdu& rsppdu, gp_id_t encapid,
			NetMsg*& msg, appladdress& peeraddr) 
{
  error_t gistres = error_ok;
  msg = NULL;

 
  // fills in encapsulated pdu if encapid was given
  fill_in_encap_pdu(rsppdu,encapid);

  set_orig_msgref(rsppdu);

  // set GIST magic number if required
  uint32 magic_number= (peeraddr.get_protocol() == prot_query_encap || peeraddr.get_protocol() == tsdb::get_udp_id()) ? GIST_magic_number : 0;
  // fills response pdu into message buffer (which is allocated within generate_pdu)
  gistres = generate_pdu(rsppdu, msg, magic_number);
  if (gistres!=error_ok) 
  {
    if (msg) delete msg;
    msg = NULL;
  } // end if generate_pdu returned error

  return gistres;
} // end process_response

/** prepares sending of a GIST PDUs (no RSP PDUs!) via the network
 * @param pdu       GIST PDU to be sent
 * @param addr      address of signaling peer
 * @param resultid  Internal Id of signaling table entry (is returned)
 * @param refid     internal id of referred PDU for encapsulation 
 * @param netbuffer NetMsg buffer for serialization of the PDU into a bytes stream (is returned)
 * @return internal GIST_error_t errorcode, usually error_ok for success
 * @note This method is not intended to send response PDUs, use process_response() instead
 */
NTLP::error_t 
NTLP::process_outgoing(known_ntlp_pdu& pdu, const appladdress& addr,
			gp_id_t resultid, gp_id_t refid, NetMsg*& netbuffer) 
{
  error_t gistres = error_ok;
  netbuffer = NULL;

  // set GIST magic number if required
  uint32 magic_number= (addr.get_protocol() == prot_query_encap || addr.get_protocol() == tsdb::get_udp_id()) ? GIST_magic_number : 0;

  // now serialize
  gistres = generate_pdu(pdu, netbuffer, magic_number);
  /*if (gistres==error_ok) 
  {
   
      
      
  } // end if generate_pdu OK
  */

  if (gistres!=error_ok) 
  {
    if (netbuffer) delete netbuffer;
    netbuffer = NULL;
  } // end if error
  return gistres;
} // end process_outgoing

/** Revoke a request after a transport protocol error or cancel a signaling
 * transaction.
 */
bool NTLP::revoke(gp_id_t reqid) 
{
  return true;
} // end revoke

/// generate_error_pdu is able to modify the peer address in order to calculate the correct error message target
NTLP::error_t 
NTLP::generate_errorpdu(NetMsg& msg,
			IEErrorList& errorlist, known_ntlp_pdu* pdu, known_ntlp_pdu*& resultpdu, appladdress& peer) {
  string errstr;
  uint32 wordpos = 0;
  uint8 bitpos = 0;
  assert(!errorlist.is_empty());
  
  IEError* iee = errorlist.get();
  assert(iee!=NULL);

  bool dgram = (peer.get_protocol() == prot_query_encap) || (peer.get_protocol() == tsdb::get_udp_id());
  bool qe = (peer.get_protocol() == prot_query_encap) || (pdu ? pdu->get_C() : false);

  //DLog(NTLP::modname,"generateErrorPdu() - entered, dgram:" << dgram << "query encap:" << qe);  
  
  mri* mr = NULL;
  sessionid* sid = NULL;
  errorobject* errobj = NULL;
  
  if (pdu) 
  {
    if (pdu->get_type() == known_ntlp_pdu::Error)
    {
      ERRLog("generate_errorpdu","Error related to Error PDU, must not send an Error back, aborting to generate Error message");
      resultpdu= NULL;
      delete pdu;
      return error_log_incpduerror_only;
    }
    if (pdu->get_mri()) mr = pdu->get_mri()->copy();
    if (pdu->get_sessionid()) sid = pdu->get_sessionid()->copy();
  }
  else
  { // some error handling in the case statements assume that pdu is not NULL
    ERRCLog("generate_errorpdu","PDU is NULL");
    // cannot continue here
    return error_log_incpduerror_only;
  }

  if (iee->err == IEError::ERROR_PROT_SPECIFIC)
  {
    GIST_Error* gisterr= dynamic_cast<GIST_Error *>(iee);
    if (gisterr)
    {
      switch (gisterr->errorcode()) 
      {
	case GIST_Error::error_gist_missing_object : 
	  {
	    GIST_MissingObject* tmp = dynamic_cast<GIST_MissingObject*>(iee);
      
	    uint16 objecttype = tmp->objecttype;
      
	    errobj = new errorobject(mr, sid, 
				     errorobject::ProtocolError, 
				     errorobject::err_ObjectTypeError, 
				     errorobject::errsub_MissingObject, 
				     pdu->get_version(), 
				     pdu->get_hops(), 
				     pdu->get_length(),
				     pdu->get_nslpid(), 
				     pdu->get_type(), 
				     pdu->get_flags(), dgram, qe, NULL, objecttype);
      
	    errstr=gisterr->getdescription();
	    break;
	  }

	case GIST_Error::error_gist_invalid_object : 
	  {
	    GIST_InvalidObject* tmp = dynamic_cast<GIST_InvalidObject*>(iee);
      
	    uint16 objecttype = tmp->objecttype;
      
	    errobj = new errorobject(mr, sid, 
				     errorobject::ProtocolError, 
				     errorobject::err_ObjectTypeError, 
				     errorobject::errsub_InvalidObject,
				     pdu->get_version(), 
				     pdu->get_hops(), 
				     pdu->get_length(), 
				     pdu->get_nslpid(), 
				     pdu->get_type(), 
				     pdu->get_flags(), dgram, qe, NULL, objecttype);
      
	    errstr=gisterr->getdescription();
	    break;
	  }


	case GIST_Error::error_gist_duplicate_object : {
	  GIST_DuplicateObject* tmp = dynamic_cast<GIST_DuplicateObject*>(iee);
      
	  uint16 objecttype = tmp->objecttype;
      
	  errobj = new errorobject(mr, sid, 
				   errorobject::ProtocolError, 
				   errorobject::err_ObjectTypeError,
				   errorobject::errsub_DuplicateObject,  
				   pdu->get_version(), 
				   pdu->get_hops(),
				   pdu->get_length(), 
				   pdu->get_nslpid(), 
				   pdu->get_type(), 
				   pdu->get_flags(), dgram, qe, NULL, objecttype);
      
	  errstr=gisterr->getdescription();
	  break;
	}
      
      
	case GIST_Error::error_gist_unrecognised_object : {
	  GIST_UnrecognisedObject* tmp = dynamic_cast<GIST_UnrecognisedObject*>(iee);
      
	  uint16 objecttype = tmp->objecttype;
      
	  errobj = new errorobject(mr, sid, 
				   errorobject::ProtocolError, 
				   errorobject::err_ObjectTypeError, 
				   errorobject::errsub_UnrecognizedObject,  
				   pdu->get_version(), 
				   pdu->get_hops(),
				   pdu->get_length(), 
				   pdu->get_nslpid(), 
				   pdu->get_type(), 
				   pdu->get_flags(), dgram, qe, NULL, objecttype);
      
	  errstr=gisterr->getdescription();
	  break;
	}
      
      
	case GIST_Error::error_gist_incorrect_object_length : {
	  GIST_IncorrectObjectLength* tmp = dynamic_cast<GIST_IncorrectObjectLength*>(iee);
      
	  errobj = new errorobject(mr, sid, 
				   errorobject::ProtocolError, 
				   errorobject::err_ObjectValueError, 
				   errorobject::errsub_IncorrectLength, 
				   pdu->get_version(), 
				   pdu->get_hops(), 
				   pdu->get_length(), 
				   pdu->get_nslpid(), 
				   pdu->get_type(), 
				   pdu->get_flags(), dgram, qe, tmp->obj->copy(), 0);
      
	  errstr=gisterr->getdescription();
	  break;
	}
      
	case GIST_Error::error_gist_value_not_supported : {
	  GIST_ValueNotSupported* tmp = dynamic_cast<GIST_ValueNotSupported*>(iee);
      
	  errobj = new errorobject(mr, sid, 
				   errorobject::ProtocolError, 
				   errorobject::err_ObjectValueError, 
				   errorobject::errsub_ValueNotSupported, 
				   pdu->get_version(), 
				   pdu->get_hops(), 
				   pdu->get_length(), 
				   pdu->get_nslpid(), 
				   pdu->get_type(), 
				   pdu->get_flags(), dgram, qe, tmp->obj->copy(), 0);
      
	  errstr=gisterr->getdescription();
	  break;
	}
      
	case GIST_Error::error_gist_invalid_flag_field_combination : {
	  GIST_InvalidFlagFieldCombination* tmp = dynamic_cast<GIST_InvalidFlagFieldCombination*>(iee);
      
	  errobj = new errorobject(mr, sid, 
				   errorobject::ProtocolError, 
				   errorobject::err_ObjectValueError, 
				   errorobject::errsub_InvalidFlagFieldCombination, 
				   pdu->get_version(), 
				   pdu->get_hops(), 
				   pdu->get_length(), 
				   pdu->get_nslpid(), 
				   pdu->get_type(), 
				   pdu->get_flags(), 
				   dgram, 
				   qe, 
				   tmp->obj->copy(), 0);
      
	  errstr=gisterr->getdescription();
	  break;
	}

	case GIST_Error::error_gist_empty_list : {
	  GIST_EmptyList* tmp = dynamic_cast<GIST_EmptyList*>(iee);
      
	  errobj = new errorobject(mr, sid, 
				   errorobject::ProtocolError, 
				   errorobject::err_ObjectValueError, 
				   errorobject::errsub_EmptyList, 
				   pdu->get_version(), 
				   pdu->get_hops(), 
				   pdu->get_length(), 
				   pdu->get_nslpid(), 
				   pdu->get_type(), 
				   pdu->get_flags(), dgram, qe, tmp->obj->copy(), 0);
      
	  errstr=gisterr->getdescription();
	  break;
	}
      
      
	case GIST_Error::error_gist_unknown_version : {
	  GIST_UnknownVersion* tmp = dynamic_cast<GIST_UnknownVersion*>(iee);
      
	  errobj = new errorobject(mr, sid, 
				   errorobject::ProtocolError, 
				   errorobject::err_CommonHeaderParseError, 
				   errorobject::errsub_UnknownVersion, 
				   tmp->version, 
				   tmp->hops, 
				   tmp->length, 
				   tmp->nslpid, 
				   tmp->type, 
				   tmp->flags, 
				   dgram, qe, NULL, 0);
      
	  errstr=gisterr->getdescription();
	  break;
	}
      
	case GIST_Error::error_gist_incorrect_msg_length : {
	  GIST_IncorrectMsgLength* tmp = dynamic_cast<GIST_IncorrectMsgLength*>(iee);
      
	  errobj = new errorobject(mr, sid, 
				   errorobject::ProtocolError, 
				   errorobject::err_CommonHeaderParseError, 
				   errorobject::errsub_IncorrectMsgLength, 
				   tmp->version, 
				   tmp->hops, 
				   tmp->length, 
				   tmp->nslpid, 
				   tmp->type, 
				   tmp->flags, 
				   dgram, qe, NULL, 0);
      
	  errstr=gisterr->getdescription();
	  break;
	}
      
      
      
	case GIST_Error::error_gist_unknown_type : {
	  GIST_UnknownType* tmp = dynamic_cast<GIST_UnknownType*>(iee);
      
	  errobj = new errorobject(mr, sid, 
				   errorobject::ProtocolError, 
				   errorobject::err_CommonHeaderParseError, 
				   errorobject::errsub_UnknownType,
				   tmp->version, 
				   tmp->hops, 
				   tmp->length, 
				   tmp->nslpid, 
				   tmp->type, 
				   tmp->flags, dgram, qe, NULL, 0);
      
	  errstr=gisterr->getdescription();
	  break;
	}
      
      
	case GIST_Error::error_gist_invalid_r_flag : {
	  GIST_InvalidRFlag* tmp = dynamic_cast<GIST_InvalidRFlag*>(iee);
		
	  errobj = new errorobject(mr, sid, 
				   errorobject::ProtocolError, 
				   errorobject::err_CommonHeaderParseError,
				   errorobject::errsub_InvalidRFlag,
				   tmp->version, 
				   tmp->hops, 
				   tmp->length, 
				   tmp->nslpid, 
				   tmp->type, 
				   tmp->flags, dgram, qe, NULL, 0);
      
	  errstr=gisterr->getdescription();
	  break;
	}

	case GIST_Error::error_gist_invalid_e_flag : {
	  GIST_InvalidEFlag* tmp = dynamic_cast<GIST_InvalidEFlag*>(iee);
		
	  errobj = new errorobject(mr, sid, 
				   errorobject::ProtocolError, 
				   errorobject::err_CommonHeaderParseError,
				   errorobject::errsub_InvalidEFlag,
				   tmp->version, 
				   tmp->hops, 
				   tmp->length, 
				   tmp->nslpid, 
				   tmp->type, 
				   tmp->flags, dgram, qe, NULL, 0);
      
	  errstr=gisterr->getdescription();
	  break;
	}
      
	case GIST_Error::error_gist_invalid_c_flag : {
	  
	  GIST_InvalidCFlag* tmp = dynamic_cast<GIST_InvalidCFlag*>(iee);
		
	  errobj = new errorobject(mr, sid, 
				   errorobject::ProtocolError, 
				   errorobject::err_CommonHeaderParseError,
				   errorobject::errsub_InvalidCFlag,
				   tmp->version, 
				   tmp->hops, 
				   tmp->length, 
				   tmp->nslpid, 
				   tmp->type, 
				   tmp->flags, dgram, qe, NULL, 0);
      
	  errstr=gisterr->getdescription();
	  break;
	}

	case GIST_Error::error_gist_invalid_ip_ttl : {
	  //GIST_InvalidIPTTL* tmp = dynamic_cast<GIST_InvalidIPTTL*>(iee);
      
	  errobj = new errorobject(mr, sid, 
				   errorobject::PermanentFailure, 
				   errorobject::err_InvalidIPTTL, 
				   errorobject::errsub_Default, 
				   pdu->get_version(), 
				   pdu->get_hops(), 
				   pdu->get_length(), 
				   pdu->get_nslpid(), 
				   pdu->get_type(), 
				   pdu->get_flags(), dgram, qe, NULL, 0);
      
	  errstr=gisterr->getdescription();
	  break;
	}

      
	case GIST_Error::error_gist_incorrect_encapsulation : {
	  //GIST_IncorrectEncapsulation* tmp = dynamic_cast<GIST_IncorrectEncapsulation*>(iee);
      
	  errobj = new errorobject(mr, sid, 
				   errorobject::ProtocolError, 
				   errorobject::err_IncorrectEncapsulation, 
				   errorobject::errsub_Default, 
				   pdu->get_version(), 
				   pdu->get_hops(), 
				   pdu->get_length(), 
				   pdu->get_nslpid(), 
				   pdu->get_type(), 
				   pdu->get_flags(), dgram, qe, NULL, 0);
      
	  errstr=gisterr->getdescription();
	  break;
	}

	case GIST_Error::error_gist_MRI_ip_version_mismatch : {
	  errobj = new errorobject(mr, sid, 
				   errorobject::ProtocolError, 
				   errorobject::err_MRIValidationFailure, 
				   errorobject::errsub_IPVersionMismatch, 
				   pdu->get_version(), 
				   pdu->get_hops(), 
				   pdu->get_length(), 
				   pdu->get_nslpid(), 
				   pdu->get_type(), 
				   pdu->get_flags(), dgram, qe, NULL, 0);
      
	  errstr=gisterr->getdescription();
	  break;
	}
	  
	default:
	  ERRLog("generate_errorpdu","Yet unhandled GIST error. Error code: " 
		 << gisterr->errorcode() << " (" << gisterr->getdescription() << ") - not sending back any error");
	  break;

      } // end switch gisterr->errorcode()
    } // endif dynamic cast to gisterr was successful
    else
    {
      ERRLog("generate_errorpdu","Protocol Specific error, but not possible to convert to gisterror. Error code: " 
	     << iee->err << " (" << iee->getstr()  << ")");
    }
  } // endif PROT_SPECIFIC
  else
  {
    //****************************** obsolete generic errors,  TODO: adapt these *****************
    switch(iee->err)
    {
      case IEError::ERROR_MSG_TOO_SHORT: {
	IEMsgTooShort* tmp = dynamic_cast<IEMsgTooShort*>(iee);
	assert(tmp!=NULL);
	wordpos = tmp->errorpos/4;
	bitpos = (tmp->errorpos%4)*8;
      
	errstr= tmp->getstr();
      } 
	break;

      case IEError::ERROR_WRONG_TYPE: {
	IEWrongType* tmp = dynamic_cast<IEWrongType*>(iee);
	assert(tmp!=NULL);
	wordpos = tmp->errorpos/4;
	bitpos = (tmp->errorpos%4)*8;
	// adjust position
	switch (tmp->category) {
	  case NTLP_IE::cat_known_pdu:
	  case NTLP_IE::cat_unknown_pdu:
	    //errorcode = errorobject::UnknownMessageType;
	    bitpos += 8;
	    break;
	  case NTLP_IE::cat_known_pdu_object:
	  case NTLP_IE::cat_raw_protocol_object:
	    //errorcode = errorobject::UnknownObjectType;
	    break;
	  case NTLP_IE::cat_ulp_data:
	    //errorclass = errorobject::resource;
	    //errorcode = errorobject::UnsupportedService;
	    break;
	  default: break;
	} // end switch category
	wordpos += (bitpos/32);
	bitpos %= 32;
	errstr = tmp->getstr();
      } break;

      case IEError::ERROR_WRONG_SUBTYPE: {
	IEWrongSubtype* tmp = dynamic_cast<IEWrongSubtype*>(iee);
	assert(tmp!=NULL);
	//errorclass = errorobject::protocol;
	wordpos = tmp->errorpos/4;
	bitpos = (tmp->errorpos%4)*8;
	// adjust position
	switch (tmp->category) {
	  case NTLP_IE::cat_known_pdu:
	  case NTLP_IE::cat_unknown_pdu:
	    //errorcode = errorobject::UnknownMessageSubtype;
	    bitpos += 16;
	    break;
	  case NTLP_IE::cat_known_pdu_object:
	  case NTLP_IE::cat_raw_protocol_object:
	    //errorcode = errorobject::UnknownObjectSubtype;
	    bitpos += 10;
	    break;
	  case NTLP_IE::cat_ulp_data:
	    //errorclass = errorobject::resource;
	    //errorcode = errorobject::UnsupportedService;
	    bitpos += 16;
	    break;
	  default: break;
	} // end switch category
	wordpos += (bitpos/32);
	bitpos %= 32;
	errstr = tmp->getstr();
      } break;

      case IEError::ERROR_WRONG_LENGTH: {
	IEWrongLength* tmp = dynamic_cast<IEWrongLength*>(iee);
	assert(tmp!=NULL);
	//errorclass = errorobject::protocol;
	//errorcode = errorobject::ObjectSyntaxError;
	wordpos = tmp->errorpos/4;
	bitpos = (tmp->errorpos%4)*8;
	// adjust position
	switch (tmp->category) {
	  case NTLP_IE::cat_known_pdu:
	  case NTLP_IE::cat_unknown_pdu:
	    wordpos += 1;
	    break;
	  case NTLP_IE::cat_known_pdu_object:
	  case NTLP_IE::cat_raw_protocol_object:
	    bitpos += 16;
	    break;
	  default: break;
	} // end switch category
	wordpos += (bitpos/32);
	bitpos %= 32;
	errstr = tmp->getstr();
      } break;

      case IEError::ERROR_TOO_BIG_FOR_IMPL: {
	IETooBigForImpl* tmp = dynamic_cast<IETooBigForImpl*>(iee);
	assert(tmp!=NULL);
	//errorclass = errorobject::protocol;
	//errorcode = errorobject::MessageTooLong;
	wordpos = tmp->errorpos/4;
	bitpos = (tmp->errorpos%4)*8;
	errstr = tmp->getstr();
      } break;

      case IEError::ERROR_WRONG_VERSION: {
	IEWrongVersion* tmp = dynamic_cast<IEWrongVersion*>(iee);
	assert(tmp!=NULL);
	//errorclass = errorobject::protocol;
	//errorcode = errorobject::UnsupportedVersion;
	wordpos = tmp->errorpos/4;
	bitpos = (tmp->errorpos%4)*8;
      } break;

      case IEError::ERROR_PDU_SYNTAX: {
	PDUSyntaxError* tmp = dynamic_cast<PDUSyntaxError*>(iee);
	assert(tmp!=NULL);
	//errorclass = errorobject::protocol;
	//errorcode = errorobject::UnexpectedObject;
	errstr = tmp->getstr();		
      } break;

      default:
	ERRCLog(NTLP::modname, "Unhandled Internal Processing Error (no GIST exception) - will emit Missing Object error");

	errobj = new errorobject(mr, sid,
				 errorobject::ProtocolError, 
				 errorobject::err_ObjectTypeError, 
				 errorobject::errsub_MissingObject, 
				 pdu->get_version(), 
				 pdu->get_hops(), 
				 pdu->get_length(),
				 pdu->get_nslpid(), 
				 pdu->get_type(), 
				 pdu->get_flags(), dgram, qe, NULL, 4095); // this object type is reserved

	errstr = iee->getstr(); 
	break;
    } // end switch
  }
  ostringstream strstream;

  //DLog(NTLP::modname,"generateErrorPdu() - before decode_common_header");
  
  // for reading the common header we must start at beginning of NetMsg
  // (current position may be anywhere in NetMsg or even at the end (e.g., wrong encap/TTL)
  // skip prepended magic number if datagram
  if (dgram)
    msg.set_pos(4);
  else
    msg.to_start();
  
  uint32 clen= 0; // clen is msg length w/o header
  ntlp_pdu::decode_common_header_ntlpv1_clen(msg, clen);
  
  msg.hexdump(strstream, 0, clen+ntlp_pdu::common_header_length);
  
  ERRLog(NTLP::modname,
      "Encountered error at octet " 
      << wordpos*4+bitpos/8 << " while parsing PDU: [" << color[bold_on] << errstr << color[bold_off] 
      << "] - Msg Dump follows -" << strstream.str().c_str());
  
  if (iee) delete iee;

  // create error pdu
  netaddress *src = protlib::util::get_src_addr(peer); 
  uint8 ttl = 0;
  nli* mynli =  new nli(ttl, gconf.getpar<uint32>(gistconf_rs_validity_time), &gconf.getparref<peer_identity>(gistconf_peerid), *src);

  resultpdu = new error(mynli, errobj);
  resultpdu->set_hops(1);
  
  // calculate correct error target address only if we HAVE a PDU
  if (pdu) 
  {
    // IP source is NOT the last GIST hop
    if (!pdu->get_S()) 
    {
      if (pdu->get_nli()) 
      {
	DLog("NTLP proto", "IP Source is NOT last GIST hop (S-Flag not set), sending to GIST denoted by NLI");
	peer.set_ip(pdu->get_nli()->get_if_address());
	peer.set_port(wkp);
	peer.set_protocol("udp");
      } 
      else 
      {
	// probably this one came through TCP but the S flag was not set
	if (peer.get_protocol() == tsdb::get_tcp_id() || peer.get_protocol() == tsdb::get_sctp_id())
	  {
	    ERRCLog("NTLP proto", "No valid target address for Error Message found (S flag=0, no NLI present).");
	  }
	else
	  {
	    ERRCLog("NTLP proto", "No valid target address for Error Message found (S flag=0, no NLI present). Dropping errormessage: " << *pdu);
	    peer.set_ip("0.0.0.0");
	    peer.set_port((uint16)0);
	  }
      }
    } 
    else 
    {
      if (pdu->get_nli() && !pdu->get_nli()->get_if_address().is_ip_unspec()) 
      {
	DLog("NTLP proto", "IP Source is last GIST hop (S-Flag set), sending to GIST denoted by NLI");
	peer.set_ip(pdu->get_nli()->get_if_address());
	peer.set_port(wkp);
	peer.set_protocol("udp");
      } 
      else
	DLog("NTLP proto", "IP Source IS last GIST hop, no (usable) NLI, sending back to source.");
    }  
  }
  
  if (peer.get_protocol()==prot_query_encap) peer.set_protocol("udp");

  if (pdu) delete pdu;
  return error_send_error_pdu;
} // end generate_errorpdu

/**
 * generates actual network pdu from first parameter
 * @param pdu - pdu to send
 * @param netmsg - pointer to netmsg buffer (which is allocated by this method)
 * @param magic_number - preprended to packet buffer if not zero (should be set for Q-mode and D-mode packets), in host byte order
 * @return error code or error_ok on successful encoding

 */
NTLP::error_t NTLP::generate_pdu(const known_ntlp_pdu& pdu, NetMsg*& netmsg, uint32 magic_number) 
{
  error_t gistres = error_ok;
  uint32 nbytes;
  netmsg = NULL;
  uint16 magic_number_bytes= magic_number ? 4 : 0;

  try 
  {
    // get expected size of pdu
    // for Query encapsulation or D-Mode add 4 bytes for the magic number
    uint32 pdusize = pdu.get_serialized_size(coding) + magic_number_bytes;

    if (pdusize>NetMsg::max_size) 
    {
      ERRCLog(NTLP::modname, "NTLP::generate_pdu() cannot allocate " << pdusize << " bytes NetMsg, PDU too big for NetMsg maxsize " << NetMsg::max_size);
      gistres = error_pdu_too_big;
    } 
    else 
    {
      // allocate a netmsg with the corresponding buffer size
      netmsg= new(nothrow) NetMsg(pdusize);
      if (netmsg) 
      {
	Log(DEBUG_LOG,LOG_UNIMP, NTLP::modname, "starting to serialize");
	// write actual pdu octetts into netmsg

	if (magic_number != 0)
	{
	  Log(DEBUG_LOG,LOG_UNIMP, NTLP::modname, "prepended magic number " << hex << magic_number << dec);

	  netmsg->encode32(magic_number);
	}

	pdu.serialize(*netmsg,coding,nbytes);
	Log(DEBUG_LOG,LOG_UNIMP, NTLP::modname, "PDU serialized.");


	if (nbytes != (pdusize - magic_number_bytes) ) {
	    ERRCLog(NTLP::modname, "NTLP::generate_pdu(), serialize method reported to have written "<< nbytes << " bytes, but PDU size is " << pdusize - magic_number_bytes );
	    gistres=error_serialize;
	}

	if (netmsg->get_bytes_left()) {
	    ERRCLog(NTLP::modname, "NTLP::generate_pdu() - There are " << netmsg->get_bytes_left() << "bytes left in NetMsg");
	  gistres = error_serialize;
	} // end if serialization error
      } 
      else 
      {
	ERRCLog(NTLP::modname, "NTLP::generate_pdu() cannot allocate " << pdusize << " bytes NetMsg");
	gistres = error_no_mem;
      } // end if netmsg
    } // end if too big
  } 
  catch(ProtLibException& e) 
  {
    ERRCLog(NTLP::modname, "NTLP::generate_pdu() serialization error: " << e.getstr());
    gistres = error_serialize;
  } // end try-catch
  
  // delete and cleanup netmsg if error occured
  if ((gistres!=error_ok) && netmsg) 
  {
    delete netmsg;
    netmsg = NULL;
  } // end if delete netmsg
  return gistres;
} // end generate_pdu;


known_ntlp_pdu* NTLP::process_encap_msg(const known_ntlp_pdu& pdu, const appladdress& peer,
		gp_id_t& refid) {

	return NULL;
} // end process_encap_msg

bool 
NTLP::get_ref_id(known_ntlp_pdu*& pdu, const appladdress& peer, gp_id_t& refid) 
{
    return true;
} // end get_ref_id

bool NTLP::fill_in_msg_ref(known_ntlp_pdu& pdu, gp_id_t refid) const {
    return true;
} // end fill_in_msg_ref

/** Fill in an encapsulated PDU if encapid is != 0
 * @param pdu 
 * @param encapid
 * @note SignalingTable must be locked. */
void NTLP::fill_in_encap_pdu(known_ntlp_pdu& pdu, gp_id_t encapid) const {
} // end fill_in_encap_pdu

//@}

} // end namespace protlib
