dnl Process this file with autoconf to produce a configure script.

AC_PREREQ(2.58)
AC_INIT([LDAP addressbook synchronization plugin], 0.20, [], [libopensync-plugin-ldap])
AM_INIT_AUTOMAKE(foreign)
AC_CONFIG_SRCDIR(src/ldap_plugin.c)
AM_CONFIG_HEADER(config.h)

AC_ISC_POSIX
AC_PROG_CC
AM_PROG_CC_STDC
AC_HEADER_STDC
AC_DISABLE_STATIC
AC_PROG_LIBTOOL

export PKG_CONFIG_PATH=${PKG_CONFIG_PATH}:/usr/lib/pkgconfig:$prefix/lib/pkgconfig:/usr/local/lib/pkgconfig
pkg_modules="glib-2.0 opensync-1.0 libxml-2.0"
PKG_CHECK_MODULES(PACKAGE, [$pkg_modules])
AC_SUBST(PACKAGE_CFLAGS)
AC_SUBST(PACKAGE_LIBS)

AC_CHECK_HEADER(ldap.h, [], AC_ERROR(ldap.h not found))
AC_CHECK_LIB(ldap, ldap_open, [], AC_ERROR(could not link to ldap))
AC_CHECK_HEADER(sasl/sasl.h, [], AC_ERROR(sasl/sasl.h not found))
AM_PATH_LIBGCRYPT(1.1.91, 
		[CFLAGS="$CFLAGS $LIBGCRYPT_CFLAGS"
                 TESTS_ENVIRONMENT="LD_LIBRARY_PATH=`$LIBGCRYPT_CONFIG --prefix`/lib"
                 AC_CHECK_HEADERS(gcrypt.h)], 
		AC_ERROR(libgcrypt not found))
AC_SUBST(LIBGCRYPT_LIBS)

## Check for libgcrypt api version ##
GCRYPT_VERSION=$(/usr/bin/libgcrypt-config --api-version 2>/dev/null || echo 0)
if test "$GCRYPT_VERSION" -eq 0; then
	AC_DEFINE(OLD_GCRYPT, 1, [Use old gcrypt API])
	AC_SUBST(OLD_GCRYPT)
fi	

## Defaults ##
CRASH_TEST=no
STRESS_TEST=no
ERROR_TEST=no


OPENSYNC_CONFIGDIR=$(pkg-config --variable=configdir opensync-1.0)
OPENSYNC_PLUGINDIR=$(pkg-config --variable=plugindir opensync-1.0)
OPENSYNC_HEADERDIR=$(pkg-config --variable=headerdir opensync-1.0)

AC_SUBST(OPENSYNC_CONFIGDIR)
AC_SUBST(OPENSYNC_PLUGINDIR)
AC_SUBST(OPENSYNC_HEADERDIR)

AC_OUTPUT([
Makefile
src/Makefile
])

echo "================================"
echo "Plugin will be installed in ${OPENSYNC_PLUGINDIR}"
echo "Default config will be installed in ${OPENSYNC_CONFIGDIR}"
echo "================================"
