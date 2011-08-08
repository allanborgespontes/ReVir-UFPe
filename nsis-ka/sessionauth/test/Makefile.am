check_PROGRAMS = test_runner

include $(top_srcdir)/../Makefile.inc

LDADD = $(LD_PROTLIB_LIB)

test_runner_SOURCES = attr_test.h generic_attr_test.cpp			\
			generic_attr_test.h				\
			session_auth_object_test.cpp test_hmac.cpp	\
			test_runner.cpp

test_runner_CPPFLAGS = -I$(top_srcdir)/include -I$(QOSNSLP_INC) -I$(QSPEC_INC) -I$(NTLP_INC) -I$(NTLP_SRC)/pdu -I$(PROTLIB_INC) -I$(FQUEUE_INC) $(CPPUNIT_CFLAGS)

test_runner_LDADD = -L../src -lnslpauth $(LD_QOSNSLP_LIB) $(LD_QSPEC_LIB) $(LD_NTLP_LIB) $(LD_PROTLIB_LIB) $(LD_FQUEUE_LIB) $(CPPUNIT_LIBS) -lrt -lssl

#  -lpthread -lipq -lssl -lcrypto
TESTS = $(check_PROGRAMS)

AM_CXXFLAGS = -Wall -ggdb -pedantic 

if USE_UNORDERED_MAP
AM_CXXFLAGS += -std=c++0x -DUSE_UNORDERED_MAP
else !USE_UNORDERED_MAP
if NSIS_NO_WARN_HASHMAP
AM_CXXFLAGS += -Wno-deprecated
endif
endif
