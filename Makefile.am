plugindir=@OPENSYNC_PLUGINDIR@
pythonplgdir=@OPENSYNC_PYTHONPLG_DIR@

INCLUDES = -I$(top_srcdir)

OPENSYNC_LIBS=-lopensync

plugin_LTLIBRARIES = python_module.la

python_module_la_SOURCES = src/python_module.c
python_module_la_LDFLAGS = @PYTHON_LDFLAGS@ -avoid-version -export-dynamic -module $(OSYNC_LIBS)
python_module_la_CFLAGS = -Wall -Werror $(OSYNC_CFLAGS)
python_module_la_CPPFLAGS = @PYTHON_CFLAGS@ -DOPENSYNC_PYTHONPLG_DIR=\"$(pythonplgdir)\"
python_module_la_LIBADD = @PYTHON_LIBS@

pythonplg_DATA = src/sample
