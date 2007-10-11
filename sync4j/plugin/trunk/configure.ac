dnl
dnl Copyright (C) 2006 Michael Kolmodin
dnl
dnl This program is free software; you can redistribute it and/or modify
dnl it under the terms of the GNU General Public License as published by
dnl the Free Software Foundation; either version 2 of the License, or
dnl (at your option) any later version.
dnl
dnl This program is distributed in the hope that it will be useful,
dnl but WITHOUT ANY WARRANTY; without even the implied warranty of
dnl MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
dnl GNU General Public License for more details.
dnl
dnl You should have received a copy of the GNU General Public License
dnl along with this program; if not, write to the Free Software
dnl Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
dnl

AC_PREREQ(2.58)
AC_INIT([OpenSync SyncML client Plugin], 0.32, [], syncml-client-plugin)
AM_INIT_AUTOMAKE(foreign)
AC_CONFIG_SRCDIR(src/syncml-client-plugin.cpp)
AC_CONFIG_HEADER(config.h)
AM_PATH_CPPUNIT(1.12.0)

dnl export PKG_CONFIG_PATH=${PKG_CONFIG_PATH}:${libdir}
dnl Setting up environment to build against opensync svn
export OPENSYNC_BUILD=/home/mlunter/unizeugs/bozen/arbeit/software/opensync-svn
export PKG_CONFIG_PATH=$OPENSYNC_BUILD/lib/pkgconfig:$PKG_CONFIG_PATH

dnl pkg_modules="opensync-1.0 glib-2.0 libxml-2.0 libxslt sync4j"
dnl PKG_CHECK_MODULES(PACKAGE, [$pkg_modules])
PKG_CHECK_MODULES(opensync, ["opensync-1.0"])
AC_SUBST(opensync_CFLAGS)
AC_SUBST(opensync_LIBS)

PKG_CHECK_MODULES(glib, ["glib-2.0"])
AC_SUBST(glib_CFLAGS)
AC_SUBST(glib_LIBS)

PKG_CHECK_MODULES(libxml, ["libxml-2.0"])
AC_SUBST(libxml_CFLAGS)
AC_SUBST(libxml_LIBS)

PKG_CHECK_MODULES(libxslt, ["libxslt"])
AC_SUBST(libxslt_CFLAGS)
AC_SUBST(libxslt_LIBS)

PKG_CHECK_MODULES(sync4j, ["sync4j"])
AC_SUBST(sync4j_CFLAGS)
AC_SUBST(sync4j_LIBS)

AC_ISC_POSIX
CXXFLAGS="-Wall -g -O0"
AC_PROG_CC
AC_PROG_CPP
AM_PROG_CC_STDC
AC_HEADER_STDC
AC_DISABLE_STATIC
AC_PROG_LIBTOOL


OPENSYNC_CONFIGDIR=$(pkg-config --variable=configdir opensync-1.0)
OPENSYNC_PLUGINDIR=$(pkg-config --variable=plugindir opensync-1.0)
OPENSYNC_FORMATSDIR=$(pkg-config --variable=formatsdir opensync-1.0)
OPENSYNC_HEADERDIR=$(pkg-config --variable=headerdir opensync-1.0)
OPENSYNC_SCHEMASDIR=$(pkg-config --variable=schemasdir opensync-1.0)


AC_SUBST(OPENSYNC_CONFIGDIR) ## This is the place where you can install the default configuration files of your plugin
AC_SUBST(OPENSYNC_PLUGINDIR) ## This is the dir where you plugin will be installed
AC_SUBST(OPENSYNC_FORMATSDIR) ## Here are format plugins installed (if any)
AC_SUBST(OPENSYNC_HEADERDIR) ## Here are the headers that a user interface may need (if any)
AC_SUBST(OPENSYNC_SCHEMASDIR)

AC_DEFINE_UNQUOTED(XSLTDIR, ["${OPENSYNC_SCHEMASDIR}"], [Full path to xslt definitions])

AC_OUTPUT([
Makefile
src/Makefile
tests/Makefile
syncml-client-plugin.spec
])
