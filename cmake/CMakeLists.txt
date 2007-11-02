# Copyright (c) 2007 Daniel Gollub <dgollub@suse.de>

CONFIGURE_FILE( "${CMAKE_CURRENT_SOURCE_DIR}/OpenSyncInternal.cmake.in" "${CMAKE_CURRENT_BINARY_DIR}/OpenSyncInternal.cmake" @ONLY )

SET( OPENSYNC_CMAKE_MODULES 
"${CMAKE_CURRENT_SOURCE_DIR}/OpenSyncPlatforms.cmake" 
"${CMAKE_CURRENT_SOURCE_DIR}/OpenSyncPackaging.cmake" 
"${CMAKE_CURRENT_SOURCE_DIR}/OpenSyncTesting.cmake" 
"${CMAKE_CURRENT_SOURCE_DIR}/OpenSyncPlugin.cmake" 
"${CMAKE_CURRENT_BINARY_DIR}/OpenSyncInternal.cmake" 
)

SET( OPENSYNC_CMAKE_DIRECTORY ${SHARE_INSTALL_DIR}/opensync/cmake/modules ) 

INSTALL( FILES ${OPENSYNC_CMAKE_MODULES} DESTINATION ${OPENSYNC_CMAKE_DIRECTORY} )

