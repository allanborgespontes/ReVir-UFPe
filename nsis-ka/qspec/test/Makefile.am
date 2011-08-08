check_PROGRAMS = test_runner

PROTLIBDIR=../../protlib

LDADD = $(PROTLIBDIR)/libprotlib.a

test_runner_SOURCES = admission_priority.cpp generic_param_test.cpp	\
	path_plr.cpp qspec_param_raw.cpp t_mod.cpp basic.cpp		\
	ie_store.cpp phb_class.cpp qspec_pdu.cpp y1541_qos_class.cpp	\
	dste_class_type.cpp path_jitter.cpp preemption_priority.cpp	\
	rph_priority.cpp excess_treatment.cpp path_latency.cpp		\
	qspec_object.cpp slack_term.cpp extension.cpp path_per.cpp	\
	qspec_param.cpp test_runner.cpp

test_runner_CPPFLAGS = -I../src -I$(PROTLIBDIR)/include  -I$(top_srcdir)/include -I$(PROTLIBDIR)/fastqueue $(CPPUNIT_CFLAGS)

test_runner_LDADD = -L../src -L$(PROTLIBDIR)/fastqueue -L$(PROTLIBDIR)/src -lqspec -lprot -lfastqueue \
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
