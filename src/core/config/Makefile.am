# Makefile.am for persistence library

noinst_LIBRARIES = libeponMgr_persistence.a

libeponMgr_persistence_a_SOURCES = \
    eponMgr_persistence.c \
    eponMgr_persistence.h

libeponMgr_persistence_a_CFLAGS = \
    -I$(top_srcdir)/include \
    -I$(top_srcdir)/src/logger \
    -I$(top_srcdir)/src/rbus \
    $(AM_CFLAGS)
