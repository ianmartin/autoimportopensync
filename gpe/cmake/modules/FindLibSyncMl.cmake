# - Try to find LibSyncMl
# Once done this will define
#
#  LibSyncMl_FOUND - system has LibSyncMl
#  LibSyncMl_INCLUDE_DIRS - the LibSyncMl include directory
#  LibSyncMl_LIBRARIES - Link these to use LibSyncMl
#  LibSyncMl_DEFINITIONS - Compiler switches required for using LibSyncMl
#  LibSyncMl_LINK_FLAGS - Link flags
#
#  Copyright (c) 2006 Andreas Schneider <mail@cynapses.org>
#  Copyright (c) 2006 Philippe Bernery <philippe.bernery@gmail.com>
#  Copytight © 2007 Juha Tuomala <tuju@iki.fi>
#
#  Redistribution and use is allowed according to the terms of the New
#  BSD license. For details see the accompanying COPYING-CMAKE-SCRIPTS file.


if (LibSyncMl_LIBRARIES AND LibSyncMl_INCLUDE_DIRS AND LibSyncMl_PUBLIC_LINK_FLAGS)
	# in cache already
	set(LibSyncMl_FOUND TRUE)
else (LibSyncMl_LIBRARIES AND LibSyncMl_INCLUDE_DIRS AND LibSyncMl_PUBLIC_LINK_FLAGS)
	include(UsePkgConfig)
	pkgconfig(libsyncml-1.0 _LibSyncMlIncDir _LibSyncMlLinkDir _LibSyncMlLinkFlags _LibSyncMlCflags)

	set(SYNCML_DEFINITIONS ${_LibSyncMlCflags})
	set(CMAKE_C_FLAGS ${_LibSyncMlCflags})

	find_path(LibSyncMl_INCLUDE_DIR
		NAMES
			syncml.h
			http_client.h
			http_server.h
			obex_client.h
			obex_server.h
			sml_auth.h
			sml_base64.h
			sml_command.h
			sml_defines.h
			sml_devinf.h
			sml_devinf_obj.h
			sml_ds_server.h
			sml_elements.h
			sml_error.h
			sml_manager.h
			sml_md5.h
			sml_notification.h
			sml_parse.h
			sml_session.h
			sml_transport.h

		PATHS
			${_LibSyncMlIncDir}
			/opt/local/include/libsyncml-1.0
			/sw/include/libsyncml-1.0
			/usr/local/include/libsyncml-1.0
			/usr/include/libsyncml-1.0
			/usr/include/libsyncml-1.0/libsyncml
	)


	find_library(LibSyncMl_LIBRARY
		NAMES
			syncml

		PATHS
			${_LibSyncMlLinkDir}
			/opt/local/lib
			/sw/lib
			/usr/lib
			/usr/local/lib
			/usr/lib64
			/usr/local/lib64
			/opt/lib64
	)


	if (LibSyncMl_LIBRARY AND LibSyncMl_INCLUDE_DIR)
		set(LibSyncMl_FOUND TRUE)
	endif (LibSyncMl_LIBRARY AND LibSyncMl_INCLUDE_DIR)

	set(SYNCML_INCLUDE_DIR ${LibSyncMl_INCLUDE_DIR})
	set(SYNCML_LIBRARIES ${LibSyncMl_LIBRARY} )
	set(SYNCML_PUBLIC_LINK_FLAGS ${_LibSyncMlLinkFlags} )

	if (LibSyncMl_INCLUDE_DIRS AND LibSyncMl_LIBRARIES)
		set(LibSyncMl_FOUND TRUE)
	endif (LibSyncMl_INCLUDE_DIRS AND LibSyncMl_LIBRARIES)

	if (LibSyncMl_FOUND)
		if (NOT LibSyncMl_FIND_QUIETLY)
			message(STATUS "Found LibSyncMl: ${LibSyncMl_LIBRARIES}")
		endif (NOT LibSyncMl_FIND_QUIETLY)
	else (LibSyncMl_FOUND)
		if (LibSyncMl_FIND_REQUIRED)
			message(FATAL_ERROR "Could not find LibSyncMl")
		endif (LibSyncMl_FIND_REQUIRED)
	endif (LibSyncMl_FOUND)

	# show the LibSyncMl_INCLUDE_DIRS and LibSyncMl_LIBRARIES variables only in the advanced view
	mark_as_advanced(LibSyncMl_INCLUDE_DIRS LibSyncMl_LIBRARIES LibSyncMl_PUBLIC_LINK_FLAGS)

endif (LibSyncMl_LIBRARIES AND LibSyncMl_INCLUDE_DIRS AND LibSyncMl_PUBLIC_LINK_FLAGS)

# vim: ts=4 sw=4
