plugindir=@OPENSYNC_PLUGINDIR@
pythonplgdir=@OPENSYNC_PYTHONPLG_DIR@

OPENSYNC_LIBS=-lopensync

#FIXME: set EXTRA_DIST


plugin_LTLIBRARIES = python_module.la

python_module_la_SOURCES = src/python_module.c
python_module_la_LDFLAGS = -avoid-version -export-dynamic -module
python_module_la_CFLAGS = -Wall -Werror
python_module_la_CPPFLAGS = @PYTHON_CFLAGS@ \
	-DOPENSYNC_PYTHONPLG_DIR=\"$(pythonplgdir)\" \
	-DOPENSYNC_PYTHONPLG_DIR=\"$(pythonplgdir)\"

noinst_PROGRAMS = opensync.so

# only -Werror. pyrex generated code doesn't support -Wall
opensync_so_CFLAGS = -Werror
opensync_so_CPPFLAGS = @PYTHON_CFLAGS@ -Werror
nodist_opensync_so_SOURCES = src/opensync.c src/pywrap.c
opensync_so_LDFLAGS = @PYTHON_LDFLAGS@ -shared
opensync_so_LDADD = @PYTHON_LIBS@ $(OPENSYNC_LIBS)

src/opensync.c: src/opensync.pyx
	pyrexc $<
