INCLUDE_DIRECTORIES( ${CMAKE_CURRENT_SOURCE_DIR} ${CHECK_INCLUDE_DIR} ${GLIB2_INCLUDE_DIRS} ${LIBXML2_INCLUDE_DIR} )

ADD_SUBDIRECTORY( mock-plugin )

########### support #################	
ADD_LIBRARY( support SHARED support.c )
TARGET_LINK_LIBRARIES( support opensync ${CHECK_LIBRARIES} )

############ unit tests ##########################	

ADD_EXECUTABLE( archive archive-tests/check_archive.c )
ADD_EXECUTABLE( client client-tests/check_client.c )
ADD_EXECUTABLE( proxy client-tests/check_proxy.c )
ADD_EXECUTABLE( datatest data-tests/check_data.c )
ADD_EXECUTABLE( engine engine-tests/check_engine.c )
ADD_EXECUTABLE( error engine-tests/check_engine_error.c )
ADD_EXECUTABLE( support engine-tests/engine_support.c )
ADD_EXECUTABLE( conv format-tests/check_conv.c )
ADD_EXECUTABLE( converter format-tests/check_converter.c )
ADD_EXECUTABLE( formatenv format-tests/check_format_env.c )
ADD_EXECUTABLE( objformat format-tests/check_objformat.c )
ADD_EXECUTABLE( time format-tests/check_time.c )
ADD_EXECUTABLE( member group-tests/check_member.c )
ADD_EXECUTABLE( ipc ipc-tests/check_ipc.c )
ADD_EXECUTABLE( mapping mapping-tests/check_mapping.c )
ADD_EXECUTABLE( capabilities merger-tests/check_capabilities.c )
ADD_EXECUTABLE( merger merger-tests/check_merger.c )
ADD_EXECUTABLE( module module-tests/check_module.c )
ADD_EXECUTABLE( sync sync-tests/check_sync.c )
ADD_EXECUTABLE( version version-tests/check_version.c )
ADD_EXECUTABLE( xmlformat merger-tests/check_xmlformat.c )


# TODO: port to new API
#ADD_EXECUTABLE( detect format-tests/check_detect.c ) 
#ADD_EXECUTABLE( hash helper-tests/check_hash.c )
#ADD_EXECUTABLE( group group-tests/check_group.c )
#ADD_EXECUTABLE( lock group-tests/check_lock.c )
#ADD_EXECUTABLE( filter sync-tests/check_filter.c )
#ADD_EXECUTABLE( multisync sync-tests/check_multisync.c )
#ADD_EXECUTABLE( syncerror sync-tests/check_sync_error.c )

# TODO: move to vformat
#ADD_EXECUTABLE( vcard conversion-tests/check_vcard.c )
#ADD_EXECUTABLE( vevent conversion-tests/check_vevent.c )
#ADD_EXECUTABLE( vnote conversion-tests/check_vnote.c )
#ADD_EXECUTABLE( vtodo conversion-tests/check_vtodo.c )
#ADD_EXECUTABLE( conversion conversion-tests/conversion.c )

TARGET_LINK_LIBRARIES( archive opensync ${CHECK_LIBRARIES} support )
TARGET_LINK_LIBRARIES( client opensync ${CHECK_LIBRARIES} support )
TARGET_LINK_LIBRARIES( proxy opensync ${CHECK_LIBRARIES} support )
TARGET_LINK_LIBRARIES( datatest opensync ${CHECK_LIBRARIES} support )
TARGET_LINK_LIBRARIES( engine opensync ${CHECK_LIBRARIES} support )
TARGET_LINK_LIBRARIES( error opensync ${CHECK_LIBRARIES} support )
TARGET_LINK_LIBRARIES( support opensync ${CHECK_LIBRARIES} support )
TARGET_LINK_LIBRARIES( conv opensync ${CHECK_LIBRARIES} support )
TARGET_LINK_LIBRARIES( converter opensync ${CHECK_LIBRARIES} support )
TARGET_LINK_LIBRARIES( formatenv opensync ${CHECK_LIBRARIES} support )
TARGET_LINK_LIBRARIES( objformat opensync ${CHECK_LIBRARIES} support )
TARGET_LINK_LIBRARIES( time opensync ${CHECK_LIBRARIES} support )
TARGET_LINK_LIBRARIES( member opensync ${CHECK_LIBRARIES} support )
TARGET_LINK_LIBRARIES( ipc opensync ${CHECK_LIBRARIES} support )
TARGET_LINK_LIBRARIES( mapping opensync ${CHECK_LIBRARIES} support )
TARGET_LINK_LIBRARIES( capabilities opensync ${CHECK_LIBRARIES} support )
TARGET_LINK_LIBRARIES( merger opensync ${CHECK_LIBRARIES} support )
TARGET_LINK_LIBRARIES( module opensync ${CHECK_LIBRARIES} support )
TARGET_LINK_LIBRARIES( sync opensync ${CHECK_LIBRARIES} support )
TARGET_LINK_LIBRARIES( version opensync ${CHECK_LIBRARIES} support )
TARGET_LINK_LIBRARIES( xmlformat opensync ${CHECK_LIBRARIES} support )

# TODO: port to new API	
#TARGET_LINK_LIBRARIES( detect opensync ${CHECK_LIBRARIES} support )
#TARGET_LINK_LIBRARIES( group opensync ${CHECK_LIBRARIES} support )
#TARGET_LINK_LIBRARIES( lock opensync ${CHECK_LIBRARIES} support )
#TARGET_LINK_LIBRARIES( hash opensync ${CHECK_LIBRARIES} support )
#TARGET_LINK_LIBRARIES( filter opensync ${CHECK_LIBRARIES} support )
#TARGET_LINK_LIBRARIES( multisync opensync ${CHECK_LIBRARIES} support )
#TARGET_LINK_LIBRARIES( syncerror opensync ${CHECK_LIBRARIES} support )

# TODO: move to vformat
#TARGET_LINK_LIBRARIES( vcard opensync ${CHECK_LIBRARIES} support )
#TARGET_LINK_LIBRARIES( vevent opensync ${CHECK_LIBRARIES} support )
#TARGET_LINK_LIBRARIES( vnote opensync ${CHECK_LIBRARIES} support )
#TARGET_LINK_LIBRARIES( vtodo opensync ${CHECK_LIBRARIES} support )
#TARGET_LINK_LIBRARIES( conversion opensync ${CHECK_LIBRARIES} support )



###### INSTALL ################### 
#INSTALL( TARGETS osyncdump osyncbinary DESTINATION ${BIN_INSTALL_DIR} ) 

