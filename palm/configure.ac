dnl Process this file with autoconf to produce a configure script.

AC_PREREQ(2.58)
AC_INIT([OpenSync Palm Plugin], 0.33, [], [libopensync-plugin-palm])
AM_INIT_AUTOMAKE(foreign)
AC_CONFIG_SRCDIR(src/palm_sync.c)
AM_CONFIG_HEADER(config.h)

pkg_modules="opensync-1.0 glib-2.0 libxml-2.0"
PKG_CHECK_MODULES(PACKAGE, [$pkg_modules])
AC_SUBST(PACKAGE_CFLAGS)
AC_SUBST(PACKAGE_LIBS)

dnl check for libpisock API
dnl pilot_version=$(pkg-config --modversion pilot-link)	
dnl if test "x$pilot_version" == "x0.11.8"; then
dnl	AC_DEFINE(OLD_PILOT_LINK, 1, [old pilot-link api used]) AC_SUBST(OLD_PILOT_LINK)
dnl fi	

AM_PATH_PILOT_LINK

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

AC_ARG_ENABLE(note,
	AS_HELP_STRING([--disable-note], [Disable note support]),
	disable_note=$enableval)
if test "x$disable_note" != "xno"; then
	AC_DEFINE(HAVE_NOTE, 1, [Enabled note support])
				AC_SUBST(HAVE_NOTE)
fi				

AC_ARG_ENABLE(todo,
	AS_HELP_STRING([--disable-todo], [Disable todo support]),
	disable_todo=$enableval)
if test "x$disable_todo" != "xno"; then
	AC_DEFINE(HAVE_TODO, 1, [Enabled todo support])
				AC_SUBST(HAVE_TODO)
fi				

AC_ARG_ENABLE(contact,
	AS_HELP_STRING([--disable-contact], [Disable contact support]),
	disable_contact=$enableval)
if test "x$disable_contact" != "xno"; then
	AC_DEFINE(HAVE_CONTACT, 1, [Enabled contact support])
				AC_SUBST(HAVE_CONTACT)
fi				

AC_ARG_ENABLE(event,
	AS_HELP_STRING([--disable-event], [Disable event support]),
	disable_event=$enableval)
if test "x$disable_event" != "xno"; then
	AC_DEFINE(HAVE_EVENT, 1, [Enabled event support])
				AC_SUBST(HAVE_EVENT)
fi				

AC_OUTPUT([
Makefile
src/Makefile
misc/palm.spec
])

echo "================================"
echo ""
echo "Plugin will be installed in ${OPENSYNC_PLUGINDIR}"
echo "Default config will be installed in ${OPENSYNC_CONFIGDIR}"
echo "Format will be installed in ${OPENSYNC_FORMATSDIR}"
echo "================================"
