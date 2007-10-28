PROJECT( OpenSync )

SET( OPENSYNC_VERSION_MAJOR "0" )
SET( OPENSYNC_VERSION_MINOR "33" )
SET( OPENSYNC_VERSION "${OPENSYNC_VERSION_MAJOR}.${OPENSYNC_VERSION_MINOR}" )	

SET( OPENSYNC_PLUGINVERSION 1 )

################################################	

SET( CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake ${CMAKE_SOURCE_DIR}/cmake/modules )

INCLUDE( Documentation )
INCLUDE( OpenSyncDefaults )

INCLUDE_DIRECTORIES( ${CMAKE_CURRENT_BINARY_DIR} "${CMAKE_CURRENT_SOURCE_DIR}/opensync" ${CMAKE_CURRENT_SOURCE_DIR} )

###############################################

FIND_PACKAGE( GLIB2 REQUIRED )
FIND_PACKAGE( Sqlite REQUIRED )
FIND_PACKAGE( LibXml2 REQUIRED )
FIND_PACKAGE( SWIG )	
FIND_PACKAGE( PythonLibs )
FIND_PACKAGE( Check )

ADD_SUBDIRECTORY( opensync )
ADD_SUBDIRECTORY( formats )
ADD_SUBDIRECTORY( tools )
ADD_SUBDIRECTORY( misc )
ADD_SUBDIRECTORY( cmake )

IF ( SWIG_FOUND )
	ADD_SUBDIRECTORY( wrapper )
ENDIF ( SWIG_FOUND )	

IF ( CHECK_FOUND )
	ADD_SUBDIRECTORY( tests )
ENDIF ( CHECK_FOUND )

############################################## 

CONFIGURE_FILE( "config.h.cmake" "config.h" )
CONFIGURE_FILE( "opensync-1.0.pc.in" "opensync-1.0.pc" )

IF ( BUILD_DOCUMENTATION)
	IF ( DOXYGEN_DOT_EXECUTABLE )
		SET( HAVE_DOT "YES" )
	ENDIF ( DOXYGEN_DOT_EXECUTABLE )
	CONFIGURE_FILE( "Doxyfile.in" "Doxyfile" )
	ADD_CUSTOM_TARGET( DoxygenDoc ${DOXYGEN_EXECUTABLE} )
ENDIF ( BUILD_DOCUMENTATION )

########## INSTALL ##############################
	
INSTALL( FILES "${CMAKE_CURRENT_BINARY_DIR}/opensync-1.0.pc" DESTINATION ${LIB_INSTALL_DIR}/pkgconfig/ )	

### CPack ########################################

SET( CPACK_GENERATOR "TGZ;TBZ2" ) # This line is need for a CMake (Version 2.4.7) Bug - Fixed in CVS
SET( CPACK_SOURCE_GENERATOR "TGZ;TBZ2") 
SET( CPACK_SOURCE_PACKAGE_FILE_NAME "libopensync-${OPENSYNC_VERSION}" )
INCLUDE( CPack )

