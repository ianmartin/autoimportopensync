dnl Process this file with autoconf to produce a configure script.

AC_PREREQ(2.58)
AC_INIT(opensync-syncml-plugin, 0.01)
AC_CONFIG_SRCDIR(src/syncml_plugin.c)
AM_INIT_AUTOMAKE([foreign])

echo a
AC_ISC_POSIX
echo b
AC_PROG_CC
echo c
AM_PROG_CC_STDC
echo d
AC_HEADER_STDC
echo e
AC_DISABLE_STATIC
echo g
AC_PROG_LIBTOOL
echo h

export PKG_CONFIG_PATH=${PKG_CONFIG_PATH}:/usr/lib/pkgconfig:$prefix/lib/pkgconfig:/usr/local/lib/pkgconfig
PKG_CHECK_MODULES(SYNCML, [libcsyncml])
PKG_CHECK_MODULES(OPENSYNC, [opensync-1.0])

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
