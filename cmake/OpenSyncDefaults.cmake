# - OpenSync Internal Definitions and Macros 
# Set OpenSync helper macros to build Plugins and internal definitions
# for OpenSync data directories like the locations of capabilities files,
# plugin configuration, ...
#
#  OPENSYNC_PLUGINDIR           Location of OpenSync plugins
#  OPENSYNC_FORMATSDIR          Location of OpenSync format plugins 
#  OPENSYNC_CAPABILITIESDIR     Location of OpenSync capabilities files
#  OPENSYNC_CONFIGDIR           Location of OpenSync plugin default configurations/templates
#  OPENSYNC_DESCRIPTIONSDIR     Location of OpenSync descriptions files
#  OPENSYNC_SCHEMASDIR          Location of OpenSync related schema files
# 
#  OPENSYNC_INCLUDE_DIR         Location of OpenSync headers
#  OPENSYNC_DATA_DIR            Location of OpenSync data directory 
# 
#  OPENSYNC_TRACE               True if tracing is enabled (debugging with env. var. OSYNC_TRACE)
#  OPENSYNC_DEBUG_MODULES       True if modules shouldn't get unloaded by OpenSync, to keep symbols of plugins
#  OPENSYNC_UNITTESTS           True if unit tests should be build
#
# Copyright (c) 2007 Daniel Gollub <dgollub@suse.de>
#

INCLUDE( OpenSyncPlatforms )

# OpenSync macros and default settings:

# Installation directories:

SET( OPENSYNC_API_DIR "opensync-1.0" )
SET( OPENSYNC_PLUGINDIR "${LIB_INSTALL_DIR}/${OPENSYNC_API_DIR}/plugins" CACHE PATH "OpenSync plugin directory" )
SET( OPENSYNC_FORMATSDIR "${LIB_INSTALL_DIR}/${OPENSYNC_API_DIR}/formats" CACHE PATH "OpenSync format plugin directory" )
SET( OPENSYNC_PYTHON_PLUGINDIR "${LIB_INSTALL_DIR}/${OPENSYNC_API_DIR}/python-plugins" CACHE PATH "OpenSync python plugin directory" )

SET( OPENSYNC_CAPABILITIESDIR "${SHARE_INSTALL_DIR}/${OPENSYNC_API_DIR}/capabilities" CACHE PATH "OpenSync capabilities directory" )
SET( OPENSYNC_CONFIGDIR "${SHARE_INSTALL_DIR}/${OPENSYNC_API_DIR}/defaults" CACHE PATH "OpenSync plugin configuration directory" )
SET( OPENSYNC_DESCRIPTIONSDIR "${SHARE_INSTALL_DIR}/${OPENSYNC_API_DIR}/descriptions" CACHE PATH "OpenSync descriptions directory" )
SET( OPENSYNC_SCHEMASDIR "${SHARE_INSTALL_DIR}/${OPENSYNC_API_DIR}/schemas" CACHE PATH "OpenSync schemas directory" )

SET( OPENSYNC_LIBRARIES_DIR "${LIB_INSTALL_DIR}" CACHE PATH "OpenSync library location" )
SET( OPENSYNC_LIBEXEC_DIR "${LIBEXEC_INSTALL_DIR}/${OPENSYNC_API_DIR}" CACHE PATH "OpenSync libexec location" )
SET( OPENSYNC_INCLUDE_DIR "${INCLUDE_INSTALL_DIR}/${OPENSYNC_API_DIR}" CACHE PATH "OpenSync headers location" )
SET( OPENSYNC_DATA_DIR "${SHARE_INSTALL_DIR}/${OPENSYNC_API_DIR}" CACHE PATH "OpenSync data directory" )

# OpenSync build options:

IF ( NOT CMAKE_BUILD_TYPE )
	SET( CMAKE_BUILD_TYPE RelWithDebInfo )
ENDIF ( NOT CMAKE_BUILD_TYPE )	

SET( OPENSYNC_TRACE TRUE CACHE BOOL "Debugging/Trace output of OpenSync" )
SET( OPENSYNC_DEBUG_MODULES FALSE CACHE BOOL "Debugging modules. Avhoid unload of modules." )
SET( OPENSYNC_UNITTESTS FALSE CACHE BOOL "Build OpenSync unit tests." )
SET( OPENSYNC_PYTHONBINDINGS TRUE CACHE BOOL "Build OpenSync with Python bindings." )


