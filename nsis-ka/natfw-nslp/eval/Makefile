#
# Makefile - Makefile for the NATFW NSLP
#
# $Id: Makefile 3095 2008-07-02 14:04:16Z bless $
# $HeadURL: https://svn.tm.kit.edu/nsis/natfw-nslp/trunk/eval/Makefile $
#
# make		build the benchmarks
# make clean	delete all generated output
#
# The following Makefile variables may be used:
#     LOGGING        Enable logging output (default: active)
#     DEBUGGING      Enable debugging (default: active)
#     OPTIMIZATION   Optimize using the given flags (default: '')
#     BENCHMARK      Create benchmarks (default: inactive)
#
DEBUGGING	= 1			# the undefined value disables debugging
LOGGING		= 1			# the undefined value disables logging
OPTIMIZATION	=			# ie. -O2
BENCHMARK	=			# assign a value to enable benchmarking

DISTDIR?=$(CURDIR)/../..
include $(DISTDIR)/Makefile.inc

# The external static libraries we depend on
#
LIB_PROT = $(PROTLIB_LIB)
LIB_FASTQUEUE = $(FQUEUE_LIB)
LIB_GIST = $(NTLP_LIB)
LIB_NATFW_MSG = ../src/msg/libnatfw_msg.a
LIB_NATFW = ../src/libnatfw.a

ALL_LIBS = $(LIB_NATFW_MSG) $(LIB_NATFW) \
		$(LIB_GIST) $(LIB_PROT) $(LIB_FASTQUEUE) 

INC =  -I../include -I$(PROTLIB_INC) -I$(FQUEUE_INC) -I$(NTLP_INC)


# The targets to build
#
SERIALIZER = serializer
DESERIALIZER = deserializer
NI = ni
NF = nf
NF2 = nf2
NF3 = nf3
NF4 = nf4
NR = nr

ALL_TARGETS = $(SERIALIZER) $(DESERIALIZER) $(NI) $(NF) $(NF2) $(NF3) $(NF4) $(NR)


# Compiler and linker settings common to all targets
#
CXX = g++
AR = ar
CXXFLAGS = -Wall -g -pedantic -Wno-long-long $(INC)
LDFLAGS = -pg $(ALL_LIBS) -lcrypto -lpthread -lipq -lssl -lrt


ifdef BENCHMARK
CXXFLAGS += -DBENCHMARK
endif

ifndef LOGGING
CXXFLAGS += -D_NO_LOGGING
endif

CXXFLAGS += $(OPTIMIZATION)


# Targets
#
.PHONY: all clean

all: $(ALL_TARGETS)


$(SERIALIZER): serializer.o benchmark.o utils.o $(ALL_LIBS)
	$(CXX) -o $@ $^ $(LDFLAGS)

$(DESERIALIZER): deserializer.o benchmark.o utils.o $(ALL_LIBS)
	$(CXX) -o $@ $^ $(LDFLAGS)

$(NI): ni.o benchmark.o utils.o $(ALL_LIBS)
	$(CXX) -o $@ $^ $(LDFLAGS)

$(NF): nf.o benchmark.o utils.o $(ALL_LIBS)
	$(CXX) -o $@ $^ $(LDFLAGS)

$(NF2): nf2.o benchmark.o utils.o $(ALL_LIBS)
	$(CXX) -o $@ $^ $(LDFLAGS)

$(NF3): nf3.o benchmark.o utils.o $(ALL_LIBS)
	$(CXX) -o $@ $^ $(LDFLAGS)

$(NF4): nf4.o benchmark.o utils.o $(ALL_LIBS)
	$(CXX) -o $@ $^ $(LDFLAGS)


$(NR): nr.o benchmark.o utils.o $(ALL_LIBS)
	$(CXX) -o $@ $^ $(LDFLAGS)

clean: 
	-rm -f $(ALL_TARGETS) $(wildcard *.o) depend

depend:
	$(CXX) -MM $(CXXFLAGS) $(wildcard *.cpp) > depend

# Include dependencies, do not delete!
-include depend
