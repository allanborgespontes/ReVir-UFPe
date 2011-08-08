#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.
# $Id$
# $HeadURL$
#
AC_INIT([protlib-ka], [0.0.0.5551], [roland.bless@kit.edu])
AM_INIT_AUTOMAKE([1.11 silent-rules -Wall -Werror foreign])

# silent build by default 
# override with ./configure --disable-silent-rules or make V=1
AM_SILENT_RULES([yes]) 

# Checks for programs.
AC_PROG_CC
AC_PROG_CXX
AC_PROG_RANLIB

# Checks for libraries.
# FIXME: Replace `main' with a function in `-lcrypto':
#AC_CHECK_LIB([crypto], [main])
# FIXME: Replace `main' with a function in `-ldl':
#AC_CHECK_LIB([dl], [main])
# FIXME: Replace `main' with a function in `-lipq':
AC_CHECK_LIB([ssl], [SSL_CTX_new])
AC_CHECK_LIB([pthread], [pthread_create])
AC_CHECK_LIB([rt], [clock_gettime])
# FIXME: Replace `main' with a function in `-lsctp':



AC_ARG_ENABLE([debug],
	[AS_HELP_STRING([--disable-debug], [disable debug code (default: debugging enabled)])],
	[protlib_debug=$enableval],
	[protlib_debug=yes])
AM_CONDITIONAL(PROTLIB_DEBUG, test "$protlib_debug" = yes)

AC_ARG_ENABLE([sctp],
	[AS_HELP_STRING([--enable-sctp], [compile with SCTP support (default: SCTP disabled)])],
	[protlib_with_sctp_kern=$enableval],
	[protlib_with_sctp_kern=no])

AM_CONDITIONAL(PROTLIB_WITH_SCTP_KERN, test "$protlib_with_sctp_kern" = yes)
if test $protlib_with_sctp_kern = yes; then
	AC_CHECK_HEADERS([netinet/sctp.h], [],
		[AC_MSG_ERROR([cannot find netinet/sctp.h in include path.
You should consider to compile without SCTP support.])])
        AC_CHECK_LIB([sctp], [sctp_send])
fi

AC_ARG_ENABLE([logging],
	[AS_HELP_STRING([--disable-logging], [disable logging output (default: logging enabled)])],
	[protlib_logging=$enableval],
	[protlib_logging=yes])
AM_CONDITIONAL(PROTLIB_DISABLE_LOGGING, test "$protlib_logging" = no)

## libnetfilter_queue or obsolete libipq
AC_ARG_ENABLE([nfq],
	[AS_HELP_STRING([--enable-nfq], [compile with libnetfilter_queue support (default: disabled)])],
	[protlib_with_nfq=$enableval],
	[protlib_with_nfq=no])

#LIB NETFILTER_QUEUE
AM_CONDITIONAL(PROTLIB_WITH_NFQ, test "$protlib_with_nfq" = yes)
if test $protlib_with_nfq = yes; then
	AC_CHECK_HEADERS([libnetfilter_queue/libnetfilter_queue.h], [],
		[AC_MSG_ERROR([cannot find libnetfilter_queue/libnetfilter_queue.h in include path, probably you need to install libnetfilter-queue-dev first])],[[#include<libnetfilter_queue/libnetfilter_queue.h>]])
	AC_CHECK_LIB([netfilter_queue],[nfq_get_payload],,[AC_MSG_ERROR([cannot find libnetfilter_queue, probably you need to install libnetfilter-queue-dev first])])
	AC_CHECK_FILE([/usr/include/libnetfilter_queue], [NETFILTER_QUEUE_INCLUDE=" -I /usr/include/libnetfilter_queue"], [])
else
	AC_CHECK_LIB([ipq], [ipq_get_packet],,[AC_MSG_ERROR([cannot find libipq, probably you need to install iptables-dev(el) first])])
	AC_CHECK_FILE([/usr/include/libipq], [NETFILTER_QUEUE_INCLUDE=" -I /usr/include/libipq"], [])
fi


AC_SUBST([NETFILTER_QUEUE_INCLUDE])


[SAVECXXFLAGS=$CXXFLAGS; CXXFLAGS="-std=c++0x"]
AC_LANG_CPLUSPLUS

AC_CACHE_CHECK([whether unordered_map is available],
			[ac_cv_unordered_map_exists],
[AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <unordered_map>]],
				[[std::unordered_map<int,int> testmap;]])],
				[ac_cv_unordered_map_exists=yes],
				[ac_cv_unordered_map_exists=no])])
[CXXFLAGS=$SAVECXXFLAGS]

AC_ARG_ENABLE([unorderedmap],
	[AS_HELP_STRING([--disable-unorderedmap], [disable unorderedmap (default: unorderedmap enabled)])],
	[protlib_unorderedmap=$enableval],
	[protlib_unorderedmap=yes])

AM_CONDITIONAL(PROTLIB_NO_WARN_HASHMAP, test "$ac_cv_unordered_map_exists" = yes)
# enable use of UNORDERED_MAP if it exists and its use it not disabled
AM_CONDITIONAL(USE_UNORDERED_MAP, [test "$ac_cv_unordered_map_exists" = yes -a "$protlib_unorderedmap" != no])


# check for RFC3542 availability, if not, we assume at least RFC 2292
AC_CHECK_FUNC([inet6_opt_init],[use_rfc2292=no],[use_rfc2292=yes])
AM_CONDITIONAL([USE_RFC2292], test "$use_rfc2292" = yes)

# Checks for header files.
AC_CHECK_HEADERS([arpa/inet.h fcntl.h netdb.h netinet/in.h stdlib.h string.h sys/socket.h sys/time.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDC
AC_HEADER_STDBOOL
AC_C_CONST
AC_C_INLINE
AC_TYPE_INT32_T
AC_TYPE_INT64_T
AC_TYPE_SIZE_T
AC_HEADER_TIME
AC_STRUCT_TM
AC_TYPE_UID_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT8_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_MEMCMP
AC_TYPE_SIGNAL
AC_FUNC_STRERROR_R
AC_FUNC_STRFTIME
AC_CHECK_FUNCS([clock_gettime gethostname gettimeofday memmove memset socket strchr strerror strtol])



#AC_CHECK_HEADER([libipq.h], [],)

AC_LANG_CPLUSPLUS

PKG_CHECK_MODULES([CPPUNIT], [cppunit], [],
	[AC_MSG_WARN($CPPUNIT_PKG_ERRORS. Make target 'check' will fail!)])

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([
	Makefile
	fastqueue/Makefile
	src/Makefile
	test/Makefile
])
AC_OUTPUT
