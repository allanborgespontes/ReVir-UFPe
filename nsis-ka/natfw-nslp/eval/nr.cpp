/*
 * Test how many CREATE messages an NR can receive.
 *
 * $Id: nr.cpp 4118 2009-07-16 16:13:10Z bless $
 * $HeadURL: https://svn.tm.kit.edu/nsis/natfw-nslp/trunk/eval/nr.cpp $
 *
 */
#include "msg/natfw_ie.h"
#include "dispatcher.h"

#include "benchmark.h"
#include "utils.h"

#include "configfile.h"

using namespace natfw;
using namespace natfw::msg;
using namespace protlib;


class nr : public benchmark {
  public:
	nr();
	~nr();
	virtual void perform_task();

  private:
	natfw_config *conf;
	session_manager *mgr;
	nat_manager *nat_mgr;
	policy_rule_installer *installer;
	nop_dispatcher *d;
	gistka_mapper mapper;

	appladdress nr_addr;
};


nr::nr() {
	tsdb::init();
	NATFW_IEManager::clear();
	NATFW_IEManager::register_known_ies();

	conf = new natfw_config();
	mgr = new session_manager(conf);
	nat_mgr = new nat_manager(conf);
	installer = new nop_policy_rule_installer(conf);
	d = new nop_dispatcher(mgr, nat_mgr, installer, conf);

	// read all config values from config file
	configfile cfgfile(configpar_repository::instance());

	try {
		cfgfile.load("../testbed/setup_1_tb21.conf");
	}
	catch(configParException& cfgerr)
	{
		ERRLog("nf", "Error occurred while reading the configuration file: " << cfgerr.what());
		cerr << cfgerr.what() << endl << "Exiting." << endl;
		exit(1);
	}

	nr_addr = appladdress("10.38.2.212", "tcp", 1234);
}


nr::~nr() {
	delete d;
	delete installer;
	delete nat_mgr;
	delete mgr;
	delete conf;
}


void nr::perform_task() {

	/*
	 * Create an new APIMsg, each with a different session ID. The
	 * final_hop transfer attribute is set to true.
	 */
	ntlp::APIMsg *msg;
	msg = build_api_create_msg(nr_addr, true);

	event *evt = mapper.map_to_event(msg);

	d->process(evt);

	delete evt;
	delete msg;
}


int main() {
	nr n;

	n.run("nr: processing incoming CREATE messages", 100000);
}

// EOF
