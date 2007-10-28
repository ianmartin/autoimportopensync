INCLUDE_DIRECTORIES( ${OPENSYNC_INCLUDE_DIR} ${GLIB2_INCLUDE_DIRS} )

# gcalendar source	
SET( gcalendar_LIB_SRCS
gcalendar.c
)

ADD_DEFINITIONS( -DGCAL_HELPER="\\\"${LIBEXEC_INSTALL_DIR}/goole-cal-helper\\\"" )
ADD_LIBRARY( gcalendar SHARED ${gcalendar_LIB_SRCS} )

TARGET_LINK_LIBRARIES( gcalendar ${OPENYNC_LIBRARIES} ${GLIB2_LIBRARIES} )

# Install plugins	
INSTALL( TARGETS gcalendar DESTINATION ${OPENSYNC_PLUGINDIR} )

# Install config template	
INSTALL( FILES google-calendar DESTINATION ${OPENSYNC_CONFIGDIR} )

# Install capabilities and descriptions
INSTALL( FILES google-calendar.xml DESTINATION ${OPENSYNC_DESCRIPTIONSDIR} )
INSTALL( FILES google-calendar-gcal-2005.xml DESTINATION ${OPENSYNC_CAPABILITIESDIR} )

