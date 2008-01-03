LINK_DIRECTORIES( ${OPENSYNC_LIBRARY_DIRS} ${GLIB2_LIBRARY_DIRS} ${LIBXML2_LIBRARY_DIRS} )
INCLUDE_DIRECTORIES( ${OPENSYNC_INCLUDE_DIRS} ${GLIB2_INCLUDE_DIRS} ${LIBXML2_INCLUDE_DIRS} )

# gcalendar source	
SET( gcalendar_LIB_SRCS
gcalendar.c
)

ADD_DEFINITIONS( -DGCAL_HELPER="\\\"${OPENSYNC_LIBEXEC_DIR}/google-cal-helper\\\"" )
OPENSYNC_PLUGIN_ADD( gcalendar ${gcalendar_LIB_SRCS} )

TARGET_LINK_LIBRARIES( gcalendar ${OPENSYNC_LIBRARIES} ${GLIB2_LIBRARIES} ${LIBXML2_LIBRARIES} )

# Install plugin and google-cal-helper	
OPENSYNC_PLUGIN_INSTALL( gcalendar )
INSTALL( PROGRAMS google-cal-helper.py DESTINATION "${OPENSYNC_LIBEXEC_DIR}" RENAME "google-cal-helper" )

# Install config template	
OPENSYNC_PLUGIN_CONFIG( google-calendar )

# Install capabilities and descriptions
OPENSYNC_PLUGIN_DESCRIPTIONS( google-calendar.xml )
OPENSYNC_PLUGIN_CAPABILITIES( google-calendar-gcal-2005.xml )

