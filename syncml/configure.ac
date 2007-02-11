dnl Process this file with autoconf to produce a configure script.

AC_PREREQ(2.58)
AC_INIT([OpenSync SyncML Plugin], 0.21, [], [libopensync-plugin-syncml])
AM_INIT_AUTOMAKE(foreign)
AC_CONFIG_SRCDIR(src/syncml_plugin.c)
AC_CONFIG_HEADER(config.h)

export PKG_CONFIG_PATH=${PKG_CONFIG_PATH}:/usr/lib/pkgconfig:$prefix/lib/pkgconfig:/usr/local/lib/pkgconfig
pkg_modules="opensync-1.0 glib-2.0 libsyncml-1.0 >= 0.4.0 libxml-2.0"
PKG_CHECK_MODULES(PACKAGE, [$pkg_modules])
AC_SUBST(PACKAGE_CFLAGS)
AC_SUBST(PACKAGE_LIBS)

AC_ISC_POSIX
AC_PROG_CC
AM_PROG_CC_STDC
AC_HEADER_STDC
AC_DISABLE_STATIC
AC_PROG_LIBTOOL

OPENSYNC_CONFIGDIR=$(pkg-config --variable=configdir opensync-1.0)
OPENSYNC_PLUGINDIR=$(pkg-config --variable=plugindir opensync-1.0)
OPENSYNC_FORMATSDIR=$(pkg-config --variable=formatsdir opensync-1.0)
OPENSYNC_HEADERDIR=$(pkg-config --variable=headerdir opensync-1.0)

AC_SUBST(OPENSYNC_CONFIGDIR) ## This is the place where you can install the default configuration files of your plugin
AC_SUBST(OPENSYNC_PLUGINDIR) ## This is the dir where you plugin will be installed
AC_SUBST(OPENSYNC_FORMATSDIR) ## Here are format plugins installed (if any)
AC_SUBST(OPENSYNC_HEADERDIR) ## Here are the headers that a user interface may need (if any)


#### Check for libsoup ####
AC_ARG_ENABLE(http, AC_HELP_STRING([--disable-http],[disable http support]),enable_http=$enableval, enable_http=auto)

if test x$enable_http != xno; then
	PKG_CHECK_MODULES(LIBSOUP, libsoup-2.2 >= 2.2.91, HAVE_LIBSOUP=yes, HAVE_LIBSOUP=no)
	if test "x${HAVE_LIBSOUP}" = "xyes"; then
		AC_SUBST(LIBSOUP_CFLAGS)
		AC_SUBST(LIBSOUP_LIBS)
		ENABLE_HTTP=yes
		AC_SUBST(ENABLE_HTTP)
		AC_DEFINE(ENABLE_HTTP,1,[Http support])
	else
		AC_MSG_ERROR("Libsoup not found")
	fi
else
	ENABLE_HTTP=no
fi
AM_CONDITIONAL(ENABLE_HTTP, test x$HAVE_LIBSOUP = xyes)


#### Check for openobex ####
AC_ARG_ENABLE(obex, AS_HELP_STRING([--disable-obex],[diable obex support]),enable_obex=$enableval, enable_obex=auto)

if test x$enable_obex != xno; then
	PKG_CHECK_MODULES(LIBOPENOBEX, openobex >= 1.1, HAVE_OPENOBEX=yes, HAVE_OPENOBEX=no)
	if test "x${HAVE_OPENOBEX}" = "xyes"; then
                AC_SUBST(LIBOPENOBEX_CFLAGS)
                AC_SUBST(LIBOPENOBEX_LIBS)
                ENABLE_OBEX=yes
                AC_SUBST(ENABLE_OBEX)
                AC_DEFINE(ENABLE_OBEX,1,[Obex support])
	else
                AC_MSG_ERROR("OpenObex not found")
	fi
else
	ENABLE_OBEX=no
fi
AM_CONDITIONAL(ENABLE_OBEX, test x$HAVE_OPENOBEX = xyes)


#### plausibility check ####
if test "x$ENABLE_HTTP" != "xyes"; then
	if test "x$ENABLE_OBEX" != "xyes"; then
		AC_MSG_ERROR("No transport protocol activated. Please enable http and/or obex support.")
	fi
fi


AC_OUTPUT([
Makefile
src/Makefile
])


echo
echo =========================
echo supported transports:
if test "x$ENABLE_HTTP" = "xyes"; then
    echo http support is ENABLED.
else
    echo http support is DISABLED.
fi
if test "x$ENABLE_OBEX" = "xyes"; then
    echo obex support is ENABLED.
else
    echo obex support is DISABLED.
fi
echo =========================
