## Process this file with automake to produce Makefile.in

SUBDIRS = mock-plugin

EXTRA_DIST = data support.h

AM_CFLAGS = @XML_CFLAGS@ -Wall -Werror @GCOV_CFLAGS@

INCLUDES = -I$(top_srcdir) @PACKAGE_CFLAGS@ -I$(top_srcdir)/osengine

if ENABLE_PROF
PROFTEST = coverage.sh
endif

if ENABLE_TESTS
TESTS = error user member locks env conv sync filter multisync vcard errorcodes $(PROFTEST)
noinst_PROGRAMS = error user member locks env conv sync filter multisync vcard vcal errorcodes
else
TESTS = 
noinst_PROGRAMS = 
endif

error_INCLUDES = @CHECK_CFLAGS@
error_SOURCES = check_error.c
error_LIBS = @CHECK_LIBS@
error_LDFLAGS = @PACKAGE_LIBS@ $(top_builddir)/opensync/libopensync.la -lcheck -R $(libdir) @GCOV_LDFLAGS@

member_INCLUDES = @CHECK_CFLAGS@
member_SOURCES = check_member.c
member_LIBS = @CHECK_LIBS@
member_LDFLAGS = @PACKAGE_LIBS@ $(top_builddir)/opensync/libopensync.la -lcheck -R $(libdir) @GCOV_LDFLAGS@

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

conv_INCLUDES = @CHECK_CFLAGS@
conv_SOURCES = check_conv.c support.c
conv_LIBS = @CHECK_LIBS@
conv_LDFLAGS = @PACKAGE_LIBS@ $(top_builddir)/opensync/libopensync.la $(top_builddir)/osengine/libosengine.la -lcheck -R $(libdir) @GCOV_LDFLAGS@

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
