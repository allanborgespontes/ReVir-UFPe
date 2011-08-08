# -----------------------------------*- mode: Makefile; -*--
# Makefile - Makefile for session authorization object
# ----------------------------------------------------------
# $Id$
# $HeadURL$
# ==========================================================
#                      
# (C)opyright 2011, all rights reserved by
# - Institute of Telematics, Karlsruhe Institute of Technology
# ==========================================================
#
#
include $(top_srcdir)/../Makefile.inc

lib_LIBRARIES = libnslpauth.a

libnslpauth_a_SOURCES = auth_attr_addr.cpp auth_attr_addr_spi.cpp	\
			auth_attr.cpp auth_attr_data.cpp		\
			auth_attr_hmac_trans_id.cpp auth_attr_ie.cpp	\
			auth_attr_portlist.cpp auth_attr_string.cpp	\
			auth_attr_time.cpp auth_nslp_object_list.cpp	\
			benchmark_journal.cpp hmac_keyregistry.cpp	\
			session_auth_api.cpp session_auth_object.cpp

include_HEADERS = ../include/auth_attr_addr.h	\
	../include/auth_attr_addr_spi.h \
	../include/auth_attr_data.h \
	../include/auth_attr.h \
	../include/auth_attr_hmac_trans_id.h \
	../include/auth_attr_ie.h \
	../include/auth_attr_portlist.h \
	../include/auth_attr_string.h \
	../include/auth_attr_time.h \
	../include/auth_nslp_object_list.h \
	../include/benchmark_journal.h \
	../include/hmac_keyregistry.h \
	../include/session_auth_api.h \
	../include/session_auth_object.h


libnslpauth_a_CPPFLAGS = -I$(top_srcdir)/include -I$(QOSNSLP_INC) -I$(QSPEC_INC) -I$(NTLP_INC) -I$(NTLP_SRC)/pdu -I$(PROTLIB_INC) -I$(FQUEUE_INC)

AM_CXXFLAGS = -Wall -pedantic -Wno-long-long

if USE_UNORDERED_MAP
AM_CXXFLAGS += -std=c++0x -DUSE_UNORDERED_MAP
else !USE_UNORDERED_MAP
if NSIS_NO_WARN_HASHMAP
AM_CXXFLAGS += -Wno-deprecated
endif
endif

if SAUTH_DISABLE_LOGGING
AM_CXXFLAGS += -D_NO_LOGGING
endif

if SAUTH_ENABLE_BENCHMARK
AM_CXXFLAGS += -DBENCHMARK
endif
