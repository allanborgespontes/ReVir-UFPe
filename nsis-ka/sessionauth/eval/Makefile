#
# Makefile - Makefile to build and run the test suite
#
# $Id$
# $HeadURL$
#
# make		build the test suite
# make test	run the test suite
# make clean	delete all generated output
#
# Note: Run the test suite from the directory below, that way the libraries
#       the test case works with are re-built if necessary.
#
NAME = auth_benchmark

DISTDIR?=$(CURDIR)/../..
include $(DISTDIR)/Makefile.inc

# The static libraries this test suite depends on.
#
LIB_SESSIONAUTH = ../obj/libnslpauth.a
LD_SESSIONAUTH = -L../obj -lnslpauth
LIB_PROT = $(PROTLIB_LIB)
LD_PROTLIB = -L $(PROTLIB_SRC) -lprot
LD_NTLPLIB = -L $(NTLP_SRC) -lGIST
LD_QOSNSLPLIB= -L $(QOSNSLP_SRC) -lqosnslp            #../../qos-nslp/src  
LD_PTHREADS= -lpthread
LD_FQUEUE=-L $(FQUEUE_SRC) -lfastqueue
LD_QSPEC=-L$(QSPEC_SRC) -lqspec
LD_IPQ=-lipq
LD_RT=-lrt

ALL_LIBS = $(LIB_SESSIONAUTH) $(LIB_PROT)


CXX = g++
CXXFLAGS = -ggdb -Wall -pedantic -Wno-long-long -DBENCHMARK -I../include -I$(PROTLIB_INC) -I$(NTLP_INC) -I$(NTLP_INC)/../src/pdu -I$(FQUEUE_INC) -I$(QSPEC_INC) -I$(QOSNSLP_INC) \
		$(shell cppunit-config --cflags)
LDFLAGS = $(LD_SESSIONAUTH) $(LD_QSPEC) $(LD_QOSNSLPLIB) $(LD_NTLPLIB) -lssl $(LD_PROTLIB) $(LD_PTHREADS) $(LD_FQUEUE) $(LD_IPQ) $(LD_RT) $(shell cppunit-config --libs) 

#
# nothing to change down here
#

SRC = $(wildcard *.cpp)
OBJ = $(patsubst %.cpp, %.o, $(SRC))
DEPS = $(patsubst %.cpp, %.d, $(SRC))

.PHONY: test clean

$(NAME): $(OBJ) $(ALL_LIBS) $(DEP_LIBS)
	$(CXX) -o $(NAME) $(OBJ) $(LDFLAGS) 

eval: $(NAME)
	@echo Running eval suite ...
	@./$(NAME)

clean: 
	-rm -f $(NAME) $(OBJ) $(DEPS)

# generate dependencies
%.d: %.cpp
	$(CXX) -M $(CXXFLAGS) $< | sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' > $@

# include Makefile dependency snippets for each source file
-include $(SRC:.cpp=.d)
