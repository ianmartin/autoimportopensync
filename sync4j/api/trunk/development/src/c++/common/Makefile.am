noinst_LTLIBRARIES = libcommon.la

AM_CPPFLAGS          = -gdwarf-2 -O0 -g3

libcommon_la_SOURCES = Log.cpp Error.cpp Unixdefs.cpp

libcommon = @SYNCML_LIBS@ -version-info 0:0:0

INCLUDES  = -I@top_srcdir@/include  @SYNCML_CFLAGS@

