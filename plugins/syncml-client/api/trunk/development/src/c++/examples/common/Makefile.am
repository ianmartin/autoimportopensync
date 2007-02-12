check_PROGRAMS          = syncmlclient

AM_CPPFLAGS             = -gdwarf-2 -O0 -g3 

syncmlclient_SOURCES    = client.cpp

AM_LDFLAGS              = @SYNCML_LIBS@  -lsyncmlclient -L@libdir@

INCLUDES                = -I/usr/local/include/syncml-client -I/usr/include/curl

memcheck:
	valgrind --leak-check=yes ./syncmlclient 

run-test:		check
	env MALLOC_CHECK_=0 ./syncmlclient

