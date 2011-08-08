#include "utils.h"
#include "Benchmark.h"

namespace nslp_auth {

class auth_deserialize : public Benchmark {

  public:
	auth_deserialize(bool do_Hmac = false);
	~auth_deserialize();

  protected:
	virtual void performTask();
  
  private:	
	auth_session_object* auth_obj;
	NetMsg* msg;
	uint32 bytes_written;
	bool do_hmac;
};

}

