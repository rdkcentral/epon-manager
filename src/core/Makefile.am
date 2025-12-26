# Makefile.am for core directory

SUBDIRS = config data_structures hal_wrapper controller

# Main EPON Manager executable
bin_PROGRAMS = epon_manager

epon_manager_SOURCES = \
    epon_manager_main.c

epon_manager_CFLAGS = \
    -I$(top_srcdir)/include \
    -I$(top_srcdir)/src/logger \
    $(AM_CFLAGS)

epon_manager_LDADD = \
    $(top_builddir)/src/core/controller/libeponMgr_controller.a \
    $(top_builddir)/src/core/hal_wrapper/libeponMgr_hal_wrapper.a \
    $(top_builddir)/src/core/data_structures/libeponMgr_datastructures.a \
    $(top_builddir)/src/core/config/libeponMgr_config.a \
    $(top_builddir)/src/logger/libeponMgr_logger.a \
    -lpthread

if ENABLE_RBUS
epon_manager_LDADD += $(top_builddir)/src/rbus/libeponMgr_rbus.a
endif

epon_manager_LDFLAGS = \
    -Wl,-rpath,$(libdir)
