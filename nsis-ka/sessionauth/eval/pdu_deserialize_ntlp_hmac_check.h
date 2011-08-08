#include "utils.h"
#include "Benchmark.h"
#include "data.h"

namespace nslp_auth {

class pdu_deserialize_ntlp_hmac_check : public Benchmark {

  public:
	pdu_deserialize_ntlp_hmac_check(bool hmac_correct);
	~pdu_deserialize_ntlp_hmac_check();

  protected:
	virtual void performTask();
  
  private:
  	bool hmac_correct;
  	uint32 bytes_written;
	NetMsg* msg;
};

}

