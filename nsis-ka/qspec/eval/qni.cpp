/*
 * QNI.cpp - a benchmark for QNIs
 *
 * $Id: qni.cpp 3236 2008-07-28 10:05:05Z bless $
 * $HeadURL: https://svn.tm.kit.edu/nsis/qspec/trunk/eval/qni.cpp $
 *
 */
#include "qspec_ie.h"
#include "qspec_pdu.h"
#include "qspec_param.h"

#include "Benchmark.h"
#include "utils.h"


namespace qspec {

class QNI : public Benchmark {

  public:
	virtual void performTask();
};


void QNI::performTask() {

#if 0
	qspec_pdu *qspec
	  = new qspec_pdu(ms_two_way_transactions, 3, 12);

	qspec_object *control_info = new qspec_object(ot_control_information);
	control_info->set_parameter(new non_qosm_hop());
	control_info->set_parameter(new excess_treatment(et_dont_care));
	qspec->set_object(control_info);

	qspec_object *qos_desired = new qspec_object(ot_qos_desired);
	qos_desired->set_read_only(true);
	qos_desired->set_parameter(new bandwidth(256.0)); // in bytes/sec.
	qos_desired->set_parameter(new path_latency(200000)); // in microsec.
	qspec->set_object(qos_desired);

	qspec_object *minimum_qos = new qspec_object(ot_minimum_qos);
	qos_desired->set_read_only(true);
	minimum_qos->set_parameter(new bandwidth(128.0)); // in bytes/sec.
	minimum_qos->set_parameter(new path_latency(400000)); // in microsec.
	qspec->set_object(minimum_qos);

	qspec_object *qos_available = new qspec_object(ot_qos_available);
	qos_available->set_parameter(new bandwidth(1024.0)); // our bandwidth
	qos_available->set_parameter(new path_latency(100));
	qspec->set_object(qos_available);


	/*
	 * Variables.
	 */
	NetMsg msg(1024);		// this will contain our QSPEC
	uint32 bytes_written;		// the number of bytes written

	/*
	 * Write the QSPEC into the NetMsg.
	 */
	try {
		qspec->serialize(msg, IE::protocol_v1, bytes_written);
		msg.truncate(bytes_written);
	}
	catch ( IEError err ) {
		std::cout << "Error: " << err.get_msg() << "\n";
		exit(1);
	}


	// Now hand the QSPEC to the QoS-NSLP layer.
	// ...

	delete qspec;
#endif
	NetMsg *msg = createNetMsg();
	delete msg;
}


} // namespace qspec


int main() {
	qspec::QNI qni;

	qni.run("QNI: Typical QNI operations", 100000);
}

// EOF
