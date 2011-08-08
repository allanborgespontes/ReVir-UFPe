#
# Makefile - Simplified Makefile building the example code.
#
# $Id: Makefile 3083 2008-07-02 11:04:52Z bless $
# $HeadURL: https://svn.tm.kit.edu/nsis/qspec/trunk/examples/Makefile $
#
# make		run the build process
# make clean	delete all generated output
#
DISTDIR?=$(CURDIR)/../..
include $(DISTDIR)/Makefile.inc

PROT_LIB	= $(PROTLIB_LIB)
PROT_INCLUDES	= $(PROTLIB_INC)

QSPEC_LIB	= ../src/libqspec.a
QSPEC_INCLUDES	= ../include


CXX		= g++
WARN		= -Wall -pedantic -Wno-long-long 
CXXFLAGS	= -I$(PROT_INCLUDES) -I$(QSPEC_INCLUDES)
LDFLAGS		= $(PROT_LIB) $(QSPEC_LIB)


.PHONY: clean all

all: read_qspec write_qspec


read_qspec: read_qspec.cpp
	$(CXX) -o read_qspec $(CXXFLAGS) read_qspec.cpp $(LDFLAGS)

write_qspec: write_qspec.cpp
	$(CXX) -o write_qspec $(CXXFLAGS) write_qspec.cpp $(LDFLAGS)


clean:
	-rm -f read_qspec write_qspec
