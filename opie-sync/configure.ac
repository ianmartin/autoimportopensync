dnl Process this file with autoconf to produce a configure script.

AC_PREREQ(2.58)
AC_INIT([OpenSync Opie Plugin], 0.32, [], [libopensync-plugin-opie])
AM_INIT_AUTOMAKE(foreign)
AC_CONFIG_SRCDIR(src/opie_sync.c)
AC_CONFIG_HEADER(config.h)

export PKG_CONFIG_PATH=${PKG_CONFIG_PATH}:/usr/lib/pkgconfig:$prefix/lib/pkgconfig:/usr/local/lib/pkgconfig
pkg_modules="opensync-1.0 glib-2.0 libxml-2.0 openssl >= 0.9.5"
PKG_CHECK_MODULES(PACKAGE, [$pkg_modules])

AC_PROG_CC

#save old cflags
OLD_CFLAGS=$CFLAGS
CFLAGS="$OLD_CFLAGS -Wno-pointer-sign"
AC_MSG_CHECKING(if gcc supports $CFLAGS compiler option)
AC_TRY_COMPILE( [], [ int main() { return 0; } ],
    ac_cv_have_no_pointer_sign=yes,
    ac_cv_have_no_pointer_sign=no)
AC_MSG_RESULT($ac_cv_have_no_pointer_sign)
if test $ac_cv_have_no_pointer_sign = yes; then
    PACKAGE_CFLAGS="$PACKAGE_CFLAGS -Wno-pointer-sign"
fi

AC_SUBST(PACKAGE_CFLAGS)
AC_SUBST(PACKAGE_LIBS)
#restore old cflags
CFLAGS=$OLD_CFLAGS
					     
AC_CHECK_LIB(curl,curl_global_init,,AC_MSG_ERROR(You must have libcurl installed.))

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

#### check for glib2-devel >= 2.10 ####
pkg-config --exists 'glib-2.0 < 2.10' && AC_DEFINE([OLD_GLIB_VER],1,[Define to 1 if glib2-devel < 2.10])
	
AC_OUTPUT([
Makefile
src/Makefile
])
