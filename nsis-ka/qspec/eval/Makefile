#
# Makefile - Makefile to build and run the benchmarks
#
# $Id: Makefile 3083 2008-07-02 11:04:52Z bless $
# $HeadURL: https://svn.tm.kit.edu/nsis/qspec/trunk/eval/Makefile $
#
# make		build the benchmarks
# make eval	run the benchmarks
# make clean	delete all generated output
#
# Note: Run the benchmarks from the directory below, that way the libraries
#       these benchmarks depend on are re-built if necessary.
#
QNI	= qni
QNE	= qne
QNR 	= qnr

DISTDIR?=$(CURDIR)/../..
include $(DISTDIR)/Makefile.inc

# The static libraries this test suite depends on.
#
LIB_QSPEC = ../src/libqspec.a
LIB_PROT = $(PROTLIB_LIB)

ALL_LIBS = $(LIB_QSPEC) $(LIB_PROT)


CXX = g++
CXXFLAGS = -Wall -pedantic -Wno-long-long -I../include -I$(PROTLIB_INC)
LDFLAGS = $(ALL_LIBS)

#
# nothing to change down here
#

ALL_TARGETS = $(QNI) $(QNE) $(QNR)

SRC = $(wildcard *.cpp)
OBJ = $(patsubst %.cpp, %.o, $(SRC))
DEPS = $(patsubst %.cpp, %.d, $(SRC))

.PHONY: all clean

all: $(ALL_TARGETS)

$(QNI): qni.o utils.o Benchmark.o $(ALL_LIBS)
	$(CXX) -o $@ $^ $(LDFLAGS) 

$(QNE): qne.o utils.o Benchmark.o $(ALL_LIBS)
	$(CXX) -o $@ $^ $(LDFLAGS) 

$(QNR): qnr.o utils.o Benchmark.o $(ALL_LIBS)
	$(CXX) -o $@ $^ $(LDFLAGS) 

clean: 
	-rm -f $(ALL_TARGETS) $(OBJ) $(DEPS)

# generate dependencies
%.d: %.cpp
	$(CXX) -M $(CXXFLAGS) $< | sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' > $@

# include Makefile dependency snippets for each source file
-include $(SRC:.cpp=.d)
