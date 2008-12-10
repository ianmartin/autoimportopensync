LINK_DIRECTORIES(  ${OPENSYNC_LIBRARY_DIRS} ${GLIB2_LIBRARY_DIRS} )
INCLUDE_DIRECTORIES( ${OPENSYNC_INCLUDE_DIRS} ${GLIB2_INCLUDE_DIRS} )

### Simple Synchronization Plugin ########
OPENSYNC_PLUGIN_ADD( example_simple_plugin simple_plugin.c )
TARGET_LINK_LIBRARIES( example_simple_plugin ${OPENSYNC_LIBRARIES} ${GLIB2_LIBRARIES} )
OPENSYNC_PLUGIN_INSTALL( example_simple_plugin )

### Advanced Synchronization Plugin ######
OPENSYNC_PLUGIN_ADD( example_plugin plugin.c )
TARGET_LINK_LIBRARIES( example_plugin ${OPENSYNC_LIBRARIES} ${GLIB2_LIBRARIES} )
OPENSYNC_PLUGIN_INSTALL( example_plugin )

### Format Plugin ########################
OPENSYNC_FORMAT_ADD( format_plugin format.c )
TARGET_LINK_LIBRARIES( format_plugin ${OPENSYNC_LIBRARIES} ${GLIB2_LIBRARIES} )
OPENSYNC_FORMAT_INSTALL( format_plugin )

