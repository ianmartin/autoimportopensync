# - Try to find libwbxml
# Find libwbxml headers, libraries and the answer to all questions.
#
#  LIBWBXML_FOUND               True if libwbxml got found
#  LIBWBXML_INCLUDE_DIR         Location of libwbxml headers 
#  LIBWBXML_LIBRARIES           List of libaries to use libwbxml
#  LIBWBXML_DEFINITIONS         Definitions to compile libwbxml 
#
# Copyright (c) 2007 Daniel Gollub <dgollub@suse.de>
#                    Bjoern Ricks  <b.ricks@fh-osnabrueck.de>
#
#  Redistribution and use is allowed according to the terms of the New
#  BSD license.
#  For details see the accompanying COPYING-CMAKE-SCRIPTS file.
#

INCLUDE( UsePkgConfig )
# Take care about libwbxml2.pc settings
# TODO: search of pkg config file should be done without version number
PKGCONFIG( libwbxml2 _libwbxml_include_DIR _libwbxml_link_DIR _libwbxml_link_FLAGS _libwbxml_cflags )

# Look for libwbxml include dir and libraries, and take care about pkg-config first...
# NO_DEFAULT_PATH = FIND_PATH does not search for libwbxml in the default paths
IF( _libwbxml_include_DIR AND _libwbxml_link_DIR )
	FIND_PATH( LIBWBXML_INCLUDE_DIR wbxml.h PATHS ${_libwbxml_include_DIR} NO_DEFAULT_PATH )
	FIND_LIBRARY( LIBWBXML_LIBRARIES wbxml2 PATHS ${_libwbxml_link_DIR} NO_DEFAULT_PATH )
	#MESSAGE( STATUS "WBXML: ${_libwbxml_include_DIR} ${LIBWBXML_INCLUDE_DIR} ${_libwbxml_link_DIR} ${LIBWBXML_LIBRARIES}" )
ELSE( _libwbxml_include_DIR AND _libwbxml_link_DIR )
	FIND_PATH( LIBWBXML_INCLUDE_DIR wbxml.h
			PATHS
			/opt/local/include/
			/sw/include/
			/usr/local/include/
			/usr/include/ )
	FIND_LIBRARY( LIBWBXML_LIBRARIES wbxml2
			PATHS
			/opt/local/lib
			/sw/lib
			/usr/lib
			/usr/local/lib
			/usr/lib64
			/usr/local/lib64
			/opt/lib64 )
ENDIF( _libwbxml_include_DIR AND _libwbxml_link_DIR )




# Report results
IF ( LIBWBXML_LIBRARIES AND LIBWBXML_INCLUDE_DIR )	
	SET( LIBWBXML_FOUND 1 )
	IF ( NOT LibWbxml2_FIND_QUIETLY )
		MESSAGE( STATUS "Found libwbxml2: ${LIBWBXML_LIBRARIES}" )
	ENDIF ( NOT LibWbxml2_FIND_QUIETLY )
ELSE ( LIBWBXML_LIBRARIES AND LIBWBXML_INCLUDE_DIR )	
	IF ( LibWbxml2_FIND_REQUIRED )
		MESSAGE( SEND_ERROR "Could NOT find libwbxml2" )
	ELSE ( LibWbxml2_FIND_REQUIRED )
		IF ( NOT LibWbxml2_FIND_QUIETLY )
			MESSAGE( STATUS "Could NOT find libwbxml2" )	
		ENDIF ( NOT LibWbxml2_FIND_QUIETLY )
	ENDIF ( LibWbxml2_FIND_REQUIRED )
ENDIF ( LIBWBXML_LIBRARIES AND LIBWBXML_INCLUDE_DIR )	

# Hide advanced variables from CMake GUIs
MARK_AS_ADVANCED( LIBWBXML_LIBRARIES LIBWBXML_INCLUDE_DIR )

