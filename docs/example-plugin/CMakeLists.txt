PROJECT( libopensync-plugin-example )

SET( VERSION "0.38" )

# Look for 3rd Party CMake modules
SET( CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/modules" )

cmake_minimum_required(VERSION 2.4.4)

FIND_PACKAGE( OpenSync REQUIRED )
FIND_PACKAGE( GLIB2 REQUIRED )

INCLUDE( OpenSyncInternal )

ADD_SUBDIRECTORY( src )

## Packaging

OPENSYNC_PACKAGE( ${CMAKE_PROJECT_NAME} ${VERSION} )

