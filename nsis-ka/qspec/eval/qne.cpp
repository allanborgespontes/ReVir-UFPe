/*
 * QNE.cpp - a benchmark for QNEs
 *
 * $Id: qne.cpp 3236 2008-07-28 10:05:05Z bless $
 * $HeadURL: https://svn.tm.kit.edu/nsis/qspec/trunk/eval/qne.cpp $
 *
 */
#include "qspec_ie.h"
#include "qspec_pdu.h"
#include "qspec_param.h"

#include "Benchmark.h"
#include "utils.h"


namespace qspec {

class QNE : public Benchmark {

  public:
	QNE();
	virtual ~QNE();
	virtual void performTask();

  private:
	NetMsg *msg;
};


QNE::QNE() {
	msg = createNetMsg();

	QSPEC_IEManager::clear();
	QSPEC_IEManager::register_known_ies();
}

QNE::~QNE() {
	delete msg;
}

void QNE::performTask() {

	msg->to_start();
	IEErrorList errlist;
	uint32 bytes_read;

	IE *ie = QSPEC_IEManager::instance()->deserialize(*msg,
			cat_qspec_pdu, IE::protocol_v1, errlist,
			bytes_read, true);

	qspec_pdu *qspec = dynamic_cast<qspec_pdu *>(ie);

	//cout << *ie << "\n";

	if ( qspec == NULL ) {
	  cout << "Parsing failed." << endl;
		exit(1);
	}
		

	/*
	 * We only support QSPEC type 12.
	 */
	if ( qspec->get_qspec_type() != 12 ) {
	  cout << "only supporting QSPEC type 12 right now" << endl;
	}


	qspec_object *desired = qspec->get_object(ot_qos_desired);

	if ( ! desired ) {
		cout << "Incomplete QSPEC\n";
		return;
	}

	// The RMF would give us that information.
	t_mod our_t_mod(2048000.0, 576.0, 100000.0, 40);
	uint32 our_latency = 1000;


	/*
	 * Examine QoS Desired.
	 */
	t_mod *bw = dynamic_cast<t_mod *>(
				desired->get_parameter(t_mod::PARAM_ID1));

	float max_t_mod_rate = bw->get_rate();


	path_latency *latency = dynamic_cast<path_latency *>(
				desired->get_parameter(path_latency::PARAM_ID));

	uint32 max_latency = latency->get_value();


	/*
	 * Examine QoS Minimum.
	 */
	qspec_object *minimum = qspec->get_object(ot_qos_desired);

	float min_t_mod_rate;
	uint32 min_latency;

	if ( minimum ) {
		t_mod *min_bw = dynamic_cast<t_mod *>(
			minimum->get_parameter(t_mod::PARAM_ID1));

		min_t_mod_rate = min_bw->get_rate();

		path_latency *min_lat = dynamic_cast<path_latency *>(
			minimum->get_parameter(path_latency::PARAM_ID));

		min_latency = min_lat->get_value();
	}
	else {
		min_t_mod_rate = max_t_mod_rate;
		min_latency = max_latency;
	}


	/*
	 * Update QoS Available.
	 */
	qspec_object *available = qspec->get_object(ot_qos_available);

	uint32 accumulated_latency = 0;

	if ( available ) {
		t_mod *avail_bw = dynamic_cast<t_mod *>(
			available->get_parameter(t_mod::PARAM_ID1));

		avail_bw->set_rate(min(our_t_mod.get_rate(), avail_bw->get_rate()));


		path_latency *avail_lat = dynamic_cast<path_latency *>(
			available->get_parameter(path_latency::PARAM_ID));

		accumulated_latency = avail_lat->get_value() + our_latency;
		avail_lat->set_value(accumulated_latency);
	}

	
	/*
	 * Check if we can fulfill the request.
	 */
	if ( our_t_mod.get_rate() < min_t_mod_rate ) {
		cout << "Not enough bandwidth to fulfill request\n";
		return;
	}

	if ( accumulated_latency > max_latency ) {
		cout << "Can't fulfill request. Path latency too high\n";
		return;
	}

	//cout << *ie << "\n";


	/*
	 * Serialize the modified QSPEC.
	 */
	NetMsg msg_out(1024);	// this will contain our QSPEC
	uint32 bytes_written;	// the number of bytes written

	try {
		qspec->serialize(msg_out, IE::protocol_v1, bytes_written);
		msg_out.truncate(bytes_written);
	}
	catch ( IEError err ) {
		std::cout << "Error: " << err.what() << "\n";
		exit(1);
	}

	// Now send msg_out to the next QNE ...
}


} // namespace qspec


int main() {
	qspec::QNE qne;

	qne.run("QNE: Typical QNE operations", 100000);
}

// EOF
