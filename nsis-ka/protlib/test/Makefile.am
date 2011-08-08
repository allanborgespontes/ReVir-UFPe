check_PROGRAMS = test_runner
test_runner_SOURCES = basic.cpp fqueue.cpp netmsg.cpp queue_manager.cpp \
		test_address.cpp test_runner.cpp test_template.cpp \
		test_tp_over_xyz.cpp test_types.cpp timer_module.cpp
test_runner_CPPFLAGS = -I$(top_srcdir)/include -I$(top_srcdir)/fastqueue $(CPPUNIT_CFLAGS)
test_runner_LDADD = $(top_builddir)/fastqueue/libfastqueue.a $(top_builddir)/src/libprot.a \
		$(CPPUNIT_LIBS) -ldl -lpthread -lipq -lssl -lcrypto
#  -lpthread -lipq -lssl -lcrypto
TESTS = $(check_PROGRAMS)


EXTRA_DIST = README

if USE_UNORDERED_MAP
AM_CXXFLAGS = -std=c++0x -DUSE_UNORDERED_MAP
else !USE_UNORDERED_MAP
if PROTLIB_NO_WARN_HASHMAP
AM_CXXFLAGS = -Wno-deprecated
endif
endif
