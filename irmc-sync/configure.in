dnl Process this file with autoconf to produce a configure script.

AC_PREREQ(2.58)
AC_INIT([OpenSync IrMC Plugin], 0.22, [], libopensync-plugin-irmc)
AM_INIT_AUTOMAKE(foreign)
AC_CONFIG_SRCDIR(src/irmc_sync.c)
AM_CONFIG_HEADER(config.h)

export PKG_CONFIG_PATH=${PKG_CONFIG_PATH}:/usr/lib/pkgconfig:$prefix/lib/pkgconfig:/usr/local/lib/pkgconfig
pkg_modules="opensync-1.0 glib-2.0 libxml-2.0"
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


#### Check for openobex ####
if $(pkg-config --exists openobex); then
	PKG_CHECK_MODULES(OPENOBEX, [openobex])
	AC_SUBST(OPENOBEX_CFLAGS)
	AC_SUBST(OPENOBEX_LIBS)
else	
	AM_PATH_OPENOBEX
fi	

if test -n "${OPENOBEX_LIBS}"; then
    AC_CHECK_HEADER([openobex/obex.h], ENABLE_OBEX="yes", ENABLE_OBEX="no")
    if test $ENABLE_OBEX = "yes" ; then
        AC_SUBST(ENABLE_OBEX)
        AC_DEFINE(ENABLE_OBEX,1,[Obex Transport])
    else
        AC_MSG_ERROR(You must have openobex 0.9.8 or openobex >= 1.0.0 installed.)
        AC_MSG_ERROR(Openobex not found!)
        AC_MSG_ERROR(You need to install openobex-devel or libopenobex-dev.)
        exit 1
    fi
else
    AC_MSG_ERROR(You must have openobex 0.9.8 or openobex >= 1.0.0 installed.)
    AC_MSG_ERROR(Openobex not found!)
    AC_MSG_ERROR(You need to install openobex-devel or libopenobex-dev.)
    exit 1
fi

#### Check for bluez ####
AC_ARG_ENABLE(bluetooth, AC_HELP_STRING([--disable-bluetooth],[disable Bluetooth support]),enable_bluetooth=$enableval, enable_bluetooth=auto)

if test x$enable_bluetooth != xno; then
    AC_CHECK_HEADER(bluetooth/bluetooth.h, BLUETOOTH=1, BLUETOOTH=0)
    AC_CHECK_LIB(bluetooth, hci_open_dev,,BLUETOOTH=0)
    AC_CHECK_LIB(sdp, sdp_connect, SDP=1,SDP=0)
    AC_MSG_CHECKING(for correct SDP lib version)


    if test "x${BLUETOOTH}" = "x0"; then
        AC_MSG_RESULT([no])
		AC_MSG_NOTICE(Bluetooth is DISABLED. You must have the bluez-libs installed.)
    fi

	AC_DEFINE_UNQUOTED(HAVE_BLUETOOTH, ${BLUETOOTH},Defined if Bluetooth is enabled.)
fi

AM_CONDITIONAL(SDP,test "${SDP}" = "1")
AM_CONDITIONAL(BLUETOOTH,test "${BLUETOOTH}" = "1")

#### Check for irda ####
AC_ARG_ENABLE(irda, AC_HELP_STRING([--disable-irda],[disable IrDA support]),enable_irda=$enableval, enable_irda=auto)

if test x$enable_irda != xno; then
    AC_MSG_CHECKING(for IrDA support)
    AC_TRY_COMPILE([
        #include <sys/socket.h>
        #include <linux/types.h>
        #include <linux/irda.h>],[],IRDA=1; AC_MSG_RESULT(yes), IRDA=0;AC_MSG_RESULT(no) )
    AC_DEFINE_UNQUOTED(HAVE_IRDA, ${IRDA},Defined if IrDA is enabled.)
fi

AC_OUTPUT([
Makefile
src/Makefile
src/bfb/Makefile
])

echo
echo ======================
echo IRMC-Sync detected features:
if test "x${IRDA}" = "x1"; then
    echo IrDA is ENABLED.
else
    echo IrDA is DISABLED.
fi
if test "x${BLUETOOTH}" = "x1"; then
    echo Bluetooth is ENABLED.
else
    echo Bluetooth is DISABLED.
fi

if test "x${SDP}" = "x1"; then
	echo Bluetooth uses SDP lib
fi

echo ======================
