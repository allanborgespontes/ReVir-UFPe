#include "utils.h"
#include "Benchmark.h"
#include "reservemsg.h"

namespace nslp_auth {

class nslp_serialize : public Benchmark {

  public:
	nslp_serialize();
	~nslp_serialize();

  protected:
	virtual void performTask();
  
  private:
  	uint32 expected_size;	
	qos_nslp::reservereq* reserve_pdu;
	NetMsg* msg;
};

}

