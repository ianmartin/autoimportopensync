plugindir=@OPENSYNC_PLUGINDIR@
pythonplgdir=@OPENSYNC_PYTHONPLG_DIR@
pythonlibdir=@OPENSYNC_PYTHONLIB_DIR@

OPENSYNC_LIBS=-lopensync

#FIXME: set EXTRA_DIST


plugin_LTLIBRARIES = python_module.la

python_module_la_SOURCES = src/python_module.c
python_module_la_LDFLAGS = @PYTHON_LDFLAGS@ -avoid-version -export-dynamic -module
python_module_la_CFLAGS = -Wall -Werror
python_module_la_CPPFLAGS = @PYTHON_CFLAGS@ \
	-DOPENSYNC_PYTHONPLG_DIR=\"$(pythonplgdir)\" \
	-DOPENSYNC_PYTHONLIB_DIR=\"$(pythonlibdir)\"
python_module_la_LIBADD = @PYTHON_LIBS@


# yes, _PROGRAMS. I am trying to avoid using libtool here
pythonlib_PROGRAMS = opensync.so

#FIXME: the lines below don't work. check why (maybe use distutils?)
# only -Werror. pyrex generated code doesn't support -Wall
opensync_so_CFLAGS = -Werror -fPIC
opensync_so_CPPFLAGS = @PYTHON_CFLAGS@ -Werror
nodist_opensync_so_SOURCES = src/opensync.c
opensync_so_LDFLAGS = @PYTHON_LDFLAGS@ -shared
opensync_so_LDADD = $(OPENSYNC_LIBS)

src/opensync.c: src/opensync.pyx
	pyrexc $<
