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
#  Copyright (c) 2008 Michael Bell <michael.bell@web.de>
#
#  Redistribution and use is allowed according to the terms of the New
#  BSD license.
#  For details see the accompanying COPYING-CMAKE-SCRIPTS file.
#


IF (GLIB2_LIBRARIES AND GLIB2_INCLUDE_DIRS )
  # in cache already
  SET(GLIB2_FOUND TRUE)
ELSE (GLIB2_LIBRARIES AND GLIB2_INCLUDE_DIRS )

  INCLUDE(FindPkgConfig)

  ## Glib
  IF ( GLIB2_FIND_REQUIRED )
    SET( _pkgconfig_REQUIRED "REQUIRED" )
  ELSE ( GLIB2_FIND_REQUIRED )
    SET( _pkgconfig_REQUIRED "" )
  ENDIF ( GLIB2_FIND_REQUIRED )

  IF ( GLIB2_MIN_VERSION )
    PKG_SEARCH_MODULE( GLIB2 ${_pkgconfig_REQUIRED} glib-2.0>=${GLIB2_MIN_VERSION} )
  ELSE ( GLIB2_MIN_VERSION )
    PKG_SEARCH_MODULE( GLIB2 ${_pkgconfig_REQUIRED} glib-2.0 )
  ENDIF ( GLIB2_MIN_VERSION )
  IF ( PKG_CONFIG_FOUND )
    IF ( GLIB2_FOUND )
      SET ( GLIB2_CORE_FOUND TRUE )
    ELSE ( GLIB2_FOUND )
      SET ( GLIB2_CORE_FOUND FALSE )
    ENDIF ( GLIB2_FOUND )
  ENDIF ( PKG_CONFIG_FOUND )

  # Look for glib2 include dir and libraries w/o pkgconfig
  IF ( NOT GLIB2_FOUND AND NOT PKG_CONFIG_FOUND )
    FIND_PATH(_glibconfig_include_DIR
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

    FIND_PATH(_glib2_include_DIR
      NAMES
	glib.h
      PATHS
	/opt/gnome/include/glib-2.0
	/opt/local/include/glib-2.0
	/sw/include/glib-2.0
	/usr/include/glib-2.0
	/usr/local/include/glib-2.0
    )

    FIND_LIBRARY( _glib2_link_DIR
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
	    SET ( _glib2_FOUND TRUE )
    ENDIF ( _glib2_include_DIR AND _glib2_link_DIR )


    IF ( _glib2_FOUND )
	    SET ( GLIB2_INCLUDE_DIRS ${_glib2_include_DIR} ${_glibconfig_include_DIR} )
	    SET ( GLIB2_LIBRARIES ${_glib2_link_DIR} )
	    SET ( GLIB2_CORE_FOUND TRUE )
    ELSE ( _glib2_FOUND )
	    SET ( GLIB2_CORE_FOUND FALSE )
    ENDIF ( _glib2_FOUND )

    # Handle dependencies
    # libintl
    IF ( NOT LIBINTL_FOUND )
      FIND_PATH(LIBINTL_INCLUDE_DIR
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

      FIND_LIBRARY(LIBINTL_LIBRARY
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

      IF (LIBINTL_LIBRARY AND LIBINTL_INCLUDE_DIR)
	SET (LIBINTL_FOUND TRUE)
      ENDIF (LIBINTL_LIBRARY AND LIBINTL_INCLUDE_DIR)
    ENDIF ( NOT LIBINTL_FOUND )

    # libiconv
    IF ( NOT LIBICONV_FOUND )
      FIND_PATH(LIBICONV_INCLUDE_DIR
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

      FIND_LIBRARY(LIBICONV_LIBRARY
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

      IF (LIBICONV_LIBRARY AND LIBICONV_INCLUDE_DIR)
	SET (LIBICONV_FOUND TRUE)
      ENDIF (LIBICONV_LIBRARY AND LIBICONV_INCLUDE_DIR)
    ENDIF ( NOT LIBICONV_FOUND )

    IF (LIBINTL_FOUND)
      SET (GLIB2_LIBRARIES ${GLIB2_LIBRARIES} ${LIBINTL_LIBRARY})
      SET (GLIB2_INCLUDE_DIRS ${GLIB2_INCLUDE_DIRS} ${LIBINTL_INCLUDE_DIR})
    ENDIF (LIBINTL_FOUND)

    IF (LIBICONV_FOUND)
      SET (GLIB2_LIBRARIES ${GLIB2_LIBRARIES} ${LIBICONV_LIBRARY})
      SET (GLIB2_INCLUDE_DIRS ${GLIB2_INCLUDE_DIRS} ${LIBICONV_INCLUDE_DIR})
    ENDIF (LIBICONV_FOUND)

  ENDIF ( NOT GLIB2_FOUND AND NOT PKG_CONFIG_FOUND )
  ##

  ## GModule
  PKG_SEARCH_MODULE(GMODULE2 gmodule-2.0 )

  # Look for gmodule2 include dir and libraries w/o pkgconfig
  IF ( NOT GMODULE2_FOUND AND NOT PKG_CONFIG_FOUND )
    FIND_PATH( GMODULE2_INCLUDE_DIR
      NAMES
	gmodule.h
      PATHS
	/opt/gnome/include/glib-2.0
	/opt/local/include/glib-2.0
	/sw/include/glib-2.0
	/usr/include/glib-2.0
	/usr/local/include/glib-2.0
    )

    FIND_LIBRARY( GMODULE2_LIBRARY
      NAMES
	gmodule-2.0
      PATHS
	/opt/gnome/lib
	/opt/local/lib
	/sw/lib
	/usr/lib
	/usr/local/lib
    )

    IF (GMODULE2_LIBRARY AND GMODULE2_INCLUDE_DIR)
      SET (GMODULE2_FOUND TRUE)
    ENDIF (GMODULE2_LIBRARY AND GMODULE2_INCLUDE_DIR)

    IF (GMODULE2_FOUND)
      SET (GMODULE2_INCLUDE_DIRS ${GMODULE2_INCLUDE_DIR})
      SET (GMODULE2_LIBRARIES ${GMODULE2_LIBRARY})
    ENDIF (GMODULE2_FOUND)


    # Handle dependencies
    # FIXME : glib2 - requires a split in different cmake modules and the user code to call the proper module instead of FindGLIB2 directly

  ENDIF ( NOT GMODULE2_FOUND AND NOT PKG_CONFIG_FOUND )
  ##

  ## GThread
  PKG_SEARCH_MODULE( GTHREAD2 gthread-2.0 )

  # Look for gthread2 include dir and libraries w/o pkgconfig
  IF ( NOT GTHREAD2_FOUND AND NOT PKG_CONFIG_FOUND )
    FIND_PATH(GTHREAD2_INCLUDE_DIR
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

    FIND_LIBRARY(GTHREAD2_LIBRARY
      NAMES
	gthread-2.0
      PATHS
	/opt/gnome/lib
	/opt/local/lib
	/sw/lib
	/usr/lib
	/usr/local/lib
    )

    IF (GTHREAD2_LIBRARY AND GTHREAD2_INCLUDE_DIR)
      SET (GTHREAD2_FOUND TRUE)
    ENDIF (GTHREAD2_LIBRARY AND GTHREAD2_INCLUDE_DIR)

    IF (GTHREAD2_FOUND)
      SET (GTHREAD2_INCLUDE_DIRS ${GTHREAD2_INCLUDE_DIR})
      SET (GTHREAD2_LIBRARIES ${GTHREAD2_LIBRARY})
    ENDIF (GTHREAD2_FOUND)

    # Handle dependencies
    # FIXME : glib2 - requires a split in different cmake modules and the user code to call the proper module instead of FindGLIB2 directly

  ENDIF ( NOT GTHREAD2_FOUND AND NOT PKG_CONFIG_FOUND )
  ##

  ## GObject
  PKG_SEARCH_MODULE( GOBJECT2 gobject-2.0 )

  # Look for gobject2 include dir and libraries w/o pkgconfig
  IF ( NOT GOBJECT2_FOUND AND NOT PKG_CONFIG_FOUND )
    FIND_PATH(GOBJECT2_INCLUDE_DIR
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

    FIND_LIBRARY(GOBJECT2_LIBRARY
      NAMES
	gobject-2.0
      PATHS
	/opt/gnome/lib
	/opt/local/lib
	/sw/lib
	/usr/lib
	/usr/local/lib
    )

    IF (GOBJECT2_LIBRARY AND GOBJECT2_INCLUDE_DIR)
      SET (GOBJECT2_FOUND TRUE)
    ENDIF (GOBJECT2_LIBRARY AND GOBJECT2_INCLUDE_DIR)

    IF (GOBJECT2_FOUND)
      SET (GOBJECT2_INCLUDE_DIRS ${GOBJECT2_INCLUDE_DIR})
      SET (GOBJECT2_LIBRARIES ${GOBJECT2_LIBRARY})
    ENDIF (GOBJECT2_FOUND)

    # Handle dependencies
    # FIXME : glib2 - requires a split in different cmake modules and the user code to call the proper module instead of FindGLIB2 directly

  ENDIF ( NOT GOBJECT2_FOUND AND NOT PKG_CONFIG_FOUND )
  ##


  IF (GMODULE2_FOUND)
    SET (GLIB2_LIBRARIES ${GLIB2_LIBRARIES} ${GMODULE2_LIBRARIES})
    SET (GLIB2_INCLUDE_DIRS ${GLIB2_INCLUDE_DIRS} ${GMODULE2_INCLUDE_DIRS})
  ENDIF (GMODULE2_FOUND)

  IF (GTHREAD2_FOUND)
    SET (GLIB2_LIBRARIES ${GLIB2_LIBRARIES} ${GTHREAD2_LIBRARIES})
    SET (GLIB2_INCLUDE_DIRS ${GLIB2_INCLUDE_DIRS} ${GTHREAD2_INCLUDE_DIRS})
  ENDIF (GTHREAD2_FOUND)

  IF (GOBJECT2_FOUND)
    SET (GLIB2_LIBRARIES ${GLIB2_LIBRARIES} ${GOBJECT2_LIBRARY})
    SET (GLIB2_INCLUDE_DIRS ${GLIB2_INCLUDE_DIRS} ${GOBJECT2_INCLUDE_DIR})
  ENDIF (GOBJECT2_FOUND)

  IF (GLIB2_CORE_FOUND AND GLIB2_INCLUDE_DIRS AND GLIB2_LIBRARIES)
    SET (GLIB2_FOUND TRUE)
  ENDIF (GLIB2_CORE_FOUND AND GLIB2_INCLUDE_DIRS AND GLIB2_LIBRARIES)

  IF (GLIB2_FOUND)
    IF (NOT GLIB2_FIND_QUIETLY)
      MESSAGE (STATUS "Found GLib2: ${GLIB2_LIBRARIES}")
    ENDIF (NOT GLIB2_FIND_QUIETLY)
  ELSE (GLIB2_FOUND)
    IF (GLIB2_FIND_REQUIRED)
      MESSAGE (SEND_ERROR "Could not find GLib2")
    ENDIF (GLIB2_FIND_REQUIRED)
  ENDIF (GLIB2_FOUND)

  # show the GLIB2_INCLUDE_DIRS and GLIB2_LIBRARIES variables only in the advanced view
  MARK_AS_ADVANCED(GLIB2_INCLUDE_DIRS GLIB2_LIBRARIES)
  MARK_AS_ADVANCED(LIBICONV_INCLUDE_DIR LIBICONV_LIBRARY)
  MARK_AS_ADVANCED(LIBINTL_INCLUDE_DIR LIBINTL_LIBRARY)

  # same for all other variables
  MARK_AS_ADVANCED(GMODULE2_INCLUDE_DIR GMODULE2_LIBRARY)
  MARK_AS_ADVANCED(GMODULE2_INCLUDE_DIRS GMODULE2_LIBRARIES)
  MARK_AS_ADVANCED(GOBJECT2_INCLUDE_DIR GOBJECT2_LIBRARY)
  MARK_AS_ADVANCED(GOBJECT2_INCLUDE_DIRS GOBJECT2_LIBRARIES)
  MARK_AS_ADVANCED(GTHREAD2_INCLUDE_DIR GTHREAD2_LIBRARY)
  MARK_AS_ADVANCED(GTHREAD2_INCLUDE_DIRS GTHREAD2_LIBRARIES)

ENDIF (GLIB2_LIBRARIES AND GLIB2_INCLUDE_DIRS)
