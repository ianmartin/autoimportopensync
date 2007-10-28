PROJECT( example-sync )

SET( EXAMPLESYNC_VERSION ${OPENSYNC_VERSION} )	

# Look for 3rd Party CMake modules
SET( CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/modules" )

FIND_PACKAGE( OpenSync REQUIRED )
FIND_PACKAGE( GLIB2 REQUIRED )

INCLUDE( OpenSyncInternal )

SET( CMAKE_BUILD_TYPE RelWithDebInfo )

ADD_SUBDIRECTORY( src )

## Packaging

SET( CPACK_GENERATOR "TGZ;TBZ2" ) # This line is need for a CMake (Version 2.4.7) Bug - Fixed in CVS
SET( CPACK_SOURCE_GENERATOR "TGZ;TBZ2")
SET( CPACK_SOURCE_PACKAGE_FILE_NAME "libopensync-plugin-example-${OPENSYNC_VERSION}" )
INCLUDE( CPack )

