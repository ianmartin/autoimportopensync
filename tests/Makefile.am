## Process this file with automake to produce Makefile.in

AM_CFLAGS = @XML_CFLAGS@ -Wall -Werror

INCLUDES = @PACKAGE_CFLAGS@ -I$(top_srcdir)/src -I$(top_srcdir)/engine

if ENABLE_TESTS
TESTS = error user member env conv sync
noinst_PROGRAMS = error user member env conv sync
else
TESTS = 
noinst_PROGRAMS = 
endif

error_INCLUDES = @CHECK_CFLAGS@
error_SOURCES = check_error.c
error_LIBS = @CHECK_LIBS@
error_LDFLAGS = @PACKAGE_LIBS@ $(top_builddir)/src/libopensync.la -lcheck

member_INCLUDES = @CHECK_CFLAGS@
member_SOURCES = check_member.c
member_LIBS = @CHECK_LIBS@
member_LDFLAGS = @PACKAGE_LIBS@ $(top_builddir)/src/libopensync.la -lcheck

env_INCLUDES = @CHECK_CFLAGS@
env_SOURCES = check_env.c
env_LIBS = @CHECK_LIBS@
env_LDFLAGS = @PACKAGE_LIBS@ $(top_builddir)/src/libopensync.la -lcheck

user_INCLUDES = @CHECK_CFLAGS@
user_SOURCES = check_user.c
user_LIBS = @CHECK_LIBS@
user_LDFLAGS = @PACKAGE_LIBS@ $(top_builddir)/src/libopensync.la -lcheck

conv_INCLUDES = @CHECK_CFLAGS@
conv_SOURCES = check_conv.c
conv_LIBS = @CHECK_LIBS@
conv_LDFLAGS = @PACKAGE_LIBS@ $(top_builddir)/src/libopensync.la -lcheck

sync_INCLUDES = @CHECK_CFLAGS@
sync_SOURCES = check_sync.c
sync_LIBS = @CHECK_LIBS@
sync_LDFLAGS = @PACKAGE_LIBS@ $(top_builddir)/src/libopensync.la $(top_builddir)/engine/libosengine.la -lcheck
