lib_LIBRARIES = libfastqueue.a
libfastqueue_a_SOURCES = fastqueue.h fastqueue.c eclock_gettime.c
libfastqueue_a_CFLAGS = -I$(top_srcdir)/fastqueue

if PROTLIB_DEBUG
AM_CPPFLAGS = -D_DEBUG
endif

check_PROGRAMS = testqueue
testqueue_SOURCES = testqueue.c fastqueue.c fastqueue.h eclock_gettime.c
testqueue_LDADD = -lrt
testqueue_CPPFLAGS = -DQUEUELEN
testqueue_CFLAGS = -g -Wall
TESTS = $(check_PROGRAMS)
