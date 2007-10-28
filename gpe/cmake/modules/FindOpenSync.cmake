# - Try to find OpenSync
# Find OpenSync headers, libraries and the answer to all questions.
#
#  OPENSYNC_FOUND               True if OpenSync got found
#  OPENSYNC_INCLUDE_DIR         Location of OpenSync headers 
#  OPENSYNC_LIBRARIES           List of libaries to use OpenSync
#  OPENSYNC_DEFINITIONS         Definitions to compile OpenSync 
#
# Copyright (c) 2007 Daniel Gollub <dgollub@suse.de>
#
#  Redistribution and use is allowed according to the terms of the New
#  BSD license.
#  For details see the accompanying COPYING-CMAKE-SCRIPTS file.
#

IF ( NOT WIN32 )
	INCLUDE( UsePkgConfig )
	# Take care about opensync-1.0.pc settings
	PKGCONFIG( opensync-1.0 _opensync_include_DIR _opensync_link_DIR _opensync_link_FLAGS _opensync_cflags )
ENDIF ( NOT WIN32 )

# Look for OpenSync include dir and libraries, and take care about pkg-config first...
FIND_PATH( OPENSYNC_INCLUDE_DIR opensync/opensync.h PATHS ${_opensync_include_DIR} PATH_SUFFIXES opensync-1.0 NO_DEFAULT_PATH )
FIND_PATH( OPENSYNC_INCLUDE_DIR opensync/opensync.h PATH_SUFFIXES opensync-1.0 )

FIND_LIBRARY( OPENSYNC_LIBRARIES opensync PATHS ${_opensync_link_DIR} NO_DEFAULT_PATH )
FIND_LIBRARY( OPENSYNC_LIBRARIES opensync )

# Report results
IF ( OPENSYNC_LIBRARIES AND OPENSYNC_INCLUDE_DIR )	
	SET( OPENSYNC_FOUND 1 )
	IF ( NOT OPENSYNC_FIND_QUIETLY )
		MESSAGE( STATUS "Found OpenSync: ${OPENSYNC_LIBRARIES}" )
	ENDIF ( NOT OPENSYNC_FIND_QUIETLY )
ELSE ( OPENSYNC_LIBRARIES AND OPENSYNC_INCLUDE_DIR )	
	IF ( NOT OPENSYNC_FIND_QUIETLY )
		MESSAGE( STATUS "Could NOT find OpenSync" )
	ENDIF ( NOT OPENSYNC_FIND_QUIETLY )
ENDIF ( OPENSYNC_LIBRARIES AND OPENSYNC_INCLUDE_DIR )	

# Hide advanced variables from CMake GUIs
MARK_AS_ADVANCED( OPENSYNC_LIBRARIES OPENSYNC_INCLUDE_DIR )

