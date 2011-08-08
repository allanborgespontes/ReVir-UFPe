/*
 * write_qspec.cpp - Write a QSPEC PDU.
 *
 * $Id: write_qspec.cpp 3236 2008-07-28 10:05:05Z bless $
 * $HeadURL: https://svn.tm.kit.edu/nsis/qspec/trunk/examples/write_qspec.cpp $
 *
 */
#include "qspec_ie.h"
#include "qspec_pdu.h"
#include "qspec_param.h"

#include "logfile.h"

using namespace qspec;


/*
 * Protlib Logging, needed for linking.
 */
protlib::log::logfile commonlog;
protlib::log::logfile &protlib::log::DefaultLog(commonlog);


int main() {
	/*
	 * Create a QSPEC PDU for reserving bandwidth (1024 bytes/sec).
	 * We use QOSM 12, which is reserved for private use.
	 * The third parameter announces that there will only be a
	 * QoS Desired and that the sender should return a
	 * QoS Reserved object if we succeed.
	 */
	qspec_pdu *qspec
	  = new qspec_pdu(ms_two_way_transactions, 1, 12);

	/*
	 * QNEs on the way should set the Non-QOSM-Hop Parameter to true
	 * if they don't support our Quality of Service Model.
	 */
	qspec_object *qos_desired = new qspec_object(ot_qos_desired);
	qos_desired->set_parameter(new t_mod(2048000.0, 576.0, 100000.0, 40));
	qspec->set_object(qos_desired);



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
	}
	catch ( IEError err ) {
		std::cout << "Error: " << err.what() << "\n";
		exit(1);
	}


	/*
	 * Print a hexdump of the generated QSPEC. We truncate the msg,
	 * otherwise it would be too long.
	 */
	msg.truncate(bytes_written);
	msg.hexdump(std::cout);

	delete qspec;	// don't forget to clean up
}
