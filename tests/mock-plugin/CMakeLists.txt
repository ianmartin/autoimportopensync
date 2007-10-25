
ADD_LIBRARY( mock-sync SHARED mock_sync.c )
ADD_LIBRARY( mock-format SHARED mock_format.c )

TARGET_LINK_LIBRARIES( mock-sync opensync ${CHECK_LIBRARIES} )
TARGET_LINK_LIBRARIES( mock-format opensync ${CHECK_LIBRARIES} )


###### INSTALL ################### 
#INSTALL( TARGETS mock-sync mock-format DESTINATION ${OPENSYNC_PLUGINDIR} ) 

