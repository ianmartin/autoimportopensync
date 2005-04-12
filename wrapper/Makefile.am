
AM_CFLAGS = -Wall -Werror

INCLUDES= @PACKAGE_CFLAGS@ $(PYTHON_INCLUDES) -I$(top_srcdir) -I$(top_srcdir)/opensync

opensync_wrap.c: opensync.i
	$(SWIG) -python -I$(top_srcdir)/opensync $(PYTHON_INCLUDES) opensync.i

EXTRA_DIST = opensync.i

CLEANFILES = \
	opensync_impl.py \
	opensync.pyc \
	opensync_wrap.c

if HAVE_PYTHON
osdir = $(pythondir)
os_PYTHON = opensync_impl.py opensync.py

osbindingsdir = $(pythondir)
osbindings_LTLIBRARIES = _opensync_impl.la
endif

_opensync_impl_la_SOURCES = opensync_wrap.c
_opensync_impl_la_LDFLAGS = @PACKAGE_LIBS@ @XML_LIBS@ -R $(libdir) -module
_opensync_impl_la_LIBADD = $(top_builddir)/opensync/libopensync.la
