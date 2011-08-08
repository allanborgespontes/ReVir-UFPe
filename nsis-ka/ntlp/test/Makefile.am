check_PROGRAMS = test_runner

include ../../Makefile.inc

API_INC		= $(top_srcdir)/include
PDU_INC		= ../src/pdu


test_runner_SOURCES = errorobject.cpp responder_cookie.cpp test_ntlp_pdu.cpp test_mri_est.cpp \
 mri_pc.cpp sessionid.cpp test_nattraversal.cpp test_suite.cpp test_runner.cpp

test_runner_CPPFLAGS = -I../src -I$(API_INC) -I$(PDU_INC) -I$(PROTLIB_INC) -I$(FQUEUE_INC) $(CPPUNIT_CFLAGS)
test_runner_LDADD = $(LD_PROTLIB_LIB) $(LD_FQUEUE_LIB) -L../src -lgist -lprot -lfastqueue $(CPPUNIT_LIBS) -lrt -lssl
#  -lpthread -lipq -lssl -lcrypto
TESTS = $(check_PROGRAMS)

AM_CXXFLAGS = -Wall -ggdb -pedantic -Wno-long-long

if USE_UNORDERED_MAP
AM_CXXFLAGS += -std=c++0x -DUSE_UNORDERED_MAP
else !USE_UNORDERED_MAP
if GIST_NO_WARN_HASHMAP
AM_CXXFLAGS += -Wno-deprecated
endif
endif

if USE_WITH_FLOWINFO
AM_CXXFLAGS += -DUSE_FLOWINFO
test_runner_SOURCES += test_flowinfo.cpp 
endif

#  TEST_CXX_FLAGS=`cppunit-config --cflags`
