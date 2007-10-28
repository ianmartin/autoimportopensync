INCLUDE_DIRECTORIES( ${OPENSYNC_INCLUDE_DIR} ${PILOTLINK_INCLUDE_DIR} ${GLIB2_INCLUDE_DIRS} )

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

ADD_LIBRARY( palm-sync SHARED ${palm_sync_LIB_SRCS} )
ADD_LIBRARY( palm-format SHARED ${palm_format_LIB_SRCS} )

TARGET_LINK_LIBRARIES( palm-sync ${OPENYNC_LIBRARIES} ${PILOTLINK_LIBRARIES} ${GLIB2_LIBRARIES} )
TARGET_LINK_LIBRARIES( palm-format ${OPENYNC_LIBRARIES} ${PILOTLINK_LIBRARIES} ${GLIB2_LIBRARIES} ) 

# Install plugins	
INSTALL( TARGETS palm-sync DESTINATION ${OPENSYNC_PLUGINDIR} )
INSTALL( TARGETS palm-format DESTINATION ${OPENSYNC_FORMATSDIR} )

# Install config template	
INSTALL( FILES palm-sync DESTINATION ${OPENSYNC_CONFIGDIR} )

