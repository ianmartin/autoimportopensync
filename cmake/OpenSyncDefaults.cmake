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

SET( OPENSYNC_PLUGINDIR "${LIB_INSTALL_DIR}/opensync/plugins" CACHE PATH "OpenSync plugin directory" )
SET( OPENSYNC_FORMATSDIR "${LIB_INSTALL_DIR}/opensync/formats" CACHE PATH "OpenSync format plugin directory" )

SET( OPENSYNC_CAPABILITIESDIR "${SHARE_INSTALL_DIR}/opensync/capabilities" CACHE PATH "OpenSync capabilities directory" )
SET( OPENSYNC_CONFIGDIR "${SHARE_INSTALL_DIR}/opensync/defaults" CACHE PATH "OpenSync plugin configuration directory" )
SET( OPENSYNC_DESCRIPTIONSDIR "${SHARE_INSTALL_DIR}/opensync/descriptions" CACHE PATH "OpenSync descriptions directory" )
SET( OPENSYNC_SCHEMASDIR "${SHARE_INSTALL_DIR}/opensync/schemas" CACHE PATH "OpenSync schemas directory" )

SET( OPENSYNC_INCLUDE_DIR "${INCLUDE_INSTALL_DIR}/opensync-1.0/" CACHE PATH "OpenSync headers location" )

# OpenSync build options:

IF ( NOT CMAKE_BUILD_TYPE )
	SET( CMAKE_BUILD_TYPE RelWithDebInfo )
ENDIF ( NOT CMAKE_BUILD_TYPE )	

SET( OPENSYNC_TRACE TRUE CACHE BOOL "Debugging/Trace output of OpenSync" )
SET( OPENSYNC_DEBUG_MODULES FALSE CACHE BOOL "Debugging modules. Avhoid unload of modules." )
SET( OPENSYNC_UNITTESTS FALSE CACHE BOOL "Build OpenSync unit tests." )

