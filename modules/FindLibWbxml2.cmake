# - Try to find libwbxml
# Find libwbxml headers, libraries and the answer to all questions.
#
#  LIBWBXML2_FOUND               True if libwbxml got found
#  LIBWBXML2_INCLUDE_DIRS        Location of libwbxml headers 
#  LIBWBXML2_LIBRARIES           List of libaries to use libwbxml
#
# Copyright (c) 2007 Daniel Gollub <dgollub@suse.de>
# Copyright (c) 2007 Bjoern Ricks  <b.ricks@fh-osnabrueck.de>
#
#  Redistribution and use is allowed according to the terms of the New
#  BSD license.
#  For details see the accompanying COPYING-CMAKE-SCRIPTS file.
#

INCLUDE( FindPkgConfig )

PKG_SEARCH_MODULE( LIBWBXML2 libwbxml2 )


IF( NOT LIBWBXML2_FOUND )
	FIND_PATH( LIBWBXML2_INCLUDE_DIRS wbxml.h
			PATHS
			/opt/local/include/
			/sw/include/
			/usr/local/include/
			/usr/include/ )
	FIND_LIBRARY( LIBWBXML2_LIBRARIES wbxml2
			PATHS
			/opt/local/lib
			/sw/lib
			/usr/lib
			/usr/local/lib
			/usr/lib64
			/usr/local/lib64
			/opt/lib64 )
	# Report results
	IF ( LIBWBXML2_LIBRARIES AND LIBWBXML2_INCLUDE_DIRS )	
		SET( LIBWBXML_FOUND 1 )
		IF ( NOT LibWbxml2_FIND_QUIETLY )
			MESSAGE( STATUS "Found libwbxml2: ${LIBWBXML2_LIBRARIES}" )
		ENDIF ( NOT LibWbxml2_FIND_QUIETLY )
	ELSE ( LIBWBXML2_LIBRARIES AND LIBWBXML2_INCLUDE_DIRS )	
		IF ( LibWbxml2_FIND_REQUIRED )
			MESSAGE( SEND_ERROR "Could NOT find libwbxml2" )
		ELSE ( LibWbxml2_FIND_REQUIRED )
			IF ( NOT LibWbxml2_FIND_QUIETLY )
				MESSAGE( STATUS "Could NOT find libwbxml2" )	
			ENDIF ( NOT LibWbxml2_FIND_QUIETLY )
		ENDIF ( LibWbxml2_FIND_REQUIRED )
	ENDIF ( LIBWBXML2_LIBRARIES AND LIBWBXML2_INCLUDE_DIRS )
ENDIF( NOT LIBWBXML2_FOUND )

# Hide advanced variables from CMake GUIs
MARK_AS_ADVANCED( LIBWBXML2_LIBRARIES LIBWBXML2_INCLUDE_DIRS )

