PROJECT( moto-sync C ) # actually we don't need any compilers

SET( MOTOSYNC_VERSION "0.33" ) 

INCLUDE( OpenSyncDefaults )
INCLUDE( OpenSyncInternal )

SET( CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/modules )

FIND_PACKAGE( OpenSync REQUIRED )
# TODO: Find PyBluez, ...

SET( OPENSYNC_PYTHONPLG_DIR "${LIB_INSTALL_DIR}/opensync/python-plugins" )

INSTALL( FILES motosync.py DESTINATION ${OPENSYNC_PYTHONPLG_DIR} )
INSTALL( FILES moto-sync DESTINATION ${OPENSYNC_CONFIGDIR} )

## Packaging

SET( CPACK_GENERATOR "TGZ;TBZ2" ) # This line is need for a CMake (Version 2.4.7) Bug - Fixed in CVS
SET( CPACK_SOURCE_GENERATOR "TGZ;TBZ2")
SET( CPACK_SOURCE_PACKAGE_FILE_NAME "libopensync-plugin-moto-${MOTOSYNC_VERSION}" )
INCLUDE( CPack )


