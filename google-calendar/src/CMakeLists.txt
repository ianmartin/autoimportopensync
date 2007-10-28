INCLUDE_DIRECTORIES( ${OPENSYNC_INCLUDE_DIR} ${GLIB2_INCLUDE_DIRS} )

# gcalendar source	
SET( gcalendar_LIB_SRCS
gcalendar.c
)

ADD_DEFINITIONS( -DGCAL_HELPER="\\\"${LIBEXEC_INSTALL_DIR}/google-cal-helper\\\"" )
ADD_LIBRARY( gcalendar SHARED ${gcalendar_LIB_SRCS} )

TARGET_LINK_LIBRARIES( gcalendar ${OPENYNC_LIBRARIES} ${GLIB2_LIBRARIES} )

# Install plugin and google-cal-helper	
INSTALL( TARGETS gcalendar DESTINATION ${OPENSYNC_PLUGINDIR} )
INSTALL( FILES google-cal-helper.py DESTINATION "${LIBEXEC_INSTALL_DIR}/google-cal-helper" )

# Install config template	
INSTALL( FILES google-calendar DESTINATION ${OPENSYNC_CONFIGDIR} )

# Install capabilities and descriptions
INSTALL( FILES google-calendar.xml DESTINATION ${OPENSYNC_DESCRIPTIONSDIR} )
INSTALL( FILES google-calendar-gcal-2005.xml DESTINATION ${OPENSYNC_CAPABILITIESDIR} )

