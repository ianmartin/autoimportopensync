INCLUDE_DIRECTORIES( ${OPENSYNC_INCLUDE_DIR} ${GNOKII_INCLUDE_DIR} ${GLIB2_INCLUDE_DIRS} ${LIBXML2_INCLUDE_DIR} )

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

OPENSYNC_PLUGIN_ADD( gnokii-sync ${gnokii_sync_LIB_SRCS} )
OPENSYNC_FORMAT_ADD( gnokii-calendar ${gnokii_calendar_LIB_SRCS} )
OPENSYNC_FORMAT_ADD( gnokii-contact ${gnokii_contact_LIB_SRCS} )

TARGET_LINK_LIBRARIES( gnokii-sync ${OPENSYNC_LIBRARIES} ${GNOKII_LIBRARIES} ${GLIB2_LIBRARIES} ${LIBXML2_LIBRARIES} )
TARGET_LINK_LIBRARIES( gnokii-calendar ${OPENSYNC_LIBRARIES} ${GNOKII_LIBRARIES} ${GLIB2_LIBRARIES} ) 
TARGET_LINK_LIBRARIES( gnokii-contact ${OPENSYNC_LIBRARIES} ${GNOKII_LIBRARIES} ${GLIB2_LIBRARIES} ) 

# Install plugins	

OPENSYNC_PLUGIN_INSTALL( gnokii-sync )
OPENSYNC_FORMAT_INSTALL( gnokii-calendar )
OPENSYNC_FORMAT_INSTALL( gnokii-contact )

# Install config template	
OPENSYNC_PLUGIN_CONFIG( gnokii-sync )

