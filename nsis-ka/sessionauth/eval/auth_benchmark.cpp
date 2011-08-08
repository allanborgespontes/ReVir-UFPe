#include "auth_serialize.h"
#include "auth_deserialize.h"
#include "pdu_serialize.h"
#include "pdu_deserialize.h"
#include "pdu_deserialize_with_hmac_check_in_ntlp.h"
#include "measure_all.h"
#include <iostream>
#include <cstdlib>

int main(int argc, char *argv[]) {
	
	if (argc > 4) {
		std::cerr << "Usage: " << argv[0] << " [num_runs] [keyid_count] [qspec obj count]" << std::endl;
		exit(EXIT_FAILURE);
	}

	unsigned int num_runs = 100000;
	unsigned int keyid_count = 1;
    uint16 qspec_param_count = 1; 

	if (argc > 1)
		num_runs = atoi(argv[1]);
		
	if (argc > 2)
		keyid_count = atoi(argv[2]);

	if (argc > 3)
		qspec_param_count = atoi(argv[3]);
		
/*	nslp_auth::auth_serialize asf(false);
	asf.run("Serialize AuthSessionObject (without serialized HMAC)", num_runs);

	nslp_auth::auth_deserialize adf(false);
	adf.run("Deserialize AuthSessionObject (without check HMAC)", num_runs);

	//nslp_auth::auth_serialize ast(true);
	//ast.run("Serialize AuthSessionObject (with serialized HMAC)", num_runs);

	//nslp_auth::auth_deserialize adt(true);
	//adt.run("Deserialize AuthSessionObject (with check HMAC)", num_runs);

	nslp_auth::pdu_serialize pst(true);
	pst.run("Serialize PDU (with AuthSessionObject & HMAC)", num_runs);

	nslp_auth::pdu_deserialize pd(true);
	pd.run("Deserialize PDU (with HMAC)", num_runs);

	nslp_auth::pdu_serialize psf(false);
	psf.run("Serialize PDU (without AuthSessionObject)", num_runs);

	nslp_auth::pdu_deserialize_with_hmac_check_in_ntlp pdwhcint(true);
	pdwhcint.run("Deserialize NTLPDATA_PDU (with AuthSessionObject & correct HMAC Check in NTLP)", num_runs);

	nslp_auth::pdu_deserialize_with_hmac_check_in_ntlp pdwhcinf(false);
	pdwhcinf.run("Deserialize NTLPDATA_PDU (with AuthSessionObject & failed HMAC Check in NTLP)", num_runs);*/
	nslp_auth::measure_all mall(keyid_count, qspec_param_count);
	mall.run("Run all micro measurements", num_runs);

	return 0;
}
