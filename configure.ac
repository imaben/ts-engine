#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT(ts-engine, 0.0.1, i@maben.me)
AM_INIT_AUTOMAKE
AC_CONFIG_SRCDIR([docid.h])
AC_CONFIG_HEADERS([cconfig.h])

# Checks for programs.
AC_PROG_CC

# Checks for libraries.
# FIXME: Replace `main' with a function in `-lpthread':
AC_CHECK_LIB([pthread], [ts_lock_init])
AC_CHECK_LIB([libprotobuf-c], [field__get_packed_size])

# Checks for header files.
AC_CHECK_HEADERS([arpa/inet.h fcntl.h netdb.h netinet/in.h stddef.h stdint.h stdlib.h string.h strings.h sys/socket.h sys/time.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_INLINE
AC_TYPE_INT32_T
AC_TYPE_MODE_T
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_UINT8_T
AC_CHECK_TYPES([ptrdiff_t])

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_CHECK_FUNCS([bzero gettimeofday memmove memset select socket strchr strdup strerror])

AC_ARG_ENABLE([debug],
    [AS_HELP_STRING([--enable-debug],[debug program(default is no)])],
    [CFLAGS="${CFLAGS} -D _DEBUG"],
    [CFLAGS="-O2"])
AC_CONFIG_FILES([Makefile])
AC_OUTPUT
