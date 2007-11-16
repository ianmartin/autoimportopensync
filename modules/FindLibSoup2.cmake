# - Try to find libsoup
# Find libsoup headers, libraries and the answer to all questions.
#
#  LIBSOUP2_FOUND               True if libsoup got found
#  LIBSOUP2_INCLUDE_DIRS         Location of libsoup headers 
#  LIBSOUP2_LIBRARIES           List of libaries to use libsoup
#
# Copyright (c) 2007 Daniel Gollub <dgollub@suse.de>
# Copyright (c) 2007 Bjoern Ricks  <b.ricks@fh-osnabrueck.de>
#
#  Redistribution and use is allowed according to the terms of the New
#  BSD license.
#  For details see the accompanying COPYING-CMAKE-SCRIPTS file.
#

INCLUDE( FindPkgConfig )

PKG_SEARCH_MODULE( LIBSOUP2 libsoup-2.2 libsoup2 )

IF( NOT LIBSOUP2_FOUND )

	FIND_PATH( _libsoup2_include_DIR libsoup/soup.h PATH_SUFFIXES libsoup libsoup-2.2 )
	FIND_LIBRARY( _libsoup2_LIBRARY soup-2.2)

	IF ( _libsoup2_include_DIR AND _libsoup2_LIBRARY )
		SET ( _libsoup2_FOUND TRUE )
	ENDIF ( _libsoup2_include_DIR AND _libsoup2_LIBRARY )

	IF ( _libsoup2_FOUND )
		SET ( LIBSOUP2_INCLUDE_DIRS ${_libsoup2_include_DIR} )
		SET ( LIBSOUP2_LIBRARIES ${_libsoup2_LIBRARY} )
	
		# find requited glib2
		IF( NOT GLIB2_FOUND )
			FIND_PACKAGE( GLIB2 REQUIRED )
			IF ( GLIB2_FOUND )
				SET ( LIBSOUP2_INCLUDE_DIRS ${LIBSOUP2_INCLUDE_DIRS} ${GLIB2_INCLUDE_DIR} )
				SET ( LIBSOUP2_LIBRARIES ${LIBSOUP2_LIBRARIES} ${GLIB2_LIBRARY} )
			ENDIF ( GLIB2_FOUND )
		ENDIF( NOT GLIB2_FOUND )
		
		# find required libxml2
		IF( NOT LIBXML2_FOUND )
			FIND_PACKAGE( LibXml2 REQUIRED )
			IF ( LIBXML2_FOUND )
				SET ( LIBSOUP2_INCLUDE_DIRS ${LIBSOUP2_INCLUDE_DIRS} ${LIBXML2_INCLUDE_DIR} )
				SET ( LIBSOUP2_LIBRARIES ${LIBSOUP2_LIBRARIES} ${LIBXML2_LIBRARIES} )
			ENDIF( LIBXML2_FOUND )
		ENDIF( NOT LIBXML2_FOUND )
		
		# find required gnutls
		IF( NOT GNUTLS_FOUND )
			FIND_PACKAGE( GNUTLS REQUIRED )
			IF ( GNUTLS_FOUND )
				SET ( LIBSOUP2_INCLUDE_DIRS ${LIBSOUP2_INCLUDE_DIRS} ${GNUTLS_INCLUDE_DIRS} )
				SET ( LIBSOUP2_LIBRARIES ${LIBSOUP2_LIBRARIES} ${GNUTLS_LIBRARIES} )
			ENDIF( GNUTLS_FOUND )
		ENDIF( NOT GNUTLS_FOUND )
	ENDIF ( _libsoup2_FOUND )

	MARK_AS_ADVANCED( _libsoup2_include_DIR  _libsoup2_LIBRARY )

	# Report results
	IF ( LIBSOUP2_LIBRARIES AND LIBSOUP2_INCLUDE_DIRS AND _libsoup2_FOUND )	
		SET( LIBSOUP2_FOUND 1 )
		IF ( NOT LibSoup2_FIND_QUIETLY )
			MESSAGE( STATUS "Found libsoup2: ${_libsoup2_LIBRARY}" )
		ENDIF ( NOT LibSoup2_FIND_QUIETLY )
	ELSE ( LIBSOUP2_LIBRARIES AND LIBSOUP_INCLUDE_DIRS AND _libsoup2_FOUND )	
		IF ( LibSoup2_FIND_REQUIRED )
			MESSAGE( FATAL_ERROR "Could NOT find libsoup2" )
		ELSE ( LibSoup2_FIND_REQUIRED )
			IF ( NOT LibSoup2_FIND_QUIETLY )
				MESSAGE( STATUS "Could NOT find libsoup2" )	
			ENDIF ( NOT LibSoup2_FIND_QUIETLY )
		ENDIF ( LibSoup2_FIND_REQUIRED )
	ENDIF ( LIBSOUP2_LIBRARIES AND LIBSOUP2_INCLUDE_DIRS AND _libsoup2_FOUND )
ENDIF( NOT LIBSOUP2_FOUND )

# Hide advanced variables from CMake GUIs
MARK_AS_ADVANCED( LIBSOUP2_LIBRARIES LIBSOUP2_INCLUDE_DIRS )

