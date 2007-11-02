INCLUDE_DIRECTORIES( ${CMAKE_CURRENT_SOURCE_DIR}/bfb ${CMAKE_CURRENT_SOURCE_DIR} 
		${OPENSYNC_INCLUDE_DIR} ${OPENOBEX_INCLUDE_DIR} ${BLUEZ_INCLUDE_DIR} ${GLIB2_INCLUDE_DIRS} )

ADD_SUBDIRECTORY( bfb )

# irmc-sync source	
SET( irmc_sync_LIB_SRCS
irmc_sync.c
cobex_bfb.c
irmc_bluetooth.c
irmc_irda.c
irmc_obex.c
)

OPENSYNC_PLUGIN_ADD( irmc-sync ${irmc_sync_LIB_SRCS} ) 

TARGET_LINK_LIBRARIES( irmc-sync ${OPENYNC_LIBRARIES} ${OPENOBEX_LIBRARIES} ${BLUEZ_LIBRARIES} ${GLIB2_LIBRARIES} bfb )

# Install plugin
OPENSYNC_PLUGIN_INSTALL( irmc-sync )

# Install config template	
OPENSYNC_PLUGIN_CONFIG( irmc-sync )

