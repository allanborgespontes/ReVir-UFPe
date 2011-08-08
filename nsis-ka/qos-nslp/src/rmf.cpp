/// ----------------------------------------*- mode: C++; -*--
/// @file rmf.cpp
/// QoS NSLP Resource Management Function
/// ----------------------------------------------------------
/// $Id: rmf.cpp 6315 2011-07-18 13:10:50Z bless $
/// $HeadURL: https://svn.tm.kit.edu/nsis/qos-nslp/trunk/src/rmf.cpp $
// ===========================================================
//                      
// Copyright (C) 2005-2010, all rights reserved by
// - Institute of Telematics, Karlsruhe Institute of Technology
//
// More information and contact:
// https://projekte.tm.uka.de/trac/NSIS
//                      
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; version 2 of the License
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
// 02110-1301 USA.
//
// ===========================================================

#include "rmf.h"
#include "logfile.h"
#include "qos_nslp_conf.h"

#include <iomanip>
#include <sstream>
#include <cstdlib>

using namespace protlib;
using namespace protlib::log;

namespace qos_nslp {

/** @addtogroup iermf Resource Management Function
 * @{
 */

/***** class rmf *****/

const uint32 rmf::max_avail_bandwidth_default = 1000000000;

const uint32 rmf::reserved_default = 0;

long rmf::AF_available_bandwidth= 400000000;	// in bit/s
long rmf::EF_available_bandwidth= 600000000;	// in bit/s
long rmf::EF_res_avg_rate= 0;				// in bit/s
long rmf::BE_available_bandwidth= 600000000;	// in bit/s

	
/** Default constructor for RMF function without any params. All values will be set to the default values.
  */
rmf::rmf() {
	set_max_avail_bandwidth(10000000);
	set_default_reserved();
	get_avail_bandwidth(available_bandwidth);
	} // end constructor


//void rmf::set_AF_available_bandwidth(long a) {
//static AF_available_bandwidth = a;
//}


/** This function sets the maximum available bandwidth to a default value.
  */
void rmf::set_default_max() {
  max_available_bandwidth = max_avail_bandwidth_default;
} // end set_default_max

/** This function sets the maximum available bandwidth.
  * @param r the value for the maximum available bandwidth.
  */
void rmf::set_max_avail_bandwidth(uint32 r) {
	max_available_bandwidth = r;
} // end set_default_max

/** This function sets the reserved bandwidth to a default value.
  */
void rmf::set_default_reserved() {
	reserved_bandwidth = reserved_default;
} // end set_default_reserved

/** This function gets the available bandwidth.
  * @param r into this value the amount of available bandwidth will be written.
  */
void rmf::get_avail_bandwidth(uint32& r)  {
    calculate_avail_bandwidth(r);
} // end get

/** This function calculates the available bandwidth.
  * @param r into this value the amount of calculated available bandwidth will be written.
  */
void rmf::calculate_avail_bandwidth(uint32& r) const {
	r = max_available_bandwidth - reserved_bandwidth;
} // end calculate

/** This function gets the reserved bandwidth.
  * @param r into this value for the amount of reserved bandwidth will be written.
  */
void rmf::get_reserved_bandwidth(uint32& r) const {
	r = reserved_bandwidth;
} // end get

/** This function processes QSPEC object e.g. from QUERY message.
  * @param q current QSPEC to be processed.
  */
void rmf::process_qspec_in_query(qspec_pdu* q)
{
  EVLog("QoS RMF", "QUERY resources");

}

/** This function reserves resources as specified in QSPEC object.
  * @param q current QSPEC containing resources to be reserved.
  */
bool 
rmf::reserve_resources(const qspec_pdu* q)
{
    
  	bool result = false;
	
	qspec::qspec_object *ob = q->get_object(0);
	
	admission_priority *ad_pr = (admission_priority*) ob->get_parameter(9);
	t_mod *tmd = (t_mod*) ob->get_parameter(1);
	long *afb = &AF_available_bandwidth;
	long *efb = &EF_available_bandwidth;
	long *ef_avg_rate = &EF_res_avg_rate;
	long *beb = &BE_available_bandwidth;
	
	
	EVLog("QoS RMF", "QoS-desired average rate: " << tmd->get_rate());
	EVLog("QoS RMF", "QoS-desired peak data rate: " << tmd->get_peak_data_rate());
	EVLog("QoS RMF", "QoS-desired bucket depth: " << tmd->get_bucket_depth());
	EVLog("QoS RMF", "QoS-desired mpu: " << tmd->get_min_policed_unit());
	
	if ( ad_pr->get_y2171value() == 1) {	
			
		if ((AF_available_bandwidth - tmd->get_peak_data_rate())>0) {	
			EVLog("QoS RMF", "RESERVE resources for AF");	
			*afb -= tmd->get_peak_data_rate();
			result = true;
		
			}
	}
	
	
	if ( (ad_pr->get_y2171value()) == 2) {
		if((EF_available_bandwidth - tmd->get_peak_data_rate())>0) {
			EVLog("QoS RMF", "RESERVE resources for EF");
			*efb -= tmd->get_peak_data_rate();		
			*ef_avg_rate += tmd->get_rate(); 
			result = true;
		}
	}
	
	 if ( (ad_pr->get_y2171value()) == 0) {
	 	if (BE_available_bandwidth - EF_res_avg_rate - tmd->get_rate()) {	
			EVLog("QoS RMF", "RESERVE resources for BE");	
			*beb -= *ef_avg_rate - tmd->get_rate();
			result = true;
	 	}
	 }
	 	
  
   	return result;
}

/** This function releases resources as specified in QSPEC object.
  * @param q current QSPEC containing resources to be released.
  */
void rmf::release_resources(const qspec_pdu* q)
{
  EVLog("QoS RMF", "RELEASE resources");
    	qspec::qspec_object *ob = q->get_object(0);
	admission_priority *ad_pr = (admission_priority*) ob->get_parameter(9);
	t_mod *tmd = (t_mod*) ob->get_parameter(1);
	long *afb = &AF_available_bandwidth;
	long *efb = &EF_available_bandwidth;
	long *ef_avg_rate = &EF_res_avg_rate;
	long *beb = &BE_available_bandwidth;	
	
	if (( ad_pr->get_y2171value())==1) {
		EVLog("QoS RMF", "RELEASE resources for AF");			
		*afb += tmd->get_peak_data_rate();
	}
	
	if ( (ad_pr->get_y2171value())==2) {
		EVLog("QoS RMF", "RELEASE resources for EF");
		*efb += tmd->get_peak_data_rate();
		*ef_avg_rate -= tmd->get_rate();
	}
	
	if ( (ad_pr->get_y2171value())==0) {
		EVLog("QoS RMF", "RELEASE resources for BE");
		*beb -= *ef_avg_rate + tmd->get_rate();
	} 

	string command = qosnslpconf.getpar<string>(qosnslpconf_rmf_vlsp_link_down_script_source);

	if (!command.empty() && command != "")
	{
		std::stringstream command_args;

		//remove-tunnel.sh <bridge> <tunnel> 
		command_args << "br0 tun14";

		string cmd= command + " " + command_args.str();
		DLog("QoS RMF","calling command " << cmd);
		int result= system(cmd.c_str());
		EVLog("QoS RMF","removed virtual link " << cmd << "[returned:"<< result << "]");
	}
}

// process incoming vlink setup request (incoming RESERVE)
void 
rmf::process_vlsp_object_setup_req(const vlsp_object* vlspobj)
{
	EVLog("QoS RMF", "processing VLSP object setup request");

	uint128 vnet_id= vlspobj->get_vnet_id();
	uint128 vnode_src_id= vlspobj->get_vnode_id_src();
	uint128 vnode_dst_id= vlspobj->get_vnode_id_dst();
	uint64  vif_if_src= vlspobj->get_vif_id_src();
	uint64  vif_if_dst= vlspobj->get_vif_id_dst();
	uint64 vlink_id_t= vlspobj->get_vlink_id();

	string command= qosnslpconf.getpar<string>(qosnslpconf_rmf_vlsp_link_up_script_drain);

	if (!command.empty() && command != "")
	{
		std::stringstream command_args;
		//./config-sink.rb –v $VMID –f $VIFID –d $DEMUXIP (-i)
		//command_args << "-v " << vnode_dst_id.w4 << " -f " << vif_if_dst << " -d 10.0.0.1 -i";
		
		//create-tunnel.sh <tunnel> <bridge> <ip-remote> <ip-local>
		//command_args << "tun14 br0 172.3.4.4 172.1.2.1";
		command_args << vnet_id << " " << vnode_dst_id.w4 << " " << vnode_src_id.w4 << " " << vif_if_dst << " " << vif_if_src << " " << vlink_id_t;
		
		string cmd= command + " " + command_args.str();
		DLog("QoS RMF","calling command " << cmd);
		int result= system(cmd.c_str());
		DLog("QoS RMF","finished VLSP object setup request. result:" << result << " for node " << vnet_id << " srcnode " << vnode_src_id << " destnode " << vnode_dst_id << " vi/f src " << vif_if_src << " vi/f dst " << vif_if_dst << "link " << vlink_id_t );
	}
}


// process incoming vlink setup response (incoming RESPONSE)
void 
rmf::process_vlsp_object_setup_resp(const vlsp_object* vlspobj)
{
	EVLog("QoS RMF", "processing VLSP object setup response");

	uint128 vnet_id= vlspobj->get_vnet_id();
	uint128 vnode_src_id= vlspobj->get_vnode_id_src();
	uint128 vnode_dst_id= vlspobj->get_vnode_id_dst();
	uint64  vif_if_src= vlspobj->get_vif_id_src();
	uint64  vif_if_dst= vlspobj->get_vif_id_dst();
	uint64 vlink_id_t= vlspobj->get_vlink_id();

	string command= qosnslpconf.getpar<string>(qosnslpconf_rmf_vlsp_link_up_script_source);

	if (!command.empty()!= 0 && command != "")
	{
		std::stringstream command_args;
		//./config-sink.rb –v $VMID –f $VIFID –d $DEMUXIP (-i)
		//command_args << "-v " << vnode_dst_id.w4 << " -f " << vif_if_dst << " -d 10.0.0.1 -i";
		
		//create-tunnel.sh <tunnel> <bridge> <ip-remote> <ip-local>
		//command_args << "tun14 br0 172.3.4.4 172.1.2.1";
		command_args << vnet_id << " " << vnode_dst_id.w4 << " " << vnode_src_id.w4 << " " << vif_if_dst << " " << vif_if_src << " " << vlink_id_t;
	
		string cmd= command + " " + command_args.str();
		DLog("QoS RMF","calling command " << cmd);
		int result= system(cmd.c_str());
		DLog("QoS RMF","finished VLSP object setup response. result:" << result << " for node " << vnet_id << " srcnode " << vnode_src_id << " destnode " << vnode_dst_id << " vi/f src " << vif_if_src << " vi/f dst " << vif_if_dst << "link " << vlink_id_t );

	}
}

void 
rmf::process_vlsp_object_teardown(const vlsp_object* vlspobj)
{
	EVLog("QoS RMF", "processing VLSP object teardown");

        uint128 vnet_id= vlspobj->get_vnet_id();
        uint128 vnode_src_id= vlspobj->get_vnode_id_src();
        uint128 vnode_dst_id= vlspobj->get_vnode_id_dst();
        uint64  vif_if_src= vlspobj->get_vif_id_src();
        uint64  vif_if_dst= vlspobj->get_vif_id_dst();
        uint64 vlink_id_t= vlspobj->get_vlink_id();

	string command= qosnslpconf.getpar<string>(qosnslpconf_rmf_vlsp_link_down_script_drain);

	if (!command.empty()!= 0 && command != "")
	{
		std::stringstream command_args;
		//./config-sink.rb –v $VMID –f $VIFID –d $DEMUXIP (-i)
		//command_args << "-v " << vnode_dst_id.w4 << " -f " << vif_if_dst << " -d 10.0.0.1 -i";
		
		//create-tunnel.sh <tunnel> <bridge> <ip-remote> <ip-local>
		//command_args << "tun14 br0 172.3.4.4 172.1.2.1";
		command_args << vnet_id << " " << vnode_dst_id.w4 << " " << vnode_src_id.w4 << " " << vif_if_dst << " " << vif_if_src << " " << vlink_id_t;
	
		string cmd= command + " " + command_args.str();
		DLog("QoS RMF","calling command " << cmd);
		int result= system(cmd.c_str());
		DLog("QoS RMF", "tear down virtual link: vnet #"<< vnet_id << " srcnode " << vnode_src_id << " destnode " << vnode_dst_id << " vi/f src " << vif_if_src << " vi/f dst " << vif_if_dst << "link " << vlink_id_t );

	}
}
//@}

} // end namespace qos_nslp
