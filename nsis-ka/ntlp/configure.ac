#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.
# $HeadURL: https://svn.tm.kit.edu/nsis/ntlp/trunk/configure.ac $
#

AC_PREREQ(2.61)
AC_INIT(gist-ka, 0.98, roland.bless@kit.edu)
AM_INIT_AUTOMAKE([1.11 silent-rules -Wall -Werror foreign])

# silent build by default 
# override with ./configure --disable-silent-rules or make V=1
AM_SILENT_RULES([yes]) 

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_PROG_MAKE_SET
AC_PROG_CPP

#if test -d "../protlib"; then
#   AC_CONFIG_SUBDIRS([../protlib])
#fi

AC_CHECK_LIB([cppunit], [main])

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([arpa/inet.h netdb.h netinet/in.h stdlib.h string.h sys/param.h sys/socket.h sys/time.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_CONST
AC_C_INLINE
AC_TYPE_MODE_T
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_HEADER_TIME
AC_STRUCT_TM
AC_TYPE_UINT32_T

# Checks for library functions.
AC_FUNC_ERROR_AT_LINE
AC_FUNC_MEMCMP
AC_TYPE_SIGNAL
AC_CHECK_FUNCS([gethostname gettimeofday memset socket strerror])

AC_PROG_RANLIB

#check for unordered map
[SAVECXXFLAGS=$CXXFLAGS; CXXFLAGS="-std=c++0x"]
AC_LANG_CPLUSPLUS

AC_CACHE_CHECK([whether unordered_map is available],
			[ac_cv_unordered_map_exists],
[AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <unordered_map>]],
				[[std::unordered_map<int,int> testmap;]])],
				[ac_cv_unordered_map_exists=yes],
				[ac_cv_unordered_map_exists=no])])
[CXXFLAGS=$SAVECXXFLAGS]

AM_CONDITIONAL(GIST_NO_WARN_HASHMAP, test "$ac_cv_unordered_map_exists" = yes)

AC_ARG_ENABLE([unorderedmap],
	[AS_HELP_STRING([--disable-unorderedmap], [disable unorderedmap (default: unorderedmap enabled)])],
	[gist_unorderedmap=$enableval],
	[gist_unorderedmap=yes])
# enable use of UNORDERED_MAP if it exists and its use it not disabled
AM_CONDITIONAL(USE_UNORDERED_MAP, [test "$ac_cv_unordered_map_exists" = yes -a "$gist_unorderedmap" != no])

# FLOWINFO USE
AC_ARG_ENABLE([flowinfo],
	[AS_HELP_STRING([--use-flowinfo], [use flowinfo module for mobility support (default: disabled)])],
	[use_flowinfo=$enableval],
	[use_flowinfo=no])
AM_CONDITIONAL(USE_WITH_FLOWINFO, test "$use_flowinfo" = yes)

# SCTP USE
AC_ARG_ENABLE([sctp],
	[AS_HELP_STRING([--enable-sctp], [include SCTP transport capability (default: disabled)])],
	[enable_sctp=$enableval],
	[enable_sctp=no])
AM_CONDITIONAL(USE_WITH_SCTP, test "$enable_sctp" = yes)

## libnetfilter_queue or obsolete libipq
AC_ARG_ENABLE([nfq],
	[AS_HELP_STRING([--enable-nfq], [compile with libnetfilter_queue support (default: disabled)])],
	[protlib_with_nfq=$enableval],
	[protlib_with_nfq=no])

# Logging
AC_ARG_ENABLE([logging],
	[AS_HELP_STRING([--disable-logging], [disable logging output (default: logging enabled)])],
	[gist_logging=$enableval],
	[gist_logging=yes])
AM_CONDITIONAL(GIST_DISABLE_LOGGING, test "$gist_logging" = no)


#LIB NETFILTER_QUEUE
AM_CONDITIONAL(PROTLIB_WITH_NFQ, test "$protlib_with_nfq" = yes)
if test $protlib_with_nfq = yes; then
	AC_CHECK_LIB([netfilter_queue],[nfq_get_payload],,[AC_MSG_ERROR([cannot find libnetfilter_queue, probably you need to install libnetfilter-queue-dev first])])
else
	AC_CHECK_LIB([ipq], [ipq_get_packet],,[AC_MSG_ERROR([cannot find libipq, probably you need to install iptables-dev(el) first])])
fi


#further CONFIG_FILES
AC_CONFIG_FILES([Makefile
                 src/Makefile
                 test/Makefile])
AC_OUTPUT
