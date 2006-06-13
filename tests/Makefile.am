## Process this file with automake to produce Makefile.in

SUBDIRS = mock-plugin

AM_CFLAGS = @XML_CFLAGS@ -Wall -Werror @GCOV_CFLAGS@ -DOPENSYNC_TESTDATA=\"$(srcdir)/\"

INCLUDES = -I$(top_srcdir) @PACKAGE_CFLAGS@ -I$(top_srcdir)/osengine

EXTRA_DIST = \
	data \
	support.h

if ENABLE_PROF
PROFTEST = coverage.sh
endif

if ENABLE_TESTS
TESTS = error module ipc checkdata objformat format-env converter
noinst_PROGRAMS = error module ipc checkdata objformat format-env converter
else
TESTS = error ipc user member group plugin locks env conv sync filter multisync vcard vcal vnote errorcodes $(PROFTEST)
noinst_PROGRAMS = error ipc user member group plugin locks env conv sync filter multisync vcard vcal vnote errorcodes
endif

error_INCLUDES = @CHECK_CFLAGS@
error_SOURCES = check_error.c support.c
error_LIBS = @CHECK_LIBS@
error_LDFLAGS = @PACKAGE_LIBS@ $(top_builddir)/opensync/libopensync.la -lcheck -R $(libdir) @GCOV_LDFLAGS@

module_INCLUDES = @CHECK_CFLAGS@
module_SOURCES = module-tests/check_module.c support.c
module_LIBS = @CHECK_LIBS@
module_LDFLAGS = @PACKAGE_LIBS@ $(top_builddir)/opensync/libopensync.la -lcheck -R $(libdir) @GCOV_LDFLAGS@

ipc_INCLUDES = @CHECK_CFLAGS@
ipc_SOURCES = ipc-tests/check_ipc.c support.c
ipc_LIBS = @CHECK_LIBS@
ipc_LDFLAGS = @PACKAGE_LIBS@ $(top_builddir)/opensync/libopensync.la -lcheck -R $(libdir) @GCOV_LDFLAGS@

checkdata_INCLUDES = @CHECK_CFLAGS@
checkdata_SOURCES = data-tests/check_data.c support.c
checkdata_LIBS = @CHECK_LIBS@
checkdata_LDFLAGS = @PACKAGE_LIBS@ $(top_builddir)/opensync/libopensync.la -lcheck -R $(libdir) @GCOV_LDFLAGS@

objformat_INCLUDES = @CHECK_CFLAGS@
objformat_SOURCES = format-tests/check_objformat.c support.c
objformat_LIBS = @CHECK_LIBS@
objformat_LDFLAGS = @PACKAGE_LIBS@ $(top_builddir)/opensync/libopensync.la -lcheck -R $(libdir) @GCOV_LDFLAGS@

format_env_INCLUDES = @CHECK_CFLAGS@
format_env_SOURCES = format-tests/check_format_env.c support.c
format_env_LIBS = @CHECK_LIBS@
format_env_LDFLAGS = @PACKAGE_LIBS@ $(top_builddir)/opensync/libopensync.la -lcheck -R $(libdir) @GCOV_LDFLAGS@

converter_INCLUDES = @CHECK_CFLAGS@
converter_SOURCES = format-tests/check_converter.c support.c
converter_LIBS = @CHECK_LIBS@
converter_LDFLAGS = @PACKAGE_LIBS@ $(top_builddir)/opensync/libopensync.la -lcheck -R $(libdir) @GCOV_LDFLAGS@

member_INCLUDES = @CHECK_CFLAGS@
member_SOURCES = check_member.c
member_LIBS = @CHECK_LIBS@
member_LDFLAGS = @PACKAGE_LIBS@ $(top_builddir)/opensync/libopensync.la -lcheck -R $(libdir) @GCOV_LDFLAGS@

group_INCLUDES = @CHECK_CFLAGS@
group_SOURCES = check_group.c support.c
group_LIBS = @CHECK_LIBS@
group_LDFLAGS = @PACKAGE_LIBS@ $(top_builddir)/opensync/libopensync.la $(top_builddir)/osengine/libosengine.la  -lcheck -R $(libdir) @GCOV_LDFLAGS@

plugin_INCLUDES = @CHECK_CFLAGS@
plugin_SOURCES = check_plugins.c support.c
plugin_LIBS = @CHECK_LIBS@
plugin_LDFLAGS = @PACKAGE_LIBS@ $(top_builddir)/opensync/libopensync.la $(top_builddir)/osengine/libosengine.la -lcheck -R $(libdir) @GCOV_LDFLAGS@

locks_INCLUDES = @CHECK_CFLAGS@
locks_SOURCES = check_lock.c support.c
locks_LIBS = @CHECK_LIBS@
locks_LDFLAGS = @PACKAGE_LIBS@ $(top_builddir)/opensync/libopensync.la $(top_builddir)/osengine/libosengine.la -lcheck -R $(libdir) @GCOV_LDFLAGS@

env_INCLUDES = @CHECK_CFLAGS@
env_SOURCES = check_env.c support.c
env_LIBS = @CHECK_LIBS@
env_LDFLAGS = @PACKAGE_LIBS@ $(top_builddir)/opensync/libopensync.la $(top_builddir)/osengine/libosengine.la -lcheck -R $(libdir) @GCOV_LDFLAGS@

user_INCLUDES = @CHECK_CFLAGS@
user_SOURCES = check_user.c
user_LIBS = @CHECK_LIBS@
user_LDFLAGS = @PACKAGE_LIBS@ $(top_builddir)/opensync/libopensync.la -lcheck -R $(libdir) @GCOV_LDFLAGS@

sync_INCLUDES = @CHECK_CFLAGS@
sync_SOURCES = check_sync.c support.c
sync_LIBS = @CHECK_LIBS@
sync_LDFLAGS = @PACKAGE_LIBS@ $(top_builddir)/opensync/libopensync.la $(top_builddir)/osengine/libosengine.la -lcheck -R $(libdir) @GCOV_LDFLAGS@

filter_INCLUDES = @CHECK_CFLAGS@
filter_SOURCES = check_filter.c support.c
filter_LIBS = @CHECK_LIBS@
filter_LDFLAGS = @PACKAGE_LIBS@ $(top_builddir)/opensync/libopensync.la $(top_builddir)/osengine/libosengine.la -lcheck -R $(libdir) @GCOV_LDFLAGS@

multisync_INCLUDES = @CHECK_CFLAGS@
multisync_SOURCES = check_multisync.c support.c
multisync_LIBS = @CHECK_LIBS@
multisync_LDFLAGS = @PACKAGE_LIBS@ $(top_builddir)/opensync/libopensync.la $(top_builddir)/osengine/libosengine.la -lcheck -R $(libdir) @GCOV_LDFLAGS@

vcard_INCLUDES = @CHECK_CFLAGS@
vcard_SOURCES = check_vcard.c support.c
vcard_LIBS = @CHECK_LIBS@
vcard_LDFLAGS = @PACKAGE_LIBS@ $(top_builddir)/opensync/libopensync.la $(top_builddir)/osengine/libosengine.la -lcheck -R $(libdir) @GCOV_LDFLAGS@

vcal_INCLUDES = @CHECK_CFLAGS@
vcal_SOURCES = check_vcal.c support.c
vcal_LIBS = @CHECK_LIBS@
vcal_LDFLAGS = @PACKAGE_LIBS@ $(top_builddir)/opensync/libopensync.la $(top_builddir)/osengine/libosengine.la -lcheck -R $(libdir) @GCOV_LDFLAGS@

vnote_INCLUDES = @CHECK_CFLAGS@
vnote_SOURCES = check_vnote.c support.c
vnote_LIBS = @CHECK_LIBS@
vnote_LDFLAGS = @PACKAGE_LIBS@ $(top_builddir)/opensync/libopensync.la $(top_builddir)/osengine/libosengine.la -lcheck -R $(libdir) @GCOV_LDFLAGS@

errorcodes_INCLUDES = @CHECK_CFLAGS@
errorcodes_SOURCES = check_codes.c support.c
errorcodes_LIBS = @CHECK_LIBS@
errorcodes_LDFLAGS = @PACKAGE_LIBS@ $(top_builddir)/opensync/libopensync.la $(top_builddir)/osengine/libosengine.la -lcheck -R $(libdir) @GCOV_LDFLAGS@

CLEANFILES = \
	*.bb \
	*.bbg \
	*.da

clean:
	rm -rf coverage/*
