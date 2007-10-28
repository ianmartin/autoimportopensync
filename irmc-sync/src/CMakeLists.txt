INCLUDE_DIRECTORIES( ${CMAKE_CURRENT_SOURCE_DIR}/bfb ${CMAKE_CURRENT_SOURCE_DIR} 
		${OPENSYNC_INCLUDE_DIR} ${OPENOBEX_INCLUDE_DIR} ${GLIB2_INCLUDE_DIRS} )

ADD_SUBDIRECTORY( bfb )

# irmc-sync source	
SET( irmc_sync_LIB_SRCS
irmc_sync.c
cobex_bfb.c
irmc_bluetooth.c
irmc_irda.c
irmc_obex.c
)

ADD_LIBRARY( irmc-sync SHARED ${irmc_sync_LIB_SRCS} )

TARGET_LINK_LIBRARIES( irmc-sync ${OPENYNC_LIBRARIES} ${OPENOBEX_LIBRARIES} ${GLIB2_LIBRARIES} bfb )

# Install plugin
INSTALL( TARGETS irmc-sync DESTINATION ${OPENSYNC_PLUGINDIR} )

# Install config template	
INSTALL( FILES irmc-sync DESTINATION ${OPENSYNC_CONFIGDIR} )

