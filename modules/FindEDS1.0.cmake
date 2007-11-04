# - Try to find evolution-data-server components
# Find evolution-data-server 1.0 headers, libraries and the answer to all questions.
#
#  EDS1.0_FOUND			  True if all components got found
#
#  LIBEBOOK1.0_FOUND               True if libebook1.0 got found
#  LIBEBOOK1.0_INCLUDE_DIR         Location of libebook1.0 headers 
#  LIBEBOOK1.0_LIBRARIES           List of libaries to use libebook1.0
#  LIBEBOOK1.0_DEFINITIONS         Definitions to compile libebook1.0 
#
#  LIBECAL1.0_FOUND               True if libecal1.0 got found
#  LIBECAL1.0_INCLUDE_DIR         Location of libecal1.0 headers 
#  LIBECAL1.0_LIBRARIES           List of libaries to use libecal1.0
#  LIBECAL1.0_DEFINITIONS         Definitions to compile libecal1.0 
#
#  LIBEDATABOOK1.0_FOUND               True if libedata-book1.0 got found
#  LIBEDATABOOK1.0_INCLUDE_DIR         Location of libedata-book1.0 headers 
#  LIBEDATABOOK1.0_LIBRARIES           List of libaries to use libedata-book1.0
#  LIBEDATABOOK1.0_DEFINITIONS         Definitions to compile libedata-book1.0 
#
#  LIBEDATACAL1.0_FOUND               True if libedata-cal1.0 got found
#  LIBEDATACAL1.0_INCLUDE_DIR         Location of libedata-cal1.0 headers 
#  LIBEDATACAL1.0_LIBRARIES           List of libaries to use libedata-cal1.0
#  LIBEDATACAL1.0_DEFINITIONS         Definitions to compile libedata-cal1.0 
#
#  LIBEDATASERVER1.0_FOUND               True if libedataserver1.0 got found
#  LIBEDATASERVER1.0_INCLUDE_DIR         Location of libedataserver1.0 headers 
#  LIBEDATASERVER1.0_LIBRARIES           List of libaries to use libedataserver1.0
#  LIBEDATASERVER1.0_DEFINITIONS         Definitions to compile libedataserver1.0 
#
# Copyright (c) 2007 Juha Tuomala <tuju@iki.fi>
# Copyright (c) 2007 Daniel Gollub <dgollub@suse.de>
# Copyright (c) 2007 Alban Browaeys <prahal@yahoo.com>
#
#  Redistribution and use is allowed according to the terms of the New
#  BSD license.
#  For details see the accompanying COPYING-CMAKE-SCRIPTS file.
#

IF ( NOT WIN32 )
	INCLUDE( UsePkgConfig )

	# Take care about evolution-data-server-1.0.pc settings
	PKGCONFIG( evolution-data-server-1.0 _evolution-data-server-1.0_include_DIR _evolution-data-server-1.0_link_DIR _evolution-data-server-1.0_link_FLAGS _evolution-data-server-1.0_cflags )

	# Take care about libebook-1.0.pc settings
	PKGCONFIG( libebook-1.0 _libebook-1.0_include_DIR _libebook-1.0_link_DIR _libebook-1.0_link_FLAGS _libebook-1.0_cflags )
	EXEC_PROGRAM( ${PKGCONFIG_EXECUTABLE} ARGS libebook-1.0 --variable=privincludedir OUTPUT_VARIABLE _libebook-1.0_include_DIR )
	STRING( REGEX REPLACE "[\r\n]" " " _libebook-1.0_include_DIR "${_libebook-1.0_include_DIR}" )

	# Take care about libecal-1.0.pc settings
	PKGCONFIG( libecal-1.0 _libecal-1.0_include_DIR _libecal-1.0_link_DIR _libecal-1.0_link_FLAGS _libecal-1.0_cflags )
	EXEC_PROGRAM( ${PKGCONFIG_EXECUTABLE} ARGS libecal-1.0 --variable=privincludedir OUTPUT_VARIABLE _libecal-1.0_include_DIR )
	STRING( REGEX REPLACE "[\r\n]" " " _libecal-1.0_include_DIR "${_libecal-1.0_include_DIR}" )

	# Take care about libedata-book-1.0.pc settings
	PKGCONFIG( libedata-book-1.0 _libedata-book-1.0_include_DIR _libedata-book-1.0_link_DIR _libedata-book-1.0_link_FLAGS _libedata-book-1.0_cflags )
	EXEC_PROGRAM( ${PKGCONFIG_EXECUTABLE} ARGS libedata-book-1.0 --variable=privincludedir OUTPUT_VARIABLE _libedata-book-1.0_include_DIR )
	STRING( REGEX REPLACE "[\r\n]" " " _libedata-book-1.0_include_DIR "${_libedata-book-1.0_include_DIR}" )

	# Take care about libedata-cal-1.0.pc settings
	PKGCONFIG( libedata-cal-1.0 _libedata-cal-1.0_include_DIR _libedata-cal-1.0_link_DIR _libedata-cal-1.0_link_FLAGS _libedata-cal-1.0_cflags )
	EXEC_PROGRAM( ${PKGCONFIG_EXECUTABLE} ARGS libedata-cal-1.0 --variable=privincludedir OUTPUT_VARIABLE _libedata-cal-1.0_include_DIR )
	STRING( REGEX REPLACE "[\r\n]" " " _libedata-cal-1.0_include_DIR "${_libedata-cal-1.0_include_DIR}" )

	# Take care about libedataserver-1.0.pc settings
	PKGCONFIG( libedataserver-1.0 _libedataserver-1.0_include_DIR _libedataserver-1.0_link_DIR _libedataserver-1.0_link_FLAGS _libedataserver-1.0_cflags )
	EXEC_PROGRAM( ${PKGCONFIG_EXECUTABLE} ARGS libedataserver-1.0 --variable=privincludedir OUTPUT_VARIABLE _libedataserver-1.0_include_DIR )
	STRING( REGEX REPLACE "[\r\n]" " " _libedataserver-1.0_include_DIR "${_libedataserver-1.0_include_DIR}" )

ENDIF ( NOT WIN32 )

# Look for libebook1.0 include dir and libraries, and take care about pkg-config first...
FIND_PATH( LIBEBOOK1.0_INCLUDE_DIR
		NAMES libebook/e-book.h
		PATHS ${_libebook-1.0_include_DIR}
		PATH_SUFFIXES evolution-data-server-1.0 
		NO_DEFAULT_PATH )

FIND_PATH( LIBEBOOK1.0_INCLUDE_DIR
		NAMES libebook/e-book.h
		PATH_SUFFIXES evolution-data-server-1.0 
		PATHS
		/opt/local/include/
		/sw/include/
		/usr/local/include/
		/usr/include/ )

FIND_LIBRARY( LIBEBOOK1.0_LIBRARIES 
		NAMES ebook-1.0
		PATHS ${_libebook-1.0_link_DIR}
		NO_DEFAULT_PATH )

FIND_LIBRARY( LIBEBOOK1.0_LIBRARIES
		NAMES ebook-1.0
		PATHS
		/opt/local/lib
		/sw/lib
		/usr/lib
		/usr/local/lib
		/usr/lib64
		/usr/local/lib64
		/opt/lib64 )

# Look for libecal1.0 include dir and libraries, and take care about pkg-config first...
FIND_PATH( LIBECAL1.0_INCLUDE_DIR
		NAMES libecal/e-cal.h
		PATHS ${_libecal-1.0_include_DIR}
		PATH_SUFFIXES evolution-data-server-1.0 NO_DEFAULT_PATH )

FIND_PATH( LIBECAL1.0_INCLUDE_DIR
		NAMES libecal/e-cal.h
		PATH_SUFFIXES evolution-data-server-1.0 
		PATHS
		/opt/local/include/
		/sw/include/
		/usr/local/include/
		/usr/include/ )

FIND_LIBRARY( LIBECAL1.0_LIBRARIES 
		NAMES ecal-1.0
		PATHS ${_libecal-1.0_link_DIR}
		NO_DEFAULT_PATH )

FIND_LIBRARY( LIBECAL1.0_LIBRARIES
		NAMES ecal-1.0
		PATHS
		/opt/local/lib
		/sw/lib
		/usr/lib
		/usr/local/lib
		/usr/lib64
		/usr/local/lib64
		/opt/lib64 )

# Look for libedata-book-1.0 include dir and libraries, and take care about pkg-config first...
FIND_PATH( LIBEDATABOOK1.0_INCLUDE_DIR
		NAMES libedata-book/e-data-book.h
		PATHS ${_libedata-book-1.0_include_DIR}
		PATH_SUFFIXES evolution-data-server-1.0 NO_DEFAULT_PATH )

FIND_PATH( LIBEDATABOOK1.0_INCLUDE_DIR
		NAMES libedata-book/e-data-book.h
		PATH_SUFFIXES evolution-data-server-1.0 
		PATHS
		/opt/local/include/
		/sw/include/
		/usr/local/include/
		/usr/include/ )

FIND_LIBRARY( LIBEDATABOOK1.0_LIBRARIES 
		NAMES edata-book-1.0
		PATHS ${_libedata-book-1.0_link_DIR}
		NO_DEFAULT_PATH )

FIND_LIBRARY( LIBEDATABOOK1.0_LIBRARIES
		NAMES edata-book-1.0
		PATHS
		/opt/local/lib
		/sw/lib
		/usr/lib
		/usr/local/lib
		/usr/lib64
		/usr/local/lib64
		/opt/lib64 )

# Look for libedata-cal-1.0 include dir and libraries, and take care about pkg-config first...
FIND_PATH( LIBEDATACAL1.0_INCLUDE_DIR
		NAMES libedata-cal/e-data-cal.h
		PATHS ${_libedata-cal-1.0_include_DIR}
		PATH_SUFFIXES evolution-data-server-1.0 NO_DEFAULT_PATH )

FIND_PATH( LIBEDATACAL1.0_INCLUDE_DIR
		NAMES libedata-cal/e-data-cal.h
		PATH_SUFFIXES evolution-data-server-1.0 
		PATHS
		/opt/local/include/
		/sw/include/
		/usr/local/include/
		/usr/include/ )

FIND_LIBRARY( LIBEDATACAL1.0_LIBRARIES 
		NAMES edata-cal-1.0
		PATHS ${_libedata-cal-1.0_link_DIR} 
		NO_DEFAULT_PATH )

FIND_LIBRARY( LIBEDATACAL1.0_LIBRARIES
		NAMES edata-cal-1.0
		PATHS
		/opt/local/lib
		/sw/lib
		/usr/lib
		/usr/local/lib
		/usr/lib64
		/usr/local/lib64
		/opt/lib64 )

# Look for libedataserver-1.0 include dir and libraries, and take care about pkg-config first...
FIND_PATH( LIBEDATASERVER1.0_INCLUDE_DIR
		NAMES libedataserver/e-data-server-module.h 
		PATHS ${_libedataserver-1.0_include_DIR} 
		PATH_SUFFIXES evolution-data-server-1.0 NO_DEFAULT_PATH )

FIND_PATH( LIBEDATASERVER1.0_INCLUDE_DIR
		NAMES libedataserver/e-data-server-module.h 
		PATH_SUFFIXES evolution-data-server-1.0 
		PATHS
		/opt/local/include/
		/sw/include/
		/usr/local/include/
		/usr/include/ )

FIND_LIBRARY( LIBEDATASERVER1.0_LIBRARIES 
		NAMES edataserver-1.0
		PATHS ${_libedataserver-1.0_link_DIR} 
		NO_DEFAULT_PATH )

FIND_LIBRARY( LIBEDATASERVER1.0_LIBRARIES
		NAMES edataserver-1.0
		PATHS
		/opt/local/lib
		/sw/lib
		/usr/lib
		/usr/local/lib
		/usr/lib64
		/usr/local/lib64
		/opt/lib64 )

# Report results
IF ( LIBEBOOK1.0_LIBRARIES AND LIBEBOOK1.0_INCLUDE_DIR )	
	SET( LIBEBOOK1.0_FOUND 1 )
	IF ( NOT LIBEBOOK1.0_FIND_QUIETLY )
		MESSAGE( STATUS "Found libebook-1.0: ${LIBEBOOK1.0_LIBRARIES}" )
	ENDIF ( NOT LIBEBOOK1.0_FIND_QUIETLY )
	SET( EDS1.0_FOUND 1 )
ELSE ( LIBEBOOK1.0_LIBRARIES AND LIBEBOOK1.0_INCLUDE_DIR )	
	IF ( LIBEBOOK1.0_FIND_REQUIRED )
		MESSAGE( SEND_ERROR "Could NOT find libebook-1.0" )
	ELSE ( LIBEBOOK1.0_FIND_REQUIRED )
		IF ( NOT LIBEBOOK1.0_FIND_QUIETLY )
			MESSAGE( STATUS "Could NOT find libebook-1.0" )	
		ENDIF ( NOT LIBEBOOK1.0_FIND_QUIETLY )
	ENDIF ( LIBEBOOK1.0_FIND_REQUIRED )
	SET( LIBEBOOK1.0_FOUND 1 )
	SET( EDS1.0_FOUND 0 )
ENDIF ( LIBEBOOK1.0_LIBRARIES AND LIBEBOOK1.0_INCLUDE_DIR )	

IF ( LIBECAL1.0_LIBRARIES AND LIBECAL1.0_INCLUDE_DIR )	
	SET( LIBECAL1.0_FOUND 1 )
	IF ( NOT LIBECAL1.0_FIND_QUIETLY )
		MESSAGE( STATUS "Found libecal-1.0: ${LIBECAL1.0_LIBRARIES}" )
	ENDIF ( NOT LIBECAL1.0_FIND_QUIETLY )
	SET( EDS1.0_FOUND 1 )
ELSE ( LIBECAL1.0_LIBRARIES AND LIBECAL1.0_INCLUDE_DIR )	
	IF ( LIBECAL1.0_FIND_REQUIRED )
		MESSAGE( SEND_ERROR "Could NOT find libecal-1.0" )
	ELSE ( LIBECAL1.0_FIND_REQUIRED )
		IF ( NOT LIBECAL1.0_FIND_QUIETLY )
			MESSAGE( STATUS "Could NOT find libecal-1.0" )	
		ENDIF ( NOT LIBECAL1.0_FIND_QUIETLY )
	ENDIF ( LIBECAL1.0_FIND_REQUIRED )
	SET( LIBECAL1.0_FOUND 0 )
	SET( EDS1.0_FOUND 0 )
ENDIF ( LIBECAL1.0_LIBRARIES AND LIBECAL1.0_INCLUDE_DIR )	

IF ( LIBEDATABOOK1.0_LIBRARIES AND LIBEDATABOOK1.0_INCLUDE_DIR )	
	SET( LIBEDATABOOK1.0_FOUND 1 )
	IF ( NOT LIBEDATABOOK1.0_FIND_QUIETLY )
		MESSAGE( STATUS "Found libedata-book-1.0: ${LIBEDATABOOK1.0_LIBRARIES}" )
	ENDIF ( NOT LIBEDATABOOK1.0_FIND_QUIETLY )
	SET( EDS1.0_FOUND 1 )
ELSE ( LIBEDATABOOK1.0_LIBRARIES AND LIBEDATABOOK1.0_INCLUDE_DIR )	
	IF ( LIBEDATABOOK1.0_FIND_REQUIRED )
		MESSAGE( SEND_ERROR "Could NOT find libedatabook-1.0" )
	ELSE ( LIBEDATABOOK1.0_FIND_REQUIRED )
		IF ( NOT LIBEDATABOOK1.0_FIND_QUIETLY )
			MESSAGE( STATUS "Could NOT find libedatabook-1.0" )	
		ENDIF ( NOT LIBEDATABOOK1.0_FIND_QUIETLY )
	ENDIF ( LIBEDATABOOK1.0_FIND_REQUIRED )
	SET( EDS1.0_FOUND 0 )
ENDIF ( LIBEDATABOOK1.0_LIBRARIES AND LIBEDATABOOK1.0_INCLUDE_DIR )	

IF ( LIBEDATACAL1.0_LIBRARIES AND LIBEDATACAL1.0_INCLUDE_DIR )	
	SET( LIBEDATACAL1.0_FOUND 1 )
	IF ( NOT LIBEDATACAL1.0_FIND_QUIETLY )
		MESSAGE( STATUS "Found libedata-cal-1.0: ${LIBEDATACAL1.0_LIBRARIES}" )
	ENDIF ( NOT LIBEDATACAL1.0_FIND_QUIETLY )
	SET( EDS1.0_FOUND 1 )
ELSE ( LIBEDATACAL1.0_LIBRARIES AND LIBEDATACAL1.0_INCLUDE_DIR )	
	IF ( LIBEDATACAL1.0_FIND_REQUIRED )
		MESSAGE( SEND_ERROR "Could NOT find libedata-cal-1.0" )
	ELSE ( LIBEDATACAL1.0_FIND_REQUIRED )
		IF ( NOT LIBEDATACAL1.0_FIND_QUIETLY )
			MESSAGE( STATUS "Could NOT find libedata-cal-1.0" )	
		ENDIF ( NOT LIBEDATACAL1.0_FIND_QUIETLY )
	ENDIF ( LIBEDATACAL1.0_FIND_REQUIRED )
	SET( EDS1.0_FOUND 0 )
ENDIF ( LIBEDATACAL1.0_LIBRARIES AND LIBEDATACAL1.0_INCLUDE_DIR )	

IF ( LIBEDATASERVER1.0_LIBRARIES AND LIBEDATASERVER1.0_INCLUDE_DIR )	
	SET( LIBEDATASERVER1.0_FOUND 1 )
	IF ( NOT LIBEDATASERVER1.0_FIND_QUIETLY )
		MESSAGE( STATUS "Found libedataserver-1.0: ${LIBEDATASERVER1.0_LIBRARIES}" )
	ENDIF ( NOT LIBEDATASERVER1.0_FIND_QUIETLY )
	SET( EDS1.0_FOUND 1 )
ELSE ( LIBEDATASERVER1.0_LIBRARIES AND LIBEDATASERVER1.0_INCLUDE_DIR )	
	IF ( LIBEDATASERVER1.0_FIND_REQUIRED )
		MESSAGE( SEND_ERROR "Could NOT find libedataserver-1.0" )
	ELSE ( LIBEDATASERVER1.0_FIND_REQUIRED )
		IF ( NOT LIBEDATASERVER1.0_FIND_QUIETLY )
			MESSAGE( STATUS "Could NOT find libedataserver-1.0" )	
		ENDIF ( NOT LIBEDATASERVER1.0_FIND_QUIETLY )
	ENDIF ( LIBEDATASERVER1.0_FIND_REQUIRED )
	SET( EDS1.0_FOUND 0 )
ENDIF ( LIBEDATASERVER1.0_LIBRARIES AND LIBEDATASERVER1.0_INCLUDE_DIR )	

# Hide advanced variables from CMake GUIs
MARK_AS_ADVANCED( LIBEBOOK1.0_LIBRARIES LIBEBOOK1.0_INCLUDE_DIR LIBECAL1.0_LIBRARIES LIBECAL1.0_INCLUDE_DIR LIBEDATABOOK1.0_LIBRARIES LIBEDATABOOK1.0_INCLUDE_DIR LIBEDATACAL1.0_LIBRARIES LIBEDATACAL1.0_INCLUDE_DIR LIBEDATASERVER1.0_LIBRARIES LIBEDATASERVER1.0_INCLUDE_DIR )

