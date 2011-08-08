lib_LIBRARIES = libprot.a


libprot_a_SOURCES = address.cpp addresslist.cpp ie.cpp tp.cpp tp_over_tcp.cpp \
		tp_over_tls_tcp.cpp tp_over_udp.cpp \
		tp_over_uds.cpp connectionmap.cpp connectionmap_uds.cpp \
		queuemanager.cpp fqueue.cpp timer.cpp timer_module.cpp \
		logfile.cpp fqueue.cpp protlibconf.cpp protlib_types.cpp threads.cpp \
		threadsafe_db.cpp tlp_list.cpp setuid.cpp messages.cpp \
		network_message.cpp configuration.cpp \
		configpar.cpp configpar_repository.cpp configfile.cpp \
		routing_util.cpp readnl.cpp cmsghdr_util.cpp

libprot_a_DEPENDENCIES = $(FQUEUE_LIB)

if PROTLIB_WITH_NFQ
libprot_a_SOURCES += tp_queryencap_nfq.cpp
else
libprot_a_SOURCES += tp_queryencap.cpp
endif

nobase_include_HEADERS=$(top_srcdir)/include/address.h			\
	$(top_srcdir)/include/addresslist.h				\
	$(top_srcdir)/include/assocdata.h				\
	$(top_srcdir)/include/assocdata_uds.h				\
	$(top_srcdir)/include/cleanuphandler.h				\
	$(top_srcdir)/include/configfile.h				\
	$(top_srcdir)/include/configpar.h				\
	$(top_srcdir)/include/configpar_repository.h			\
	$(top_srcdir)/include/configuration.h				\
	$(top_srcdir)/include/connectionmap.h				\
	$(top_srcdir)/include/connectionmap_uds.h			\
	$(top_srcdir)/include/fqueue.h $(top_srcdir)/include/ie.h	\
	$(top_srcdir)/include/llhashers.h				\
	$(top_srcdir)/include/logfile.h					\
	$(top_srcdir)/include/messages.h				\
	$(top_srcdir)/include/network_message.h				\
	$(top_srcdir)/include/protlibconf.h				\
	$(top_srcdir)/include/protlib_types.h				\
	$(top_srcdir)/include/queuemanager.h				\
	$(top_srcdir)/include/radixtrie.h				\
	$(top_srcdir)/include/readnl.h					\
	$(top_srcdir)/include/rfc5014_hack.h				\
	$(top_srcdir)/include/routing_util.h				\
	$(top_srcdir)/include/setuid.h					\
	$(top_srcdir)/include/test_util.h				\
	$(top_srcdir)/include/threadsafe_db.h				\
	$(top_srcdir)/include/threads.h 				\
	$(top_srcdir)/include/timer.h					\
	$(top_srcdir)/include/timer_module.h				\
	$(top_srcdir)/include/tlp_list.h				\
	$(top_srcdir)/include/tperror.h 				\
	$(top_srcdir)/include/tp.h					\
	$(top_srcdir)/include/tp_over_sctp.h				\
	$(top_srcdir)/include/tp_over_tcp.h				\
	$(top_srcdir)/include/tp_over_tls_tcp.h				\
	$(top_srcdir)/include/tp_over_udp.h				\
	$(top_srcdir)/include/tp_over_uds.h				\
	$(top_srcdir)/src/cmsghdr_util.h

if PROTLIB_WITH_NFQ
nobase_include_HEADERS += $(top_srcdir)/include/tp_queryencap_nfq.h
else
nobase_include_HEADERS += $(top_srcdir)/include/tp_queryencap.h
endif


libprot_a_CPPFLAGS = -I$(top_srcdir)/include -I$(top_srcdir)/fastqueue $(NETFILTER_QUEUE_INCLUDE)
libprot_a_LIBADD = $(top_builddir)/fastqueue/libfastqueue.a 

AM_CXXFLAGS = -Wall -pedantic -Wno-long-long

if PROTLIB_WITH_SCTP_KERN
AM_CXXFLAGS += -D_USE_SCTP
libprot_a_SOURCES += tp_over_sctp.cpp
else
AM_CXXFLAGS += -D_USE_SCTP_WRAPPERS
endif

if PROTLIB_DEBUG
AM_CXXFLAGS += -g -D_DEBUG
endif

#PROTLIB_CXXFLAGS       += -DHAS_RFC3542 # for newer glibc with advanced IPv6 API support according to RFC3542
if USE_RFC2292
AM_CXXFLAGS += -DHAS_RFC2292
endif

if PROTLIB_DISABLE_LOGGING
AM_CXXFLAGS += -D_NO_LOGGING
endif


if USE_UNORDERED_MAP
AM_CXXFLAGS += -std=c++0x -DUSE_UNORDERED_MAP
else !USE_UNORDERED_MAP
if PROTLIB_NO_WARN_HASHMAP
AM_CXXFLAGS += -Wno-deprecated
endif
endif



