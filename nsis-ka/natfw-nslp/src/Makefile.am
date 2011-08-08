# -----------------------------------*- mode: Makefile; -*--
# Makefile.am - Makefile.am for NATFW NSLP
# ----------------------------------------------------------
# $Id$
# $HeadURL$
# ==========================================================
#                      
# (C)opyright, all rights reserved by
# - Institute of Telematics, Karlsruhe Institute of Technology
# ==========================================================
#
#
include ../../Makefile.inc

bin_PROGRAMS = natfwd test_client test_stress test_stress2

#dist_natfw_scripts = 


LDADD = $(PROTLIB_LIB)

NATFW_LLIB      = natfw

NATFWMSG_LLIB   = natfw_msg

lib_LIBRARIES 	= libnatfw.a

API_INC		= $(top_srcdir)/include
INC_DIR 	= ../include/
NATFWMSG_INCDIR	= $(INC_DIR)/msg
NATFWMSG_LIBDIR	= msg


libnatfw_a_CPPFLAGS = -I$(API_INC) -I$(NATFW_INC) -I$(NATFWMSG_INCDIR) -I$(NTLP_INC) -I$(PROTLIB_INC) -I$(FQUEUE_INC)

natfwd_CPPFLAGS = -I$(API_INC) -I$(NATFW_INC) -I$(NATFWMSG_INCDIR) -I$(NTLP_INC) -I$(PROTLIB_INC) -I$(FQUEUE_INC)

test_client_CPPFLAGS = -I$(API_INC) -I$(NATFW_INC) -I$(QSPEC_INC) -I$(NTLP_INC) -I$(PROTLIB_INC) -I$(FQUEUE_INC)

test_stress_CPPFLAGS = -I$(API_INC) -I$(NATFW_INC) -I$(QSPEC_INC) -I$(NTLP_INC) -I$(PROTLIB_INC) -I$(FQUEUE_INC)

test_stress2_CPPFLAGS = -I$(API_INC) -I$(NATFW_INC) -I$(QSPEC_INC) -I$(NTLP_INC) -I$(PROTLIB_INC) -I$(FQUEUE_INC)


NATFW_SOURCEFILES = session.cpp session_manager.cpp dispatcher.cpp natfw_daemon.cpp \
	session_id.cpp ni_session.cpp nf_session.cpp nr_session.cpp nr_ext_session.cpp \
	nf_edge_ext_session.cpp nf_non_edge_ext_session.cpp natfw_config.cpp \
	policy_rule.cpp policy_rule_installer.cpp iptables_policy_rule_installer.cpp \
	nat_manager.cpp gistka_mapper.cpp natfw_timers.cpp benchmark_journal.cpp


libnatfw_a_SOURCES = $(NATFW_SOURCEFILES)

natfwd_SOURCES = natfw_daemon.cpp main.cpp
natfwd_DEPENDENCIES = libnatfw.a

test_stress_SOURCES = test_stress.cpp
test_stress_DEPENDENCIES = libnatfw.a

test_stress2_SOURCES = test_stress2.cpp
test_stress2_DEPENDENCIES = libnatfw.a

if USE_WITH_SCTP
LD_SCTP_LIB= -lsctp
endif

natfwd_LDADD = -L. -l$(NATFW_LLIB) -L$(NATFWMSG_LIBDIR) -l$(NATFWMSG_LLIB) $(LD_QSPEC_LIB) $(LD_NTLP_LIB) $(LD_PROTLIB_LIB) $(LD_FQUEUE_LIB) -lipq -lssl -lrt $(LD_SCTP_LIB)

test_stress_LDADD = -L. -l$(NATFW_LLIB) -L$(NATFWMSG_LIBDIR) -l$(NATFWMSG_LLIB) $(LD_QSPEC_LIB) $(LD_NTLP_LIB) $(LD_PROTLIB_LIB) $(LD_FQUEUE_LIB) -lipq -lssl -lrt $(LD_SCTP_LIB)

test_stress2_LDADD = -L. -l$(NATFW_LLIB) -L$(NATFWMSG_LIBDIR) -l$(NATFWMSG_LLIB) $(LD_QSPEC_LIB) $(LD_NTLP_LIB) $(LD_PROTLIB_LIB) $(LD_FQUEUE_LIB) -lipq -lssl -lrt $(LD_SCTP_LIB)

test_client_SOURCES = test_client.cpp

test_client_LDADD = -L. -l$(NATFW_LLIB) -L$(NATFWMSG_LIBDIR) -l$(NATFWMSG_LLIB) $(LD_QSPEC_LIB) $(LD_NTLP_LIB) $(LD_PROTLIB_LIB) $(LD_FQUEUE_LIB) -lipq -lssl -lrt $(LD_SCTP_LIB)



include_HEADERS = $(INC_DIR)/benchmark_journal.h $(INC_DIR)/nat_manager.h $(INC_DIR)/ni_session.h	\
			$(INC_DIR)/policy_rule_installer.h $(INC_DIR)/dispatcher.h		\
			$(INC_DIR)/natfw_config.h $(INC_DIR)/nf_edge_ext_session.h		\
			$(INC_DIR)/nr_ext_session.h $(INC_DIR)/session.h $(INC_DIR)/events.h		\
			$(INC_DIR)/natfw_daemon.h $(INC_DIR)/nf_non_edge_ext_session.h	\
			$(INC_DIR)/nr_session.h $(INC_DIR)/session_id.h $(INC_DIR)/gistka_mapper.h	\
			$(INC_DIR)/natfw_timers.h $(INC_DIR)/nf_session.h $(INC_DIR)/policy_rule.h	\
			$(INC_DIR)/session_manager.h \
			$(NATFWMSG_INCDIR)/data_terminal_info.h $(NATFWMSG_INCDIR)/icmp_types.h			\
	        	$(NATFWMSG_INCDIR)/msg_sequence_number.h $(NATFWMSG_INCDIR)/natfw_ie.h $(NATFWMSG_INCDIR)/natfw_object.h		\
	        	$(NATFWMSG_INCDIR)/ntlp_msg.h $(NATFWMSG_INCDIR)/extended_flow_info.h $(NATFWMSG_INCDIR)/ie_store.h		\
	       	 	$(NATFWMSG_INCDIR)/natfw_create.h $(NATFWMSG_INCDIR)/natfw_msg.h $(NATFWMSG_INCDIR)/natfw_response.h		\
	        	$(NATFWMSG_INCDIR)/session_lifetime.h $(NATFWMSG_INCDIR)/external_address.h			\
	        	$(NATFWMSG_INCDIR)/information_code.h $(NATFWMSG_INCDIR)/natfw_ext.h $(NATFWMSG_INCDIR)/natfw_notify.h $(NATFWMSG_INCDIR)/nonce.h

AM_CXXFLAGS = -Wall -pedantic -Wno-long-long

if USE_UNORDERED_MAP
AM_CXXFLAGS += -std=c++0x -DUSE_UNORDERED_MAP
else !USE_UNORDERED_MAP
if NSIS_NO_WARN_HASHMAP
AM_CXXFLAGS += -Wno-deprecated
endif
endif

if NATFWNSLP_DISABLE_LOGGING
AM_CXXFLAGS += -D_NO_LOGGING
endif

if NATFWNSLP_ENABLE_BENCHMARK
AM_CXXFLAGS += -DBENCHMARK
endif


# end of Makefile.am