plugindir=@OPENSYNC_PLUGINDIR@
pythonplgdir=@OPENSYNC_PYTHONPLG_DIR@

#FIXME: set EXTRA_DIST

AM_CPPFLAGS = -Wall -Werror -DOPENSYNC_PYTHONPLG_DIR=\"$(pythonplgdir)\"

plugin_LTLIBRARIES = python_module.la

python_module_la_SOURCES = src/python_module.c
python_module_la_LDFLAGS = -avoid-version -export-dynamic -module
