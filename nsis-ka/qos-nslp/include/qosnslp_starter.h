/// ----------------------------------------*- mode: C++; -*--
/// @file qosnslp_starter.h
/// QoS NSLP Starter
/// ----------------------------------------------------------
/// $Id: qosnslp_starter.h 5377 2010-06-09 20:42:12Z bless $
/// $HeadURL: https://svn.tm.kit.edu/nsis/qos-nslp/trunk/include/qosnslp_starter.h $
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

#ifndef _QOSNSLP_STARTER_H_
#define _QOSNSLP_STARTER_H_

#include "threads.h"
#include "ntlp_starter.h"

namespace qos_nslp {

using namespace protlib;
using namespace ntlp;

// starter module parameters
struct QoSNSLPStarterParam : public ThreadParam {
	QoSNSLPStarterParam(NTLPStarterParam *ntlppar = NULL);

	NTLPStarterParam *ntlppar;
	AddressList *addresses;
	Flowinfo *fi_service;
};

class QoSNSLPStarter : public Thread {

public:
	QoSNSLPStarter(const QoSNSLPStarterParam& p);
	/// destructor
	~QoSNSLPStarter();
	/// module main loop
	void main_loop(uint32 nr);

	const QoSNSLPStarterParam& get_param() const { return param; };

private:
	QoSNSLPStarterParam param;

}; // end class QoSNSLPStarter

} // end namespace qos_nslp

#endif
