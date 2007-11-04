INCLUDE_DIRECTORIES( ${OPENSYNC_INCLUDE_DIR} ${GLIB2_INCLUDE_DIRS} ${LIBXML2_INCLUDE_DIR} )


SET( gpe_LIB_SRCS
gpe_sync.c 
calendar.c 
contacts.c 
todo.c 
gpe_xml.c 
gpesync_client.c 
utils.c
)

OPENSYNC_PLUGIN_ADD( gpe-sync ${gpe_LIB_SRCS} )

TARGET_LINK_LIBRARIES( gpe-sync ${OPENSYNC_LIBRARY} ${GLIB2_LIBRARIES} ${LIBXML2_LIBRARIES} )

###### INSTALL ################### 
OPENSYNC_PLUGIN_INSTALL( gpe-sync ) 

OPENSYNC_PLUGIN_CONFIG( gpe-sync )
