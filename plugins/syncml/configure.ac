dnl Process this file with autoconf to produce a configure script.

AC_PREREQ(2.58)
AC_INIT(opensync-syncml-plugin, 0.01)
AC_CONFIG_SRCDIR(src/syncml_plugin.c)
AM_INIT_AUTOMAKE([foreign])

dnl AC_ISC_POSIX
AC_PROG_CC
AM_PROG_CC_STDC
AC_HEADER_STDC
AC_DISABLE_STATIC
AC_PROG_LIBTOOL

export PKG_CONFIG_PATH=${PKG_CONFIG_PATH}:/usr/lib/pkgconfig:$prefix/lib/pkgconfig:/usr/local/lib/pkgconfig
PKG_CHECK_MODULES(SYNCML, [libcsyncml])
PKG_CHECK_MODULES(OPENSYNC, [opensync-1.0])
dnl Dependencies of the syncml library: it exposes glib and libsoup
PKG_CHECK_MODULES(SYNCML_DEPS, [glib-2.0, libsoup-2.2])

dnl libwbxml2 checking code, stolen from libcsyncml
AC_CHECK_LIB(wbxml2, wbxml_conv_xml2wbxml, [],
[AC_MSG_ERROR([Could not find libwbxml2.])])

dnl Checks for header files.
AC_HEADER_STDC

WBXML_CFLAGS=
AC_CHECK_HEADER(wbxml.h, [found_wbxml_header=yes],
  [found_wbxml_header=no])

if test $found_wbxml_header = no; then
  AC_MSG_NOTICE([trying to find wbxml.h in /usr/include/wbxml2])
  AC_CHECK_HEADER(/usr/include/wbxml2/wbxml.h, [], 
    [AC_MSG_ERROR([Could not find header wbxml.h.])])
  WBXML_CFLAGS="-I/usr/include/wbxml2"
fi

dnl Add wbxml to the libcsyncml deps
SYNCML_DEPS_CFLAGS="$SYNCML_DEPS_CFLAGS $WBXML_CFLAGS"


OPENSYNC_CONFIGDIR=$(pkg-config --variable=configdir opensync-1.0)
OPENSYNC_PLUGINDIR=$(pkg-config --variable=plugindir opensync-1.0)
OPENSYNC_FORMATSDIR=$(pkg-config --variable=formatsdir opensync-1.0)
OPENSYNC_HEADERDIR=$(pkg-config --variable=headerdir opensync-1.0)

AC_SUBST(OPENSYNC_CONFIGDIR)
AC_SUBST(OPENSYNC_PLUGINDIR)
AC_SUBST(OPENSYNC_FORMATSDIR)
AC_SUBST(OPENSYNC_HEADERDIR)

AC_OUTPUT([
Makefile
])

echo "================================"
echo "Plugin will be installed in ${OPENSYNC_PLUGINDIR}"
echo "Default config will be installed in ${OPENSYNC_CONFIGDIR}"
echo "Format will be installed in ${OPENSYNC_FORMATSDIR}"
echo "================================"
