## Process this file with automake to produce Makefile.in

EXTRA_DIST = data support.h

AM_CFLAGS = @XML_CFLAGS@ -Wall -Werror

INCLUDES = @PACKAGE_CFLAGS@ -I$(top_srcdir)/src -I$(top_srcdir)/engine

if ENABLE_TESTS
TESTS = error user member env conv sync filter multisync
noinst_PROGRAMS = error user member env conv sync filter multisync
else
TESTS = 
noinst_PROGRAMS = 
endif

error_INCLUDES = @CHECK_CFLAGS@
error_SOURCES = check_error.c
error_LIBS = @CHECK_LIBS@
error_LDFLAGS = @PACKAGE_LIBS@ $(top_builddir)/src/libopensync.la -lcheck -R $(libdir)

member_INCLUDES = @CHECK_CFLAGS@
member_SOURCES = check_member.c
member_LIBS = @CHECK_LIBS@
member_LDFLAGS = @PACKAGE_LIBS@ $(top_builddir)/src/libopensync.la -lcheck -R $(libdir)

env_INCLUDES = @CHECK_CFLAGS@
env_SOURCES = check_env.c support.c
env_LIBS = @CHECK_LIBS@
env_LDFLAGS = @PACKAGE_LIBS@ $(top_builddir)/src/libopensync.la $(top_builddir)/engine/libosengine.la -lcheck -R $(libdir)

user_INCLUDES = @CHECK_CFLAGS@
user_SOURCES = check_user.c
user_LIBS = @CHECK_LIBS@
user_LDFLAGS = @PACKAGE_LIBS@ $(top_builddir)/src/libopensync.la -lcheck -R $(libdir)

conv_INCLUDES = @CHECK_CFLAGS@
conv_SOURCES = check_conv.c
conv_LIBS = @CHECK_LIBS@
conv_LDFLAGS = @PACKAGE_LIBS@ $(top_builddir)/src/libopensync.la -lcheck -R $(libdir)

sync_INCLUDES = @CHECK_CFLAGS@
sync_SOURCES = check_sync.c support.c
sync_LIBS = @CHECK_LIBS@
sync_LDFLAGS = @PACKAGE_LIBS@ $(top_builddir)/src/libopensync.la $(top_builddir)/engine/libosengine.la -lcheck -R $(libdir)

filter_INCLUDES = @CHECK_CFLAGS@
filter_SOURCES = check_filter.c support.c
filter_LIBS = @CHECK_LIBS@
filter_LDFLAGS = @PACKAGE_LIBS@ $(top_builddir)/src/libopensync.la $(top_builddir)/engine/libosengine.la -lcheck -R $(libdir)

multisync_INCLUDES = @CHECK_CFLAGS@
multisync_SOURCES = check_multisync.c support.c
multisync_LIBS = @CHECK_LIBS@
multisync_LDFLAGS = @PACKAGE_LIBS@ $(top_builddir)/src/libopensync.la $(top_builddir)/engine/libosengine.la -lcheck -R $(libdir)
