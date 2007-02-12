
AM_CFLAGS = -Werror

INCLUDES= @PACKAGE_CFLAGS@ $(PYTHON_INCLUDES) -I$(top_srcdir) -I$(top_srcdir)/opensync @XML_CFLAGS@

opensync_wrap.c: $(srcdir)/opensync.i
	$(SWIG) -Werror -python -modern -I$(top_srcdir)/opensync $(PYTHON_INCLUDES) -o opensync_wrap.c $(srcdir)/opensync.i

EXTRA_DIST = opensync.i

CLEANFILES = \
	opensync.py \
	opensync.pyc \
	opensync_wrap.c

if HAVE_PYTHON
osdir = $(pyexecdir)
nodist_os_PYTHON = opensync.py

osbindingsdir = $(pyexecdir)
osbindings_LTLIBRARIES = _opensync.la
endif

nodist__opensync_la_SOURCES = opensync_wrap.c
_opensync_la_LDFLAGS = @PACKAGE_LIBS@ @XML_LIBS@ $(PYTHON_LDFLAGS) -R $(libdir) -module -avoid-version
_opensync_la_LIBADD = $(top_builddir)/opensync/libopensync.la
