INCLUDE( OpenSyncPlugin )

OPENSYNC_PLUGIN_ADD( mock-sync mock_sync.c )
OPENSYNC_FORMAT_ADD( mock-format mock_format.c )

TARGET_LINK_LIBRARIES( mock-sync opensync )
TARGET_LINK_LIBRARIES( mock-format opensync )

###### INSTALL ################### 
#OPENSYNC_PLUGIN_INSTALL( mock-sync ) 
#OPENSYNC_FORMAT_INSTALL( mock-format ) 

