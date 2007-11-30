# Copyright (c) 2007 Daniel Gollub <dgollub@suse.de>

# OpenSync platform macros:

SET( LIB_SUFFIX "" CACHE STRING "The library directory suffix. 32bit empty string, 64 for 64bit." )

IF (NOT WIN32)
	SET( LIB_INSTALL_DIR  "${CMAKE_INSTALL_PREFIX}/lib${LIB_SUFFIX}" )
	SET( LIBEXEC_INSTALL_DIR  "${CMAKE_INSTALL_PREFIX}/lib" )
	SET( BIN_INSTALL_DIR  "${CMAKE_INSTALL_PREFIX}/bin" )
	SET( SHARE_INSTALL_DIR  "${CMAKE_INSTALL_PREFIX}/share" )
	SET( INCLUDE_INSTALL_DIR  "${CMAKE_INSTALL_PREFIX}/include" )
ELSE (NOT WIN32)
	# Windows stuff goes here...	
	SET( LIB_INSTALL_DIR  "${CMAKE_INSTALL_PREFIX}/lib${LIB_SUFFIX}" )
	SET( LIBEXEC_INSTALL_DIR  "${CMAKE_INSTALL_PREFIX}/lib" )
	SET( BIN_INSTALL_DIR  "${CMAKE_INSTALL_PREFIX}/bin" )
	SET( SHARE_INSTALL_DIR  "${CMAKE_INSTALL_PREFIX}/share" )
	SET( INCLUDE_INSTALL_DIR  "${CMAKE_INSTALL_PREFIX}/include" )
ENDIF (NOT WIN32)

