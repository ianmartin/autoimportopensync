# - Try to find GLib2
# Once done this will define
#
#  GLIB2_FOUND - system has GLib2
#  GLIB2_INCLUDE_DIRS - the GLib2 include directory
#  GLIB2_LIBRARIES - Link these to use GLib2
#
#  Copyright (c) 2006 Andreas Schneider <mail@cynapses.org>
#  Copyright (c) 2006 Philippe Bernery <philippe.bernery@gmail.com>
#  Copyright (c) 2007 Daniel Gollub <dgollub@suse.de>
#  Copyright (c) 2007 Alban Browaeys <prahal@yahoo.com>
#
#  Redistribution and use is allowed according to the terms of the New
#  BSD license.
#  For details see the accompanying COPYING-CMAKE-SCRIPTS file.
#


if (GLIB2_LIBRARIES AND GLIB2_INCLUDE_DIRS )
  # in cache already
  set(GLIB2_FOUND TRUE)
else (GLIB2_LIBRARIES AND GLIB2_INCLUDE_DIRS )

  include(FindPkgConfig)

  ## Glib
  IF ( GLIB2_FIND_REQUIRED )
    SET( _pkgconfig_REQUIRED "REQUIRED" )
  ELSE ( GLIB2_FIND_REQUIRED )
    SET( _pkgconfig_REQUIRED "" )
  ENDIF ( GLIB2_FIND_REQUIRED )

  pkg_search_module( GLIB2 ${_pkgconfig_REQUIRED} glib-2.0 )

  # Look for glib2 include dir and libraries w/o pkgconfig
  IF ( NOT GLIB2_FOUND AND NOT PKG_CONFIG_FOUND )
    find_path(_glibconfig_include_DIR
      NAMES
	glibconfig.h
      PATHS
	/opt/gnome/lib64/glib-2.0/include
	/opt/gnome/lib/glib-2.0/include
	/opt/lib/glib-2.0/include
	/opt/local/lib/glib-2.0/include
	/sw/lib/glib-2.0/include
	/usr/lib64/glib-2.0/include
	/usr/lib/glib-2.0/include
	/usr/local/include/glib-2.0
    )

    find_path(_glib2_include_DIR
      NAMES
	glib.h
      PATHS
	/opt/gnome/include/glib-2.0
	/opt/local/include/glib-2.0
	/sw/include/glib-2.0
	/usr/include/glib-2.0
	/usr/local/include/glib-2.0
    )

    find_library( _glib2_link_DIR
      NAMES
	glib-2.0
      PATHS
	/opt/gnome/lib
	/opt/local/lib
	/sw/lib
	/usr/lib
	/usr/local/lib
    )
    IF ( _glib2_include_DIR AND _glib2_link_DIR )
	    SET ( _gconf2_FOUND TRUE )
    ENDIF ( _glib2_include_DIR AND _glib2_link_DIR )


    IF ( _glib2_FOUND )
	    SET ( GLIB2_INCLUDE_DIRS ${_glib2_include_DIR} ${_glibconfig_include_DIR} )
	    SET ( GLIB2_LIBRARIES ${_glib2_link_DIR} )
    ENDIF ( _gconf2_FOUND )

    # Handle dependencies
    # libintl
    IF ( NOT LIBINTL_FOUND )
      find_path(LIBINTL_INCLUDE_DIR
	NAMES
	  libintl.h
	NO_DEFAULT_PATH
	PATHS
	  /opt/gnome/include
	  /opt/local/include
	  /sw/include
	  /usr/include
	  /usr/local/include
      )

      find_library(LIBINTL_LIBRARY
	NAMES
	  intl
	NO_DEFAULT_PATH
	PATHS
	  /opt/gnome/lib
	  /opt/local/lib
	  /sw/lib
	  /usr/local/lib
	  /usr/lib
      )

      if (LIBINTL_LIBRARY AND LIBINTL_INCLUDE_DIR)
	set(LIBINTL_FOUND TRUE)
      endif (LIBINTL_LIBRARY AND LIBINTL_INCLUDE_DIR)
    ENDIF ( NOT LIBINTL_FOUND )

    # libiconv
    IF ( NOT LIBICONV_FOUND )
      find_path(LIBICONV_INCLUDE_DIR
	NAMES
	  iconv.h
	NO_DEFAULT_PATH
	PATHS
	  /opt/gnome/include/glib-2.0
	  /opt/local/include/glib-2.0
	  /opt/local/include
	  /sw/include
	  /sw/include/glib-2.0
	  /usr/local/include/glib-2.0
	  /usr/include/glib-2.0
      )

      find_library(LIBICONV_LIBRARY
	NAMES
	  iconv
	NO_DEFAULT_PATH 
	PATHS
	  /opt/gnome/lib
	  /opt/local/lib
	  /sw/lib
	  /usr/lib
	  /usr/local/lib
      )

      if (LIBICONV_LIBRARY AND LIBICONV_INCLUDE_DIR)
	set(LIBICONV_FOUND TRUE)
      endif (LIBICONV_LIBRARY AND LIBICONV_INCLUDE_DIR)
    ENDIF ( NOT LIBICONV_FOUND )

    if (LIBINTL_FOUND)
      set(GLIB2_LIBRARIES ${GLIB2_LIBRARIES} ${LIBINTL_LIBRARY})
      set(GLIB2_INCLUDE_DIRS ${GLIB2_INCLUDE_DIRS} ${LIBINTL_INCLUDE_DIR})
    endif (LIBINTL_FOUND)

    if (LIBICONV_FOUND)
      set(GLIB2_LIBRARIES ${GLIB2_LIBRARIES} ${LIBICONV_LIBRARY})
      set(GLIB2_INCLUDE_DIRS ${GLIB2_INCLUDE_DIRS} ${LIBICONV_INCLUDE_DIR})
    endif (LIBICONV_FOUND)

  ENDIF ( NOT GLIB2_FOUND AND NOT PKG_CONFIG_FOUND )
  ##

  ## GModule
  pkg_search_module(GMODULE2 gmodule-2.0 )

  # Look for gmodule2 include dir and libraries w/o pkgconfig
  IF ( NOT GMODULE2_FOUND AND NOT PKG_CONFIG_FOUND )
    find_path( GMODULE2_INCLUDE_DIR
      NAMES
	gmodule.h
      PATHS
	/opt/gnome/include/glib-2.0
	/opt/local/include/glib-2.0
	/sw/include/glib-2.0
	/usr/include/glib-2.0
	/usr/local/include/glib-2.0
    )

    find_library( GMODULE2_LIBRARY
      NAMES
	gmodule-2.0
      PATHS
	/opt/gnome/lib
	/opt/local/lib
	/sw/lib
	/usr/lib
	/usr/local/lib
    )

    if (GMODULE2_LIBRARY AND GMODULE2_INCLUDE_DIR)
      set(GMODULE2_FOUND TRUE)
    endif (GMODULE2_LIBRARY AND GMODULE2_INCLUDE_DIR)

    if (GMODULE2_FOUND)
      set(GMODULE2_INCLUDE_DIRS ${GMODULE2_INCLUDE_DIR})
      set(GMODULE2_LIBRARIES ${GMODULE2_LIBRARY})
    endif (GMODULE2_FOUND)


    # Handle dependencies
    # FIXME : glib2 - requires a split in different cmake modules and the user code to call the proper module instead of FindGLIB2 directly

  ENDIF ( NOT GMODULE2_FOUND AND NOT PKG_CONFIG_FOUND )
  ##

  ## GThread
  pkg_search_module( GTHREAD2 gthread-2.0)

  # Look for gthread2 include dir and libraries w/o pkgconfig
  IF ( NOT GTHREAD2_FOUND AND NOT PKG_CONFIG_FOUND )
    find_path(GTHREAD2_INCLUDE_DIR
      NAMES
	gthread.h
      PATHS
	/opt/gnome/include/glib-2.0
	/opt/local/include/glib-2.0
	/sw/include/glib-2.0
	/usr/include/glib-2.0
	/usr/local/include/glib-2.0
      PATH_SUFFIXES
	glib
    )

    find_library(GTHREAD2_LIBRARY
      NAMES
	gthread-2.0
      PATHS
	/opt/gnome/lib
	/opt/local/lib
	/sw/lib
	/usr/lib
	/usr/local/lib
    )

    if (GTHREAD2_LIBRARY AND GTHREAD2_INCLUDE_DIR)
      set(GTHREAD2_FOUND TRUE)
    endif (GTHREAD2_LIBRARY AND GTHREAD2_INCLUDE_DIR)

    if (GTHREAD2_FOUND)
      set(GTHREAD2_INCLUDE_DIRS ${GTHREAD2_INCLUDE_DIR})
      set(GTHREAD2_LIBRARIES ${GTHREAD2_LIBRARY})
    endif (GTHREAD2_FOUND)

    # Handle dependencies
    # FIXME : glib2 - requires a split in different cmake modules and the user code to call the proper module instead of FindGLIB2 directly

  ENDIF ( NOT GTHREAD2_FOUND AND NOT PKG_CONFIG_FOUND )
  ##

  ## GObject
  pkg_search_module( GOBJECT2 gobject-2.0)

  # Look for gmodule2 include dir and libraries w/o pkgconfig
  IF ( NOT GOBJECT2_FOUND AND NOT PKG_CONFIG_FOUND )
    find_path(GOBJECT2_INCLUDE_DIR
      NAMES
	gobject.h
      PATHS
	/opt/gnome/include/glib-2.0
	/opt/local/include/glib-2.0
	/sw/include/glib-2.0
	/usr/include/glib-2.0
	/usr/local/include/glib-2.0
      PATH_SUFFIXES
	gobject
    )

    find_library(GOBJECT2_LIBRARY
      NAMES
	gobject-2.0
      PATHS
	/opt/gnome/lib
	/opt/local/lib
	/sw/lib
	/usr/lib
	/usr/local/lib
    )

    if (GOBJECT2_LIBRARY AND GOBJECT2_INCLUDE_DIR)
      set(GOBJECT2_FOUND TRUE)
    endif (GOBJECT2_LIBRARY AND GOBJECT2_INCLUDE_DIR)

    if (GOBJECT2_FOUND)
      set(GOBJECT2_INCLUDE_DIRS ${GOBJECT2_INCLUDE_DIR})
      set(GOBJECT2_LIBRARIES ${GOBJECT2_LIBRARY})
    endif (GOBJECT2_FOUND)

    # Handle dependencies
    # FIXME : glib2 - requires a split in different cmake modules and the user code to call the proper module instead of FindGLIB2 directly

  ENDIF ( NOT GOBJECT2_FOUND AND NOT PKG_CONFIG_FOUND )
  ##






  if (GMODULE2_FOUND)
    set(GLIB2_LIBRARIES ${GLIB2_LIBRARIES} ${GMODULE2_LIBRARIES})
    set(GLIB2_INCLUDE_DIRS ${GLIB2_INCLUDE_DIRS} ${GMODULE2_INCLUDE_DIRS})
  endif (GMODULE2_FOUND)

  if (GTHREAD2_FOUND)
    set(GLIB2_LIBRARIES ${GLIB2_LIBRARIES} ${GTHREAD2_LIBRARIES})
    set(GLIB2_INCLUDE_DIRS ${GLIB2_INCLUDE_DIRS} ${GTHREAD2_INCLUDE_DIRS})
  endif (GTHREAD2_FOUND)

  if (GOBJECT2_FOUND)
    set(GLIB2_LIBRARIES ${GLIB2_LIBRARIES} ${GOBJECT2_LIBRARY})
    set(GLIB2_INCLUDE_DIRS ${GLIB2_INCLUDE_DIRS} ${GOBJECT2_INCLUDE_DIR})
  endif (GOBJECT2_FOUND)

  if (GLIB2_INCLUDE_DIRS AND GLIB2_LIBRARIES)
    set(GLIB2_FOUND TRUE)
  endif (GLIB2_INCLUDE_DIRS AND GLIB2_LIBRARIES)

    if (GLIB2_FOUND)
      if (NOT GLIB2_FIND_QUIETLY)
	message(STATUS "Found GLib2: ${GLIB2_LIBRARIES}")
      endif (NOT GLIB2_FIND_QUIETLY)
    else (GLIB2_FOUND)
      if (GLIB2_FIND_REQUIRED)
	message(SEND_ERROR "Could not find GLib2")
      endif (GLIB2_FIND_REQUIRED)
    endif (GLIB2_FOUND)

  # show the GLIB2_INCLUDE_DIRS and GLIB2_LIBRARIES variables only in the advanced view
  mark_as_advanced(GLIB2_INCLUDE_DIRS GLIB2_LIBRARIES)
  mark_as_advanced(LIBICONV_INCLUDE_DIR LIBICONV_LIBRARY)
  mark_as_advanced(LIBINTL_INCLUDE_DIR LIBINTL_LIBRARY)

  # same for all other variables
  mark_as_advanced(GMODULE2_INCLUDE_DIR GMODULE2_LIBRARY)
  mark_as_advanced(GMODULE2_INCLUDE_DIRS GMODULE2_LIBRARIES)
  mark_as_advanced(GOBJECT2_INCLUDE_DIR GOBJECT2_LIBRARY)
  mark_as_advanced(GOBJECT2_INCLUDE_DIRS GOBJECT2_LIBRARIES)
  mark_as_advanced(GTHREAD2_INCLUDE_DIR GTHREAD2_LIBRARY)
  mark_as_advanced(GTHREAD2_INCLUDE_DIRS GTHREAD2_LIBRARIES)

endif (GLIB2_LIBRARIES AND GLIB2_INCLUDE_DIRS)
