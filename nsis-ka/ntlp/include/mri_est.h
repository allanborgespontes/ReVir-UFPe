/// ----------------------------------------*- mode: C++; -*--
/// @file mri_est.h
/// GIST Explicit Signaling Target  MRI (Message Routing Information)
/// ----------------------------------------------------------
/// $Id: mri_est.h 5249 2010-04-26 08:40:59Z roehricht $
/// $HeadURL: https://svn.tm.kit.edu/nsis/ntlp/trunk/include/mri_est.h $
// ===========================================================
//                      
// Copyright (C) 2007-2010, all rights reserved by
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
// format:
//     0                   1                   2                   3
//    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
//                                   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//                                   |IP-Ver |P|T|F|S|A|B|D|Reserved |
//   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//   //                       Source Address                        //
//   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//   //                      Destination Address                    //
//   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//   | Source Prefix |  Dest Prefix  |   Protocol    | DS-field  |Rsv|
//   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//   :       Reserved        |              Flow Label               :
//   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//   :                              SPI                              :
//   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//   :          Source Port          :       Destination Port        :
//   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//   | IPVerS| Reserved              | IPVerI| Reserved              |
//   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//   //                   Origin Signaling Address                  //
//   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//   //                Destination Signaling Address                //
//   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//   //                     Ingress Node Address                    //
//   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//
//

// ===========================================================
#ifndef NTLP__MRI_EST_H
#define NTLP__MRI_EST_H

#include "mri.h"
#include "mri_pc.h"
#include "ntlp_global_constants.h"

namespace ntlp {
    using namespace protlib;


/**
 * Routing information for the Path-Coupled MRM.
 *
 * This class represents the MRI for the Path-Coupled Message Routing Method.
 * There are several constructors, but using the 5-argument one (addresses,
 * prefixes, direction) is preferred. More MRI fields can be set using the 
 * provided setter methods.
 */
class mri_explicitsigtarget : public mri {

  public:
        mri_explicitsigtarget();
	mri_explicitsigtarget(const mri_explicitsigtarget& n);

        mri_explicitsigtarget(const mri_pathcoupled& pc_mri, const hostaddress& origin_sig_address, const hostaddress& dest_sig_address, const hostaddress& ingress_node_addr= in6addr_any);

        virtual ~mri_explicitsigtarget() {};

	virtual mri_explicitsigtarget *new_instance() const;
	virtual mri_explicitsigtarget *copy() const;

	virtual mri_explicitsigtarget *deserialize(NetMsg &msg, coding_t cod,
		IEErrorList &errorlist, uint32 &bread, bool skip=true);
	virtual mri_explicitsigtarget *deserializeNoHead(NetMsg &msg, coding_t cod,
		IEErrorList &errorlist, uint32 &bread, bool skip=true);

	virtual void serialize(NetMsg &msg, coding_t cod, uint32 &wbytes) const;
	virtual void serializeNoHead(NetMsg &msg, coding_t cod, uint32 &wbytes) const;

	virtual bool check() const;
	virtual size_t get_serialized_size(coding_t coding) const;

	virtual bool operator==(const IE& ie) const;
	virtual const char *get_ie_name() const;
	virtual ostream &print(ostream &os, uint32 level, const uint32 indent,
		const char *name=NULL) const;

	virtual istream &input(istream &is, bool istty, uint32 level,
		const uint32 indent, const char *name=NULL);

        virtual const hostaddress& get_sourceaddress() const { return mri_pc.get_sourceaddress(); }
        virtual const hostaddress& get_destaddress() const { return mri_pc.get_destaddress(); }

        virtual bool get_downstream() const { return mri_pc.get_downstream(); }

        virtual appladdress* determine_dest_address() const { return new appladdress(dest_signaling_addr,prot_udp,GIST_default_port); }
    
	// Returns the IP Version
        virtual uint8 get_ip_version() const { return mri_pc.get_ip_version(); }

        virtual void invertDirection() { mri_pc.invertDirection(); }

        // special methods for this MRI
        const mri_pathcoupled& get_mri_pc() const { return mri_pc; }
	const hostaddress& get_origin_sig_address() const { return origin_signaling_addr; }
	const hostaddress& get_dest_sig_address() const { return dest_signaling_addr; }
	const hostaddress& get_ingress_address() const { return ingress_node_addr; }

        void set_mri_pc(const mri_pathcoupled& mripc) { mri_pc= mripc; }
	void set_origin_sig_address(const hostaddress& origin) { origin_signaling_addr= origin; }
	void set_dest_sig_address(const hostaddress& dest) { dest_signaling_addr= dest; }
	void set_ingress_address(const hostaddress& ingress) { ingress_node_addr= ingress; }

private:
  // data flow descriptor
  mri_pathcoupled mri_pc;
  // new for path_coupled signaling

  // osig/dsig version, ingress version can be derived
  
  // explicit origin and destination for signaling
  hostaddress origin_signaling_addr;
  hostaddress dest_signaling_addr;
  hostaddress ingress_node_addr;

  virtual mri_explicitsigtarget *deserializeEXT(NetMsg &msg, coding_t cod,
					   IEErrorList &errorlist, uint32 &bytes_read, bool skip,
					   bool read_header);
  virtual void serializeEXT(NetMsg &msg, coding_t cod,
			    uint32 &bytes_written, bool write_header) const;
  
  static const char *const iename;
};


/**
 * Default constructor, only used for registering with the IE manager.
 */
inline mri_explicitsigtarget::mri_explicitsigtarget()
		: mri(mri_t_explicitsigtarget, false, explicitsigtarget),
		  origin_signaling_addr(in6addr_any),
		  dest_signaling_addr(in6addr_any),
		  ingress_node_addr(in6addr_any)
{
  // nothing to do
}


/**
 * Minimal constructor.
 *
 * This initializes the object with all mandatory fields (but not more).
 * All other fields are not set with their respective flags being 0. Unless
 * they are set later manually, they are interpreted as fully wildcarded.
 */
inline mri_explicitsigtarget::mri_explicitsigtarget(
					  const mri_pathcoupled& pc_mri, 
					  const hostaddress& origin_sig_address, 
					  const hostaddress& dest_sig_address, 
					  const hostaddress& ingress_node_addr)
		: mri(mri_t_explicitsigtarget, false, explicitsigtarget),
		  mri_pc(pc_mri),
		  origin_signaling_addr(origin_sig_address),
		  dest_signaling_addr(dest_sig_address),
		  ingress_node_addr(ingress_node_addr)
{
  // nothing to do
}



/**
 * Copy constructor.
 */
inline mri_explicitsigtarget::mri_explicitsigtarget(const mri_explicitsigtarget& n)
		: mri(mri_t_explicitsigtarget, false, explicitsigtarget),
		  mri_pc(n.mri_pc),
		  origin_signaling_addr(n.origin_signaling_addr),
		  dest_signaling_addr(n.dest_signaling_addr),
		  ingress_node_addr(n.ingress_node_addr)
		  
{ 	
  // nothing to do
}




} // namespace ntlp

#endif // NTLP__MRI_EST_H
