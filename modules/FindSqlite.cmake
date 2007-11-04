# - Try to find the sqlite library
# Once done this will define
#
#  SQLITE_FOUND - system has sqlite
#  SQLITE_INCLUDE_DIR - the sqlite include directory
#  SQLITE_LIBRARIES - Link these to use sqlite
#  SQLITE_DEFINITIONS - Compiler switches required for using sqlite
#
	
IF ( SQLITE_INCLUDE_DIR AND SQLITE_LIBRARIES )
    SET( Sqlite_FIND_QUIETLY TRUE )
ENDIF ( SQLITE_INCLUDE_DIR AND SQLITE_LIBRARIES )

IF ( NOT WIN32 )
   INCLUDE( UsePkgConfig )
   PKGCONFIG( sqlite3 _LibSQLITEIncDir _LibSQLITELinkDir _LibSQLITELinkFlags _LibSQLITECflags )
ENDIF ( NOT WIN32 )

# Look for sqlite3 include dir and libraries
FIND_PATH( SQLITE_INCLUDE_DIR sqlite3.h PATHS ${_LibSQLITEIncDir} )

FIND_LIBRARY( SQLITE_LIBRARIES NAMES sqlite3 PATHS ${_LibSQLITELinkDir} )

IF ( SQLITE_INCLUDE_DIR AND SQLITE_LIBRARIES )
	SET( SQLITE_FOUND 1 )
	IF ( NOT Sqlite_FIND_QUIETLY )
		MESSAGE ( STATUS "Found sqlite3: ${SQLITE_LIBRARIES}" )
	ENDIF ( NOT Sqlite_FIND_QUIETLY )
ELSE ( SQLITE_INCLUDE_DIR AND SQLITE_LIBRARIES )
	IF ( Sqlite_FIND_REQUIRED )
		MESSAGE( SEND_ERROR "Could NOT find sqlite3" )
	ELSE ( Sqlite_FIND_REQUIRED )
		IF ( NOT Sqlite_FIND_QUIETLY )
			MESSAGE( STATUS "Could NOT find sqlite3" )	
		ENDIF ( NOT Sqlite_FIND_QUIETLY )
	ENDIF ( Sqlite_FIND_REQUIRED )
ENDIF ( SQLITE_INCLUDE_DIR AND SQLITE_LIBRARIES )

# Hide advanced variables from CMake GUIs
MARK_AS_ADVANCED( SQLITE_INCLUDE_DIR SQLITE_LIBRARIES )

