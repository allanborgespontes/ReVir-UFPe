/// ----------------------------------------*- mode: C++; -*--
/// @file packet_classifier.h
/// QoS NSLP Packet Classifier
/// ----------------------------------------------------------
/// $Id: packet_classifier.h 5253 2010-04-26 13:41:09Z roehricht $
/// $HeadURL: https://svn.tm.kit.edu/nsis/qos-nslp/trunk/include/packet_classifier.h $
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
/** @ingroup iepacket_classifier
 * @file
 * NSLP Packet_Classifier
 */

#ifndef _NSLP__PACKET_CLASSIFIER_H_
#define _NSLP__PACKET_CLASSIFIER_H_

#include "protlib_types.h"
#include "nslp_object.h"

using namespace protlib;

namespace qos_nslp {

/** @addtogroup iepacket_classifier PACKET_CLASSIFIER
 * @ingroup ienslpobject
 * @{
 */

/// NSLP PACKET_CLASSIFIER
class packet_classifier : public known_nslp_object {
/***** inherited from IE ****/
public:
	virtual packet_classifier* new_instance() const;
	virtual packet_classifier* copy() const;
	virtual packet_classifier* deserialize(NetMsg& msg,coding_t cod,IEErrorList& errorlist,uint32& bread,bool skip = true);
	virtual void serialize(NetMsg& msg, coding_t cod, uint32& wbytes) const;
	virtual bool check() const;
	virtual size_t get_serialized_size(coding_t coding) const;
	virtual bool operator==(const IE& ie) const;
	virtual const char* get_ie_name() const;
	virtual ostream& print(ostream& os, uint32 level, const uint32 indent, const char* name = NULL) const;
	virtual istream& input(istream& is, bool istty, uint32 level, const uint32 indent, const char* name = NULL);
/***** new members *****/
public:

	// specific flags for PACKET_CLASSIFIER object
        #define SOURCE_ADDRESS_AND_PREFIX_FLAG             0x8000   // 1000 0000 0000 0000
	#define DEST_ADDRESS_AND_PREFIX_FLAG               0x4000   // 0100 0000 0000 0000
	#define PROTOCOL_FLAG                              0x2000   // 0010 0000 0000 0000
	#define TRAFFIC_CLASS_FLAG                         0x1000   // 0001 0000 0000 0000
	#define FLOW_LABLE_FLAG                            0x0800   // 0000 1000 0000 0000
	#define SPI_FLAG                                   0x0400   // 0000 0100 0000 0000
	#define SOURCE_PORT_FLAG                           0x0200   // 0000 0010 0000 0000
	#define DESTINATION_PORT_FLAG                      0x0100   // 0000 0001 0000 0000

	static const uint16 default_mrm;
	static const uint16 default_flags;
	// @{
	/// constructor
	packet_classifier();
	packet_classifier(uint16 mrm, uint16 flags);
	// @}
	/// copy constructor
	packet_classifier(const packet_classifier& n);
	/// assignment
	packet_classifier& operator=(const packet_classifier& n);
	/// set to default
	void set_default();
	/// get mrm or flags
	void get_mrm(uint16& m) const;
	void get_flags(uint16& f) const;
	// @{
	/// set mrm or flags
	void set_mrm(uint16 m);
	void set_flags(uint16 f);
	// @}

	/// set SOURCE_ADDRESS_AND_PREFIX_FLAG
        void set_source_address_and_prefix_flag();
        /// set DEST_ADDRESS_AND_PREFIX_FLAG
        void set_destination_address_and_prefix_flag();
        /// set PROTOCOL_FLAG 
        void set_protocol_flag();
        /// set TRAFFIC_CLASSL_FLAG
        void set_traffic_class_flag();
        /// set FLOW_LABLE_FLAG
        void set_flow_label_flag();
	/// set SPI_FLAG
        void set_spi_flag();
	/// set SOURCE_PORT_FLAG
        void set_source_port_flag();
	/// set DESTINATION_PORT_FLAG
        void set_destination_port_flag();

	/// is SOURCE_ADDRESS_AND_PREFIX_FLAG set
        bool is_source_address_and_prefix_flag();
        /// is DEST_ADDRESS_AND_PREFIX_FLAG set
        bool is_destination_address_and_prefix_flag();
        /// is PROTOCOL_FLAG set
        bool is_protocol_flag();
        /// is TRAFFIC_CLASSL_FLAG set
        bool is_traffic_class_flag();
        /// is FLOW_LABLE_FLAG set
        bool is_flow_label_flag();
	/// is SPI_FLAG set
        bool is_spi_flag();
	/// is SOURCE_PORT_FLAG set
        bool is_source_port_flag();
	/// is DESTINATION_PORT_FLAG set
        bool is_destination_port_flag();

	// @{
	/// tests
	bool is_packet_classifier() const;
	// @}
private:
	uint16 mrmref;
	uint16 flagsref;
	static const char* const iename;
	uint16 contlen;
}; // end packet_classifier

//@}

} // end namespace qos_nslp

#endif // _NSLP__PACKET_CLASSIFIER_H_
