/*
 * read_qspec.cpp - Read a QSPEC PDU.
 *
 * $Id: read_qspec.cpp 3236 2008-07-28 10:05:05Z bless $
 * $HeadURL: https://svn.tm.kit.edu/nsis/qspec/trunk/examples/read_qspec.cpp $
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


/*
 * Create a QSPEC that we can read later. See write_qspec.cpp for more
 * information on creating QSPECs.
 */
void generate_qspec(NetMsg &msg, uint32 &length) {

	qspec_pdu *qspec
	  = new qspec_pdu(ms_two_way_transactions, 1, 12);

	qspec_object *qos_desired = new qspec_object(ot_qos_desired);
	qos_desired->set_parameter(new t_mod(2048000.0, 576.0, 100000.0, 40));
	qspec->set_object(qos_desired);

	qspec->serialize(msg, IE::protocol_v1, length);

	delete qspec;
}


int main() {
	/*
	 * Setup the QSPEC_IEManager.
	 */
	QSPEC_IEManager::clear();
	QSPEC_IEManager::register_known_ies();

	IEManager *iem = QSPEC_IEManager::instance();


	/*
	 * Variables.
	 */
	NetMsg msg(1024);		// this contains our QSPEC
	uint32 length;			// the QSPEC's length
	uint32 bytes_read;		// the number of bytes read
	IEErrorList errorlist;		// a queue containing IEError objects


	/*
	 * Get a QSPEC from somewhere. It is VERY IMPORTANT that the
	 * NetMsg has exactly the size of the QSPEC and that the position
	 * pointer is set to 0.
	 */
	generate_qspec(msg, length);
	msg.truncate(length);		// very important!
	msg.set_pos(0);			// dito.


	/*
	 * Read a QSPEC PDU from msg. Try to skip errors if possible but
	 * store all encountered errors in errorlist. Return the number of
	 * read bytes in bytes_read. Return the deserialized IE on
	 * success, and NULL on error.
	 */
	IE *ie = iem->deserialize(msg, cat_qspec_pdu, IE::protocol_v1,
			errorlist, bytes_read, true);


	/*
	 * Print all errors encountered during reading.
	 */
	if ( ! errorlist.is_empty() ) {
		IEError *err;

		while ( (err = errorlist.get()) != NULL ) {
			std::cout << *err << "\n";
			delete err;
		}
	}


	/*
	 * Quit if there was a fatal error.
	 */
	if ( ie == NULL ) {
		std::cout << "Error: QSPEC PDU unparsable.\n";
		exit(1);
	}


	qspec_pdu *pdu = dynamic_cast<qspec_pdu *>(ie); // check the cast!


	/*
	 * Now we have a qspec_pdu. Use it.
	 */
	if ( pdu->get_qspec_type() != 12 ) {
		std::cout << "Error: QSPEC type unsupported.\n";
		exit(1);
	}

	qspec_object *qos_desired = pdu->get_object(ot_qos_desired);

	if ( qos_desired == NULL ) {
		std::cout << "Error: QSPEC incomplete: QoS Desired missing.\n";
		exit(1);
	}

	t_mod *bw = dynamic_cast<t_mod *>(
		qos_desired->get_parameter(t_mod::PARAM_ID1));

	if ( bw == NULL ) {
		std::cout << "Error: QSPEC incomplete: TMOD-1 missing.\n";
		exit(1);
	}

	std::cout << "Success: QNI wants to reserve a bandwidth of "
		<< bw->get_rate() << " bytes per second.\n";

	delete ie;	// don't forget to clean up
}
