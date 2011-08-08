/** @ingroup ievlsp
 * @file
 * NSLP VLSPObject -- carries addressing information for virtual link setup
 */

#ifndef _NSLP__VLSP_H_
#define _NSLP__VLSP_H_

#include "protlib_types.h"
#include "nslp_object.h"

using namespace protlib;

namespace qos_nslp {

/** @addtogroup ievlsp Virtual Link Setup Protocol Object
 * @ingroup ienslpobject
 * @{
 */

/**
 Tunnel types from RADIUS IANA considerations http://www.iana.org/assignments/radius-types
 Sub-registry: Values for RADIUS Attribute 64, Tunnel-Type 
 Reference: [RFC2868]
 Registration Procedures: IETF Consensus

 Registry:
 Value  Description                                                   Reference
 -----  ------------------------------------------------------------  ---------
 1      Point-to-Point Tunneling Protocol (PPTP)                      [RFC2868]
 2      Layer Two Forwarding (L2F)                                    [RFC2868]
 3      Layer Two Tunneling Protocol (L2TP)                           [RFC2868]
 4      Ascend Tunnel Management Protocol (ATMP)                      [RFC2868]
 5      Virtual Tunneling Protocol (VTP)                              [RFC2868]
 6      IP Authentication Header in the Tunnel-mode (AH)              [RFC2868]
 7      IP-in-IP Encapsulation (IP-IP)                                [RFC2868]
 8      Minimal IP-in-IP Encapsulation (MIN-IP-IP)                    [RFC2868]
 9      IP Encapsulating Security Payload in the Tunnel-mode (ESP)    [RFC2868]
 10     Generic Route Encapsulation (GRE)                             [RFC2868]
 11     Bay Dial Virtual Services (DVS)                               [RFC2868]
 12     IP-in-IP Tunneling                                            [RFC2868]
 13     Virtual LANs (VLAN)                                           [RFC3580]

**/


/// NSLP VLSP
class vlsp_object : public known_nslp_object {
/***** inherited from IE ****/
public:
		typedef uint128 vnet_id_t;
		typedef uint128 vnode_id_t;
		typedef uint64  vif_id_t;
		typedef uint64  vlink_id_t;

		typedef enum 
		{
			tunnel_undefined,
			tunnel_L2F,
			tunnel_L2TP,
			tunnel_ATMP,
			tunnel_VTP,
			tunnel_AH,
			tunnel_IP_in_IP_Encap,
			tunnel_Min_IP_in_IP,
			tunnel_ESP,
			tunnel_GRE,
			tunnel_DVS,
			tunnel_IP_in_IP,
			tunnel_VLAN
		} tunnel_t;

		virtual vlsp_object* new_instance() const;
		virtual vlsp_object* copy() const;
		virtual vlsp_object* deserialize(NetMsg& msg, coding_t cod, IEErrorList& errorlist, uint32& bread, bool skip = true);
		virtual void serialize(NetMsg& msg, coding_t cod, uint32& wbytes) const;
		virtual bool check() const;
		virtual size_t get_serialized_size(coding_t coding) const;
		virtual bool operator==(const IE& ie) const;
		virtual const char* get_ie_name() const;
		virtual ostream& print(ostream& os, uint32 level, const uint32 indent, const char* name = NULL) const;
		virtual istream& input(istream& is, bool istty, uint32 level, const uint32 indent, const char* name = NULL);
/***** new members *****/
public:
	static const uint32 vlspdefault;
	// @{
	/// constructor
	vlsp_object();

	/// constructor with values
	vlsp_object(vnet_id_t vnetid, vnode_id_t vnode_src, vnode_id_t vnode_dst, vif_id_t vif_src, vif_id_t vif_dst, vlink_id_t vlink_id, tunnel_t tunnel_type);
	// @}

	/// copy constructor
	vlsp_object(const vlsp_object& n);
	/// assignment
	vlsp_object& operator=(const vlsp_object& n);
	/// tests
	virtual bool is_vlsp_object() const;
	// @}

	void set_vnet_id(vnet_id_t vnetid) { vnet_id= vnetid; }
	void set_vnode_id_src(vnode_id_t vns) { vnode_id_src= vns; }
	void set_vnode_id_dst(vnode_id_t vnd) { vnode_id_dst= vnd; }
	void set_vif_id_src(vif_id_t vis) { vif_id_src= vis; }
	void set_vif_id_dst(vif_id_t vid) { vif_id_dst= vid; }
	void set_vlink_id(vlink_id_t vlid) { vlink_id= vlid; }

	vnet_id_t get_vnet_id() const { return vnet_id; }
	vnode_id_t get_vnode_id_src() const { return vnode_id_src; }
	vnode_id_t get_vnode_id_dst() const { return vnode_id_dst; }
	vif_id_t get_vif_id_src() const { return vif_id_src; }
	vif_id_t get_vif_id_dst() const { return vif_id_dst; }
	vlink_id_t get_vlink_id() const { return vlink_id; }

	uint8 get_tunnel_type() const { return tunnel_type; }
	void set_tunnel_type(tunnel_t tunneltype) { tunnel_type= tunneltype; }

private:
        // VNetID, VNodeID_src, VNodeID_dst, VIf-ID (src/dest), VLinkID (optional)
	vnet_id_t  vnet_id;
	vnode_id_t vnode_id_src;
	vnode_id_t vnode_id_dst;
	vif_id_t   vif_id_src;
	vif_id_t   vif_id_dst;
	vlink_id_t vlink_id;
	uint8   tunnel_type;

	static const char* const iename;
	static const uint16 contlen;
}; // end vlsp_object


/** Check function for VLSP_OBJECT object.
  * @returns TRUE if the current object is of type VLSP_OBJECT.
  */
inline
bool 
vlsp_object::is_vlsp_object() const {
	return (type == VLSP_OBJECT);
} // end is_vlsp_object

//@}

} // end namespace qos_nslp

#endif // _NSLP__VLSP_H_
