check_PROGRAMS = test_runner

include ../../Makefile.inc

test_runner_SOURCES = test_rsn.cpp test_suite.cpp test_runner.cpp

test_runner_CPPFLAGS = -I../src -I$(QOSNSLP_INC) -I$(QSPEC_INC) -I$(NTLP_INC) -I$(PROTLIB_INC) -I$(FQUEUE_INC) $(CPPUNIT_CFLAGS)
test_runner_LDADD = -L../src $(LD_QOSNSLP_LIB) $(LD_NTLP_LIB) $(LD_PROTLIB_LIB) $(LD_FQUEUE_LIB)  \
		$(CPPUNIT_LIBS) -lrt -lssl
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

#  TEST_CXX_FLAGS=`cppunit-config --cflags`
