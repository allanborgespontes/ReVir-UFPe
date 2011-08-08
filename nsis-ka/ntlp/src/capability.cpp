/// ----------------------------------------*- mode: C++; -*--
/// @file capability.cpp
/// Determines GIST capabilities and defines stack profiles
/// ----------------------------------------------------------
/// $Id: capability.cpp 5924 2011-02-23 23:34:08Z bless $
/// $HeadURL: https://svn.tm.kit.edu/nsis/ntlp/trunk/src/capability.cpp $
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
#include <algorithm>
#include "capability.h"

#include "gist_conf.h" // required to to access to config parameters

  using namespace ntlp;
  using namespace protlib;
  using namespace protlib::log;

// constructor, sets up all the data about connection modes of this node
capability::capability(uint16 udpport, uint16 tcpport, uint16 tlsport, uint16 sctpport)
{
  DLog("capability", "Using ports: UDP=" << udpport << " TCP=" << tcpport << " TLS/TCP=" << tlsport << " SCTP=" << sctpport); 

  //##########################################################
  //
  // Fill "reliable" profiles/configurations
  //
  //##########################################################

  ma_protocol_option* maopt_tmp=NULL;
  ma_protocol_option* query_maopt_tmp=NULL;
  stackprofile* stackprofile_tmp= NULL;
  // index starts from 1, 0 means "all profiles"
  uint16 profindex= 1;
  uint16 secureprofindex= 1;

#ifdef _USE_SCTP
  // 1st: SCTP @ port tcpport ( "3" == SCTP )
  stackprofile_tmp=new stackprofile;
  stackprofile_tmp->addprotocol(stackprofile::MA_Protocol_ID_SCTP);
  stackprops.push_back(stackprofile_tmp);
  maopt_tmp=new ma_protocol_option(stackprofile::MA_Protocol_ID_SCTP,profindex,false);
  query_maopt_tmp=new ma_protocol_option(*maopt_tmp);
  querystackconfs.push_back(query_maopt_tmp); // omit options data for query
  maopt_tmp->add16(sctpport); // add port number for response
  stackconfs.push_back(maopt_tmp);
  profindex++;
#endif

  // TPC @ port tcpport ( "1" == TCP )

  stackprofile_tmp=new stackprofile;
  stackprofile_tmp->addprotocol(stackprofile::MA_Protocol_ID_FORWARDS_TCP);
  stackprops.push_back(stackprofile_tmp); 
  maopt_tmp=new ma_protocol_option(stackprofile::MA_Protocol_ID_FORWARDS_TCP,profindex,false);
  query_maopt_tmp=new ma_protocol_option(*maopt_tmp);
  querystackconfs.push_back(query_maopt_tmp); // omit options data for query
  maopt_tmp->add16(tcpport); // add port number for response
  stackconfs.push_back(maopt_tmp);
  profindex++;
	
  //#########################################################
  //
  // Fill "secure" profiles/configurations
  //
  //#########################################################


  // Support TLS over TCP

  stackprofile_tmp=new stackprofile;
  stackprofile_tmp->addprotocol(stackprofile::MA_Protocol_ID_TLS);

  stackprofile_tmp->addprotocol(stackprofile::MA_Protocol_ID_FORWARDS_TCP);
  stackprops.push_back(stackprofile_tmp);

  stackprofile* secstackprofile_tmp= new stackprofile(*stackprofile_tmp);
  securestackprops.push_back(secstackprofile_tmp);

  maopt_tmp=new ma_protocol_option(stackprofile::MA_Protocol_ID_FORWARDS_TCP,profindex,false);
  query_maopt_tmp=new ma_protocol_option(*maopt_tmp);
  querystackconfs.push_back(query_maopt_tmp); // omit options data for query
  maopt_tmp->add16(tlsport);
  stackconfs.push_back(maopt_tmp);
  profindex++;

  maopt_tmp=new ma_protocol_option(stackprofile::MA_Protocol_ID_FORWARDS_TCP,secureprofindex,false);
  query_maopt_tmp=new ma_protocol_option(*maopt_tmp);
  querysecurestackconfs.push_back(query_maopt_tmp); // omit options data for query
  maopt_tmp->add16(tlsport);
  securestackconfs.push_back(maopt_tmp);
  secureprofindex++;


  //#########################################################
  //
  // Fill vectors with IP protocol IDs
  //
  //#########################################################

  //secureprotocols.push_back(0);

  // TCP
  reliableprotocols.push_back(prot_tcp);

#ifdef _USE_SCTP
  reliableprotocols.push_back(prot_sctp);
#endif

  // TLSTCP Pseudo-Protocol-ID of Framework
  reliableprotocols.push_back(prot_tls_tcp);
  secureprotocols.push_back(prot_tls_tcp);

  EVLog("GIST Processing", "Available Protocol configurations initialized");

}


capability::~capability()
{
	// release all previously allocated vectors
        for_each (securestackprops.begin(), securestackprops.end(), delete_stackprofile );
	for_each (securestackconfs.begin(), securestackconfs.end(), delete_ma_protocol_option );
	for_each (querysecurestackconfs.begin(), querysecurestackconfs.end(), delete_ma_protocol_option );

	for_each (stackprops.begin(), stackprops.end(), delete_stackprofile );
	for_each (stackconfs.begin(), stackconfs.end(), delete_ma_protocol_option );
	for_each (querystackconfs.begin(), querystackconfs.end(),delete_ma_protocol_option );
}


// returns an address usable for sending via TP modules in C-Mode
// @return NULL if no suitable address could be found, otherwise a suitable target address
appladdress* 
capability::get_address(const stackprop* sp, const stack_conf_data* sc, const nli* nl, bool secure) 
{
  appladdress* addr = new appladdress;

  uint32 index;
  bool found_valid_protocol= false;
  
  if (!secure) 
  {
    // if SCTP enabled and allowed to advertise, use it
    if (gconf.getpar<bool>(gistconf_advertise_sctp))
    {
      if (sp->find_profile(stackprofile::MA_Protocol_ID_SCTP, index)) 
      {
	DLog("capability", "Setting up SCTP as protocol for C-Mode");
      
	// SCTP is the topmost and the only member
	addr->set_protocol(prot_sctp);
	
	DLog("capability", "Looking up configuration data for stack profile at index " << index);
	
	const ma_protocol_option* my_prot_opt = NULL;
	
	// Now set the port (get it from ma_protocol_option, match protocol id and applicable number)
	for (unsigned int i=0; i<sc->length(); i++) {
	  my_prot_opt = sc->get_protoption(i);
	  if ((my_prot_opt->get_protocol() == stackprofile::MA_Protocol_ID_SCTP) && (my_prot_opt->get_profile() == index)) 
	  {
	    // consider the D-Flag (do not use MA)
	    if (my_prot_opt->get_DoNotUseMA())
	    {
	      DLog("capability", "D-Flag set, not allowed to use SCTP");
	    }
	    else
	    {
	      addr->set_port(my_prot_opt->get16(0));
	      DLog("capability", "Succeeded in constructing matching target address from profile, chose SCTP port#" << addr->get_port());
	      found_valid_protocol= true;
	    }
	  }
	} // end for
      
	// Now set the IP address
	if (found_valid_protocol)
	{
	  addr->set_ip(nl->get_if_address());
	  return addr;
	}
      }
    } // if sctp enable


    // if not secure, use TCP if applicable. "1" == GIST number for forwards-TCP
    // find_profile returns the profile where FORWARDS_TCP is the first member
    if (sp->find_profile(stackprofile::MA_Protocol_ID_FORWARDS_TCP, index)) 
    {
      DLog("capability", "Setting up TCP as protocol for C-Mode");
      
      // TCP is the topmost and the only member
      addr->set_protocol(prot_tcp);
      
      DLog("capability", "Looking up configuration data for stack profile at index " << index);
      
      const ma_protocol_option* my_prot_opt = NULL;
      
      // Now set the port (get it from ma_protocol_option, match protocol id and applicable number)
      for (unsigned int i=0; i<sc->length(); i++) {
	my_prot_opt = sc->get_protoption(i);
	if ((my_prot_opt->get_protocol() == stackprofile::MA_Protocol_ID_FORWARDS_TCP) && (my_prot_opt->get_profile() == index)) 
	{
	  // consider the D-Flag (do not use MA)
	  if (my_prot_opt && my_prot_opt->get_DoNotUseMA())
	  {
	    DLog("capability", "D-Flag set, not allowed to use TCP");
	  }
	  else
	  {	  
	    addr->set_port(my_prot_opt->get16(0));
	    DLog("capability", "Succeeded in constructing matching target address from profile, chose TCP port#" << addr->get_port());
	    found_valid_protocol= true;
	  }
	}
      } // end for
      
      // Now set the IP address
      addr->set_ip(nl->get_if_address());
    } else {
      ERRCLog("capability", "Could not set IP for TCP operation");
    }
  } 

  if ( !found_valid_protocol )
  { // secure C-mode required or no unsecure protocol available
    if (sp->find_profile(stackprofile::MA_Protocol_ID_TLS, index)) 
    {
      DLog("capability", "Setting up TLS as protocol for C-Mode");
      
      // TLS is the topmost member
      addr->set_protocol(prot_tls_tcp);
      
      DLog("capability", "Looking up configuration data for stack profile at index " << index);
      const ma_protocol_option* my_prot_opt = NULL;
      
      // Now set the port for TCP over TLS (get it from ma_protocol_option, match protocol id and applicable number)
      for (unsigned int i=0; i<sc->length(); i++) 
      {
	my_prot_opt = sc->get_protoption(i);

	if ((my_prot_opt->get_protocol() == stackprofile::MA_Protocol_ID_FORWARDS_TCP) && (my_prot_opt->get_profile() == index)) {

	  // consider the D-Flag (do not use MA)
	  if (my_prot_opt && my_prot_opt->get_DoNotUseMA())
	  {
	    DLog("capability", "D-Flag set, not allowed to use TLS/TCP");
	  }
	  else
	  {	  
	    addr->set_port(my_prot_opt->get16(0));
	    DLog("capability", "Succeeded in constructing matching target address from profile, chose TLS/TCP port#" << addr->get_port());
	    found_valid_protocol= true;
	  }
	}
      } // end for
      
      // Now set the IP address
      addr->set_ip(nl->get_if_address());
    } 
    else 
    {
      ERRCLog("capability", "Could not find TLS profile - IP address not set for TLS operation");
    }
    
  } // end else
  
  if (addr->get_port() == 0)
      ERRCLog("capability", "Port # is 0");

  if (found_valid_protocol)
  {
    return addr;  
  }
  else
  {
    ERRCLog("capability", "Could not determine a suitable transport protocol");
    delete addr;
    return NULL;
  }
}


// create a stack proposal out of our capabilities
stackprop* 
capability::query_stackprop(bool secure) 
{ 
  stackprop* sp = new stackprop;

  // do not propose unsecure protocols if security is required
  if (!secure)
  {
    for (unsigned int i = 0; i<stackprops.size();i++) 
    {
	    if (stackprops[i])
	    {
		    // only add SCTP if configured
		    if ( stackprops[i]->length() > 0 && stackprops[i]->prof_vec[0] == stackprofile::MA_Protocol_ID_SCTP )
		    {
			    if (gconf.getpar<bool>(gistconf_advertise_sctp))
			    {
				    //DLog("capability", "query_stackprop(): including SCTP");
				    sp->add_profile(*stackprops[i]);
			    }
			    else
			    {
				    //DLog("capability", "query_stackprop(): excluding SCTP, because advertising disabled by config");
			    }
		    }
		    else
		    { // no restrictions on other protocols so far
			    sp->add_profile(*stackprops[i]);
		    }
	    }
    } // end for
  }
  else   // secure protocols can always be proposed
  for (unsigned int i = 0; i<securestackprops.size();i++) 
  {
    if (securestackprops[i])
      sp->add_profile(*securestackprops[i]);
  }

  return sp;
}


// create stack configuation data
stack_conf_data* 
capability::query_stackconf(bool secure) 
{	
  stack_conf_data* sc = new stack_conf_data;

  // do not propose unsecure protocols if security is required
  if (!secure)
  {
    // for forwards TCP it is define in sec 5.7.2 that there should be no config data
    // apart from field header (i.e. no options data is present)
    for (unsigned int i = 0; i<querystackconfs.size(); i++)
    {
      if (querystackconfs[i])
      {
	      if (querystackconfs[i]->get_protocol() == stackprofile::MA_Protocol_ID_SCTP)
	      {
		      if (gconf.getpar<bool>(gistconf_advertise_sctp))
		      {
			      sc->add_protoption(*querystackconfs[i]);
		      }
		      else
		      {
			      //DLog("capability", "query_stackconf(): excluding SCTP, because advertising disabled by config");
		      }
	      }
	      else
	      {
		      sc->add_protoption(*querystackconfs[i]);
	      }
      }
    } // endfor
  }
  else
  {
    // secure protocols can always be proposed
    for (unsigned int i = 0; i<querysecurestackconfs.size(); i++)
    {
      if (querysecurestackconfs[i])
	sc->add_protoption(*querysecurestackconfs[i]);
    } // endfor
  }

  sc->set_ma_hold_time(gconf.getpar<uint32>(gistconf_ma_hold_time)); // ma_hold_time par is in ms
  return sc;
  
}


// create a responder stack proposal
// @return newly allocated responder stack proposal
stackprop* 
capability::resp_stackprop(const stackprop* sp, bool secure) const
{
  
  stackprop* sp2=new stackprop;
  if (!secure)
  {
    for (unsigned int i = 0; i<stackprops.size();i++)
    {
      if (stackprops[i])
      {
	      // only add SCTP if configured
	      if (stackprops[i]->length() > 0 && stackprops[i]->prof_vec[0] == stackprofile::MA_Protocol_ID_SCTP )
	      {
		      if (gconf.getpar<bool>(gistconf_advertise_sctp))
			      sp2->add_profile(*stackprops[i]);			    
	      }
	      else
	      { // no restrictions on other protocols so far
		      sp2->add_profile(*stackprops[i]);
	      }
      }
    } // end for
  }
  else
  {
    for (unsigned int i = 0; i<securestackprops.size();i++)
    {
      if (securestackprops[i])
	sp2->add_profile(*securestackprops[i]);
    }
  }
  return sp2;
}

// create responder stack configuration data
// @param sc existing stack configuration data which is used to put the result in
//           may be NULL (then a new one is allocated)
stack_conf_data* 
capability::resp_stackconf(stack_conf_data* sc, bool secure) const
{

  stack_conf_data* sc2 = sc?sc:new stack_conf_data;
  if (!secure)
  {
    for (unsigned int i = 0; i<stackconfs.size(); i++) 
    {
	    if (stackconfs[i])
	    {
		    if (stackconfs[i]->get_protocol() == stackprofile::MA_Protocol_ID_SCTP)
		    {
			    if (gconf.getpar<bool>(gistconf_advertise_sctp))
			    {
				    sc2->add_protoption(*stackconfs[i]);
			    }
		    }
		    else
		    {
			    sc2->add_protoption(*stackconfs[i]);
		    }
		    
	    }
    } // end for
  }
  else
  {
    for (unsigned int i = 0; i<securestackconfs.size(); i++) 
    {
      if (securestackconfs[i])
	sc2->add_protoption(*securestackconfs[i]);
    }
  }

  sc2->set_ma_hold_time(gconf.getpar<uint32>(gistconf_ma_hold_time)); // ma_hold_time par is in ms
  return sc2;
  
}

// returns true, if it is feasible that we have issued this stack proposal
bool 
capability::accept_proposal(const stackprop* sp, bool secure) const
{
  bool success= false;
  // create own stack proposal
  stackprop* ourproposal= resp_stackprop(NULL, secure);

  if (sp && ourproposal && *ourproposal == *sp)
  {
    success= true;
  }

  if (ourproposal)
    delete ourproposal;

  return success;
}



// returns true, if the protocol is defined as "secure"
// please note that this returns false if secureprotocols is empty
bool 
capability::is_secure(const appladdress* addr) const
{
  
  bool secure= false;
  protocol_t proto = addr->get_protocol();
  
  for (unsigned int i=0; i<secureprotocols.size(); i++) {
    if (secureprotocols[i]==proto) secure=true;
  }
  
  return secure;
  
}


/**
 * check whether we know the current protocol as being reliable
 * @return true, if the protocol is defined as being "reliable"
 **/
bool 
capability::is_reliable(const appladdress* addr) const
{
  
  bool reliable= false;
  protocol_t proto = addr->get_protocol();
  
  for (unsigned int i=0; i<reliableprotocols.size(); i++) {
    if (reliableprotocols[i]==proto) reliable=true;
  }
  
  return reliable;
  
}

/** compares a stack profile and stack configuration data
 *  and checks for consistency, e.g., whether a stack profile 
 *  referenced within a stack configuration data section is 
 *  really available
 */
bool
capability::is_consistent(const stackprop* sp, const stack_conf_data* sc)
{
    if ( sp == NULL || sc == NULL )
		return true;

    const ma_protocol_option* ma_prot_opt= NULL;
    const stackprofile* stackprof= NULL;
    uint8 profile_index= 0;
    // the following checks whether protocols at the 
    // stack conf and stack profile are the same at the same index
    for (unsigned int i = 0; i<sc->length(); i++)
    {
	    ma_prot_opt= sc->get_protoption(i);
	    if ( ma_prot_opt )
	    {
		    profile_index= ma_prot_opt->get_profile();
		    if (profile_index > 0)
		    {
			    stackprof= sp->get_profile(profile_index-1);
			    bool found_protid= false;
			    for (unsigned int j= 0; j<stackprof->prof_vec.size(); j++)
				    if ( stackprof && ma_prot_opt->get_protocol() == stackprof->prof_vec.at(j) )
				    {
					    found_protid= true;
				    }
			    if (!found_protid) // protocol mismatch at the profile index
				    return false;
		    }
	    }
    } // end for
    return true;
}

