## -----------------------------------*- mode: Makefile; -*--
# Top Level Makefile for inclusion for the distribution tree of NSIS-ka
# -------------------------------------------------------
# $HeadURL: https://svn.tm.kit.edu/nsis/dist/nsis-ka/trunk/Makefile.inc $
# $Id: Makefile.inc 6175 2011-05-20 07:39:28Z bless $
# -------------------------------------------------------
# Several definitions (directories and linker options) for nsis-ka components
#

DISTDIR=$(top_srcdir)/..


PROTLIB_DIR=$(DISTDIR)/protlib
PROTLIB_SRC=$(PROTLIB_DIR)/src
PROTLIB_INC=$(PROTLIB_DIR)/include
PROTLIB_LIBDIR=$(PROTLIB_SRC)
PROTLIB_LIB=$(PROTLIB_LIBDIR)/libprot.a
LD_PROTLIB_LIB=-L $(PROTLIB_LIBDIR) -lprot


FQUEUE_DIR=$(PROTLIB_DIR)/fastqueue
FQUEUE_SRC=$(FQUEUE_DIR)
FQUEUE_INC=$(FQUEUE_DIR)
FQUEUE_LIBDIR=$(FQUEUE_DIR)
FQUEUE_LIB=$(FQUEUE_LIBDIR)/libfastqueue.a
LD_FQUEUE_LIB=-L $(FQUEUE_LIBDIR) -lfastqueue


NTLP_DIR=$(DISTDIR)/ntlp
NTLP_SRC=$(NTLP_DIR)/src
NTLP_INC=$(NTLP_DIR)/include
NTLP_LIBDIR=$(NTLP_SRC)
NTLP_LIB=$(NTLP_LIBDIR)/libgist.a
LD_NTLP_LIB=-L $(NTLP_LIBDIR) -lgist


QOSNSLP_DIR=$(DISTDIR)/qos-nslp
QOSNSLP_SRC=$(QOSNSLP_DIR)/src
QOSNSLP_INC=$(QOSNSLP_DIR)/include
QOSNSLP_LIBDIR=$(QOSNSLP_SRC)
QOSNSLP_LIB=$(QOSNSLP_LIBDIR)/libqosnslp.a
LD_QOSNSLP_LIB=-L $(QOSNSLP_LIBDIR) -lqosnslp


NATFW_DIR=$(DISTDIR)/natfw-nslp
NATFW_SRC=$(NATFW_DIR)/src
NATFW_INC=$(NATFW_DIR)/include
NATFW_LIBDIR=$(NATFW_SRC)
NATFW_MSG_LIBDIR=$(NATFW_SRC)/msg
NATFW_LIB=$(NATFW_LIBDIR)/libnatfw.a
NATFW_MSG_LIB=$(NATFW_LIBDIR)/libnatfw_msg.a
LD_NATFW_LIB=-L $(NATFW_LIBDIR) -lnatfw -L $(NATFW_MSG_LIBDIR) -lnatfw_msg


QSPEC_DIR=$(DISTDIR)/qspec
QSPEC_SRC=$(QSPEC_DIR)/src
QSPEC_INC=$(QSPEC_DIR)/include
QSPEC_LIBDIR=$(QSPEC_SRC)
QSPEC_LIB=$(QSPEC_LIBDIR)/libqspec.a
LD_QSPEC_LIB=-L $(QSPEC_LIBDIR) -lqspec


AUTH_DIR=$(DISTDIR)/sessionauth
AUTH_SRC=$(AUTH_DIR)/src
AUTH_INC=$(AUTH_DIR)/include
AUTH_LIBDIR=$(AUTH_DIR)/src
AUTH_LIB=$(AUTH_LIBDIR)/libnslpauth.a
LD_AUTH_LIB=-L $(AUTH_LIBDIR) -lnslpauth

# end of Makefile.inc
