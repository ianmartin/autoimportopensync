INCLUDES= @PACKAGE_CFLAGS@ $(PYTHON_INCLUDES)

AM_CFLAGS = @XMLCFLAGS@

libdir = /home/azrael/test
lib_LTLIBRARIES = libosync.la

libosync_la_LDFLAGS = -al -rdynamic -rpath /usr/local/lib -module -avoid-version -export-symbols-regex initlibosync @XMLLDFLAGS@
libosync_la_LIBADD = @PACKAGE_LIBS@ -L../src -lopensync
libosync_la_SOURCES = libosync.c

EXTRA_DIST = \
	libosync.pyx
        
CLEANFILES = \
	libosync.c

libosync.c: libosync.pyx
	-pyrexc libosync.pyx
