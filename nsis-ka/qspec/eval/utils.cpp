/*
 * utils.cpp - Helper functions
 *
 * $Id: utils.cpp 3236 2008-07-28 10:05:05Z bless $
 * $HeadURL: https://svn.tm.kit.edu/nsis/qspec/trunk/eval/utils.cpp $
 *
 */
#include "qspec_ie.h"
#include "qspec_pdu.h"
#include "qspec_param.h"


namespace qspec {

NetMsg *createNetMsg() {

	qspec_pdu *qspec
	  = new qspec_pdu(ms_two_way_transactions, 3, 12, false);

	qspec_object *qos_desired = new qspec_object(ot_qos_desired);
	qos_desired->set_parameter(new t_mod(256000.0, 576.0, 100000.0, 40)); // in bytes/sec.
	qos_desired->set_parameter(new path_latency(200000)); // in microsec.
	qspec->set_object(qos_desired);

	qspec_object *minimum_qos = new qspec_object(ot_minimum_qos);
	minimum_qos->set_parameter(new t_mod(128000.0,576.0,100000.0,40)); // in bytes/sec.
	minimum_qos->set_parameter(new path_latency(400000)); // in microsec.
	qspec->set_object(minimum_qos);

	qspec_object *qos_reserved = new qspec_object(ot_qos_reserved);
	qos_reserved->set_parameter(new t_mod(1024000.0,576.0,100000.0,40)); // our bandwidth
	qos_reserved->set_parameter(new excess_treatment(et_shape));
	qspec->set_object(qos_reserved);

	qspec_object *qos_available = new qspec_object(ot_qos_available);
	qos_available->set_parameter(new t_mod(1024000.0,576.0,100000.0,40)); // our bandwidth
	qos_available->set_parameter(new path_latency(100));
	qspec->set_object(qos_available);


	/*
	 * Variables.
	 */
	NetMsg *msg = new NetMsg(1024);	// this will contain our QSPEC
	uint32 bytes_written;		// the number of bytes written

	/*
	 * Write the QSPEC into the NetMsg.
	 */
	try {
		qspec->serialize(*msg, IE::protocol_v1, bytes_written);
		msg->truncate(bytes_written);
	}
	catch ( IEError err ) {
		std::cout << "Error: " << err.what() << "\n";
		exit(1);
	}

	delete qspec;

	return msg;
}

} // namespace qspec

// EOF
