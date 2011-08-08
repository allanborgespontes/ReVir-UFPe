# -----------------------------------*- mode: Makefile; -*--
# Makefile - Makefile for QSPEC template
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
include $(top_srcdir)/../Makefile.inc

lib_LIBRARIES = libqspec.a

libqspec_a_CPPFLAGS = -I$(top_srcdir)/include -I./pdu -I$(PROTLIB_INC) -I$(FQUEUE_INC)

libqspec_a_SOURCES= 	admission_priority.cpp path_jitter.cpp		\
			phb_class.cpp qspec_param.cpp slack_term.cpp	\
			dste_class_type.cpp path_latency.cpp		\
			preemption_priority.cpp qspec_param_raw.cpp	\
			t_mod.cpp excess_treatment.cpp path_per.cpp	\
			qspec_ie.cpp qspec_pdu.cpp y1541_qos_class.cpp	\
			ie_store.cpp path_plr.cpp qspec_object.cpp	\
			rph_priority.cpp

include_HEADERS = ../include/admission_priority.h			\
			../include/path_latency.h			\
			../include/qspec_ie.h				\
			../include/rph_priority.h			\
			../include/dste_class_type.h			\
			../include/path_per.h				\
			../include/qspec_object.h			\
			../include/slack_term.h				\
			../include/excess_treatment.h			\
			../include/path_plr.h ../include/qspec_param.h	\
			../include/t_mod.h ../include/ie_store.h	\
			../include/phb_class.h				\
			../include/qspec_param_raw.h			\
			../include/y1541_qos_class.h			\
			../include/path_jitter.h			\
			../include/preemption_priority.h		\
			../include/qspec_pdu.h

AM_CXXFLAGS = -Wall -pedantic -Wno-long-long

if USE_UNORDERED_MAP
AM_CXXFLAGS += -std=c++0x -DUSE_UNORDERED_MAP
else !USE_UNORDERED_MAP
if NSIS_NO_WARN_HASHMAP
AM_CXXFLAGS += -Wno-deprecated
endif
endif

if QSPEC_DISABLE_LOGGING
AM_CXXFLAGS += -D_NO_LOGGING
endif

if QSPEC_ENABLE_BENCHMARK
AM_CXXFLAGS += -DBENCHMARK
endif
