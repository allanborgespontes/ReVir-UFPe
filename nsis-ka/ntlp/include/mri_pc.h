/// ----------------------------------------*- mode: C++; -*--
/// @file mri_pc.h
/// GIST Path coupled MRI (Message Routing Information)
/// ----------------------------------------------------------
/// $Id: mri_pc.h 5249 2010-04-26 08:40:59Z roehricht $
/// $HeadURL: https://svn.tm.kit.edu/nsis/ntlp/trunk/include/mri_pc.h $
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
#ifndef NTLP__MRI_PC_H
#define NTLP__MRI_PC_H

#include "mri.h"


namespace ntlp {
    using namespace protlib;

/**
 * @ingroup mri
 * @{
 */

/**
 * Routing information for the Path-Coupled MRM.
 *
 * This class represents the MRI for the Path-Coupled Message Routing Method.
 * There are several constructors, but using the 5-argument one (addresses,
 * prefixes, direction) is preferred. More MRI fields can be set using the 
 * provided setter methods.
 */
class mri_pathcoupled : public mri {

  public:
	mri_pathcoupled();
	mri_pathcoupled(const mri_pathcoupled& n);

	mri_pathcoupled(hostaddress sourceaddress, uint8 sourceprefix,
		hostaddress destaddress, uint8 destprefix, bool downstream);

	mri_pathcoupled(hostaddress sourceaddress, uint8 sourceprefix,
		port_t sourceport, hostaddress destaddress, uint8 destprefix,
		port_t destport, const char *protoname, uint16 flowlabel,
		uint8 ds_field, uint32 spi, bool downstream);

	mri_pathcoupled(hostaddress sourceaddress, uint8 sourceprefix,
		port_t sourceport, hostaddress destaddress, uint8 destprefix,
		port_t destport, protocol_t protonum, uint16 flowlabel,
		uint8 ds_field, uint32 spi, bool downstream);

	virtual inline ~mri_pathcoupled() { }

	virtual mri_pathcoupled *new_instance() const;
	virtual mri_pathcoupled *copy() const;

	virtual mri_pathcoupled *deserialize(NetMsg &msg, coding_t cod,
		IEErrorList &errorlist, uint32 &bread, bool skip=true);
	virtual mri_pathcoupled *deserializeNoHead(NetMsg &msg, coding_t cod,
		IEErrorList &errorlist, uint32 &bread, bool skip=true);

	virtual void serialize(NetMsg &msg, coding_t cod, uint32 &wbytes) const;
	virtual void serializeNoHead(NetMsg &msg, coding_t cod, uint32 &wbytes)
		const;

	virtual bool check() const;
	virtual size_t get_serialized_size(coding_t coding) const;

	virtual bool operator==(const IE& ie) const;
	virtual const char *get_ie_name() const;
	virtual ostream &print(ostream &os, uint32 level, const uint32 indent,
		const char *name=NULL) const;

	virtual istream &input(istream &is, bool istty, uint32 level,
		const uint32 indent, const char *name=NULL);

	virtual appladdress* determine_dest_address() const;


	const hostaddress& get_sourceaddress() const { return sourceaddress; }
	port_t get_sourceport() const { return sourceport; }
	uint8 get_sourceprefix() const { return sourceprefix; }

	const hostaddress& get_destaddress() const { return destaddress; }
	port_t get_destport() const { return destport; }
 	uint8 get_destprefix() const { return destprefix; }

	uint8 get_ip_version() const { return ip_version; };
	protocol_t get_protocol() const { return protocol; }
	uint16 get_flowlabel() const { return flowlabel; }
	uint16 get_ds_field() const { return ds_field; }
	uint32 get_spi() const { return spi; }

	bool get_downstream() const { return ! ( flags & Direction ); }
	void set_downstream(bool down);

	uint16 get_flags() const { return flags; }
  
        void set_sourceaddress(const hostaddress& sa) { sourceaddress= sa; rebuild_flags(); set_sourceprefix( sa.is_ipv6() ? 128:32); }
        void set_destaddress(const hostaddress& da) { destaddress= da; rebuild_flags(); set_destprefix(da.is_ipv6() ? 128:32); }

	void set_protocol(protocol_t p);
	void set_protocol(const std::string &name);
	void set_sourceprefix(uint8 srcprefix) { sourceprefix= srcprefix; }
	void set_destprefix(uint8 dstprefix) { destprefix= dstprefix; }
	void set_sourceport(port_t sp);
	void set_destport(port_t dp);
	void set_ds_field(uint8 dscp);
	void set_flowlabel(uint16 fl);
	void set_spi(uint32 spi);

	void invertDirection();

	enum flags_t {
		IP_protocol      = 1 << 11,
		DS_Field         = 1 << 10,
		Flow_Label       = 1 << 9,
		SPI              = 1 << 8,
		Source_Port      = 1 << 7,
		Destination_Port = 1 << 6,
		Direction        = 1 << 5
	};

	struct ltmri {
		inline bool operator()(const mri_pathcoupled *a,
		    const mri_pathcoupled *b) const {
		    	hostaddress::ltaddr ltaddr;
			if (ltaddr(&a->sourceaddress, &b->sourceaddress))
				return true;
			return ltaddr(&a->destaddress, &b->destaddress);
		}
	};

  private:
	uint8 ip_version;	// IP Version (only 4 bit used)
	uint16 flags;		// Flags field

	// source and dest address contain IP-Address, prefix length and port
	hostaddress sourceaddress;
	uint8 sourceprefix;
	port_t sourceport;

	hostaddress destaddress;
	uint8 destprefix;
	port_t destport;

	uint16 flowlabel;		// Flow Label
	protocol_t protocol;		// Protocol
	uint8 ds_field;		        // DS Field [RFC 3260,2474] (6 bits wide)
	uint32 spi;			// IPsec SPI

	bool rebuild_flags();

	virtual mri_pathcoupled *deserializeEXT(NetMsg &msg, coding_t cod,
		IEErrorList &errorlist, uint32 &bytes_read, bool skip,
		bool read_header);
	virtual void serializeEXT(NetMsg &msg, coding_t cod,
		uint32 &bytes_written, bool write_header) const;
    
	static const char *const iename;
};


/**
 * Default constructor, only used for registering with the IE manager.
 */
inline mri_pathcoupled::mri_pathcoupled()
		: mri(mri_t_pathcoupled, false, pathcoupled),
		  ip_version(0), flags(0), sourceprefix(0), sourceport(0),
		  destprefix(0), destport(0), flowlabel(0),
		  protocol(tsdb::get_tcp_id()), ds_field(0), spi(0) {

	rebuild_flags();
}


/**
 * Minimal constructor.
 *
 * This initializes the object with all mandatory fields (but not more).
 * All other fields are not set with their respective flags being 0. Unless
 * they are set later manually, they are interpreted as fully wildcarded.
 */
inline mri_pathcoupled::mri_pathcoupled(
		hostaddress sourceaddress, uint8 sourceprefix,
		hostaddress destaddress, uint8 destprefix, bool downstream) 
		: mri(mri_t_pathcoupled, false, pathcoupled),
		  flags(0),
		  sourceaddress(sourceaddress), sourceprefix(sourceprefix),
		  sourceport(0), destaddress(destaddress),
		  destprefix(destprefix), destport(0), flowlabel(0),
		  protocol(0), ds_field(0), spi(0) {

	if ( ! downstream )
		flags = flags | Direction;

	rebuild_flags();
}


/**
 * Constructor (deprecated).
 *
 * Deprecated, because not all valid flag-field combinations are possible
 * using this constructor.
 */
inline mri_pathcoupled::mri_pathcoupled(hostaddress sourceaddress,
		uint8 sourceprefix, port_t sourceport, hostaddress destaddress,
		uint8 destprefix, port_t destport, protocol_t protonum,
		uint16 flowlabel, uint8 dscp, uint32 spi,
		bool downstream)
		: mri(mri_t_pathcoupled, false, pathcoupled),
		  flags(0), sourceaddress(sourceaddress),
		  sourceprefix(sourceprefix),
		  sourceport(sourceport), destaddress(destaddress),
		  destprefix(destprefix), destport(destport),
		  flowlabel(flowlabel), protocol(protonum),
		  ds_field(dscp), spi(spi) {

	if ( ! downstream )
		flags = flags | Direction;

	rebuild_flags();
}


/**
 * Constructor (deprecated; accepts a string for the protocol parameter).
 *
 * Deprecated, because not all valid flag-field combinations are possible
 * using this constructor.
 */
inline mri_pathcoupled::mri_pathcoupled(hostaddress sourceaddress,
		uint8 sourceprefix, port_t sourceport, hostaddress destaddress,
		uint8 destprefix, port_t destport,
		const char* protoname, uint16 flowlabel, uint8 dscp,
		uint32 spi, bool downstream)
		: mri(mri_t_pathcoupled, false, pathcoupled),
		  flags(0), sourceaddress(sourceaddress),
		  sourceprefix(sourceprefix), sourceport(sourceport),
		  destaddress(destaddress), destprefix(destprefix),
		  destport(destport), flowlabel(flowlabel),
		  protocol(tsdb::getprotobyname(protoname)),
		  ds_field(dscp), spi(spi) {

	if ( ! downstream )
		flags = flags | Direction;
	
	rebuild_flags();
}


/**
 * Copy constructor.
 */
inline mri_pathcoupled::mri_pathcoupled(const mri_pathcoupled& n)
		: mri(mri_t_pathcoupled, n.nat_flag, n.subtype),
		  ip_version(n.ip_version), flags(n.flags),
		  sourceaddress(n.sourceaddress), sourceprefix(n.sourceprefix),
		  sourceport(n.sourceport), destaddress(n.destaddress),
		  destprefix(n.destprefix), destport(n.destport),
		  flowlabel(n.flowlabel), protocol(n.protocol),
		  ds_field(n.ds_field), spi(n.spi) {

	// nothing to do
}


/**
 * Reset the flags based on the object's attributes.
 *
 * Used in the constructors, can be called any time to set the flags field
 * according to current data in the mri_pathcoupled object
 */
inline bool mri_pathcoupled::rebuild_flags() {

	// set the IP Version to "6" if one of the addresses is a native v6 or
	// IPv4-mapped
	if ( destaddress.is_ipv6() || sourceaddress.is_ipv6() )
		ip_version = 6;
	else
		ip_version = 4;
    
	// set the flags accordingly
	if ( protocol != 0 )		flags = flags | IP_protocol;
	if ( ds_field != 0 )	        flags = flags | DS_Field;
	if ( flowlabel != 0 )		flags = flags | Flow_Label;
	if ( spi != 0 )			flags = flags | SPI;
	if ( destport != 0 )		flags = flags | Destination_Port;
	if ( sourceport != 0 )		flags = flags | Source_Port;
 
	return 1;
}


inline void mri_pathcoupled::set_protocol(protocol_t p) {
	flags |= IP_protocol;
	protocol = p;
}

inline void mri_pathcoupled::set_protocol(const std::string &name) {
	set_protocol(tsdb::getprotobyname(name));
}

// May only be set if the protocol is set, too.
inline void mri_pathcoupled::set_sourceport(port_t sp) {
	flags |= Source_Port;
	sourceport = sp;
}

// May only be set if the protocol is set, too.
inline void mri_pathcoupled::set_destport(port_t dp) {
	flags |= Destination_Port;
	destport = dp;
}

inline void mri_pathcoupled::set_ds_field(uint8 dscp) {
	flags |= DS_Field;
	ds_field = dscp;
}

// Only allowed for IPv6.
inline void mri_pathcoupled::set_flowlabel(uint16 fl) {
	flags |= Flow_Label;
	flowlabel = fl;
}

inline void mri_pathcoupled::set_spi(uint32 s) {
	flags |= SPI;
	spi = s;
}

inline void mri_pathcoupled::set_downstream(bool down) {
//	if (! down)
//		flags = flags | Direction;
	if(down) {
		// D-Flag = 0
		flags = flags & (~Direction);
	}
	else {
		// D-Flag = 1
		flags = flags | Direction;
	}
}

//@}

} // namespace ntlp

#endif // NTLP__MRI_PC_H
