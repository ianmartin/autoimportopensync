PROJECT( libopensync-plugin-evolution2 C )

SET( VERSION "0.33" )	

SET( CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/modules" )

message( "modulepath: ${CMAKE_MODULE_PATH}" )

FIND_PACKAGE( EDS REQUIRED )
FIND_PACKAGE( OpenSync REQUIRED )
FIND_PACKAGE( GLIB2 REQUIRED )
FIND_PACKAGE( LibXml2 REQUIRED )
FIND_PACKAGE( GConf2 REQUIRED )
FIND_PACKAGE( Check )

message( "modulepath2: ${CMAKE_MODULE_PATH}" )

INCLUDE( OpenSyncInternal )

ADD_SUBDIRECTORY( src )

IF ( CHECK_FOUND )
  ADD_SUBDIRECTORY( tests )
ENDIF ( CHECK_FOUND )

#############################################

CONFIGURE_FILE( misc/evolution2.spec.in misc/evolution2.spec )

OPENSYNC_PACKAGE( ${PROJECT_NAME} ${VERSION} )

