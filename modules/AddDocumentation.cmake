# - InstallDocumentation.cmake
# Installs different types of documentation
#
# Copyright (c) 2008 Michael Bell  <michael.bell@web.de>
#
#  Redistribution and use is allowed according to the terms of the New
#  BSD license.
#  For details see the accompanying COPYING-CMAKE-SCRIPTS file.
#

INCLUDE( Documentation )

SET( SHARE_INSTALL_DIR    "${CMAKE_INSTALL_PREFIX}/share" CACHE INTERNAL "share location" )
SET( DOC_MAN_INSTALL_DIR  "${SHARE_INSTALL_DIR}/man" CACHE INTERNAL "man page location" )
SET( DOC_INSTALL_DIR      "${SHARE_INSTALL_DIR}/doc/${PROJECT_NAME}" CACHE INTERNAL "documentation location" )
SET( DOC_TEXT_INSTALL_DIR "${DOC_INSTALL_DIR}" CACHE INTERNAL "text documentation location" )
SET( DOC_HTML_INSTALL_DIR "${DOC_INSTALL_DIR}/html" CACHE INTERNAL "HTML documentation location" )

MACRO( ADD_DOCUMENTATION _formatName _fileType )

	# _formatName - HTML, TEXT, MAN
	# _fileType   - DIRECTORY, FILE
	# ARGN        - filenames

	# check that the documentation is built

	IF( NOT BUILD_DOCUMENTATION )
		MESSAGE( SEND_ERROR "ADD_DOCUMENTATION requires to build the documentation first." )
		RETURN()
	ENDIF( NOT BUILD_DOCUMENTATION )

	# check _formatName

	STRING( TOUPPER ${_formatName} FORMAT_NAME )
	STRING( COMPARE EQUAL ${FORMAT_NAME} "HTML" FORMAT_IS_HTML )
	STRING( COMPARE EQUAL ${FORMAT_NAME} "TEXT" FORMAT_IS_TEXT )
	STRING( COMPARE EQUAL ${FORMAT_NAME} "MAN" FORMAT_IS_MAN )
	IF( NOT ${FORMAT_IS_HTML} AND NOT ${FORMAT_IS_TEXT} AND NOT ${FORMAT_IS_MAN} )
		MESSAGE( SEND_ERROR "ADD_DOCUMENTATION only support HTML, TEXT and MAN as formats." )
		RETURN()
	ENDIF( NOT ${FORMAT_IS_HTML} AND NOT ${FORMAT_IS_TEXT} AND NOT ${FORMAT_IS_MAN} )

	# check _fileType

	STRING( TOUPPER ${_fileType} FILE_TYPE )
	STRING( COMPARE EQUAL ${FILE_TYPE} "DIRECTORY" FILE_IS_DIRECTORY )
	STRING( COMPARE EQUAL ${FILE_TYPE} "FILE" FILE_IS_FILE )
	IF( NOT ${FILE_IS_DIRECTORY} AND NOT ${FILE_IS_FILE} )
		MESSAGE( SEND_ERROR "ADD_DOCUMENTATION only support DIRECTORY and FILE as file types." )
		RETURN()
	ENDIF( NOT ${FILE_IS_DIRECTORY} AND NOT ${FILE_IS_FILE} )

	# install HTML documenation

	IF( ${FORMAT_IS_HTML} )
		IF( ${FILE_IS_DIRECTORY} )
			INSTALL( DIRECTORY ${ARGN} DESTINATION ${DOC_HTML_INSTALL_DIR} )
		ELSE( ${FILE_IS_DIRECTORY} )
			INSTALL( FILES ${ARGN} DESTINATION ${DOC_HTML_INSTALL_DIR} )
		ENDIF( ${FILE_IS_DIRECTORY} )
	ENDIF( ${FORMAT_IS_HTML} )

	# install TEXT documenation

	IF( ${FORMAT_IS_TEXT} )
		IF( ${FILE_IS_DIRECTORY} )
			INSTALL( DIRECTORY ${ARGN} DESTINATION ${DOC_TEXT_INSTALL_DIR} )
		ELSE( ${FILE_IS_DIRECTORY} )
			INSTALL( FILES ${ARGN} DESTINATION ${DOC_TEXT_INSTALL_DIR} )
		ENDIF( ${FILE_IS_DIRECTORY} )
	ENDIF( ${FORMAT_IS_TEXT} )

	# install man pages

	IF( ${FORMAT_IS_MAN} )
		IF( ${FILE_IS_DIRECTORY} )
			INSTALL( DIRECTORY ${ARGN} DESTINATION ${DOC_MAN_INSTALL_DIR} )
		ELSE( ${FILE_IS_DIRECTORY} )
			INSTALL( FILES ${ARGN} DESTINATION ${DOC_MAN_INSTALL_DIR} )
		ENDIF( ${FILE_IS_DIRECTORY} )
	ENDIF( ${FORMAT_IS_MAN} )

ENDMACRO( ADD_DOCUMENTATION _formatName _fileType )
