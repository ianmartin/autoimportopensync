## Process this file with automake to produce Makefile.in

AM_CFLAGS = @XMLCFLAGS@ -Wall -Werror

INCLUDES = @PACKAGE_CFLAGS@ -I$(top_srcdir)/src

if ENABLE_TESTS
TESTS = user member env conv
noinst_PROGRAMS = user member env conv
else
TESTS = 
noinst_PROGRAMS = 
endif

member_INCLUDES = @CHECK_CFLAGS@
member_SOURCES = check_member.c
member_LIBS = @CHECK_LIBS@
member_LDFLAGS = @PACKAGE_LIBS@ $(top_srcdir)/src/libopensync.la -lcheck

env_INCLUDES = @CHECK_CFLAGS@
env_SOURCES = check_env.c
env_LIBS = @CHECK_LIBS@
env_LDFLAGS = @PACKAGE_LIBS@ $(top_srcdir)/src/libopensync.la -lcheck

user_INCLUDES = @CHECK_CFLAGS@
user_SOURCES = check_user.c
user_LIBS = @CHECK_LIBS@
user_LDFLAGS = @PACKAGE_LIBS@ $(top_srcdir)/src/libopensync.la -lcheck

conv_INCLUDES = @CHECK_CFLAGS@
conv_SOURCES = check_conv.c
conv_LIBS = @CHECK_LIBS@
conv_LDFLAGS = @PACKAGE_LIBS@ $(top_srcdir)/src/libopensync.la -lcheck