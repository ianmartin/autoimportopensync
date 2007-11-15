# - Try to find GNUTLS 
# Find GNUTLS headers, libraries and the answer to all questions.
#
#  GNUTLS_FOUND               True if gconf2 got found
#  GNUTLS_INCLUDEDIR          Location of gconf2 headers 
#  GNUTLS_LIBRARIES           List of libaries to use gconf2
#
# Copyright (c) 2007 Bjoern Ricks <b.ricks@fh-osnabrueck.de>
#
#  Redistribution and use is allowed according to the terms of the New
#  BSD license.
#  For details see the accompanying COPYING-CMAKE-SCRIPTS file.
#

INCLUDE( FindPkgConfig )

PKG_SEARCH_MODULE( GNUTLS gnutls )

IF( NOT GNUTLS_FOUND )
	FIND_PATH( GNUTLS_INCLUDE_DIRS gnutls/gnutls.h )
	FIND_LIBRARY( GNUTLS_LIBRARIES gnutls)

	# Report results
	IF ( GNUTLS_LIBRARIES AND GNUTLS_INCLUDE_DIRS )	
		SET( GNUTLS_FOUND 1 )
		IF ( NOT GNUTLS_FIND_QUIETLY )
			MESSAGE( STATUS "Found gnutls: ${GNUTLS_LIBRARIES}" )
		ENDIF ( NOT GNUTLS_FIND_QUIETLY )
	ELSE ( GNUTLS_LIBRARIES AND GNUTLS_INCLUDE_DIRS )	
		IF ( GNUTLS_FIND_REQUIRED )
			MESSAGE( SEND_ERROR "Could NOT find gnutls" )
		ELSE ( GNUTLS_FIND_REQUIRED )
			IF ( NOT GNUTLS_FIND_QUIETLY )
				MESSAGE( STATUS "Could NOT find gnutls" )	
			ENDIF ( NOT GNUTLS_FIND_QUIETLY )
		ENDIF ( GNUTLS_FIND_REQUIRED )
	ENDIF ( GNUTLS_LIBRARIES AND GNUTLS_INCLUDE_DIRS )
ENDIF( NOT GNUTLS_FOUND )

MARK_AS_ADVANCED( GNUTLS_LIBRARIES GNUTLS_INCLUDE_DIRS )
