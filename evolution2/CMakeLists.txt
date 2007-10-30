PROJECT( evo2-sync C )

SET( FILESYNC_VERSION "0.33" )	

SET( CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/modules" )

FIND_PACKAGE( EDS REQUIRED )
FIND_PACKAGE( OpenSync REQUIRED )
FIND_PACKAGE( GLIB2 REQUIRED )
FIND_PACKAGE( LibXml2 REQUIRED )
FIND_PACKAGE( GConf2 REQUIRED )
FIND_PACKAGE( Check )

INCLUDE( OpenSyncInternal )

SET( CMAKE_BUILD_TYPE RelWithDebInfo )

ADD_SUBDIRECTORY( src )

IF ( CHECK_FOUND )
  ADD_SUBDIRECTORY( tests )
ENDIF ( CHECK_FOUND )

#############################################

CONFIGURE_FILE( misc/evolution2.spec.in misc/evolution2.spec )

## Packaging ################################

SET( CPACK_GENERATOR "TGZ;TBZ2" ) # This line is need for a CMake (Version 2.4.7) Bug - Fixed in CVS
SET( CPACK_SOURCE_GENERATOR "TGZ;TBZ2")
SET( CPACK_SOURCE_PACKAGE_FILE_NAME "libopensync-plugin-evolution2-${FILESYNC_VERSION}" )
INCLUDE( CPack )

