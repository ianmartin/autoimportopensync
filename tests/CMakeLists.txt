INCLUDE_DIRECTORIES( ${CMAKE_CURRENT_SOURCE_DIR} ${CHECK_INCLUDE_DIR} ${GLIB2_INCLUDE_DIRS} ${LIBXML2_INCLUDE_DIR} )

ADD_SUBDIRECTORY( mock-plugin )

########### support #################	
ADD_LIBRARY( support SHARED support.c )
TARGET_LINK_LIBRARIES( support opensync ${CHECK_LIBRARIES} )

############ unit tests ##########################	

SET( TEST_TARGET_LIBRARIES support ) 

OPENSYNC_ADD_TEST( archive archive-tests/check_archive.c ${TEST_TARGET_LIBRARIES} )
OPENSYNC_ADD_TEST( capabilities merger-tests/check_capabilities.c ${TEST_TARGET_LIBRARIES} )
OPENSYNC_ADD_TEST( client client-tests/check_client.c ${TEST_TARGET_LIBRARIES} )
OPENSYNC_ADD_TEST( filter sync-tests/check_filter.c ${TEST_TARGET_LIBRARIES} )
OPENSYNC_ADD_TEST( conv format-tests/check_conv.c ${TEST_TARGET_LIBRARIES} )
OPENSYNC_ADD_TEST( converter format-tests/check_converter.c ${TEST_TARGET_LIBRARIES} )
OPENSYNC_ADD_TEST( datatest data-tests/check_data.c ${TEST_TARGET_LIBRARIES} )
OPENSYNC_ADD_TEST( detect format-tests/check_detect.c ${TEST_TARGET_LIBRARIES} ) 
OPENSYNC_ADD_TEST( engine engine-tests/check_engine.c ${TEST_TARGET_LIBRARIES} )
OPENSYNC_ADD_TEST( error engine-tests/check_engine_error.c ${TEST_TARGET_LIBRARIES} )
OPENSYNC_ADD_TEST( formatenv format-tests/check_format_env.c ${TEST_TARGET_LIBRARIES} )
OPENSYNC_ADD_TEST( group group-tests/check_group.c ${TEST_TARGET_LIBRARIES} )
#OPENSYNC_ADD_TEST( hash helper-tests/check_hash.c ${TEST_TARGET_LIBRARIES} )
OPENSYNC_ADD_TEST( lock group-tests/check_lock.c ${TEST_TARGET_LIBRARIES} )
OPENSYNC_ADD_TEST( ipc ipc-tests/check_ipc.c ${TEST_TARGET_LIBRARIES} )
OPENSYNC_ADD_TEST( mapping mapping-tests/check_mapping.c ${TEST_TARGET_LIBRARIES} )
OPENSYNC_ADD_TEST( member group-tests/check_member.c ${TEST_TARGET_LIBRARIES} )
OPENSYNC_ADD_TEST( merger merger-tests/check_merger.c ${TEST_TARGET_LIBRARIES} )
OPENSYNC_ADD_TEST( module module-tests/check_module.c ${TEST_TARGET_LIBRARIES} )
#OPENSYNC_ADD_TEST( multisync sync-tests/check_multisync.c ${TEST_TARGET_LIBRARIES} )
OPENSYNC_ADD_TEST( sync sync-tests/check_sync.c ${TEST_TARGET_LIBRARIES} )
#OPENSYNC_ADD_TEST( syncerror sync-tests/check_sync_error.c ${TEST_TARGET_LIBRARIES} )
OPENSYNC_ADD_TEST( objformat format-tests/check_objformat.c ${TEST_TARGET_LIBRARIES} )
OPENSYNC_ADD_TEST( proxy client-tests/check_proxy.c ${TEST_TARGET_LIBRARIES} )
OPENSYNC_ADD_TEST( time format-tests/check_time.c ${TEST_TARGET_LIBRARIES} )
OPENSYNC_ADD_TEST( version version-tests/check_version.c ${TEST_TARGET_LIBRARIES} )
OPENSYNC_ADD_TEST( xmlformat merger-tests/check_xmlformat.c ${TEST_TARGET_LIBRARIES} )

