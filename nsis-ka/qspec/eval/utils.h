/*
 * utils.h - Helper functions
 *
 * $Id: utils.h 896 2005-11-17 11:54:18Z stud-matfried $
 * $HeadURL: https://svn.tm.kit.edu/nsis/qspec/trunk/eval/utils.h $
 *
 */
#include "network_message.h"


namespace qspec {

template<class T> inline T min(T a, T b) {
	return ( a < b ) ? a : b;
}

NetMsg *createNetMsg();

}
