dnl Process this file with autoconf to produce a configure script.

AC_PREREQ(2.58)
AC_INIT([OpenSync Kdepim Plugin], 0.21, [], [libopensync-plugin-kdepim])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_AUX_DIR([aux])
AC_CONFIG_SRCDIR(src/kdepim_sync.cpp)
AM_INIT_AUTOMAKE(foreign)
AM_CONFIG_HEADER(config.h)

# make sure $ACLOCAL_FLAGS are used during a rebuild.
AC_SUBST([ACLOCAL_AMFLAGS], ["-I $ac_macro_dir \${ACLOCAL_FLAGS}"])

dnl Checks for programs.
AC_CHECK_COMPILERS
AC_DISABLE_STATIC
AC_PROG_LIBTOOL

KDE_SET_PREFIX

dnl for NLS support. Call them in this order!
dnl WITH_NLS is for the po files
AM_KDE_WITH_NLS

KDE_USE_QT(3.0.0)
AC_PATH_KDE

export PKG_CONFIG_PATH=${PKG_CONFIG_PATH}:/usr/lib/pkgconfig:$prefix/lib/pkgconfig:/usr/local/lib/pkgconfig
pkg_modules="opensync-1.0"
PKG_CHECK_MODULES(OSYNC, [opensync-1.0])

# Needed only because opensync-xml leaks libxml and glib
PKG_CHECK_MODULES(XML, [libxml-2.0])
PKG_CHECK_MODULES(GLIB, [glib-2.0])

OPENSYNC_CONFIGDIR=$(pkg-config --variable=configdir opensync-1.0)
OPENSYNC_PLUGINDIR=$(pkg-config --variable=plugindir opensync-1.0)
OPENSYNC_FORMATSDIR=$(pkg-config --variable=formatsdir opensync-1.0)
OPENSYNC_HEADERDIR=$(pkg-config --variable=headerdir opensync-1.0)
OPENSYNC_INCLUDEDIR=$(pkg-config --variable=includedir opensync-1.0)
OPENSYNC_LIBDIR=$(pkg-config --variable=libdir opensync-1.0)

AC_SUBST(OPENSYNC_CONFIGDIR)
AC_SUBST(OPENSYNC_PLUGINDIR)
AC_SUBST(OPENSYNC_FORMATSDIR)
AC_SUBST(OPENSYNC_HEADERDIR)
AC_SUBST(OPENSYNC_INCLUDEDIR)
AC_SUBST(OPENSYNC_LIBDIR)
AC_DEFINE_UNQUOTED([PLUGINDIR], "$OPENSYNC_PLUGINDIR", [OpenSync Plugin Dir])
AC_DEFINE_UNQUOTED([KDEPIM_LIBDIR], "$OPENSYNC_PLUGINDIR", [KDEPIM Lib Dir])

## AC_CHECK_HEADER(${$ac_kde_includes}/libkcal/resourcecalendar.h)
#### Defaults #####
ENABLE_TESTS=no

#### Check for unit tests ####
AC_ARG_ENABLE(unit-tests,
  AS_HELP_STRING([--enable-unit-tests], [enable unit tests]),
  ENABLE_TESTS=$enableval)
testdir=""
if test "x${ENABLE_TESTS}" = "xyes"; then
        AC_CHECK_HEADER(check.h,HAVE_CHECK=1,HAVE_CHECK=0)
        if test "x${HAVE_CHECK}" = "x1"; then
                AM_PATH_CHECK(0.9.0)
        fi

fi
AM_CONDITIONAL(ENABLE_TESTS, test x$HAVE_CHECK = x1)

dnl ======================== Set some gcc CFLAGS to reduce build warnings
if test "x$GCC" = "xyes"; then
  OLD_CXXFLAGS="$CXXFLAGS"

  CXXFLAGS="-fvisibility=hidden"
  AC_MSG_CHECKING(if gcc supports $CXXFLAGS compiler option)
  AC_TRY_COMPILE( [], [ int main() { return 0; } ],
                ac_cv_have_hidden_visibility=yes,
                ac_cv_have_hidden_visibility=no)
  AC_MSG_RESULT($ac_cv_have_hidden_visibility)
  CXXFLAGS="$OLD_CXXFLAGS"
  if test $ac_cv_have_hidden_visibility = yes; then
    CXXFLAGS="$CXXFLAGS -fvisibility=hidden"
    AC_DEFINE([GCC_HASCLASSVISIBILITY], [], [GCC visibility supported])
  fi
fi
dnl PACKAGE set before
AC_C_BIGENDIAN
AC_CHECK_KDEMAXPATHLEN

AC_CONFIG_FILES([ Makefile src/Makefile tests/Makefile ])
AC_OUTPUT

if test "$all_tests" = "bad"; then
  if test ! "$cache_file" = "/dev/null"; then
    echo ""
    echo "Please remove the file $cache_file after changing your setup"
    echo "so that configure will find the changes next time."
    echo ""
  fi
else
  echo ""
  echo "Good - your configure finished. Start make now"
  echo ""
fi
