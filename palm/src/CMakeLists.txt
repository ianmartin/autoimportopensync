INCLUDE_DIRECTORIES( ${OPENSYNC_INCLUDE_DIR} ${PILOTLINK_INCLUDE_DIR} ${GLIB2_INCLUDE_DIRS} ${LIBXML2_INCLUDE_DIRS} )

# palm-sync source	
SET( palm_sync_LIB_SRCS
palm_sync.c
palm_contact.c
# TODO Port to OpenSync API 0.30
#palm_event.c
#palm_note.c
#palm_todo.c
)

# palm-format source	
SET( palm_format_LIB_SRCS
palm_format.c  
)

OPENSYNC_PLUGIN_ADD( palm-sync ${palm_sync_LIB_SRCS} )
OPENSYNC_FORMAT_ADD( palm-format ${palm_format_LIB_SRCS} )

TARGET_LINK_LIBRARIES( palm-sync ${OPENSYNC_LIBRARIES} ${PILOTLINK_LIBRARIES} ${GLIB2_LIBRARIES} ${LIBXML2_LIBRARIES} )
TARGET_LINK_LIBRARIES( palm-format ${OPENSYNC_LIBRARIES} ${PILOTLINK_LIBRARIES} ${GLIB2_LIBRARIES} ${LIBXML2_LIBRARIES} ) 

# Install plugins	
OPENSYNC_PLUGIN_INSTALL( palm-sync )
OPENSYNC_FORMAT_INSTALL( palm-format )

# Install config template	
OPENSYNC_PLUGIN_CONFIG( palm-sync )

