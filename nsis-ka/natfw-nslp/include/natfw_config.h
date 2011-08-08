/// ----------------------------------------*- mode: C++; -*--
/// @file natfw_config.h
/// This file defines constants used throughout this implementation.
/// ----------------------------------------------------------
/// $Id: natfw_config.h 4118 2009-07-16 16:13:10Z bless $
/// $HeadURL: https://svn.tm.kit.edu/nsis/natfw-nslp/trunk/include/natfw_config.h $
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
#ifndef NATFW__NATFW_CONFIG_H
#define NATFW__NATFW_CONFIG_H

#include "messages.h"
#include "address.h"

#include "configpar.h"
#include "configpar_repository.h"

// since we re-use some GIST parameter, we need to define them here
#include "gist_conf.h"

namespace natfw {
  using namespace protlib;

  // 0 = global realm, 1 = protlib_realm, 2 = gist_realm

  const realm_id_t natfw_realm= 4;


  enum natfw_configpar_id_t {
    natfwconf_invalid,
    natfwconf_conffilename,
    natfwconf_dispatcher_threads,
    /* NI  */
    natfwconf_ni_session_lifetime,
    natfwconf_ni_response_timeout,
    natfwconf_ni_max_session_lifetime,
    natfwconf_ni_max_retries,
    /* NF  */
    natfwconf_nf_max_session_lifetime,
    natfwconf_nf_response_timeout,
    natfwconf_nf_is_nat,
    natfwconf_nf_is_firewall,
    natfwconf_nf_is_edge_node,
    natfwconf_nf_is_edge_nat,
    natfwconf_nf_is_edge_firewall,
    natfwconf_nf_private_networks,
    natfwconf_nf_nat_public_address,
    natfwconf_nf_nat_public_port_begin,
    natfwconf_nf_nat_public_port_end,
    natfwconf_nf_install_policy_rules,
    /* NR  */
    natfwconf_nr_max_session_lifetime,
    /* NR ext */
    natfwconf_nr_ext_session_lifetime,
    natfwconf_nr_ext_max_retries,
    natfwconf_nr_ext_response_timeout,
    natfwconf_maxparno
  };



/**
 * The central configuration point for a NATFW instance.
 */
class natfw_config {

  public:
	natfw_config(configpar_repository *cfpgar_rep= NULL) : cfgpar_rep(cfpgar_rep) {};
	
	void repository_init();

	void setRepository(configpar_repository* cfp_rep= configpar_repository::instance());

	/// register copy of configuration parameter instance
	void registerPar(const configparBase& configparid) { cfgpar_rep->registerPar(configparid); }
	/// register instance configuration parameter directly
	void registerPar(configparBase* configparid) { cfgpar_rep->registerPar(configparid);  }

	// these are just convenience functions
	template <class T> void setpar(natfw_configpar_id_t configparid, const T& value);
	template <class T> T getpar(natfw_configpar_id_t configparid) const;
	template <class T> T& getparref(natfw_configpar_id_t configparid) const;
	string getparname(natfw_configpar_id_t configparid);

        bool has_ipv4_address() const { return ntlp::gconf.getparref<hostaddresslist_t>(ntlp::gistconf_localaddrv4).size() > 0; }
	const hostaddresslist_t &get_ipv4_addresses() const {
	  return ntlp::gconf.getparref<hostaddresslist_t>(ntlp::gistconf_localaddrv4); }

        bool has_ipv6_address() const { return ntlp::gconf.getparref<hostaddresslist_t>(ntlp::gistconf_localaddrv6).size() > 0; }
	const hostaddresslist_t &get_ipv6_addresses() const {
		return ntlp::gconf.getparref<hostaddresslist_t>(ntlp::gistconf_localaddrv6); }

        uint16 get_gist_port_udp() const { return ntlp::gconf.getpar<uint16>(ntlp::gistconf_udpport); }
	uint16 get_gist_port_tcp() const { return ntlp::gconf.getpar<uint16>(ntlp::gistconf_tcpport); }
	uint16 get_gist_port_tls() const { return ntlp::gconf.getpar<uint16>(ntlp::gistconf_tlsport); }

	uint32 get_num_dispatcher_threads() const {
	  return getpar<uint32>(natfwconf_dispatcher_threads); }

        uint32 get_ni_session_lifetime() const { return getpar<uint32>(natfwconf_ni_max_session_lifetime); }
	uint32 get_ni_max_retries() const { return getpar<uint32>(natfwconf_ni_max_retries); }
        uint32 get_ni_response_timeout() const { return getpar<uint32>(natfwconf_ni_response_timeout); }

	uint32 get_nf_max_session_lifetime() const { return getpar<uint32>(natfwconf_nf_max_session_lifetime); }
	uint32 get_nf_response_timeout() const { return getpar<uint32>(natfwconf_nf_response_timeout); }
	bool is_nf_nat() const { return getpar<bool>(natfwconf_nf_is_nat); }
	bool is_nf_firewall() const { return getpar<bool>(natfwconf_nf_is_firewall); }
	bool is_nf_edge_node() const { return getpar<bool>(natfwconf_nf_is_edge_node); }
	bool is_nf_edge_nat() const { return getpar<bool>(natfwconf_nf_is_edge_nat); }
	bool is_nf_edge_firewall() const { return getpar<bool>(natfwconf_nf_is_edge_firewall); }

	const std::list<netaddress> &get_nf_private_networks() {
		return getparref< list<netaddress> >(natfwconf_nf_private_networks); }

	const hostaddress& get_nf_nat_public_address() const {
		return getparref<hostaddress>(natfwconf_nf_nat_public_address); }
	uint16 get_nf_nat_public_port_begin() const {
		return getpar<uint16>(natfwconf_nf_nat_public_port_begin); }
	uint16 get_nf_nat_public_port_end() const {
		return getpar<uint16>(natfwconf_nf_nat_public_port_end); }
	bool get_nf_install_policy_rules() const {
		return getpar<bool>(natfwconf_nf_install_policy_rules); }

	uint32 get_nr_max_session_lifetime() const {
		return getpar<uint32>(natfwconf_nr_max_session_lifetime); }

	uint32 get_nr_ext_max_retries() const { return getpar<uint32>(natfwconf_nr_ext_max_retries); }
	uint32 get_nr_ext_response_timeout() const {
		return getpar<uint32>(natfwconf_nr_ext_response_timeout); }


	/// The ID of the queue that receives messages from the NTLP.
	static const message::qaddr_t INPUT_QUEUE_ADDRESS
		= message::qaddr_api_2;

	/// The ID of the queue inside the NTLP that we send messages to.
	static const message::qaddr_t OUTPUT_QUEUE_ADDRESS
		= message::qaddr_coordination;

	/// The timer module's queue address.
	static const message::qaddr_t TIMER_MODULE_QUEUE_ADDRESS
		= message::qaddr_timer;


	/// The IANA-assigned protocol identifier for NATFW used by the NTLP.
	static const uint16 NSLP_ID				= 2;

	static const uint32 SEND_MSG_TIMEOUT			= 5;
	static const uint16 SEND_MSG_IP_TTL			= 100;
	static const uint32 SEND_MSG_GIST_HOP_COUNT		= 10;

	/// Whether to use colours in the logging messages.
	static const bool USE_COLOURS				= true;

  protected:
	configpar_repository* cfgpar_rep;

	hostaddress get_hostaddress(const std::string &key);

	void registerAllPars();
};


// this is just a convenience function
template <class T>
void 
natfw_config::setpar(natfw_configpar_id_t configparid, const T& value)
{
	cfgpar_rep->setPar(natfw_realm, configparid, value);
}


// this is just a convenience function
template <class T> T
natfw_config::getpar(natfw_configpar_id_t configparid) const
{
	return cfgpar_rep->getPar<T>(natfw_realm, configparid);
}


// this is just a convenience function
template <class T> T&
natfw_config::getparref(natfw_configpar_id_t configparid) const
{
	return cfgpar_rep->getParRef<T>(natfw_realm, configparid);
}


// this is just a convenience function
inline
string
natfw_config::getparname(natfw_configpar_id_t configparid)
{
	// reference to the config repository singleton
	return cfgpar_rep->getConfigPar(natfw_realm, configparid)->getName();
}


} // namespace natfw

#endif // NATFW__NATFW_CONFIG_H
