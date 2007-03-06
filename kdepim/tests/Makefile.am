## Process this file with automake to produce Makefile.in

EXTRA_DIST = check_contact check_event check_todo check_note
INCLUDES = $(OSYNC_CFLAGS) $(XML_CFLAGS) $(GLIB_CFLAGS) $(KDE_INCLUDES) $(QT_INCLUDES) -I$(top_srcdir) -I$(top_srcdir)/src
#TESTS = check_contact check_event check_todo check_note
if ENABLE_TESTS
TESTS = address
noinst_PROGRAMS = address
else
TESTS =
noinst_PROGRAMS =
endif

address_INCLUDES = @CHECK_CFLAGS@
address_SOURCES = check_address.cpp $(top_srcdir)/src/kaddrbook.cpp
address_LIBS = @CHECK_LIBS@
address_LDFLAGS = $(KDE_RPATH) $(LIB_KABC) -lkabc_file -lkaddressbook -L$(libdir)  @OSYNC_LIBS@ -lopensync-xml -lcheck -R $(libdir)

