#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT(syncml-client, 0.1, [mk<at>kolmodin net])
AC_LANG(C++)
export _GNU_SOURCE=1
AC_CONFIG_SRCDIR([include/unixdefs.h])
AC_CONFIG_HEADER([config.h])

AM_INIT_AUTOMAKE
AC_PROG_LIBTOOL

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC

# Checks for libraries.

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([locale.h stdint.h stdlib.h string.h sys/time.h values.h wchar.h wctype.h bits/c++io.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_CONST
AC_C_INLINE
AC_TYPE_SIZE_T
AC_HEADER_TIME
AC_STRUCT_TM

# Checks for library functions.
AC_FUNC_ERROR_AT_LINE
AC_FUNC_MALLOC
AC_FUNC_MEMCMP
AC_FUNC_REALLOC
AC_CHECK_FUNCS([gettimeofday localtime_r memmove memset setlocale strchr strstr])

# Checks for installed packages
# FIXME: split to separate tests - better error messages and configure options
PKG_CHECK_MODULES([SYNCML],[glib-2.0 gtk+-2.0 pango atk gconf-2.0])

AC_CHECK_PROG(CURL_PROG,curl-config,yes,no)
if test "$CURL_PROG" = "no" 
then
    echo "Cannot locate curl-config (package curl not installed?)"
    exit 1
fi
SYNCML_CFLAGS="$SYNCML_CFLAGS -I`curl-config --prefix`/include/curl"
SYNCML_LIBS="$SYNCML_LIBS `curl-config --libs`"
AC_SUBST(SYNCML_CFLAGS)
AC_SUBST(SYNCML_LIBS)

AC_CONFIG_FILES([c++/common/Makefile
                 c++/http/common/Makefile
                 c++/http/linux/Makefile
                 c++/ptypes/Makefile
                 c++/spdm/common/Makefile
                 c++/spdm/gconf/Makefile
                 c++/spds/common/Makefile
                 c++/examples/common/Makefile
                 include/Makefile
                 ./Makefile
                 ./syncml-client.pc
                 ./syncml-client.spec])
AC_OUTPUT
