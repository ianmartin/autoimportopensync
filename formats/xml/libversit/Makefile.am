# Makefile for libversit.a

# $Id: Makefile.am,v 1.2.4.1 2004/10/28 14:07:32 azrael2 Exp $

VERSIT_SRC =		\
	vcc.y		\
	vcc.h		\
	vobject.c	\
	vobject.h	\
	port.h		\
	vcaltmp.c	\
	vcaltmp.h

#lib_LTLIBRARIES = libversit.la
noinst_LTLIBRARIES = libversit.la

libversit_la_SOURCES = $(VERSIT_SRC)

# vctest test program
AM_CFLAGS = -D_VCTEST_CONSOLE
noinst_PROGRAMS = vctest
vctest_SOURCES = vctest.c
vctest_LDADD = libversit.la

EXTRA_DIST = README.TXT vcaltest.c vctest.c

AM_YFLAGS=-pversit_
