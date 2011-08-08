#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.61)
AC_INIT(qosnslp-ka, 0.98, roland.bless@kit.edu)
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
#if test -d "../ntlp"; then
#   AC_CONFIG_SUBDIRS([../ntlp])
#fi
#if test -d "../qspec"; then
#   AC_CONFIG_SUBDIRS([../qspec])
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

AC_ARG_ENABLE([unorderedmap],
	[AS_HELP_STRING([--disable-unorderedmap], [disable unorderedmap (default: unorderedmap enabled)])],
	[qosnslp_unorderedmap=$enableval],
	[qosnslp_unorderedmap=yes])

# enable use of UNORDERED_MAP if it exists and its use it not disabled
AM_CONDITIONAL(USE_UNORDERED_MAP, [test "$ac_cv_unordered_map_exists" = yes -a "$qosnslp_unorderedmap" != no])

AM_CONDITIONAL(NSIS_NO_WARN_HASHMAP, test "$ac_cv_unordered_map_exists" = yes)

# AHO USE
AC_ARG_ENABLE([aho],
	[AS_HELP_STRING([--use-aho], [use anticipated handover support for advanced mobility (default: disabled)])],
	[use_aho=$enableval],
	[use_aho=no])
AM_CONDITIONAL(USE_AHO, test "$use_aho" = yes)

# FLOWINFO USE
AC_ARG_ENABLE([flowinfo],
	[AS_HELP_STRING([--use-flowinfo], [use flowinfo module for mobility support (default: disabled)])],
	[use_flowinfo=$enableval],
	[use_flowinfo=no])
AM_CONDITIONAL(USE_WITH_FLOWINFO, test "$use_flowinfo" = yes)

# SCTP USE
AC_ARG_ENABLE([sctp],
	[AS_HELP_STRING([--enable-sctp], [include SCTP transport capability (default: enabled)])],
	[enable_sctp=$enableval],
	[enable_sctp=no])
AM_CONDITIONAL(USE_WITH_SCTP, test "$enable_sctp" = yes)

# Logging
AC_ARG_ENABLE([logging],
	[AS_HELP_STRING([--disable-logging], [disable logging output (default: logging enabled)])],
	[qosnslp_logging=$enableval],
	[qosnslp_logging=yes])
AM_CONDITIONAL(QOSNSLP_DISABLE_LOGGING, test "$qosnslp_logging" = no)

# Benchmark
AC_ARG_ENABLE([benchmark],
	[AS_HELP_STRING([--enable-benchmark], [enable benchmark output (default: benchmark disabled)])],
	[qosnslp_benchmark=$enableval],
	[qosnslp_benchmark=no])
AM_CONDITIONAL(QOSNSLP_ENABLE_BENCHMARK, test "$qosnslp_benchmark" = yes)

#further CONFIG_FILES
AC_CONFIG_FILES([Makefile
                 src/Makefile
                 unittests/Makefile])
AC_OUTPUT

#end configure.ac
