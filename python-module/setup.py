from distutils.core import setup, Extension
setup(name='opensync',
      version='0.3',
      ext_modules=[
          Extension(
              'opensync', ['src/opensync.c', 'src/pywrap.c'],
              libraries=['opensync']
          )
      ],
)
