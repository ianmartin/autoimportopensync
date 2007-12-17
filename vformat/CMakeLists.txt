PROJECT( libopensync-plugin-vformat C )

SET( VERSION "0.34" )

SET( CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/modules" )

SET( BUILD_CONTACT TRUE CACHE BOOL "Build contact support for vCard 2.1 and vCard 3.0" )
SET( BUILD_CALENDAR TRUE CACHE BOOL "Build calendar support for vCalendar and iCalendar" )
SET( BUILD_NOTE TRUE CACHE BOOL "Build note support for vNote" )
SET( BUILD_XMLFORMAT CACHE TRUE BOOL "Build format converters for OpenSync XMLFormat" )

FIND_PACKAGE( OpenSync REQUIRED )
FIND_PACKAGE( GLIB2 REQUIRED )
FIND_PACKAGE( Check )

INCLUDE( OpenSyncInternal )
INCLUDE( OpenSyncPlatforms )
ADD_SUBDIRECTORY( src )
ADD_SUBDIRECTORY( tools )

CONFIGURE_FILE( "config.h.cmake" "config.h" )

IF ( CHECK_FOUND ) 
	ADD_SUBDIRECTORY( tests )
ENDIF ( CHECK_FOUND )

OPENSYNC_PACKAGE( ${PROJECT_NAME} ${VERSION} )


## Packaging

SET( CPACK_GENERATOR "TGZ;TBZ2" ) # This line is need for a CMake (Version 2.4.7) Bug - Fixed in CVS
SET( CPACK_SOURCE_GENERATOR "TGZ;TBZ2")
SET( CPACK_SOURCE_PACKAGE_FILE_NAME "libopensync-plugin-vformat-${VERSION}" )
INCLUDE( CPack )

