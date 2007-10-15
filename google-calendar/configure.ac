dnl Process this file with autoconf to produce a configure script.

AC_PREREQ(2.58)
AC_INIT([OpenSync Google Calendar Plugin], 0.33, [], [libopensync-plugin-google-calendar])
AM_INIT_AUTOMAKE(foreign)
AC_CONFIG_SRCDIR(src/gcalendar.c)
AM_CONFIG_HEADER(config.h)

export PKG_CONFIG_PATH=${PKG_CONFIG_PATH}:/usr/lib/pkgconfig:$prefix/lib/pkgconfig:/usr/local/lib/pkgconfig
pkg_modules="glib-2.0 opensync-1.0 libxml-2.0"
PKG_CHECK_MODULES(PACKAGE, [$pkg_modules])
AC_SUBST(PACKAGE_CFLAGS)
AC_SUBST(PACKAGE_LIBS)

AC_CACHE_CHECK([for python httplib2 ], act_httplib, [ prog="
import sys
try:
    import httplib2
except:
    sys.exit(1)
sys.exit(0)
"
	
if `python -c "$prog"` ; then
    act_httplib="yes"
else
    echo "you need httplib2. (http://bitworking.org/projects/httplib2/)"
    act_httplib="no"
fi
])
			
if test "$act_httplib" != "yes"; then
    exit 1
fi 

## Defaults ##
CRASH_TEST=no
STRESS_TEST=no
ERROR_TEST=no

AC_ISC_POSIX
AC_PROG_CC
AM_PROG_CC_STDC
AC_HEADER_STDC
AC_DISABLE_STATIC
AC_PROG_LIBTOOL

OPENSYNC_CONFIGDIR=$(pkg-config --variable=configdir opensync-1.0)
OPENSYNC_PLUGINDIR=$(pkg-config --variable=plugindir opensync-1.0)
OPENSYNC_FORMATSDIR=$(pkg-config --variable=formatsdir opensync-1.0)
OPENSYNC_CAPABILITIESDIR=$(pkg-config --variable=capabilitiesdir opensync-1.0)
OPENSYNC_DESCRIPTIONDIR=$(pkg-config --variable=descriptionsdir opensync-1.0)	
OPENSYNC_HEADERDIR=$(pkg-config --variable=headerdir opensync-1.0)

AC_SUBST(OPENSYNC_CONFIGDIR)
AC_SUBST(OPENSYNC_PLUGINDIR)
AC_SUBST(OPENSYNC_FORMATSDIR)
AC_SUBST(OPENSYNC_CAPABILITIESDIR)
AC_SUBST(OPENSYNC_DESCRIPTIONDIR)
AC_SUBST(OPENSYNC_HEADERDIR)

AC_OUTPUT([
Makefile
src/Makefile
])

echo "================================"
echo "Plugin will be installed in ${OPENSYNC_PLUGINDIR}"
echo "Default config will be installed in ${OPENSYNC_CONFIGDIR}"
echo "Format will be installed in ${OPENSYNC_FORMATSDIR}"
echo "================================"
