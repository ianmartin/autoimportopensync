LINK_DIRECTORIES( ${GLIB2_LIBRARY_DIRS} ${LIBXML2_LIBRARY_DIRS} ${LIBXSLT_LIBRARY_DIRS} ${LIBEXSLT_LIBRARY_DIRS} ${SQLITE3_LIBRARY_DIRS} )
INCLUDE_DIRECTORIES( ${GLIB2_INCLUDE_DIRS} ${LIBXML2_INCLUDE_DIRS} ${LIBXSLT_INCLUDE_DIRS} ${LIBEXSLT_INCLUDE_DIRS} ${SQLITE3_INCLUDE_DIRS} )

INCLUDE( Compiler )

SET( libopensync_LIB_SRCS
   opensync_context.c
   opensync_error.c
   opensync_list.c
   opensync_support.c
   opensync_xml.c
   archive/opensync_archive.c
   client/opensync_client.c
   client/opensync_client_proxy.c
   data/opensync_change.c
   data/opensync_data.c
   db/opensync_db.c
   engine/opensync_engine.c
   engine/opensync_mapping_engine.c
   engine/opensync_mapping_entry_engine.c
   engine/opensync_obj_engine.c
   engine/opensync_sink_engine.c
   engine/opensync_status.c
   format/opensync_converter.c
   format/opensync_filter.c
   format/opensync_format_env.c
   format/opensync_objformat.c
   format/opensync_objformat_sink.c
   format/opensync_time.c
   group/opensync_group.c
   group/opensync_group_env.c
   group/opensync_member.c
   group/opensync_updater.c
   helper/opensync_anchor.c
   helper/opensync_hashtable.c
   ipc/opensync_message.c
   ipc/opensync_queue.c
   ipc/opensync_serializer.c
   mapping/opensync_mapping.c
   mapping/opensync_mapping_entry.c
   mapping/opensync_mapping_table.c
   mapping/opensync_mapping_view.c
   merger/opensync_capabilities.c
   merger/opensync_capability.c
   merger/opensync_merger.c
   module/opensync_module.c
   plugin/opensync_plugin_authentication.c
   plugin/opensync_plugin.c
   plugin/opensync_plugin_config.c
   plugin/opensync_plugin_connection.c
   plugin/opensync_plugin_env.c
   plugin/opensync_plugin_info.c
   plugin/opensync_plugin_localization.c
   plugin/opensync_plugin_ressource.c
   plugin/opensync_sink.c
   version/opensync_version.c
   xmlformat/opensync_xmlfield.c
   xmlformat/opensync_xmlfieldlist.c
   xmlformat/opensync_xmlformat.c
   xmlformat/opensync_xmlformat_schema.c
)

ADD_LIBRARY( opensync SHARED ${libopensync_LIB_SRCS} )

TARGET_LINK_LIBRARIES( opensync ${GLIB2_LIBRARIES} ${GTHREAD2_LIBRARIES} ${GMODULE2_LIBRARIES} ${LIBXML2_LIBRARIES} ${LIBXSLT_LIBRARIES} ${LIBEXSLT_LIBRARIES} ${SQLITE3_LIBRARIES} )

SET_TARGET_PROPERTIES( opensync PROPERTIES VERSION ${OPENSYNC_VERSION} SOVERSION ${OPENSYNC_SOVERSION} )
SET_TARGET_PROPERTIES( opensync PROPERTIES COMPILE_FLAGS ${SYMBOLS_VISIBILITY} )

IF ( OPENSYNC_UNITTESTS )
	ADD_LIBRARY( opensync-testing SHARED ${libopensync_LIB_SRCS} )
	TARGET_LINK_LIBRARIES( opensync-testing ${GLIB2_LIBRARIES} ${GTHREAD2_LIBRARIES} ${GMODULE2_LIBRARIES} ${LIBXML2_LIBRARIES}  ${LIBXSLT_LIBRARIES} ${LIBEXSLT_LIBRARIES} ${SQLITE3_LIBRARIES} )
ENDIF ( OPENSYNC_UNITTESTS )

INSTALL( TARGETS opensync DESTINATION ${LIB_INSTALL_DIR} )

INSTALL( DIRECTORY 
  archive
  client
  data
  db
  engine
  format
  group
  helper
  ipc
  mapping
  merger
  module
  plugin
  version
  xmlformat
DESTINATION ${OPENSYNC_INCLUDE_DIR}/opensync 
PATTERN "*/*_internals.h" EXCLUDE 
PATTERN ".*" EXCLUDE 
PATTERN "*.c*" EXCLUDE
PATTERN "*.h.*" EXCLUDE )

INSTALL( FILES
   opensync.h
   opensync-archive.h
   opensync-client.h
   opensync-context.h
   opensync-data.h
   opensync-engine.h
   opensync-error.h
   opensync-format.h
   opensync-group.h
   opensync-helper.h
   opensync-ipc.h
   opensync_list.h
   opensync-mapping.h
   opensync-merger.h
   opensync-module.h
   opensync-plugin.h
   opensync-serializer.h
   opensync-support.h
   opensync-time.h
   opensync-version.h
   opensync-xmlformat.h
DESTINATION ${OPENSYNC_INCLUDE_DIR}/opensync )


########### osplugin ##################

SET( osplugin_SRCS
   client/osplugin.c
)
ADD_EXECUTABLE( osplugin ${osplugin_SRCS} )

IF(COMMAND cmake_policy)
  cmake_policy(SET CMP0005 OLD)
ENDIF(COMMAND cmake_policy)

ADD_DEFINITIONS( -DOSPLUGIN="\\\"${OPENSYNC_LIBEXEC_DIR}/osplugin\\\"" )

TARGET_LINK_LIBRARIES( osplugin opensync ${GLIB2_LIBRARIES} ${GTHREAD2_LIBRARIES} ${GMODULE2_LIBRARIES} )

# TODO: osplugin in libexec dir?
INSTALL( TARGETS osplugin DESTINATION ${OPENSYNC_LIBEXEC_DIR} ) 

