plugindir=@OPENSYNC_PLUGINDIR@
pythonplgdir=@OPENSYNC_PYTHONPLG_DIR@

OPENSYNC_LIBS=-lopensync

#FIXME: set EXTRA_DIST

EXTRA_DIST = \
	src/pywrap.c \
	src/opensync.pyx

plugin_LTLIBRARIES = python_module.la

python_module_la_SOURCES = src/python_module.c
python_module_la_LDFLAGS = @PYTHON_LDFLAGS@ -avoid-version -export-dynamic -module $(OSYNC_LIBS)
python_module_la_CFLAGS = -Wall -Werror $(OSYNC_CFLAGS)
python_module_la_CPPFLAGS = @PYTHON_CFLAGS@ \
	-DOPENSYNC_PYTHONPLG_DIR=\"$(pythonplgdir)\"
python_module_la_LIBADD = @PYTHON_LIBS@



src/opensync.c: src/opensync.pyx
	pyrexc $<

# opensync python module building rules
# It uses setup.py and distutils to
# make things easier
opensync.so: src/opensync.c setup.py
	CFLAGS="$$CFLAGS $(OSYNC_CFLAGS)" python setup.py build_ext

all-local: opensync.so

install-exec-local:
	python setup.py install

clean-local:
	python setup.py clean
