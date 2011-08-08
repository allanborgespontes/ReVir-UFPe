/*
 * Deserialize a NATFW CREATE message.
 *
 * $Id: deserializer.cpp 2276 2006-11-06 08:03:31Z stud-matfried $
 * $HeadURL: https://svn.tm.kit.edu/nsis/natfw-nslp/trunk/eval/deserializer.cpp $
 *
 */
#include "msg/natfw_ie.h"
#include "msg/natfw_msg.h"

#include "benchmark.h"
#include "utils.h"

using namespace natfw;
using namespace natfw::msg;
using namespace protlib;


class deserializer : public benchmark {
  public:
	deserializer();
	virtual void perform_task();

  private:
	NetMsg *msg;
};


deserializer::deserializer() {
	msg = build_create_msg();

	NATFW_IEManager::clear();
	NATFW_IEManager::register_known_ies();
}

void deserializer::perform_task() {

	msg->to_start();
	IEErrorList errlist;
	uint32 bytes_read;

	IE *ie = NATFW_IEManager::instance()->deserialize(*msg,
		cat_natfw_msg, IE::protocol_v1, errlist, bytes_read, true);

	if ( ie == NULL ) {
		std::cerr << "Parsing failed.\n";
		exit(1);
	}
}


int main() {
	deserializer d;

	d.run("deserializer: deserializing a NATFW CREATE message", 1000000);
}

// EOF
