#ifndef __ROUTING_UTIL_H
#define __ROUTING_UTIL_H
#include "address.h"

namespace protlib {

	namespace util {
		uint16 get_local_if(const hostaddress& sourceaddress, const hostaddress& destaddress);
		netaddress * get_src_addr(const netaddress &dest);
	}

}
#endif
