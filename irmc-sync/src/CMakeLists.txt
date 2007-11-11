INCLUDE_DIRECTORIES( ${CMAKE_CURRENT_SOURCE_DIR}/bfb ${CMAKE_CURRENT_SOURCE_DIR} 
		${OPENSYNC_INCLUDE_DIR} ${OPENOBEX_INCLUDE_DIR} ${BLUEZ_INCLUDE_DIR} 
		${GLIB2_INCLUDE_DIRS} ${LIBXML2_INCLUDE_DIR} )

# irmc-sync source	
SET( irmc_sync_LIB_SRCS
bfb/bfb.c
bfb/bfb_io.c
bfb/crc.c
irmc_sync.c
cobex_bfb.c
irmc_irda.c
irmc_obex.c
)

OPENSYNC_PLUGIN_ADD( irmc-sync ${irmc_sync_LIB_SRCS} ) 

TARGET_LINK_LIBRARIES( irmc-sync ${OPENSYNC_LIBRARIES} ${OPENOBEX_LIBRARIES} ${BLUEZ_LIBRARIES} ${GLIB2_LIBRARIES} ${LIBXML2_LIBRARIES} )

# Install plugin
OPENSYNC_PLUGIN_INSTALL( irmc-sync )

# Install config template	
OPENSYNC_PLUGIN_CONFIG( irmc-sync )

