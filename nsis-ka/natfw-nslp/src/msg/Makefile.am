# -----------------------------------*- mode: Makefile; -*--
# Makefile.am - Makefile.am for NATFW NSLP message lib
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
include ../../../Makefile.inc

LDADD = $(PROTLIB_LIB)

NATFWMSG_LLIB   = natfw_msg

lib_LIBRARIES 	= libnatfw_msg.a

API_INC		= $(top_srcdir)/include
INC_DIR 	= ../../include/msg

libnatfw_msg_a_CPPFLAGS = -I. -I$(API_INC) -I$(NATFW_INC) -I$(NTLP_INC) -I$(PROTLIB_INC) -I$(FQUEUE_INC)


libnatfw_msg_a_SOURCES = data_terminal_info.cpp ie_store.cpp		\
				 msg_sequence_number.cpp natfw_msg.cpp	\
				 nonce.cpp extended_flow_info.cpp	\
				 information_code.cpp natfw_create.cpp	\
				 natfw_notify.cpp ntlp_msg.cpp		\
				 external_address.cpp natfw_ext.cpp	\
				 natfw_object.cpp session_lifetime.cpp	\
				 icmp_types.cpp natfw_ie.cpp		\
				 natfw_response.cpp



include_HEADERS = 	$(INC_DIR)/data_terminal_info.h $(INC_DIR)/icmp_types.h			\
	        	$(INC_DIR)/msg_sequence_number.h $(INC_DIR)/natfw_ie.h $(INC_DIR)/natfw_object.h		\
	        	$(INC_DIR)/ntlp_msg.h $(INC_DIR)/extended_flow_info.h $(INC_DIR)/ie_store.h		\
	       	 	$(INC_DIR)/natfw_create.h $(INC_DIR)/natfw_msg.h $(INC_DIR)/natfw_response.h		\
	        	$(INC_DIR)/session_lifetime.h $(INC_DIR)/external_address.h			\
	        	$(INC_DIR)/information_code.h $(INC_DIR)/natfw_ext.h $(INC_DIR)/natfw_notify.h $(INC_DIR)/nonce.h

AM_CXXFLAGS = -Wall -pedantic

if USE_UNORDERED_MAP
AM_CXXFLAGS += -std=c++0x -DUSE_UNORDERED_MAP
else !USE_UNORDERED_MAP
if NSIS_NO_WARN_HASHMAP
AM_CXXFLAGS += -Wno-deprecated
endif
endif


# end of Makefile.am