# -----------------------------------*- mode: Makefile; -*--
# Makefile.am - Makefile for QOS NSLP
# ----------------------------------------------------------
# $Id$
# $HeadURL: https://svn.ipv6.tm.uka.de/nsis/ntlp/branches/20091014-autotools/src/Makefile $
# ==========================================================
#                      
# (C)opyright, all rights reserved by
# - Institute of Telematics, Karlsruhe Institute of Technology
# ==========================================================

include ../../Makefile.inc

bin_PROGRAMS = qosnslpd client vlsp-client

dist_qosnslp_scripts = start-qosnslp scripts/ar_shaper scripts/be_shaper scripts/start_ds_marking scripts/stop_ds_marking 

QOSNSLP_LLIB      = qosnslp

lib_LIBRARIES = libqosnslp.a

API_INC		= $(top_srcdir)/include

libqosnslp_a_CPPFLAGS = -I$(API_INC) -I$(QOSNSLP_INC) -I$(QSPEC_INC) -I$(AUTH_INC) -I$(NTLP_INC) -I$(PROTLIB_INC) -I$(FQUEUE_INC)

qosnslpd_CPPFLAGS = -I$(API_INC) -I$(QOSNSLP_INC) -I$(QSPEC_INC) -I$(AUTH_INC) -I$(NTLP_INC) -I$(PROTLIB_INC) -I$(FQUEUE_INC)

client_CPPFLAGS = -I$(API_INC) -I$(QOSNSLP_INC) -I$(QSPEC_INC)  -I$(AUTH_INC) -I$(NTLP_INC) -I$(PROTLIB_INC) -I$(FQUEUE_INC)

vlsp_client_CPPFLAGS = -I$(API_INC) -I$(QOSNSLP_INC) -I$(QSPEC_INC)  -I$(AUTH_INC) -I$(NTLP_INC) -I$(PROTLIB_INC) -I$(FQUEUE_INC)

qosnslpd_DEPENDENCIES = $(NTLP_LIB) $(QSPEC_LIB) $(AUTH_LIB) libqosnslp.a 
vlsp_client_DEPENDENCIES = $(NTLP_LIB) $(QSPEC_LIB) $(AUTH_LIB) libqosnslp.a
client_DEPENDENCIES = $(NTLP_LIB) $(QSPEC_LIB) $(AUTH_LIB) libqosnslp.a

QOSNSLP_SOURCEFILES =  qos_nslp.cpp ProcessingModule.cpp rsn.cpp rii.cpp refresh_period.cpp bound_sessionid.cpp qspec.cpp nslp_ie.cpp \
		reservemsg.cpp responsemsg.cpp querymsg.cpp notifymsg.cpp packet_classifier.cpp nslp_session_context_map.cpp \
		info_spec.cpp nslp_object.cpp nslp_pdu.cpp nslp_session_context.cpp nslp_flow_context.cpp aggregate.cpp \
		rmf.cpp QoS_StateModule.cpp QoS_Appl_Msg.cpp SignalingAppl.cpp QoS_NSLP_API.cpp QoS_NSLP_Client_API.cpp \
	       	QoS_NSLP_UDS_API.cpp TestConsole.cpp session_id_list.cpp rsn_list.cpp qos_nslp_conf.cpp qosnslp_starter.cpp \
		vlsp_object.cpp context_manager.cpp \
		benchmark_journal.cpp benchmark_journal.h

if USE_AHO
QOSNSLP_SOURCEFILES += qos_nslp_aho_contextmap.cpp nslp_aho_context.cpp QoS_StateModule_AHO_Processing.cpp
qosnslpd_CPPFLAGS += -DUSE_AHO
client_CPPFLAGS += -DUSE_AHO
endif

if USE_WITH_SCTP
LD_SCTP_LIB= -lsctp
endif

libqosnslp_a_SOURCES = $(QOSNSLP_SOURCEFILES)


qosnslpd_SOURCES = qosnslpd.cpp

qosnslpd_LDADD = -L. -l$(QOSNSLP_LLIB) $(LD_QSPEC_LIB) $(LD_AUTH_LIB) $(LD_NTLP_LIB) $(LD_PROTLIB_LIB) $(LD_FQUEUE_LIB) -lipq -lssl -lrt $(LD_SCTP_LIB)


client_SOURCES = client.cpp

client_LDADD = -L. -l$(QOSNSLP_LLIB) $(LD_QSPEC_LIB) $(LD_AUTH_LIB) $(LD_NTLP_LIB) $(LD_PROTLIB_LIB) $(LD_FQUEUE_LIB) -lipq -lssl -lrt $(LD_SCTP_LIB)


vlsp_client_SOURCES = vlsp-client.cpp

vlsp_client_LDADD = -L. -l$(QOSNSLP_LLIB) $(LD_QSPEC_LIB) $(LD_AUTH_LIB) $(LD_NTLP_LIB) $(LD_PROTLIB_LIB) $(LD_FQUEUE_LIB) -lipq -lssl -lrt $(LD_SCTP_LIB)


nobase_include_HEADERS=$(top_srcdir)/include/aggregate.h $(top_srcdir)/include/all_nslp_ies.h \
	$(top_srcdir)/include/bound_sessionid.h $(top_srcdir)/include/info_spec.h \
	$(top_srcdir)/include/notifymsg.h $(top_srcdir)/include/nslp_aho_context.h \
	$(top_srcdir)/include/nslp_flow_context.h $(top_srcdir)/include/nslp_ie.h \
	$(top_srcdir)/include/nslp_object.h $(top_srcdir)/include/nslp_pdu.h \
	$(top_srcdir)/include/nslp_session_context.h \
	$(top_srcdir)/include/nslp_session_context_map.h $(top_srcdir)/include/packet_classifier.h \
	$(top_srcdir)/include/ProcessingModule.h $(top_srcdir)/include/qos_aggr_object.h \
	$(top_srcdir)/include/QoS_Appl_Msg.h $(top_srcdir)/include/qos_nslp_aho_contextmap.h \
	$(top_srcdir)/include/QoS_NSLP_API.h $(top_srcdir)/include/QoS_NSLP_Client_API.h \
	$(top_srcdir)/include/qos_nslp.h \
	$(top_srcdir)/include/qosnslp_starter.h $(top_srcdir)/include/QoS_NSLP_UDS_API.h \
	$(top_srcdir)/include/QoS_StateModule.h $(top_srcdir)/include/qspec.h $(top_srcdir)/include/querymsg.h \
	$(top_srcdir)/include/refresh_period.h $(top_srcdir)/include/reservemsg.h \
	$(top_srcdir)/include/responsemsg.h $(top_srcdir)/include/rii.h $(top_srcdir)/include/rmf.h \
	$(top_srcdir)/include/rsn.h $(top_srcdir)/include/rsn_list.h $(top_srcdir)/include/session_id_list.h \
	$(top_srcdir)/include/SignalingAppl.h $(top_srcdir)/include/TestConsole.h $(top_srcdir)/include/qos_nslp_conf.h \
	$(top_srcdir)/include/vlsp_object.h $(top_srcdir)/include/context_manager.h

AM_CXXFLAGS = -Wall -pedantic -Wno-long-long

if USE_UNORDERED_MAP
AM_CXXFLAGS += -std=c++0x -DUSE_UNORDERED_MAP
else !USE_UNORDERED_MAP
if NSIS_NO_WARN_HASHMAP
AM_CXXFLAGS += -Wno-deprecated
endif
endif

if QOSNSLP_DISABLE_LOGGING
AM_CXXFLAGS += -D_NO_LOGGING
endif

if USE_WITH_FLOWINFO
ifdef AM_CXXFLAGS += -DUSE_FLOWINFO
endif

if QOSNSLP_ENABLE_BENCHMARK
AM_CXXFLAGS += -DBENCHMARK
endif


# end of Makefile.am