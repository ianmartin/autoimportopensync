INCLUDE_DIRECTORIES( ${OPENSYNC_INCLUDE_DIR} ${GLIB2_INCLUDE_DIRS} )

# gcalendar source	
SET( gcalendar_LIB_SRCS
gcalendar.c
)

ADD_DEFINITIONS( -DGCAL_HELPER="\\\"${LIBEXEC_INSTALL_DIR}/google-cal-helper\\\"" )
OPENSYNC_PLUGIN_ADD( gcalendar ${gcalendar_LIB_SRCS} )

TARGET_LINK_LIBRARIES( gcalendar ${OPENYNC_LIBRARIES} ${GLIB2_LIBRARIES} )

# Install plugin and google-cal-helper	
OPENSYNC_PLUGIN_INSTALL( gcalendar )
INSTALL( FILES google-cal-helper.py DESTINATION "${LIBEXEC_INSTALL_DIR}/google-cal-helper" )

# Install config template	
OPENSYNC_PLUGIN_CONFIG( google-calendar )

# Install capabilities and descriptions
OPENSYNC_PLUGIN_DESCRIPTIONS( google-calendar.xml )
OPENSYNC_PLUGIN_CAPABILITIES( google-calendar-gcal-2005.xml )

