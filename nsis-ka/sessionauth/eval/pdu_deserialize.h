#include "utils.h"
#include "Benchmark.h"
#include "data.h"

namespace nslp_auth {

class pdu_deserialize : public Benchmark {

  public:
	pdu_deserialize(bool with_hmac);
	~pdu_deserialize();

  protected:
	virtual void performTask();
  
  private:
  	uint32 bytes_written;
	NetMsg* msg;
};

}

