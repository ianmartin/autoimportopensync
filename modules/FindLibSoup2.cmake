# - Try to find libsoup
# Find libsoup headers, libraries and the answer to all questions.
#
#  LIBSOUP_FOUND               True if libsoup got found
#  LIBSOUP_INCLUDE_DIR         Location of libsoup headers 
#  LIBSOUP_LIBRARIES           List of libaries to use libsoup
#  LIBSOUP_DEFINITIONS         Definitions to compile libsoup 
#
# Copyright (c) 2007 Daniel Gollub <dgollub@suse.de>
#                    Bjoern Ricks  <b.ricks@fh-osnabrueck.de>
#
#  Redistribution and use is allowed according to the terms of the New
#  BSD license.
#  For details see the accompanying COPYING-CMAKE-SCRIPTS file.
#

INCLUDE( UsePkgConfig )
# Take care about libsoup-2.2.pc settings
# TODO: search of pkg config file should be done without version number
#PKGCONFIG( libsoup-2.2 _libsoup_include_DIR _libsoup_link_DIR _libsoup_link_FLAGS _libsoup_cflags )

# Look for libsoup include dir and libraries, and take care about pkg-config first...
# NO_DEFAULT_PATH = FIND_PATH does not search for libsoup in the default paths
IF( _libsoup_include_DIR AND _libsoup_link_DIR )
	FIND_PATH( LIBSOUP_INCLUDE_DIR libsoup/soup.h PATHS ${_libsoup_include_DIR} PATH_SUFFIXES libsoup libsoup-2.2 NO_DEFAULT_PATH )
#	MESSAGE( STATUS "pkg-config ${LIBSOUP_INCLUDE_DIR} :: ${_libsoup_include_DIR}" )
	FIND_LIBRARY( LIBSOUP_LIBRARIES soup-2.2 PATHS ${_libsoup_link_DIR} NO_DEFAULT_PATH )
ELSE( _libsoup_include_DIR AND _libsoup_link_DIR )
	FIND_PATH( LIBSOUP_INCLUDE_DIR libsoup/soup.h
			PATHS
			PATH_SUFFIXES libsoup libsoup-2.2 
			/opt/local/include/
			/sw/include/
			/usr/local/include/
			/usr/include/ )
	FIND_LIBRARY( LIBSOUP_LIBRARIES soup-2.2
			PATHS
			/opt/local/lib
			/sw/lib
			/usr/lib
			/usr/local/lib
			/usr/lib64
			/usr/local/lib64
			/opt/lib64 )
ENDIF( _libsoup_include_DIR AND _libsoup_link_DIR )




# Report results
IF ( LIBSOUP_LIBRARIES AND LIBSOUP_INCLUDE_DIR )	
	SET( LIBSOUP_FOUND 1 )
	IF ( NOT LibSoup2_FIND_QUIETLY )
		MESSAGE( STATUS "Found libsoup2: ${LIBSOUP_LIBRARIES}" )
	ENDIF ( NOT LibSoup2_FIND_QUIETLY )
ELSE ( LIBSOUP_LIBRARIES AND LIBSOUP_INCLUDE_DIR )	
	IF ( LibSoup2_FIND_REQUIRED )
		MESSAGE( SEND_ERROR "Could NOT find libsoup2" )
	ELSE ( LibSoup2_FIND_REQUIRED )
		IF ( NOT LibSoup2_FIND_QUIETLY )
			MESSAGE( STATUS "Could NOT find libsoup2" )	
		ENDIF ( NOT LibSoup2_FIND_QUIETLY )
	ENDIF ( LibSoup2_FIND_REQUIRED )
ENDIF ( LIBSOUP_LIBRARIES AND LIBSOUP_INCLUDE_DIR )	

# Hide advanced variables from CMake GUIs
MARK_AS_ADVANCED( LIBSOUP_LIBRARIES LIBSOUP_INCLUDE_DIR )

