check_PROGRAMS = test_runner

include ../../Makefile.inc

test_runner_SOURCES = test_runner.cpp basic.cpp generic_object_test.h natfw_ext.cpp	\
			nf_session.cpp session_id.cpp benchmark.cpp	\
			icmp_types.cpp natfw_msg.cpp ni_session.cpp	\
			session_lifetime.cpp data_terminal_info.cpp	\
			information_code.cpp natfw_response.cpp		\
			nr_ext_session.cpp session_manager.cpp		\
			dispatcher.cpp nat_manager.cpp nr_session.cpp	\
			external_address.cpp nf_edge_ext_session.cpp	\
			ntlp_integration.cpp utils.cpp			\
			generic_object_test.cpp natfw_create.cpp	\
			nf_non_edge_ext_session.cpp utils.h


if USE_WITH_SCTP
LD_SCTP_LIB= -lsctp
endif


test_runner_CPPFLAGS = -I../src -I../src/msg -I$(NATFW_INC) $(CPPUNIT_CFLAGS)  -I$(NTLP_INC) -I$(PROTLIB_INC) -I$(FQUEUE_INC) 
test_runner_LDADD = -L../src $(LD_NATFW_LIB) $(LD_NTLP_LIB) $(LD_PROTLIB_LIB) $(LD_FQUEUE_LIB) $(CPPUNIT_LIBS) $(LD_SCTP_LIB) -lipq -lrt -lssl 

#  -lpthread -lipq -lssl -lcrypto
TESTS = $(check_PROGRAMS)

AM_CXXFLAGS = -Wall -ggdb -pedantic -Wno-long-long

if USE_UNORDERED_MAP
AM_CXXFLAGS += -std=c++0x -DUSE_UNORDERED_MAP
else !USE_UNORDERED_MAP
if NSIS_NO_WARN_HASHMAP
AM_CXXFLAGS += -Wno-deprecated
endif
endif

#  TEST_CXX_FLAGS=`cppunit-config --cflags`
