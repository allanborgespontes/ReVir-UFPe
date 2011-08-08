// ----------------------------------------*- mode: C++; -*--
// errorobject.cpp - Test the errorobject class
// ----------------------------------------------------------
// $Id: mri_pc.cpp 2862 2008-02-15 23:25:27Z bless $
// $HeadURL: https://svn.ipv6.tm.uka.de/nsis/ntlp/trunk/test/mri_pc.cpp $
// ==========================================================
//                      
// Copyright (C) 2008, all rights reserved by
// - Institute of Telematics, University of Karlsruhe (TH)
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
// ==========================================================
#include "test_suite.h"

#include "ntlp_errorobject.h"
#include "ntlp_pdu.h"
#include "query_cookie.h"
#include "mri_pc.h"
using namespace ntlp;

class errorobject_test : public CppUnit::TestCase {

  CPPUNIT_TEST_SUITE( errorobject_test );

  CPPUNIT_TEST( test_register_ie );

  CPPUNIT_TEST( test_errobj_CommonHeaderParseError_UnknownVersion );
  CPPUNIT_TEST( test_errobj_CommonHeaderParseError_UnknownType );
  CPPUNIT_TEST( test_errobj_CommonHeaderParseError_InvalidRFlag );
  CPPUNIT_TEST( test_errobj_CommonHeaderParseError_IncorrectMsgLength );

  CPPUNIT_TEST( test_errobj_HopLimitExceededError );

  CPPUNIT_TEST( test_errobj_IncorrectEncapsulationError );

  CPPUNIT_TEST( test_errobj_IncorrectlyDeliveredMessageError );

  CPPUNIT_TEST( test_errobj_NoRoutingStateError );

  CPPUNIT_TEST( test_errobj_UnknownNSLPIDError );

  CPPUNIT_TEST( test_errobj_EndpointFoundError );

  CPPUNIT_TEST( test_errobj_MessageTooLarge );

  CPPUNIT_TEST( test_errobj_ObjectTypeError_DuplicateObject );
  CPPUNIT_TEST( test_errobj_ObjectTypeError_UnrecognizedObject );
  CPPUNIT_TEST( test_errobj_ObjectTypeError_MissingObject );
  CPPUNIT_TEST( test_errobj_ObjectTypeError_InvalidObject );
  CPPUNIT_TEST( test_errobj_ObjectTypeError_UntranslatedObject );
  CPPUNIT_TEST( test_errobj_ObjectTypeError_InvalidExtensibilityFlags );

  CPPUNIT_TEST( test_errobj_ObjectValueError_IncorrectLength );
  CPPUNIT_TEST( test_errobj_ObjectValueError_ValueNotSupported );
  CPPUNIT_TEST( test_errobj_ObjectValueError_InvalidFlagFieldCombination );
  CPPUNIT_TEST( test_errobj_ObjectValueError_EmptyList );
  CPPUNIT_TEST( test_errobj_ObjectValueError_InvalidCookie );
  CPPUNIT_TEST( test_errobj_ObjectValueError_SP_SCD_Mismatch );

  CPPUNIT_TEST( test_errobj_InvalidIPTTL );

  CPPUNIT_TEST( test_errobj_MRIValidationFailure_MRITooWild );
  CPPUNIT_TEST( test_errobj_MRIValidationFailure_IPVersionMismatch );
  CPPUNIT_TEST( test_errobj_MRIValidationFailure_IngressFilterFailure );

  CPPUNIT_TEST_SUITE_END();

  public:
  void test_register_ie()
  {
    // register for parsing
    NTLP_IE *tmpie= new errorobject;
    NTLP_IEManager::instance()->register_ie(tmpie);

    // register for parsing (used as Object value info)
    tmpie= new querycookie;
    NTLP_IEManager::instance()->register_ie(tmpie);

  }


  /***
A.4.4.  Error Catalogue

   This section lists all the possible GIST errors, including when they
   are raised and what additional information fields MUST be carried in
   the error object.

A.4.4.1.  Common Header Parse Error

   Class:              Protocol-Error
   Code:               1
   Additional Info:    For subcode 3 only, Message Length Info carries
                       the calculated message length.

   This message is sent if a GIST node receives a message where the
   common header cannot be parsed correctly, or where an error in the
   overall message format is detected.  Note that in this case the
   original MRI and Session ID MUST NOT be included in the Error Object.
   This error code is split into subcodes as follows:

   0: Unknown Version:  The GIST version is unknown.  The (highest)
      supported version supported by the node can be inferred from the
      Common Header of the Error message itself.

   1: Unknown Type:  The GIST message type is unknown.

   2: Invalid R-flag:  The R flag in the header is inconsistent with the
      message type.

   3: Incorrect Message Length:  The overall message length is not
      consistent with the set of objects carried.

   4: Invalid E-flag:  The E flag is set in the header but this is not a
      Data message.
	  ***/


  void test_errobj_CommonHeaderParseError_UnknownVersion()
  {
	  mri_pathcoupled mri1(hostaddress("fe80::dead:beef"), 128,
			       hostaddress("2001:db8::bad:c0:ffee"), 64, true);

	  sessionid testsid(1,2,3,4);

	  errorobject snderrtestobj(&mri1, 
				    testsid.copy(), 
		       errorobject::ProtocolError, 
		       errorobject::err_CommonHeaderParseError,
		       errorobject::errsub_UnknownVersion, 
		       1,  // version
		       64, // hops
		       264,// length 
		       42, // nslpid
		       ntlp_pdu::S_Flag | ntlp_pdu::R_Flag | ntlp_pdu::E_Flag, //flags
		       known_ntlp_pdu::Query,
		       true, // dgram
		       true, // query encapsulation
		       NULL// object value info
		       //known_ntlp_object::QueryCookie // object type info
		       //0, // MTU info
		       //0  // MLI 
		       );

	  CPPUNIT_ASSERT (snderrtestobj.get_errorcode() == errorobject::err_CommonHeaderParseError);

	  test_basics(snderrtestobj);
  }

  void test_errobj_CommonHeaderParseError_UnknownType()
  {
	  mri_pathcoupled mri1(hostaddress("fe80::dead:beef"), 128,
			       hostaddress("2001:db8::bad:c0:ffee"), 64, true);

	  sessionid testsid(1,2,3,4);

	  errorobject snderrtestobj(&mri1, 
				    testsid.copy(), 
		       errorobject::ProtocolError, 
		       errorobject::err_CommonHeaderParseError,
		       errorobject::errsub_UnknownType, 
		       1,  // version
		       64, // hops
		       264,// length 
		       42, // nslpid
		       ntlp_pdu::S_Flag | ntlp_pdu::R_Flag | ntlp_pdu::E_Flag, //flags
		       known_ntlp_pdu::Query,
		       true, // dgram
		       true, // query encapsulation
		       NULL// object value info
		       //known_ntlp_object::QueryCookie // object type info
		       //0, // MTU info
		       //0  // MLI 
		       );
	  CPPUNIT_ASSERT (snderrtestobj.get_errorcode() == errorobject::err_CommonHeaderParseError);

	  test_basics(snderrtestobj);
  }

  void test_errobj_CommonHeaderParseError_InvalidRFlag()
  {
	  mri_pathcoupled mri1(hostaddress("fe80::dead:beef"), 128,
			       hostaddress("2001:db8::bad:c0:ffee"), 64, true);

	  sessionid testsid(1,2,3,4);

	  errorobject snderrtestobj(&mri1, 
				    testsid.copy(), 
		       errorobject::ProtocolError, 
		       errorobject::err_CommonHeaderParseError,
		       errorobject::errsub_InvalidRFlag, 
		       1,  // version
		       64, // hops
		       264,// length 
		       42, // nslpid
		       ntlp_pdu::S_Flag | ntlp_pdu::R_Flag | ntlp_pdu::E_Flag, //flags
		       known_ntlp_pdu::Query,
		       true, // dgram
		       true, // query encapsulation
		       NULL// object value info
		       //known_ntlp_object::QueryCookie // object type info
		       //0, // MTU info
		       //0  // MLI 
		       );

	  test_basics(snderrtestobj);
  }

  void test_errobj_CommonHeaderParseError_IncorrectMsgLength()
  {
	  mri_pathcoupled mri1(hostaddress("fe80::dead:beef"), 128,
			       hostaddress("2001:db8::bad:c0:ffee"), 64, true);

	  sessionid testsid(1,2,3,4);

	  errorobject snderrtestobj(&mri1, 
				    testsid.copy(), 
		       errorobject::ProtocolError, 
		       errorobject::err_CommonHeaderParseError,
		       errorobject::errsub_IncorrectMsgLength, 
		       1,  // version
		       64, // hops
		       264,// length 
		       42, // nslpid
		       ntlp_pdu::S_Flag | ntlp_pdu::R_Flag | ntlp_pdu::E_Flag, //flags
		       known_ntlp_pdu::Query,
		       true, // dgram
		       true, // query encapsulation
		       NULL,// object value info
		       errorobject::oti_not_set,//known_ntlp_object::QueryCookie // object type info
		       0, // MTU info
		       420  // MLI 
		       );

	  CPPUNIT_ASSERT (snderrtestobj.get_errorcode() == errorobject::err_CommonHeaderParseError);

	  test_basics(snderrtestobj);
  }


  /***
A.4.4.2.  Hop Limit Exceeded

   Class:              Permanent-Failure
   Code:               2
   Additional Info:    None

   This message is sent if a GIST node receives a message with a GIST
   hop count of zero, or a GIST node tries to forward a message after
   its GIST hop count has been decremented to zero on reception.  This
   message indicates either a routing loop or too small an initial hop
   count value.
  ***/

  void test_errobj_HopLimitExceededError()
  {
	  mri_pathcoupled mri1(hostaddress("fe80::dead:beef"), 128,
			       hostaddress("2001:db8::bad:c0:ffee"), 64, true);

	  sessionid testsid(1,2,3,4);

	  errorobject snderrtestobj(&mri1, 
				    testsid.copy(), 
		       errorobject::ProtocolError, 
		       errorobject::err_HopLimitExceeded,
		       errorobject::errsub_Default, 
		       1,  // version
		       64, // hops
		       264,// length 
		       42, // nslpid
		       ntlp_pdu::S_Flag | ntlp_pdu::R_Flag | ntlp_pdu::E_Flag, //flags
		       known_ntlp_pdu::Query,
		       true, // dgram
		       true, // query encapsulation
		       NULL// object value info
		       //known_ntlp_object::QueryCookie // object type info
		       //0, // MTU info
		       //0  // MLI 
		       );

	  CPPUNIT_ASSERT (snderrtestobj.get_errorcode() == errorobject::err_HopLimitExceeded);

	  test_basics(snderrtestobj);
  }

  /***
A.4.4.3.  Incorrect Encapsulation

   Class:              Protocol-Error
   Code:               3
   Additional Info:    None

   This message is sent if a GIST node receives a message which uses an
   incorrect encapsulation method (e.g. a Query arrives over an MA, or
   the Confirm for a handshake that sets up a messaging association
   arrives in D mode).
  ***/

  void test_errobj_IncorrectEncapsulationError()
  {
	  mri_pathcoupled mri1(hostaddress("fe80::dead:beef"), 128,
			       hostaddress("2001:db8::bad:c0:ffee"), 64, true);

	  sessionid testsid(1,2,3,4);

	  errorobject snderrtestobj(&mri1, 
				    testsid.copy(), 
		       errorobject::ProtocolError, 
		       errorobject::err_IncorrectEncapsulation,
		       errorobject::errsub_Default, 
		       1,  // version
		       64, // hops
		       264,// length 
		       42, // nslpid
		       ntlp_pdu::S_Flag | ntlp_pdu::R_Flag | ntlp_pdu::E_Flag, //flags
		       known_ntlp_pdu::Query,
		       true, // dgram
		       true, // query encapsulation
		       NULL// object value info
		       //known_ntlp_object::QueryCookie // object type info
		       //0, // MTU info
		       //0  // MLI 
		       );

	  CPPUNIT_ASSERT (snderrtestobj.get_errorcode() == errorobject::err_IncorrectEncapsulation);

	  test_basics(snderrtestobj);
  }

  /***
A.4.4.4.  Incorrectly Delivered Message

   Class:              Protocol-Error
   Code:               4
   Additional Info:    None

   This message is sent if a GIST node receives a message over an MA
   which is not associated with the MRI/NSLPID/SID combination in the

   message.
  ***/

  void test_errobj_IncorrectlyDeliveredMessageError()
  {
	  mri_pathcoupled mri1(hostaddress("fe80::dead:beef"), 128,
			       hostaddress("2001:db8::bad:c0:ffee"), 64, true);

	  sessionid testsid(1,2,3,4);

	  errorobject snderrtestobj(&mri1, 
				    testsid.copy(), 
		       errorobject::ProtocolError, 
		       errorobject::err_IncorrectlyDeliveredMessage,
		       errorobject::errsub_Default, 
		       1,  // version
		       64, // hops
		       264,// length 
		       42, // nslpid
		       ntlp_pdu::S_Flag | ntlp_pdu::R_Flag | ntlp_pdu::E_Flag, //flags
		       known_ntlp_pdu::Query,
		       true, // dgram
		       true, // query encapsulation
		       NULL// object value info
		       //known_ntlp_object::QueryCookie // object type info
		       //0, // MTU info
		       //0  // MLI 
		       );

	  CPPUNIT_ASSERT (snderrtestobj.get_errorcode() == errorobject::err_IncorrectlyDeliveredMessage);

	  test_basics(snderrtestobj);
  }

  /***
A.4.4.5.  No Routing State

   Class:              Protocol-Error
   Code:               5
   Additional Info:    None

   This message is sent if a node receives a message for which routing
   state should exist, but has not yet been created and thus there is no
   appropriate Querying-SM or Responding-SM.  This can occur on
   receiving a Data or Confirm message at a node whose policy requires
   routing state to exist before such messages can be accepted.  See
   also Section 6.1 and Section 6.3.

  ***/

  void test_errobj_NoRoutingStateError()
  {
	  mri_pathcoupled mri1(hostaddress("fe80::dead:beef"), 128,
			       hostaddress("2001:db8::bad:c0:ffee"), 64, true);

	  sessionid testsid(1,2,3,4);

	  errorobject snderrtestobj(&mri1, 
				    testsid.copy(), 
		       errorobject::ProtocolError, 
		       errorobject::err_NoRoutingState,
		       errorobject::errsub_Default, 
		       1,  // version
		       64, // hops
		       264,// length 
		       42, // nslpid
		       ntlp_pdu::S_Flag | ntlp_pdu::R_Flag | ntlp_pdu::E_Flag, //flags
		       known_ntlp_pdu::Query,
		       true, // dgram
		       true, // query encapsulation
		       NULL// object value info
		       //known_ntlp_object::QueryCookie // object type info
		       //0, // MTU info
		       //0  // MLI 
		       );

	  CPPUNIT_ASSERT (snderrtestobj.get_errorcode() == errorobject::err_NoRoutingState);

	  test_basics(snderrtestobj);
  }

  /***

A.4.4.6.  Unknown NSLPID

   Class:              Permanent-Failure
   Code:               6
   Additional Info:    None

   This message is sent if a router receives a directly addressed
   message for an NSLP which it does not support.

  ***/

  void test_errobj_UnknownNSLPIDError()
  {
	  mri_pathcoupled mri1(hostaddress("fe80::dead:beef"), 128,
			       hostaddress("2001:db8::bad:c0:ffee"), 64, true);

	  sessionid testsid(1,2,3,4);

	  errorobject snderrtestobj(&mri1, 
				    testsid.copy(), 
		       errorobject::ProtocolError, 
		       errorobject::err_UnknownNSLPID,
		       errorobject::errsub_Default, 
		       1,  // version
		       64, // hops
		       264,// length 
		       42, // nslpid
		       ntlp_pdu::S_Flag | ntlp_pdu::R_Flag | ntlp_pdu::E_Flag, //flags
		       known_ntlp_pdu::Query,
		       true, // dgram
		       true, // query encapsulation
		       NULL// object value info
		       //known_ntlp_object::QueryCookie // object type info
		       //0, // MTU info
		       //0  // MLI 
		       );

	  CPPUNIT_ASSERT (snderrtestobj.get_errorcode() == errorobject::err_UnknownNSLPID);

	  test_basics(snderrtestobj);
  }

  /***

A.4.4.7.  Endpoint Found

   Class:              Permanent-Failure
   Code:               7
   Additional Info:    None

   This message is sent if a GIST node at a flow endpoint receives a
   Query message for an NSLP which it does not support.

  ***/

  void test_errobj_EndpointFoundError()
  {
	  mri_pathcoupled mri1(hostaddress("fe80::dead:beef"), 128,
			       hostaddress("2001:db8::bad:c0:ffee"), 64, true);

	  sessionid testsid(1,2,3,4);

	  errorobject snderrtestobj(&mri1, 
				    testsid.copy(), 
		       errorobject::ProtocolError, 
		       errorobject::err_EndpointFound,
		       errorobject::errsub_Default, 
		       1,  // version
		       64, // hops
		       264,// length 
		       42, // nslpid
		       ntlp_pdu::S_Flag | ntlp_pdu::R_Flag | ntlp_pdu::E_Flag, //flags
		       known_ntlp_pdu::Query,
		       true, // dgram
		       true, // query encapsulation
		       NULL// object value info
		       //known_ntlp_object::QueryCookie // object type info
		       //0, // MTU info
		       //0  // MLI 
		       );

	  CPPUNIT_ASSERT (snderrtestobj.get_errorcode() == errorobject::err_EndpointFound);

	  test_basics(snderrtestobj);
  }

  /***

A.4.4.8.  Message Too Large

   Class:              Permanent-Failure
   Code:               8
   Additional Info:    MTU Info

   A router receives a message which it can't forward because it exceeds
   the IP MTU on the next or subsequent hops.

  ***/

  void test_errobj_MessageTooLarge()
  {
	  mri_pathcoupled mri1(hostaddress("fe80::dead:beef"), 128,
			       hostaddress("2001:db8::bad:c0:ffee"), 64, true);

	  sessionid testsid(1,2,3,4);

	  errorobject snderrtestobj(&mri1, 
				    testsid.copy(), 
		       errorobject::ProtocolError, 
		       errorobject::err_MessageTooLarge,
		       errorobject::errsub_Default, 
		       1,  // version
		       64, // hops
		       264,// length 
		       42, // nslpid
		       ntlp_pdu::S_Flag | ntlp_pdu::R_Flag | ntlp_pdu::E_Flag, //flags
		       known_ntlp_pdu::Query,
		       true, // dgram
		       true, // query encapsulation
		       NULL, // object value info
		       errorobject::oti_not_set, //known_ntlp_object::QueryCookie // object type info
		       4096 // MTU info
		       //0  // MLI 
		       );

	  CPPUNIT_ASSERT (snderrtestobj.get_errorcode() == errorobject::err_MessageTooLarge);

	  test_basics(snderrtestobj);
  }

  /***
A.4.4.9.  Object Type Error

   Class:              Protocol-Error
   Code:               9
   Additional Info:    Object Type Info

   This message is sent if a GIST node receives a message containing a
   TLV object with an invalid type.  The message indicates the object
   type at fault in the additional info field.  This error code is split
   into subcodes as follows:

   0: Duplicate Object:  This subcode is used if a GIST node receives a
      message containing multiple instances of an object which may only
      appear once in a message.  In the current specification, this
      applies to all objects.

   1: Unrecognised Object:  This subcode is used if a GIST node receives
      a message containing an object which it does not support, and the
      extensibility flags AB=00.

   2: Missing Object:  This subcode is used if a GIST node receives a
      message which is missing one or more mandatory objects.  This
      message is also sent if a Stack-Proposal is sent without a
      matching Stack-Configuration-Data object when one was necessary,
      or vice versa.

   3: Invalid Object Type:  This subcode is used if the object type is
      known, but it is not valid for this particular GIST message type.

   4: Untranslated Object:  This subcode is used if the object type is
      known and is mandatory to interpret, but it contains addressing
      data which has not been translated by an intervening NAT.

   5: Invalid Extensibility Flags:  This subcode is used if an object is
      received with the extensibility flags AB=11.

  ***/

  void test_errobj_ObjectTypeError_DuplicateObject()
  {
	  mri_pathcoupled mri1(hostaddress("fe80::dead:beef"), 128,
			       hostaddress("2001:db8::bad:c0:ffee"), 64, true);

	  sessionid testsid(1,2,3,4);

	  errorobject snderrtestobj(&mri1, 
				    testsid.copy(), 
		       errorobject::ProtocolError, 
		       errorobject::err_ObjectTypeError,
		       errorobject::errsub_DuplicateObject, 
		       1,  // version
		       64, // hops
		       264,// length 
		       42, // nslpid
		       ntlp_pdu::S_Flag | ntlp_pdu::R_Flag | ntlp_pdu::E_Flag, //flags
		       known_ntlp_pdu::Query,
		       true, // dgram
		       true, // query encapsulation
		       NULL, // object value info
		       known_ntlp_object::QueryCookie // object type info
		       //4096 // MTU info
		       //0  // MLI 
		       );

	  CPPUNIT_ASSERT (snderrtestobj.get_errorcode() == errorobject::err_ObjectTypeError);

	  test_basics(snderrtestobj);
  }

  void test_errobj_ObjectTypeError_UnrecognizedObject()
  {
	  mri_pathcoupled mri1(hostaddress("fe80::dead:beef"), 128,
			       hostaddress("2001:db8::bad:c0:ffee"), 64, true);

	  sessionid testsid(1,2,3,4);

	  errorobject snderrtestobj(&mri1, 
				    testsid.copy(), 
		       errorobject::ProtocolError, 
		       errorobject::err_ObjectTypeError,
		       errorobject::errsub_UnrecognizedObject, 
		       1,  // version
		       64, // hops
		       264,// length 
		       42, // nslpid
		       ntlp_pdu::S_Flag | ntlp_pdu::R_Flag | ntlp_pdu::E_Flag, //flags
		       known_ntlp_pdu::Query,
		       true, // dgram
		       true, // query encapsulation
		       NULL, // object value info
		       known_ntlp_object::QueryCookie // object type info
		       //4096 // MTU info
		       //0  // MLI 
		       );

	  CPPUNIT_ASSERT (snderrtestobj.get_errorcode() == errorobject::err_ObjectTypeError);

	  test_basics(snderrtestobj);
  }

  void test_errobj_ObjectTypeError_MissingObject()
  {
	  mri_pathcoupled mri1(hostaddress("fe80::dead:beef"), 128,
			       hostaddress("2001:db8::bad:c0:ffee"), 64, true);

	  sessionid testsid(1,2,3,4);

	  errorobject snderrtestobj(&mri1, 
				    testsid.copy(), 
		       errorobject::ProtocolError, 
		       errorobject::err_ObjectTypeError,
		       errorobject::errsub_MissingObject, 
		       1,  // version
		       64, // hops
		       264,// length 
		       42, // nslpid
		       ntlp_pdu::S_Flag | ntlp_pdu::R_Flag | ntlp_pdu::E_Flag, //flags
		       known_ntlp_pdu::Query,
		       true, // dgram
		       true, // query encapsulation
		       NULL, // object value info
		       known_ntlp_object::QueryCookie // object type info
		       //4096 // MTU info
		       //0  // MLI 
		       );

	  CPPUNIT_ASSERT (snderrtestobj.get_errorcode() == errorobject::err_ObjectTypeError);

	  test_basics(snderrtestobj);
  }

  void test_errobj_ObjectTypeError_InvalidObject()
  {
	  mri_pathcoupled mri1(hostaddress("fe80::dead:beef"), 128,
			       hostaddress("2001:db8::bad:c0:ffee"), 64, true);

	  sessionid testsid(1,2,3,4);

	  errorobject snderrtestobj(&mri1, 
				    testsid.copy(), 
		       errorobject::ProtocolError, 
		       errorobject::err_ObjectTypeError,
		       errorobject::errsub_InvalidObject, 
		       1,  // version
		       64, // hops
		       264,// length 
		       42, // nslpid
		       ntlp_pdu::S_Flag | ntlp_pdu::R_Flag | ntlp_pdu::E_Flag, //flags
		       known_ntlp_pdu::Query,
		       true, // dgram
		       true, // query encapsulation
		       NULL, // object value info
		       known_ntlp_object::QueryCookie // object type info
		       //4096 // MTU info
		       //0  // MLI 
		       );

	  CPPUNIT_ASSERT (snderrtestobj.get_errorcode() == errorobject::err_ObjectTypeError);

	  test_basics(snderrtestobj);
  }

  void test_errobj_ObjectTypeError_UntranslatedObject()
  {
	  mri_pathcoupled mri1(hostaddress("fe80::dead:beef"), 128,
			       hostaddress("2001:db8::bad:c0:ffee"), 64, true);

	  sessionid testsid(1,2,3,4);

	  errorobject snderrtestobj(&mri1, 
				    testsid.copy(), 
		       errorobject::ProtocolError, 
		       errorobject::err_ObjectTypeError,
		       errorobject::errsub_UntranslatedObject, 
		       1,  // version
		       64, // hops
		       264,// length 
		       42, // nslpid
		       ntlp_pdu::S_Flag | ntlp_pdu::R_Flag | ntlp_pdu::E_Flag, //flags
		       known_ntlp_pdu::Query,
		       true, // dgram
		       true, // query encapsulation
		       NULL, // object value info
		       known_ntlp_object::QueryCookie // object type info
		       //4096 // MTU info
		       //0  // MLI 
		       );

	  CPPUNIT_ASSERT (snderrtestobj.get_errorcode() == errorobject::err_ObjectTypeError);

	  test_basics(snderrtestobj);
  }

  void test_errobj_ObjectTypeError_InvalidExtensibilityFlags()
  {
	  mri_pathcoupled mri1(hostaddress("fe80::dead:beef"), 128,
			       hostaddress("2001:db8::bad:c0:ffee"), 64, true);

	  sessionid testsid(1,2,3,4);

	  errorobject snderrtestobj(&mri1, 
				    testsid.copy(), 
		       errorobject::ProtocolError, 
		       errorobject::err_ObjectTypeError,
		       errorobject::errsub_InvalidExtensibilityFlags, 
		       1,  // version
		       64, // hops
		       264,// length 
		       42, // nslpid
		       ntlp_pdu::S_Flag | ntlp_pdu::R_Flag | ntlp_pdu::E_Flag, //flags
		       known_ntlp_pdu::Query,
		       true, // dgram
		       true, // query encapsulation
		       NULL, // object value info
		       known_ntlp_object::QueryCookie // object type info
		       //4096 // MTU info
		       //0  // MLI 
		       );

	  CPPUNIT_ASSERT (snderrtestobj.get_errorcode() == errorobject::err_ObjectTypeError);

	  test_basics(snderrtestobj);
  }

  /***
A.4.4.10.  Object Value Error

   Class:              Protocol-Error
   Code:               10
   Additional Info:    1 or 2 Object Value Info fields as given below

   This message is sent if a node receives a message containing an
   object which cannot be properly parsed.  The error message contains a
   single Object Value Info object, except for subcode 5 as stated
   below.  This error code is split into subcodes as follows:

   0: Incorrect Length:  The overall length does not match the object
      length calculated from the object contents.

   1: Value Not Supported:  The value of a field is not supported by the
      GIST node.

   2: Invalid Flag-Field Combination:  An object contains an invalid
      combination of flags and/or fields.  At the moment this only
      relates to the Path-Coupled MRI (Appendix A.3.1.1), but in future
      there may be more.

   3: Empty List:  At the moment this only relates to Stack-Proposals.
      The error message is sent if a stack proposal with a length > 0
      contains only null bytes (a length of 0 is handled as "Value Not
      Supported").

   4: Invalid Cookie:  The message contains a cookie which could not be
      verified by the node.

   5: Stack-Proposal - Stack-Configuration-Data Mismatch:  This subcode
      is used if a GIST node receives a message in which the data in the
      Stack-Proposal object is inconsistent with the information in the
      Stack Configuration Data object.  In this case, both the Stack-
      Proposal object and Stack-Configuration-Data object MUST be
      included in separate Object Value Info fields in that order.

  ***/

  void test_errobj_ObjectValueError_IncorrectLength()
  {
	  mri_pathcoupled mri1(hostaddress("fe80::dead:beef"), 128,
			       hostaddress("2001:db8::bad:c0:ffee"), 64, true);

	  sessionid testsid(1,2,3,4);

	  errorobject snderrtestobj(&mri1, 
				    testsid.copy(), 
		       errorobject::ProtocolError, 
		       errorobject::err_ObjectValueError,
		       errorobject::errsub_IncorrectLength, 
		       1,  // version
		       64, // hops
		       264,// length 
		       42, // nslpid
		       ntlp_pdu::S_Flag | ntlp_pdu::R_Flag | ntlp_pdu::E_Flag, //flags
		       known_ntlp_pdu::Query,
		       true, // dgram
		       true, // query encapsulation
		       NULL, // object value info
		       errorobject::oti_not_set, //known_ntlp_object::QueryCookie // object type info
		       4096, // MTU info
		       420  // MLI
		       );

	  CPPUNIT_ASSERT (snderrtestobj.get_errorcode() == errorobject::err_ObjectValueError);

	  test_basics(snderrtestobj);
  }

  void test_errobj_ObjectValueError_ValueNotSupported()
  {
	  mri_pathcoupled mri1(hostaddress("fe80::dead:beef"), 128,
			       hostaddress("2001:db8::bad:c0:ffee"), 64, true);

	  sessionid testsid(1,2,3,4);

	  errorobject snderrtestobj(&mri1, 
				    testsid.copy(), 
		       errorobject::ProtocolError, 
		       errorobject::err_ObjectValueError,
		       errorobject::errsub_ValueNotSupported, 
		       1,  // version
		       64, // hops
		       264,// length 
		       42, // nslpid
		       ntlp_pdu::S_Flag | ntlp_pdu::R_Flag | ntlp_pdu::E_Flag, //flags
		       known_ntlp_pdu::Query,
		       true, // dgram
		       true, // query encapsulation
		       NULL // object value info
		       //known_ntlp_object::oti_not_set, // object type info
		       //4096, // MTU info
		       //420  // MLI
		       );

	  CPPUNIT_ASSERT (snderrtestobj.get_errorcode() == errorobject::err_ObjectValueError);

	  test_basics(snderrtestobj);
  }

  void test_errobj_ObjectValueError_InvalidFlagFieldCombination()
  {
	  mri_pathcoupled mri1(hostaddress("fe80::dead:beef"), 128,
			       hostaddress("2001:db8::bad:c0:ffee"), 64, true);

	  sessionid testsid(1,2,3,4);

	  errorobject snderrtestobj(&mri1, 
				    testsid.copy(), 
		       errorobject::ProtocolError, 
		       errorobject::err_ObjectValueError,
		       errorobject::errsub_InvalidFlagFieldCombination,
		       1,  // version
		       64, // hops
		       264,// length 
		       42, // nslpid
		       ntlp_pdu::S_Flag | ntlp_pdu::R_Flag | ntlp_pdu::E_Flag, //flags
		       known_ntlp_pdu::Query,
		       true, // dgram
		       true, // query encapsulation
		       NULL // object value info
		       //known_ntlp_object::oti_not_set, // object type info
		       //4096, // MTU info
		       //420  // MLI
		       );

	  CPPUNIT_ASSERT (snderrtestobj.get_errorcode() == errorobject::err_ObjectValueError);

	  test_basics(snderrtestobj);
  }

  void test_errobj_ObjectValueError_EmptyList()
  {
	  mri_pathcoupled mri1(hostaddress("fe80::dead:beef"), 128,
			       hostaddress("2001:db8::bad:c0:ffee"), 64, true);

	  sessionid testsid(1,2,3,4);

	  errorobject snderrtestobj(&mri1, 
				    testsid.copy(), 
		       errorobject::ProtocolError, 
		       errorobject::err_ObjectValueError,
		       errorobject::errsub_EmptyList,
		       1,  // version
		       64, // hops
		       264,// length 
		       42, // nslpid
		       ntlp_pdu::S_Flag | ntlp_pdu::R_Flag | ntlp_pdu::E_Flag, //flags
		       known_ntlp_pdu::Query,
		       true, // dgram
		       true, // query encapsulation
		       NULL // object value info
		       //known_ntlp_object::oti_not_set, // object type info
		       //4096, // MTU info
		       //420  // MLI
		       );

	  CPPUNIT_ASSERT (snderrtestobj.get_errorcode() == errorobject::err_ObjectValueError);

	  test_basics(snderrtestobj);
  }

  void test_errobj_ObjectValueError_InvalidCookie()
  {
	  mri_pathcoupled mri1(hostaddress("10.0.0.1"), 32,
			       hostaddress("10.0.0.2"), 20, true);

	  sessionid testsid(1,2,3,4);
	  const char* buf="12345-querycookietest-67890";
	  querycookie qcookie(reinterpret_cast<const unsigned char*>(buf),sizeof(buf));

	  errorobject snderrtestobj(&mri1, 
				    testsid.copy(), 
		       errorobject::ProtocolError, 
		       errorobject::err_ObjectValueError, 
		       errorobject::errsub_InvalidCookie, 
		       1,  // version
		       64, // hops
		       264,// length 
		       42, // nslpid
		       ntlp_pdu::S_Flag | ntlp_pdu::R_Flag | ntlp_pdu::E_Flag, //flags
		       known_ntlp_pdu::Query,
		       true, // dgram
		       true, // query encapsulation
		       qcookie.copy() // object value info
				    //known_ntlp_object::QueryCookie // object type info
				    //0, // MTU info
				    //0  // MLI 
		       );

	  CPPUNIT_ASSERT (snderrtestobj.get_errorcode() == errorobject::err_ObjectValueError);

	  test_basics(snderrtestobj);
  }

  void test_errobj_ObjectValueError_SP_SCD_Mismatch()
  {
	  mri_pathcoupled mri1(hostaddress("fe80::dead:beef"), 128,
			       hostaddress("2001:db8::bad:c0:ffee"), 64, true);

	  sessionid testsid(1,2,3,4);

	  errorobject snderrtestobj(&mri1, 
				    testsid.copy(), 
		       errorobject::ProtocolError, 
		       errorobject::err_ObjectValueError,
		       errorobject::errsub_SP_SCD_Mismatch,
		       1,  // version
		       64, // hops
		       264,// length 
		       42, // nslpid
		       ntlp_pdu::S_Flag | ntlp_pdu::R_Flag | ntlp_pdu::E_Flag, //flags
		       known_ntlp_pdu::Query,
		       true, // dgram
		       true, // query encapsulation
		       NULL // object value info
		       //known_ntlp_object::oti_not_set, // object type info
		       //4096, // MTU info
		       //420  // MLI
		       );

	  CPPUNIT_ASSERT (snderrtestobj.get_errorcode() == errorobject::err_ObjectValueError);

	  test_basics(snderrtestobj);
  }

  /***
A.4.4.11.  Invalid IP layer TTL

   Class:              Permanent-Failure
   Code:               11
   Additional Info:    None

   This error indicates that a message was received with an IP layer TTL
   outside an acceptable range; for example, that an upstream Query was
   received with an IP layer TTL of less than 254 (i.e. more than one IP
   hop from the sender).  The actual IP distance can be derived from the
   IP-TTL information in the NLI object carried in the same message.

  ***/

  void test_errobj_InvalidIPTTL()
  {
	  mri_pathcoupled mri1(hostaddress("fe80::dead:beef"), 128,
			       hostaddress("2001:db8::bad:c0:ffee"), 64, true);

	  sessionid testsid(1,2,3,4);

	  errorobject snderrtestobj(&mri1, 
				    testsid.copy(), 
		       errorobject::ProtocolError, 
		       errorobject::err_InvalidIPTTL,
		       errorobject::errsub_Default,
		       1,  // version
		       64, // hops
		       264,// length 
		       42, // nslpid
		       ntlp_pdu::S_Flag | ntlp_pdu::R_Flag | ntlp_pdu::E_Flag, //flags
		       known_ntlp_pdu::Query,
		       true, // dgram
		       true, // query encapsulation
		       NULL // object value info
		       //known_ntlp_object::oti_not_set, // object type info
		       //4096, // MTU info
		       //420  // MLI
		       );

	  CPPUNIT_ASSERT (snderrtestobj.get_errorcode() == errorobject::err_InvalidIPTTL);

	  test_basics(snderrtestobj);
  }

  /***
A.4.4.12.  MRI Validation Failure

   Class:              Permanent-Failure
   Code:               12
   Additional Info:    Object Value Info

   This error indicates that a message was received with an MRI that
   could not be accepted, e.g. because of too much wildcarding or
   failing some validation check (cf. Section 5.8.1.2).  The Object
   Value Info includes the MRI so the error originator can indicate the
   part of the MRI which caused the problem.  The error code is divided
   into subcodes as follows:

   0: MRI Too Wild:  The MRI contained too much wildcarding (e.g. too
      short a destination address prefix) to be forwarded correctly down
      a single path.

   1: IP Version Mismatch:  The MRI in a path-coupled Query message
      refers to an IP version which is not implemented on the interface
      used, or is different from the IP version of the Query
      encapsulation (see Section 7.4).

   2: Ingress Filter Failure:  The MRI in a path-coupled Query message
      describes a flow which would not pass ingress filtering on the
      interface used.

  ***/
  
  void test_errobj_MRIValidationFailure_MRITooWild()
  {
	  mri_pathcoupled mri1(hostaddress("fe80::dead:beef"), 128,
			       hostaddress("2001:db8::bad:c0:ffee"), 64, true);

	  sessionid testsid(1,2,3,4);

	  errorobject snderrtestobj(&mri1, 
				    testsid.copy(), 
		       errorobject::ProtocolError, 
		       errorobject::err_MRIValidationFailure,
		       errorobject::errsub_MRITooWild,
		       1,  // version
		       64, // hops
		       264,// length 
		       42, // nslpid
		       ntlp_pdu::S_Flag | ntlp_pdu::R_Flag | ntlp_pdu::E_Flag, //flags
		       known_ntlp_pdu::Query,
		       true, // dgram
		       true, // query encapsulation
		       NULL // object value info
		       //known_ntlp_object::oti_not_set, // object type info
		       //4096, // MTU info
		       //420  // MLI
		       );

	  CPPUNIT_ASSERT (snderrtestobj.get_errorcode() == errorobject::err_MRIValidationFailure);

	  test_basics(snderrtestobj);
  }

  void test_errobj_MRIValidationFailure_IPVersionMismatch()
  {
	  mri_pathcoupled mri1(hostaddress("fe80::dead:beef"), 128,
			       hostaddress("2001:db8::bad:c0:ffee"), 64, true);

	  sessionid testsid(1,2,3,4);

	  errorobject snderrtestobj(&mri1, 
				    testsid.copy(), 
		       errorobject::ProtocolError, 
		       errorobject::err_MRIValidationFailure,
		       errorobject::errsub_IPVersionMismatch,
		       1,  // version
		       64, // hops
		       264,// length 
		       42, // nslpid
		       ntlp_pdu::S_Flag | ntlp_pdu::R_Flag | ntlp_pdu::E_Flag, //flags
		       known_ntlp_pdu::Query,
		       true, // dgram
		       true, // query encapsulation
		       NULL // object value info
		       //known_ntlp_object::oti_not_set, // object type info
		       //4096, // MTU info
		       //420  // MLI
		       );

	  CPPUNIT_ASSERT (snderrtestobj.get_errorcode() == errorobject::err_MRIValidationFailure);

	  test_basics(snderrtestobj);
  }

  void test_errobj_MRIValidationFailure_IngressFilterFailure()
  {
	  mri_pathcoupled mri1(hostaddress("fe80::dead:beef"), 128,
			       hostaddress("2001:db8::bad:c0:ffee"), 64, true);

	  sessionid testsid(1,2,3,4);

	  errorobject snderrtestobj(&mri1,
				    testsid.copy(),
		       errorobject::ProtocolError,
		       errorobject::err_MRIValidationFailure,
		       errorobject::errsub_IngressFilterFailure,
		       1,  // version
		       64, // hops
		       264,// length
		       42, // nslpid
		       ntlp_pdu::S_Flag | ntlp_pdu::R_Flag | ntlp_pdu::E_Flag, //flags
		       known_ntlp_pdu::Query,
		       true, // dgram
		       true, // query encapsulation
		       NULL // object value info
		       //known_ntlp_object::oti_not_set, // object type info
		       //4096, // MTU info
		       //420  // MLI
		       );

	  CPPUNIT_ASSERT (snderrtestobj.get_errorcode() == errorobject::err_MRIValidationFailure);

	  test_basics(snderrtestobj);
  }


  void test_basics(const errorobject& snderrtestobj) 
  {

	  errorobject rcverrtestobj;

	  NetMsg netmsg(5000);
	  NetMsg netmsg_cpy(5000);
	  // must subtract common header
	  size_t serialized_size=  snderrtestobj.get_serialized_size(IE::protocol_v1) - 4;
	  uint32 written_bytes= 0;
	  snderrtestobj.serializeEXT(netmsg, IE::protocol_v1, written_bytes, false);

	  // number of written_bytes correct?
	  CPPUNIT_ASSERT( written_bytes == serialized_size );

	  // Copy constructor test
	  written_bytes= 0;
	  errorobject cpyerrtestobj(snderrtestobj);
	  CPPUNIT_ASSERT(snderrtestobj == cpyerrtestobj);

	  // serialization reproducable?
	  cpyerrtestobj.serializeEXT(netmsg_cpy, IE::protocol_v1, written_bytes, false);
	  CPPUNIT_ASSERT( memcmp(netmsg.get_buffer(), netmsg_cpy.get_buffer(), written_bytes) == 0 );

	  // assignment operator
	  cpyerrtestobj= snderrtestobj;
	  CPPUNIT_ASSERT(snderrtestobj == cpyerrtestobj);

	  // serialize again, this time with header
	  written_bytes= 0;
	  netmsg.to_start();
	  snderrtestobj.serialize(netmsg, IE::protocol_v1, written_bytes);

	  // deserialize
	  IEErrorList ieerrlist;
	  uint32 read_bytes= 0;
	  netmsg.to_start();
	  rcverrtestobj.deserialize(netmsg, IE::protocol_v1, ieerrlist, read_bytes, false);

	  CPPUNIT_ASSERT( snderrtestobj == rcverrtestobj );
	}
};

CPPUNIT_TEST_SUITE_REGISTRATION( errorobject_test );
