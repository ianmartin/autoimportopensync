noinst_LTLIBRARIES = libhttplinux.la

AM_CPPFLAGS          = -gdwarf-2 -O0 -g3

libhttplinux_la_SOURCES = TransportAgent.cpp GenericTransportAgent.cpp

libhttplinux = @SYNCML_LIBS@ -version-info 0:0:0

INCLUDES  = -I@top_srcdir@/include  @SYNCML_CFLAGS@
