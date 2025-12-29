#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([epon-manager], [1.0.0], [support@rdkcentral.com])
AC_CONFIG_SRCDIR([src/core/epon_manager_main.c])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_AUX_DIR([cfg])

AM_INIT_AUTOMAKE([foreign subdir-objects -Wall -Werror])
AM_PROG_AR

LT_INIT

# Checks for programs.
AC_PROG_CC
AC_PROG_CXX
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET

# Checks for libraries.
AC_CHECK_LIB([pthread], [pthread_create], [], [
    AC_MSG_ERROR([pthread library is required])
])

# Checks for header files.
AC_CHECK_HEADERS([arpa/inet.h fcntl.h netdb.h netinet/in.h stddef.h stdint.h stdlib.h string.h sys/socket.h sys/time.h syslog.h unistd.h pthread.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_INT32_T
AC_TYPE_INT64_T
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_UINT8_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_CHECK_FUNCS([clock_gettime gettimeofday memset socket strchr strdup strerror strstr])

# Option to enable/disable RBUS support
AC_ARG_ENABLE([rbus],
    AS_HELP_STRING([--enable-rbus], [Enable RBUS support (default=yes)]),
    [enable_rbus=$enableval],
    [enable_rbus=yes])

if test "x$enable_rbus" = "xyes"; then
    # RBUS_CFLAGS can be set externally via environment or will use default
    AC_ARG_VAR([RBUS_CFLAGS], [C compiler flags for RBUS])
    if test "x$RBUS_CFLAGS" = "x"; then
        RBUS_CFLAGS=""
    fi
fi

AM_CONDITIONAL([ENABLE_RBUS], [test "x$enable_rbus" = "xyes"])

# Option to enable/disable telemetry
AC_ARG_ENABLE([telemetry],
    AS_HELP_STRING([--enable-telemetry], [Enable telemetry support (default=yes)]),
    [enable_telemetry=$enableval],
    [enable_telemetry=yes])
AM_CONDITIONAL([ENABLE_TELEMETRY], [test "x$enable_telemetry" = "xyes"])

# Option to enable/disable HAL mock library
AC_ARG_ENABLE([tests],
    AS_HELP_STRING([--enable-tests], [Build HAL mock library for integration testing (default=yes)]),
    [enable_tests=$enableval],
    [enable_tests=yes])
AM_CONDITIONAL([ENABLE_TESTS], [test "x$enable_tests" = "xyes"])

# Option for debug build
AC_ARG_ENABLE([debug],
    AS_HELP_STRING([--enable-debug], [Enable debug build (default=no)]),
    [enable_debug=$enableval],
    [enable_debug=no])

if test "x$enable_debug" = "xyes"; then
    CFLAGS="-g -O0 -DDEBUG"
else
    CFLAGS="-O2 -DNDEBUG"
fi

# Add common compiler flags
CFLAGS="$CFLAGS -Wall -Wextra -fPIC"

# Configuration summary
AC_MSG_RESULT([
    $PACKAGE_NAME $PACKAGE_VERSION Configuration:
    
    Prefix:             $prefix
    Compiler:           $CC
    CFLAGS:             $CFLAGS
    LDFLAGS:            $LDFLAGS
    
    Features:
    RBUS support:       $enable_rbus
    Telemetry:          $enable_telemetry
    HAL mock library:   $enable_tests
    Debug build:        $enable_debug
])

AC_CONFIG_FILES([
    Makefile
    include/Makefile
    src/Makefile
    src/logger/Makefile
    src/core/Makefile
    src/core/config/Makefile
    src/core/data_structures/Makefile
    src/core/hal_wrapper/Makefile
    src/core/controller/Makefile
    src/rbus/Makefile
    src/telemetry/Makefile
    tests/Makefile
    tests/hal_mock/Makefile
])

AC_OUTPUT
