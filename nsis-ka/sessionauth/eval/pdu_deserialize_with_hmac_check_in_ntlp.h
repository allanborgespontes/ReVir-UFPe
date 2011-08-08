#include "utils.h"
#include "Benchmark.h"
#include "data.h"

namespace nslp_auth {

class pdu_deserialize_with_hmac_check_in_ntlp : public Benchmark {

  public:
	pdu_deserialize_with_hmac_check_in_ntlp(bool hmac_correct);
	~pdu_deserialize_with_hmac_check_in_ntlp();

  protected:
	virtual void performTask();
  
  private:
  	bool hmac_correct;
  	uint32 bytes_written;
	NetMsg* msg;
};

}

