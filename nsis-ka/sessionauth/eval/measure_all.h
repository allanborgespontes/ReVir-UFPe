#include "utils.h"
#include "Benchmark.h"
#include "data.h"

namespace nslp_auth {

class measure_all : public Benchmark {

  public:
	measure_all(uint32 keyid_count=1, uint16 qspec_param_count=1);
	~measure_all();

  protected:
	virtual void performTask();
  
  private:
    uint32 keyid_count;
    uint32 cur_keyid;
    uint16 qspec_param_count;
  	uint32 expected_size;
	ntlp::data* data_pdu;
	ntlp::data* data_pdu_noauth;
	NetMsg* msg;
};

}

