/*
 * QNR.cpp - a benchmark for QNRs
 *
 * $Id: qnr.cpp 3236 2008-07-28 10:05:05Z bless $
 * $HeadURL: https://svn.tm.kit.edu/nsis/qspec/trunk/eval/qnr.cpp $
 *
 */
#include "qspec_ie.h"
#include "qspec_pdu.h"
#include "qspec_param.h"

#include "Benchmark.h"
#include "utils.h"


namespace qspec {

class QNR : public Benchmark {

  public:
	QNR();
	virtual ~QNR();
	virtual void performTask();

  private:
	NetMsg *msg;
};


QNR::QNR() {
	msg = createNetMsg();

	QSPEC_IEManager::clear();
	QSPEC_IEManager::register_known_ies();
}

QNR::~QNR() {
	delete msg;
}

void QNR::performTask() {

	msg->to_start();
	IEErrorList errlist;
	uint32 bytes_read;

	IE *ie = QSPEC_IEManager::instance()->deserialize(*msg,
			cat_qspec_pdu, IE::protocol_v1, errlist,
			bytes_read, true);

	qspec_pdu *qspec = dynamic_cast<qspec_pdu *>(ie);

	//cout << *ie << "\n";

	if ( qspec == NULL ) {
		cout << "Parsing failed.\n";
		exit(1);
	}
		

	/*
	 * We only support QOSM 12.
	 */
	if ( qspec->get_qspec_type() != 12 ) {
	  cout << "only supporting QSPEC type 12 right now" << endl;
	}


	qspec_object *desired = qspec->get_object(ot_qos_desired);

	if ( ! desired ) {
		cout << "Incomplete QSPEC\n";
		return;
	}

	// Access the QSPEC, create a response.
}


} // namespace qspec


int main() {
	qspec::QNR qnr;

	qnr.run("QNR: Typical QNR operations", 100000);
}

// EOF
