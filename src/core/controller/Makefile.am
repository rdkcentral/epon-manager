# Makefile.am for controller library

noinst_LIBRARIES = libeponMgr_controller.a

libeponMgr_controller_a_SOURCES = \
    eponMgr_controller.c \
    eponMgr_controller.h

libeponMgr_controller_a_CFLAGS = \
    -I$(top_srcdir)/include \
    -I$(top_srcdir)/src/logger \
    -I$(top_srcdir)/src/core/config \
    -I$(top_srcdir)/src/core/data_structures \
    -I$(top_srcdir)/src/core/hal_wrapper \
    -I$(top_srcdir)/src/rbus \
    $(AM_CFLAGS)
