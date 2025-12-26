# Makefile.am for logger library

lib_LTLIBRARIES = libeponMgr_logger.la

libeponMgr_logger_la_SOURCES = \
    eponMgr_logger.c \
    eponMgr_logger.h

libeponMgr_logger_la_CFLAGS = \
    -I$(top_srcdir)/include \
    $(AM_CFLAGS)

libeponMgr_logger_la_LDFLAGS = \
    -version-info 1:0:0

# Also create static archive for internal use
noinst_LIBRARIES = libeponMgr_logger.a

libeponMgr_logger_a_SOURCES = $(libeponMgr_logger_la_SOURCES)
libeponMgr_logger_a_CFLAGS = $(libeponMgr_logger_la_CFLAGS)
