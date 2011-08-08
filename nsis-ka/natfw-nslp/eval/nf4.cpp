/*
 * Test the throughput of an NF.
 *
 * Firstly, sessions in state WAITRESP are created. The benchmark is then
 * executed on the returning RESPONSE messages.
 *
 * $Id: nf3.cpp 2347 2006-11-20 11:48:59Z stud-matfried $
 * $HeadURL: https://i72projekte.tm.uka.de/nsis/natfw-nslp/trunk/eval/nf3.cpp $
 *
 */
#include "msg/natfw_ie.h"
#include "dispatcher.h"

#include "benchmark.h"
#include "benchmark_journal.h"
#include "utils.h"

#include "configfile.h"

using namespace natfw;
using namespace natfw::msg;
using namespace protlib;


#ifdef BENCHMARK
extern benchmark_journal journal;
#endif


class nf : public benchmark {
  public:
	nf(const std::string &config_filename);
	~nf();
	virtual void perform_task();
	virtual void prepare_sessions(int num);

  private:
	natfw_config *conf;
	session_manager *mgr;
	nat_manager *nat_mgr;
	policy_rule_installer *installer;
	nop_dispatcher *d;
	gistka_mapper mapper;

	appladdress nr_addr;
};


nf::nf(const std::string &config_filename) {
	tsdb::init();
	NATFW_IEManager::clear();
	NATFW_IEManager::register_known_ies();

	conf = new natfw_config();

	// read all config values from config file
	configfile cfgfile(configpar_repository::instance());

	try {
		cfgfile.load(config_filename);
	}
	catch(configParException& cfgerr)
	{
		ERRLog("nf", "Error occurred while reading the configuration file: " << cfgerr.what());
		cerr << cfgerr.what() << endl << "Exiting." << endl;
		exit(1);
	}

	mgr = new session_manager(conf);
	nat_mgr = new nat_manager(conf);
	installer = new iptables_policy_rule_installer(conf);
	d = new nop_dispatcher(mgr, nat_mgr, installer, conf);

	try {
		installer->setup();
	}
	catch ( policy_rule_installer_error &e ) {
		std::cerr << "Policy rule installer setup failed." << std::endl;
	}

	if ( conf->is_nf_nat() ) {
		appladdress nr_priv_addr("10.38.2.212", "tcp", 12345);
		nr_addr = nat_mgr->reserve_external_address(nr_priv_addr);
	}
	else
		nr_addr = appladdress("10.38.2.212", "tcp", 1234);
}


nf::~nf() {
	try {
		installer->remove_all();
	}
	catch ( policy_rule_installer_error &e ) {
		std::cerr << "Policy rule installer shutdown failed."
			<< std::endl;
	}

	delete d;
	delete installer;
	delete nat_mgr;
	delete mgr;
	delete conf;
}

void nf::prepare_sessions(int num) {

	std::cout << "preparing " << num << " sessions ..." << std::endl;
	
	for (int i=0; i < num; i++) {
		ntlp::APIMsg *msg;
		msg = build_api_create_msg(nr_addr, false);

		event *evt = mapper.map_to_event(msg);

		d->process(evt);

		delete evt;
		delete msg;
	}

	std::cout << "sessions initiated, sending responses." << std::endl;

	for (int i=0; i < num; i++) {
		ntlp::APIMsg *msg;
		msg = build_api_response_msg(nr_addr, false);

		event *evt = mapper.map_to_event(msg);

		d->process(evt);

		delete evt;
		delete msg;
	}

	std::cout << "preparation done." << std::endl;
}

void nf::perform_task() {

	/*
	 * Create an new APIMsg, each with a different session ID. The
	 * final_hop transfer attribute is set to false.
	 */
	ntlp::APIMsg *msg;
	msg = build_api_teardown_msg(nr_addr, false);

	event *evt = mapper.map_to_event(msg);

	d->process(evt);

	delete evt;
	delete msg;
}


int main(int argc, char *argv[]) {
	if ( argc != 2 ) {
		std::cerr << "Usage: nf config_filename" << std::endl;
		exit(1);
	}

	nf n(argv[1]);

	int num_sessions = 1000;
	n.prepare_sessions(num_sessions);

#ifdef BENCHMARK
	journal.restart();	// clear journal
#endif

	n.run("nf4: processing teardown messages", num_sessions);
}

// EOF
