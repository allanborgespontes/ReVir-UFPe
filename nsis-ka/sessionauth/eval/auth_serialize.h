#include "utils.h"
#include "Benchmark.h"

namespace nslp_auth {

class auth_serialize : public Benchmark {

  public:
	auth_serialize(bool do_Hmac=false);
	~auth_serialize();

  protected:
	virtual void performTask();
  
  private:	
	auth_session_object* auth_obj;
	NetMsg* msg;
	bool do_hmac;
};

}

