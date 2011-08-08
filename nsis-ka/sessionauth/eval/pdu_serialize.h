#include "utils.h"
#include "Benchmark.h"
#include "data.h"

namespace nslp_auth {

class pdu_serialize : public Benchmark {

  public:
	pdu_serialize(bool auth_session_object);
	~pdu_serialize();

  protected:
	virtual void performTask();
  
  private:
  	uint32 expected_size;
	ntlp::data* data_pdu;
	NetMsg* msg;
};

}

