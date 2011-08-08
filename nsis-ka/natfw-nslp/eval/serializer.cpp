/*
 * Serialize a NATFW CREATE message.
 *
 * $Id: serializer.cpp 2276 2006-11-06 08:03:31Z stud-matfried $
 * $HeadURL: https://svn.tm.kit.edu/nsis/natfw-nslp/trunk/eval/serializer.cpp $
 *
 */
#include "msg/natfw_msg.h"

#include "benchmark.h"
#include "utils.h"

using namespace natfw;
using namespace protlib;


class serializer : public benchmark {
  public:
	virtual void perform_task();
};


void serializer::perform_task() {
	NetMsg *msg = build_create_msg();
	delete msg;
}


int main() {
	serializer s;

	s.run("serializer: serializing a NATFW CREATE message", 1000000);
}

// EOF
