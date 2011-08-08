/*
 * Test the throughput of an NF.
 *
 * $Id: nf.cpp 4118 2009-07-16 16:13:10Z bless $
 * $HeadURL: https://svn.tm.kit.edu/nsis/natfw-nslp/trunk/eval/nf.cpp $
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

class nf : public benchmark {
  public:
	nf(const std::string &config_filename);
	~nf();
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
	}

	mgr = new session_manager(conf);
	nat_mgr = new nat_manager(conf);
	installer = new nop_policy_rule_installer(conf);
	d = new nop_dispatcher(mgr, nat_mgr, installer, conf);

	if ( conf->is_nf_nat() ) {
		appladdress nr_priv_addr("10.38.2.212", "tcp", 12345);
		nr_addr = nat_mgr->reserve_external_address(nr_priv_addr);
	}
}


nf::~nf() {
	delete d;
	delete installer;
	delete nat_mgr;
	delete mgr;
	delete conf;
}


void nf::perform_task() {

	/*
	 * Create an new APIMsg, each with a different session ID. The
	 * final_hop transfer attribute is set to false.
	 */
	ntlp::APIMsg *msg;
	msg = build_api_create_msg(nr_addr, false);

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

	n.run("nf: processing incoming CREATE messages", 100000);
}

// EOF
