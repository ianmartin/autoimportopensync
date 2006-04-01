noinst_LTLIBRARIES   = libptypes.la

AM_CPPFLAGS          = -gdwarf-2 -O0 -g3

libptypes_la_SOURCES = pfatal.cxx            \
                       pobjlist.cxx          \
                       pstrmanip.cxx         \
                       patomic.cxx           \
                       pmem.cxx              \
                       pstring.cxx           \
                       punknown.cxx

libptypes            = @SYNCML_LIBS@ -version-info 0:0:0

INCLUDES             = -I@top_srcdir@/include  @SYNCML_CFLAGS@

