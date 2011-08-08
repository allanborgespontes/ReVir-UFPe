/*
 * Test the throughput of an NI.
 *
 * $Id: ni.cpp 2293 2006-11-08 09:44:29Z stud-matfried $
 * $HeadURL: https://svn.tm.kit.edu/nsis/natfw-nslp/trunk/eval/ni.cpp $
 *
 */
#include "msg/natfw_ie.h"
#include "dispatcher.h"

#include "benchmark.h"
#include "utils.h"

using namespace natfw;
using namespace natfw::msg;
using namespace protlib;



class ni : public benchmark {
  public:
	ni();
	~ni();
	virtual void perform_task();

  private:
	natfw_config *conf;
	session_manager *mgr;
	nat_manager *nat_mgr;
	nop_dispatcher *d;
	gistka_mapper mapper;

	hostaddress source;
	hostaddress destination;
};


ni::ni() {
	tsdb::init();
	NATFW_IEManager::clear();
	NATFW_IEManager::register_known_ies();

	conf = new natfw_config();
	mgr = new session_manager(conf);
	nat_mgr = new nat_manager(conf);
	d = new nop_dispatcher(mgr, nat_mgr, NULL, conf);

	source = hostaddress("141.3.70.5");
	destination = hostaddress("141.3.70.4");
}


ni::~ni() {
	delete d;
	delete nat_mgr;
	delete mgr;
	delete conf;
}


void ni::perform_task() {
	event *e1 = new api_create_event(source, destination, 1234, 4321, 6,
			std::list<uint8>(), 30);
	NatFwEventMsg *msg = new NatFwEventMsg(session_id(), e1);

	event *e2 = mapper.map_to_event(msg);

	d->process(e2);

	delete e2;
}


int main() {
	ni n;

	n.run("ni: processing api_create_events", 100000);
}

// EOF
