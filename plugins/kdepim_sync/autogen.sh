#!/bin/sh
# Run this to generate all the initial makefiles, etc.

make -f Makefile.cvs && ./configure "$@"
