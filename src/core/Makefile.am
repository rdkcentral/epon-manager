# Makefile.am for core directory

SUBDIRS = config data_structures stats_poller controller

# Main EPON Manager executable
bin_PROGRAMS = epon_manager

epon_manager_SOURCES = \
    epon_manager_main.c

epon_manager_CFLAGS = \
    -I$(top_srcdir)/include \
    -I$(top_srcdir)/src/logger \
    -I$(top_srcdir)/src/core/config \
    -I$(top_srcdir)/src/core/data_structures \
    -I$(top_srcdir)/src/core/stats_poller \
    -I$(top_srcdir)/src/core/controller \
    -I$(top_srcdir)/src/rbus \
    -I$(top_srcdir)/src/rbus/tr181 \
    -I$(top_srcdir)/src/telemetry \
    -I${PKG_CONFIG_SYSROOT_DIR}$(includedir)/epon \
    -Wunused-variable -Wunused-function -Wunused-parameter \
    $(AM_CFLAGS)

epon_manager_LDADD = \
    $(top_builddir)/src/core/controller/libeponMgr_controller.a \
    $(top_builddir)/src/core/stats_poller/libeponMgr_stats_poller.a \
    $(top_builddir)/src/core/data_structures/libeponMgr_datastructures.a \
    $(top_builddir)/src/core/config/libeponMgr_persistence.a \
    -lpthread

if ENABLE_RBUS
epon_manager_LDADD += $(top_builddir)/src/rbus/libeponMgr_rbus.a -lrbus
endif

if ENABLE_TESTS
# Link with HAL mock library for testing
epon_manager_LDADD += $(top_builddir)/tests/hal_mock/libepon_hal_mock.la
else
# Link with actual HAL library for production
epon_manager_LDADD += -lcomcast_epon_hal
endif

epon_manager_LDADD += -lrdkloggers -lpthread

epon_manager_LDFLAGS = \
    -Wl,-rpath,$(libdir)
