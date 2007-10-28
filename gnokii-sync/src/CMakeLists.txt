INCLUDE_DIRECTORIES( ${OPENSYNC_INCLUDE_DIR} ${GNOKII_INCLUDE_DIR} ${GLIB2_INCLUDE_DIRS} )

# gnokii-sync source	
SET( gnokii_sync_LIB_SRCS
gnokii_comm.c
gnokii_config.c
gnokii_sync.c
gnokii_calendar.c
gnokii_contact.c
)

# gnokii-calendar format plugin source	
SET( gnokii_calendar_LIB_SRCS
gnokii_calendar_format.c
gnokii_calendar_utils.c
)

# gnokii-contact format plugin source	
SET( gnokii_contact_LIB_SRCS
gnokii_contact_format.c
gnokii_contact_utils.c
)

ADD_LIBRARY( gnokii-sync SHARED ${gnokii_sync_LIB_SRCS} )
ADD_LIBRARY( gnokii-calendar SHARED ${gnokii_calendar_LIB_SRCS} )
ADD_LIBRARY( gnokii-contact SHARED ${gnokii_contact_LIB_SRCS} )

TARGET_LINK_LIBRARIES( gnokii-sync ${OPENYNC_LIBRARIES} ${GNOKII_LIBRARIES} ${GLIB2_LIBRARIES} )
TARGET_LINK_LIBRARIES( gnokii-calendar ${OPENYNC_LIBRARIES} ${GNOKII_LIBRARIES} ${GLIB2_LIBRARIES} ) 
TARGET_LINK_LIBRARIES( gnokii-contact ${OPENYNC_LIBRARIES} ${GNOKII_LIBRARIES} ${GLIB2_LIBRARIES} ) 

# Install plugins	
INSTALL( TARGETS gnokii-sync DESTINATION ${OPENSYNC_PLUGINDIR} )
INSTALL( TARGETS gnokii-calendar DESTINATION ${OPENSYNC_FORMATSDIR} )
INSTALL( TARGETS gnokii-contact DESTINATION ${OPENSYNC_FORMATSDIR} )

# Install config template	
INSTALL( FILES gnokii-sync DESTINATION ${OPENSYNC_CONFIGDIR} )

